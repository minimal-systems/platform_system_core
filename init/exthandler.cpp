/*
 * Copyright (C) 2024 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 */

#include "exthandler.h"
#include "log_new.h"

#include <fnmatch.h>
#include <grp.h>
#include <pwd.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>

#include <fstream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

/**
 * Trims leading and trailing spaces from a string.
 *
 * @param str The input string to trim.
 * @return A string with whitespace removed from both ends.
 */
std::string Trim(const std::string& str) {
    size_t first = str.find_first_not_of(" ");
    if (first == std::string::npos) return "";
    size_t last = str.find_last_not_of(" ");
    return str.substr(first, last - first + 1);
}

/**
 * Splits a string into tokens using the specified delimiter.
 *
 * @param str The input string.
 * @param delimiter The delimiter to split on.
 * @return A vector of substrings.
 */
std::vector<std::string> Split(const std::string& str, const std::string& delimiter) {
    std::vector<std::string> tokens;
    size_t start = 0, end;
    while ((end = str.find(delimiter, start)) != std::string::npos) {
        tokens.push_back(str.substr(start, end - start));
        start = end + delimiter.length();
    }
    tokens.push_back(str.substr(start));
    return tokens;
}

/**
 * Reads all available data from a file descriptor into a string.
 *
 * @param fd A readable file descriptor.
 * @param content Pointer to string that will receive the data.
 * @return true on success, false on error.
 */
bool ReadFdToString(int fd, std::string* content) {
    char buffer[1024];
    ssize_t bytes_read;
    while ((bytes_read = read(fd, buffer, sizeof(buffer) - 1)) > 0) {
        buffer[bytes_read] = '\0';
        *content += buffer;
    }
    return bytes_read >= 0;
}

/**
 * Creates a socketpair and assigns file descriptors.
 *
 * @param domain Socket domain (e.g., AF_UNIX).
 * @param type Socket type (e.g., SOCK_STREAM).
 * @param protocol Usually 0.
 * @param fd1[out] First file descriptor.
 * @param fd2[out] Second file descriptor.
 * @return true on success, false on failure.
 */
bool Socketpair(int domain, int type, int protocol, int* fd1, int* fd2) {
    int sv[2];
    if (socketpair(domain, type, protocol, sv) == -1) {
        return false;
    }
    *fd1 = sv[0];
    *fd2 = sv[1];
    return true;
}

/**
 * Executes an external handler binary with environment overrides and UID/GID.
 *
 * - Forks a child process
 * - Sets environment variables
 * - Drops privileges (setuid/setgid)
 * - Redirects stdout/stderr to capture output
 * - Executes the handler command
 *
 * Captured stdout is returned as string if execution succeeds.
 * Captured stderr is logged. All failures result in empty string return.
 *
 * @param handler The command to execute (e.g. "/bin/myscript --flag").
 * @param uid The user ID to switch to before exec.
 * @param gid The group ID to switch to before exec.
 * @param envs_map A map of environment variables to apply before exec.
 * @return The trimmed stdout content on success, empty string otherwise.
 */
std::string RunExternalHandler(const std::string& handler, uid_t uid, gid_t gid,
                               std::unordered_map<std::string, std::string>& envs_map) {
    int child_stdout, parent_stdout;
    int child_stderr, parent_stderr;

    // Create pipes to capture stdout and stderr from the child process
    if (!Socketpair(AF_UNIX, SOCK_STREAM, 0, &child_stdout, &parent_stdout) ||
        !Socketpair(AF_UNIX, SOCK_STREAM, 0, &child_stderr, &parent_stderr)) {
        LOGE("Socketpair() failed");
        return "";
    }

    signal(SIGCHLD, SIG_DFL);
    pid_t pid = fork();
    if (pid < 0) {
        LOGE("fork() failed");
        return "";
    }

    if (pid == 0) {
        // Child process setup
        for (const auto& [key, value] : envs_map) {
            setenv(key.c_str(), value.c_str(), 1);
        }

        // Redirect I/O
        close(parent_stdout);
        close(parent_stderr);
        dup2(child_stdout, STDOUT_FILENO);
        dup2(child_stderr, STDERR_FILENO);

        // Prepare argv[]
        auto args = Split(handler, " ");
        std::vector<char*> c_args;
        for (auto& arg : args) {
            c_args.emplace_back(arg.data());
        }
        c_args.emplace_back(nullptr);  // null-terminate the argv array

        // Drop privileges
        if (gid != 0 && setgid(gid) != 0) {
            fprintf(stderr, "setgid() failed: %s", strerror(errno));
            _exit(EXIT_FAILURE);
        }

        if (setuid(uid) != 0) {
            fprintf(stderr, "setuid() failed: %s", strerror(errno));
            _exit(EXIT_FAILURE);
        }

        // Execute handler
        execv(c_args[0], c_args.data());
        fprintf(stderr, "exec() failed: %s", strerror(errno));
        _exit(EXIT_FAILURE);
    }

    // Parent process cleanup
    close(child_stdout);
    close(child_stderr);

    int status;
    pid_t waited_pid = waitpid(pid, &status, 0);
    if (waited_pid == -1) {
        LOGE("waitpid() failed");
        return "";
    }

    std::string stdout_content;
    if (!ReadFdToString(parent_stdout, &stdout_content)) {
        LOGE("ReadFdToString() for stdout failed");
        return "";
    }

    std::string stderr_content;
    if (ReadFdToString(parent_stderr, &stderr_content)) {
        for (const auto& message : Split(stderr_content, "\n")) {
            if (!message.empty()) {
                LOGE("External Handler: %s", message.c_str());
            }
        }
    } else {
        LOGE("ReadFdToString() for stderr failed");
    }

    // Process exit handling
    if (WIFEXITED(status)) {
        if (WEXITSTATUS(status) == EXIT_SUCCESS) {
            return Trim(stdout_content);
        } else {
            LOGE("Exited with status %d", WEXITSTATUS(status));
            return "";
        }
    } else if (WIFSIGNALED(status)) {
        LOGE("Killed by signal %d", WTERMSIG(status));
        return "";
    }

    LOGE("Unexpected exit status %d", status);
    return "";
}
