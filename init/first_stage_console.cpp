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

static bool KernelConsolePresent(const std::string& cmdline) {
    size_t pos = 0;
    while (true) {
        pos = cmdline.find("ro.boot.console=", pos);
        if (pos == std::string::npos) return false;
        if (pos == 0 || cmdline[pos - 1] == ' ') return true;
        pos++;
    }
}

static bool SetupConsole() {
    if (mknod("/dev/console", S_IFCHR | 0600, makedev(5, 1)) && errno != EEXIST) {
        LOGE("Unable to create /dev/console: %m");
        return false;
    }

    int fd = -1;
    int tries = 50;  // Timeout after 5s

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

static pid_t SpawnImage(const char* file) {
    const char* argv[] = {file, NULL};
    const char* envp[] = {NULL};

    char* const* argvp = const_cast<char* const*>(argv);
    char* const* envpp = const_cast<char* const*>(envp);

    pid_t pid;
    errno = posix_spawn(&pid, argv[0], NULL, NULL, argvp, envpp);
    if (!errno) return pid;

   LOGE("Failed to spawn '%s': %m", file);
   return 0;
}

namespace minimal_systems {
namespace init {

void StartConsole(const std::string& cmdline) {

    bool console = KernelConsolePresent(cmdline);

    struct sigaction chld_act = {};
    chld_act.sa_flags = SA_NOCLDWAIT;
    chld_act.sa_handler = SIG_DFL;
    sigaction(SIGCHLD, &chld_act, nullptr);

    pid_t pid = fork();
    if (pid != 0) {
        wait(NULL);
        LOGE("Console shell exited");
        return;
    }

    if (console) console = SetupConsole();

   LOGI("Attempting to run /first_stage.sh...");
    if (SpawnImage("/first_stage.sh")) {
        wait(NULL);
        LOGI("/first_stage.sh exited");
    }

    if (console) {
        if (SpawnImage("/bin/sh")) wait(NULL);
    }
    _exit(127);
}

int FirstStageConsole(const std::string& cmdline, const std::string& bootconfig) {
    const std::string kKey = "ro.boot.first_stage_console";

    // 1. Try explicit bootconfig string first
    auto pos = bootconfig.find(kKey + "=");
    if (pos != std::string::npos) {
        int val = 0;
        if (sscanf(bootconfig.c_str() + pos, "ro.boot.first_stage_console=%d", &val) == 1) {
            return val;
        }
    }

    // 2. Fallback to merged cmdline
    pos = cmdline.find(kKey + "=");
    if (pos != std::string::npos) {
        int val = 0;
        if (sscanf(cmdline.c_str() + pos, "ro.boot.first_stage_console=%d", &val) == 1) {
            return val;
        }
    }

    // 3. Try bootcfg global if available (optional fallback)
    const std::string from_bootcfg = minimal_systems::bootcfg::Get(kKey, "0");
    int fallback_val = 0;
    if (sscanf(from_bootcfg.c_str(), "%d", &fallback_val) == 1) {
        return fallback_val;
    }

    return 0;  // DISABLED
}

}  // namespace init
}  // namespace minimal_systems
