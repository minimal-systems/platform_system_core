/*
 * Copyright (C) 2025 Minimal Systems Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * You may not use this file except in compliance with the License.
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

#include <linux/reboot.h>
#include <sys/capability.h>
#include <sys/reboot.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <algorithm>
#include <csignal>
#include <cstdlib>
#include <fstream>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

#include "capabilities.h"
#include "log_new.h"
#include "reboot_utils.h"
#include "util.h"

namespace minimal_systems {

static std::string fatal_reboot_target = "bootloader";
static bool fatal_panic = false;

// Read file contents into a string
bool ReadFileToString(const std::string& path, std::string* content) {
    std::ifstream file(path);
    if (!file.is_open()) {
        LOGE("Failed to open file: %s", path.c_str());
        return false;
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    *content = buffer.str();
    return true;
}

// Extract boot parameters from /proc/cmdline
void SetFatalRebootTarget(const std::optional<std::string>& reboot_target) {
    std::string cmdline;
    ReadFileToString("/proc/cmdline", &cmdline);

    cmdline.erase(std::remove(cmdline.begin(), cmdline.end(), '\n'), cmdline.end());

    const std::string kFatalPanicParam = "init_fatal_panic";
    if (cmdline.find(kFatalPanicParam) == std::string::npos) {
        fatal_panic = false;
    } else {
        fatal_panic = cmdline.find(kFatalPanicParam + "=true") != std::string::npos;
    }

    if (reboot_target) {
        fatal_reboot_target = *reboot_target;
        return;
    }

    const std::string kRebootTargetString = "init_fatal_reboot_target=";
    size_t start_pos = cmdline.find(kRebootTargetString);
    if (start_pos != std::string::npos) {
        start_pos += kRebootTargetString.length();
        size_t end_pos = cmdline.find(' ', start_pos);
        fatal_reboot_target = cmdline.substr(start_pos, end_pos - start_pos);
    }
}

bool IsRebootCapable() {
    if (!CAP_IS_SUPPORTED(CAP_SYS_BOOT)) {
        LOGW("CAP_SYS_BOOT is not supported");
        return false;
    }

    cap_t caps = cap_get_proc();
    if (!caps) {
        LOGW("cap_get_proc() failed");
        return false;
    }

    cap_flag_value_t value = CAP_SET;
    if (cap_get_flag(caps, CAP_SYS_BOOT, CAP_EFFECTIVE, &value) != 0) {
        LOGW("cap_get_flag(CAP_SYS_BOOT, EFFECTIVE) failed");
        cap_free(caps);
        return false;
    }

    cap_free(caps);
    return value == CAP_SET;
}

void __attribute__((noreturn)) RebootSystem(unsigned int cmd, const std::string& reboot_target,
                                            const std::string& reboot_reason) {
    LOGI("Rebooting system...");

    if (!IsRebootCapable()) {
        LOGE("Reboot capability not available, exiting.");
        exit(0);
    }

    switch (cmd) {
        case RB_POWER_OFF:
            reboot(RB_POWER_OFF);
            break;
        case RB_AUTOBOOT:
            syscall(__NR_reboot, LINUX_REBOOT_MAGIC1, LINUX_REBOOT_MAGIC2,
                    LINUX_REBOOT_CMD_RESTART2, reboot_target.c_str());
            break;
        case RB_HALT_SYSTEM:
            reboot(RB_HALT_SYSTEM);
            break;
    }
    LOGE("Reboot call returned unexpectedly, aborting.");
    abort();
}

void __attribute__((noreturn)) FatalRebootHandler(int signal_number) {
    pid_t pid = fork();

    if (pid == -1) {
        RebootSystem(RB_AUTOBOOT, fatal_reboot_target, "");
    } else if (pid == 0) {
        sleep(5);
        RebootSystem(RB_AUTOBOOT, fatal_reboot_target, "");
    }

    LOGE("Fatal error detected. Signal: %d", signal_number);
    if (fatal_panic) {
        std::ofstream sysrq("/proc/sysrq-trigger");
        if (sysrq.is_open()) {
            sysrq << "c";
        } else {
            LOGE("Sys-Rq failed, falling back to exit.");
            _exit(signal_number);
        }
    }
    RebootSystem(RB_AUTOBOOT, fatal_reboot_target, "");
}

void InstallRebootSignalHandlers() {
    struct sigaction action;
    memset(&action, 0, sizeof(action));
    sigfillset(&action.sa_mask);
    action.sa_handler = [](int signal) {
        if (getpid() != 1) {
            _exit(signal);
        }
        FatalRebootHandler(signal);
    };
    action.sa_flags = SA_RESTART;
    sigaction(SIGABRT, &action, nullptr);
    sigaction(SIGBUS, &action, nullptr);
    sigaction(SIGFPE, &action, nullptr);
    sigaction(SIGILL, &action, nullptr);
    sigaction(SIGSEGV, &action, nullptr);
#if defined(SIGSTKFLT)
    sigaction(SIGSTKFLT, &action, nullptr);
#endif
    sigaction(SIGSYS, &action, nullptr);
    sigaction(SIGTRAP, &action, nullptr);
}

}  // namespace minimal_systems
