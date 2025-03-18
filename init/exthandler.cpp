/*
 * Copyright (C) 2024 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
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

std::string Trim(const std::string& str) {
    size_t first = str.find_first_not_of(" ");
    if (first == std::string::npos) return "";
    size_t last = str.find_last_not_of(" ");
    return str.substr(first, last - first + 1);
}

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

bool ReadFdToString(int fd, std::string* content) {
    char buffer[1024];
    ssize_t bytes_read;
    while ((bytes_read = read(fd, buffer, sizeof(buffer) - 1)) > 0) {
        buffer[bytes_read] = '\0';
        *content += buffer;
    }
    return bytes_read >= 0;
}

bool Socketpair(int domain, int type, int protocol, int* fd1, int* fd2) {
    int sv[2];
    if (socketpair(domain, type, protocol, sv) == -1) {
        return false;
    }
    *fd1 = sv[0];
    *fd2 = sv[1];
    return true;
}

std::string RunExternalHandler(const std::string& handler, uid_t uid, gid_t gid,
                               std::unordered_map<std::string, std::string>& envs_map) {
    int child_stdout, parent_stdout;
    int child_stderr, parent_stderr;
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
        for (const auto& [key, value] : envs_map) {
            setenv(key.c_str(), value.c_str(), 1);
        }
        close(parent_stdout);
        close(parent_stderr);
        dup2(child_stdout, STDOUT_FILENO);
        dup2(child_stderr, STDERR_FILENO);

        auto args = Split(handler, " ");
        std::vector<char*> c_args;
        for (auto& arg : args) {
            c_args.emplace_back(arg.data());
        }
        c_args.emplace_back(nullptr);

        if (gid != 0 && setgid(gid) != 0) {
            fprintf(stderr, "setgid() failed: %s", strerror(errno));
            _exit(EXIT_FAILURE);
        }

        if (setuid(uid) != 0) {
            fprintf(stderr, "setuid() failed: %s", strerror(errno));
            _exit(EXIT_FAILURE);
        }

        execv(c_args[0], c_args.data());
        fprintf(stderr, "exec() failed: %s", strerror(errno));
        _exit(EXIT_FAILURE);
    }

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
