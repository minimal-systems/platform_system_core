/*
 * Copyright (C) 2020 The Android Open Source Project
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

#include "first_stage_console.h"

#include <spawn.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <fcntl.h>
#include <unistd.h>
#include <string>
#include <thread>
#include <chrono>
#include <iostream>
#include "log_new.h"
#include "bootcfg.h"

namespace {

/**
 * @brief Detects whether the kernel command line declares a serial console.
 *
 * Searches for the presence of `ro.boot.console=` token as a standalone key.
 * This is used to determine if a serial console should be attached early.
 *
 * @param cmdline The full `/proc/cmdline` contents.
 * @return true if a `ro.boot.console=` entry exists.
 */
bool KernelConsolePresent(const std::string& cmdline) {
    size_t pos = 0;
    while (true) {
        pos = cmdline.find("ro.boot.console=", pos);
        if (pos == std::string::npos) return false;
        if (pos == 0 || cmdline[pos - 1] == ' ') return true;
        pos++;
    }
}

/**
 * @brief Sets up /dev/console and redirects standard I/O to it.
 *
 * Creates the `/dev/console` device node if necessary, opens it,
 * attaches it as controlling terminal, and dup2()s it to stdin/stdout/stderr.
 *
 * @return true on success, false on failure.
 */
bool SetupConsole() {
    if (mknod("/dev/console", S_IFCHR | 0600, makedev(5, 1)) && errno != EEXIST) {
        LOGE("Unable to create /dev/console: %m");
        return false;
    }

    int fd = -1;
    int tries = 50;  // Retry for up to 5 seconds
    while (tries--) {
        fd = open("/dev/console", O_RDWR);
        if (fd != -1) break;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    if (fd == -1) {
        LOGE("Could not open /dev/console: %m");
        return false;
    }

    ioctl(fd, TIOCSCTTY, 0);
    dup2(fd, STDIN_FILENO);
    dup2(fd, STDOUT_FILENO);
    dup2(fd, STDERR_FILENO);
    close(fd);
    return true;
}

/**
 * @brief Spawns an image (binary/script) with default environment and args.
 *
 * @param file Path to the executable (e.g. `/bin/sh`).
 * @return The child PID on success, 0 on error.
 */
pid_t SpawnImage(const char* file) {
    const char* argv[] = {file, nullptr};
    const char* envp[] = {nullptr};

    char* const* argvp = const_cast<char* const*>(argv);
    char* const* envpp = const_cast<char* const*>(envp);

    pid_t pid;
    errno = posix_spawn(&pid, argv[0], nullptr, nullptr, argvp, envpp);
    if (!errno) return pid;

    LOGE("Failed to spawn '%s': %m", file);
    return 0;
}

}  // namespace

namespace minimal_systems {
namespace init {

/**
 * @brief Entry point to start the early system console and/or shell.
 *
 * This is invoked if `ro.boot.first_stage_console=1` is set in:
 * - Bootconfig
 * - Kernel cmdline
 * - Parsed bootcfg fallback
 *
 * Steps:
 * - Forks a child to avoid blocking init
 * - Optionally sets up /dev/console if kernel requests a serial console
 * - Runs `/first_stage.sh` if present
 * - Drops into `/bin/sh` if console enabled
 *
 * @param cmdline The merged kernel command line.
 */
void StartConsole(const std::string& cmdline) {
    bool console = KernelConsolePresent(cmdline);

    // Avoid zombie reaping in parent
    struct sigaction chld_act = {};
    chld_act.sa_flags = SA_NOCLDWAIT;
    chld_act.sa_handler = SIG_DFL;
    sigaction(SIGCHLD, &chld_act, nullptr);

    pid_t pid = fork();
    if (pid != 0) {
        wait(nullptr);
        LOGE("Console shell exited");
        return;
    }

    // Setup console only if present in cmdline
    if (console) {
        console = SetupConsole();
    }

    LOGI("Attempting to run /first_stage.sh...");
    if (SpawnImage("/first_stage.sh")) {
        wait(nullptr);
        LOGI("/first_stage.sh exited");
    }

    // Launch interactive shell
    if (console) {
        if (SpawnImage("/bin/sh")) {
            wait(nullptr);
        }
    }

    _exit(127);  // Hard fallback exit
}

/**
 * @brief Determines if the first-stage console is explicitly enabled.
 *
 * Evaluation priority:
 *  1. Check `bootconfig` string for `ro.boot.first_stage_console=1`
 *  2. Check merged `cmdline` for same key
 *  3. Fallback to bootcfg parsed key (if available)
 *
 * This function allows dynamic toggling of early interactive recovery.
 *
 * @param cmdline Full kernel command line.
 * @param bootconfig Full bootconfig string (if available).
 * @return 1 if console enabled, 0 otherwise.
 */
int FirstStageConsole(const std::string& cmdline, const std::string& bootconfig) {
    const std::string kKey = "ro.boot.first_stage_console";

    // 1. Check bootconfig directly
    size_t pos = bootconfig.find(kKey + "=");
    if (pos != std::string::npos) {
        int val = 0;
        if (sscanf(bootconfig.c_str() + pos, "ro.boot.first_stage_console=%d", &val) == 1) {
            return val;
        }
    }

    // 2. Check /proc/cmdline or merged string
    pos = cmdline.find(kKey + "=");
    if (pos != std::string::npos) {
        int val = 0;
        if (sscanf(cmdline.c_str() + pos, "ro.boot.first_stage_console=%d", &val) == 1) {
            return val;
        }
    }

    // 3. Optional fallback from bootcfg map
    std::string from_bootcfg = minimal_systems::bootcfg::Get(kKey, "0");
    int fallback_val = 0;
    if (sscanf(from_bootcfg.c_str(), "%d", &fallback_val) == 1) {
        return fallback_val;
    }

    return 0;
}

}  // namespace init
}  // namespace minimal_systems
