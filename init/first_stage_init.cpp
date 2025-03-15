#define LOG_TAG "init"

#include <fcntl.h>
#include <linux/mount.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <unistd.h>

#include <algorithm>
#include <fstream>
#include <regex>
#include <string>
#include <vector>

#include "log_new.h"
#include "property_manager.h"
#include "util.h"
#include "verify.h"

namespace minimal_systems {
namespace init {

enum class BootMode {
    NORMAL_MODE,
    RECOVERY_MODE,
    CHARGER_MODE,
};

void FreeRamdisk() {
    if (!IsRunningInRamdisk()) {
        LOGI("Not running in a ramdisk, skipping cleanup.");
        return;
    }

    const std::string ramdisks[] = {"/dev/ram0", "/dev/initrd"};

    for (const auto& ramdisk : ramdisks) {
        if (umount(ramdisk.c_str()) == 0) {
            LOGI("Unmounted %s", ramdisk.c_str());
        } else {
            LOGE("Failed to unmount %s: %s", ramdisk.c_str(), strerror(errno));
        }

        if (unlink(ramdisk.c_str()) == 0) {
            LOGI("Removed %s", ramdisk.c_str());
        } else {
            LOGE("Failed to remove %s: %s", ramdisk.c_str(), strerror(errno));
        }
    }
}

bool IsChargerMode() {
    return GetProperty("ro.bootmode") == "charger";
}

bool IsRecoveryMode() {
    return GetProperty("ro.bootmode") == "recovery";
}

bool IsNormalBootForced() {
    return GetProperty("ro.bootmode") == "normal";
}

const bool ktestingflag = true;

int LoadKernelModule(const std::string& module_path) {
    int fd = open(module_path.c_str(), O_RDONLY | O_CLOEXEC);
    if (fd < 0) {
        LOGE("Failed to open module: %s, error: %s", module_path.c_str(), strerror(errno));
        // If the testing flag is set, don't return immediately
        if (!ktestingflag) {
            return -1;
        }
    }

    if (syscall(__NR_finit_module, fd, "", 0) != 0) {
        LOGE("Failed to load module %s: %s", module_path.c_str(), strerror(errno));
        close(fd);
        // If the testing flag is set, don't return immediately
        if (!ktestingflag) {
            return -1;
        }
    }

    LOGI("Loaded module: %s", module_path.c_str());
    close(fd);
    return 0;
}

/**
 * Loads kernel modules from a file, skipping blacklisted ones.
 */
int LoadKernelModulesFromFile(const std::string& list_path,
                              const std::vector<std::string>& blacklist) {
    std::ifstream file(list_path);
    if (!file.is_open()) {
        LOGE("Cannot open module list: %s", list_path.c_str());
        return -1;
    }

    std::string module_name;
    int failed_count = 0;

    // Determine the base directory of the module list.
    std::string module_list_dir = list_path.substr(0, list_path.find_last_of('/'));

    while (std::getline(file, module_name)) {
        if (module_name.empty()) continue;

        // Skip blacklisted modules using regex matching.
        if (std::any_of(blacklist.begin(), blacklist.end(), [&](const std::string& bl_mod) {
                return std::regex_match(module_name, std::regex(bl_mod));
            })) {
            LOGW("Skipping blacklisted module: %s", module_name.c_str());
            continue;
        }

        // Load module from the same directory as modules-load.list
        std::string module_path = JoinPath(module_list_dir, module_name);

        if (LoadKernelModule(module_path) != 0) {
            setprop("ro.boot.module_load_error", module_name);
            failed_count++;
            LOGW("Continuing despite failed module: %s", module_name.c_str());
        } else {
            LOGI("Successfully loaded module: %s", module_name.c_str());
        }
    }

    return failed_count == 0 ? 0 : -1;
}

/**
 * Loads kernel modules from multiple paths, checking blacklists.
 */
int LoadKernelModules() {
    struct ModulePath {
        std::string path;
        std::string prop_key;
    };

    std::vector<ModulePath> module_paths = {
            {getprop("ro.boot.modules") + "/modules-load.list", "ro.boot.module_load_custom"},
            {"/lib/modules/modules-load.list", "ro.boot.module_load_primary"},
            {"/etc/modules-load.d/modules.conf", "ro.boot.module_load_primary"},
            {"/etc/modules", "ro.boot.module_load_primary"},
            {"/usr/lib/modules/modules-load.list", "ro.boot.module_load_primary"},
            {"./lib/modules/modules-load.list", "ro.boot.module_load_primary"},
            {"./firmware/lib/modules/module_load.list", "ro.boot.module_load_fallback"}};

    // Read and parse blacklist modules
    std::vector<std::string> blacklist_modules;
    std::string blacklist = getprop("ro.vendor.modules.blacklist");

    if (!blacklist.empty()) {
        size_t start = 0, end;
        while ((end = blacklist.find(',', start)) != std::string::npos) {
            blacklist_modules.emplace_back(blacklist.substr(start, end - start));
            start = end + 1;
        }
        if (start < blacklist.length()) {
            blacklist_modules.emplace_back(blacklist.substr(start));
        }
    }

    if (!blacklist_modules.empty()) {
        LOGI("Blacklisted modules:");
        for (const auto& mod : blacklist_modules) {
            LOGI(" - %s", mod.c_str());
        }
    }

    bool has_errors = false;

    for (const auto& module : module_paths) {
        if (module.path.empty() || !FileExists(module.path)) {
            LOGW("Skipping missing module list: %s", module.path.c_str());
            continue;
        }

        LOGI("Loading kernel modules from: %s", module.path.c_str());
        setprop(module.prop_key, "1");

        if (LoadKernelModulesFromFile(module.path, blacklist_modules) != 0) {
            LOGW("Errors loading kernel modules from: %s", module.path.c_str());
            has_errors = true;
        }

        if (module.prop_key != "ro.boot.module_load_custom") {
            break;
        }
    }

    return has_errors ? -1 : 0;
}

BootMode GetBootMode() {
    if (IsChargerMode()) return BootMode::CHARGER_MODE;
    if (IsRecoveryMode() && !IsNormalBootForced()) return BootMode::RECOVERY_MODE;
    return BootMode::NORMAL_MODE;
}

int FirstStageMain(int argc, char** argv) {
    std::string cmdline = ReadFirstLine(NormalizePath("/proc/cmdline"));
    if (cmdline.empty()) {
        cmdline = ReadFirstLine("./proc/cmdline");
    }

    if (LoadKernelModules() != 0) {
        LOGE("Kernel module loading failed");
        return -1;
    }

    ExtractRootUUID(cmdline);

    DetectAndSetGPUType();
    FreeRamdisk();
    return 0;
}

}  // namespace init
}  // namespace minimal_systems
