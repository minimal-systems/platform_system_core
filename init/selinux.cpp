#define LOG_TAG "selinux"

#include "selinux.h"
#include "util.h"
#include "boot_clock.h"
#include "property_manager.h"
#include "fs_mgr.h"
#include "log_new.h"
#include <fstream>
#include <dirent.h>
#include <cstring>
#include <sys/stat.h>
#include <memory>


namespace minimal_systems {
namespace init {

const std::vector<std::string> selinux_whitelist = {
    "/etc/selinux",
    "/oem/etc/selinux",
    "/usr/share/etc/selinux"
};

std::unique_ptr<SELinuxEntry> selinux_entries_head;

EnforcingStatus StatusFromProperty() {
    std::string value;
    if (minimal_systems::fs_mgr::GetKernelCmdline("sysboot.selinux", &value) && value == "permissive") {
        return SELINUX_PERMISSIVE;
    }
    if (minimal_systems::fs_mgr::GetBootconfig("sysboot.selinux", &value) && value == "permissive") {
        return SELINUX_PERMISSIVE;
    }
    return SELINUX_ENFORCING;
}

bool IsEnforcing() {
#ifdef ALLOW_PERMISSIVE_SELINUX
    return StatusFromProperty() == SELINUX_ENFORCING;
#else
    return true;
#endif
}

bool is_whitelisted_path(const std::string& path) {
    for (const auto& whitelist_path : selinux_whitelist) {
        if (whitelist_path == path) {
            return true;
        }
    }
    return false;
}

void store_selinux_entry(const std::string& entry) {
    auto new_entry = std::make_unique<SELinuxEntry>();
    new_entry->entry = entry;
    new_entry->next = std::move(selinux_entries_head);
    selinux_entries_head = std::move(new_entry);
}

void parse_selinux_config(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        LOGE("Failed to open SELinux config file: %s", filepath.c_str());
        return;
    }

    auto& props = PropertyManager::instance();

    std::string line;
    std::string selinux_state = "unknown";
    std::string selinux_type = "unknown";

    LOGI("Parsing SELinux configuration from %s", filepath.c_str());
    while (std::getline(file, line)) {
        line.erase(0, line.find_first_not_of(" \t")); // Trim leading whitespace

        if (line.empty() || line[0] == '#') {
            continue;
        }

        if (line.find("SELINUX=") == 0) {
            selinux_state = line.substr(8);
        } else if (line.find("SELINUXTYPE=") == 0) {
            selinux_type = line.substr(12);
        }
    }

    if (selinux_state == "disabled") {
        LOGW("SELinux is disabled in the configuration. Switching to permissive mode.");
        selinux_state = "permissive";
        props.set("ro.sysboot.selinux", "false");
    } else {
        props.set("ro.sysboot.selinux", "true");
    }

    props.set("ro.boot.selinux", selinux_state);
    props.set("ro.boot.selinux_type", selinux_type);

    LOGI("SELinux state: %s", selinux_state.c_str());
    LOGI("SELinux type: %s", selinux_type.c_str());
}

void parse_cmdline_for_selinux() {
    std::ifstream file("/proc/cmdline");
    if (!file.is_open()) {
        LOGE("Unable to read /proc/cmdline");
        return;
    }

    auto& props = PropertyManager::instance();

    std::string cmdline;
    if (std::getline(file, cmdline)) {
        if (cmdline.find("sysboot.selinux.permissive=true") != std::string::npos) {
            LOGW("Detected SELinux permissive mode from kernel command line.");
            props.set("ro.sysboot.selinux", "false");
        }
    }
}

void parse_selinux_file(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        LOGE("Failed to open SELinux policy file: %s", filepath.c_str());
        return;
    }

    std::string line;
    LOGI("Parsing SELinux policy from file: %s", filepath.c_str());
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#' || line.find("system_u:object_") == std::string::npos) {
            continue;
        }

        store_selinux_entry(line);
    }
}

void traverse_and_parse(const std::string& dir_path) {
    DIR* dir = opendir(dir_path.c_str());
    if (!dir) {
        LOGE("Failed to open SELinux directory: %s", dir_path.c_str());
        return;
    }

    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        if (std::strcmp(entry->d_name, ".") == 0 || std::strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        std::string full_path = dir_path + "/" + entry->d_name;

        if (entry->d_type == DT_DIR) {
            traverse_and_parse(full_path);
        } else if (entry->d_type == DT_REG) {
            parse_selinux_file(full_path);
        }
    }

    closedir(dir);
}

int SetupSelinux(char** argv) {
    SetStdioToDevNull(argv);
    InitKernelLogging(argv);

    //TODO: Implement reboot to bootloader on SELinux panic
    //if (REBOOT_BOOTLOADER_ON_PANIC) {
    //    InstallRebootSignalHandlers();
    //}


    bool loaded = false;
    auto& props = PropertyManager::instance();
    LOGI("Starting SELinux initialization.");

    parse_selinux_config("/etc/selinux/config");
    parse_cmdline_for_selinux();

    for (const auto& selinux_path : selinux_whitelist) {
        LOGI("Scanning SELinux directory: %s", selinux_path.c_str());
        traverse_and_parse(selinux_path);
        loaded = true;
    }

    if (!loaded) {
        LOGE("No valid SELinux policy found in whitelisted directories.");
        return 0;
    }

    std::string selinux_mode = props.get("ro.boot.selinux", "enforcing");
    if (selinux_mode == "permissive") {
        LOGI("SELinux is in permissive mode.");
    } else {
        LOGI("SELinux is in enforcing mode.");
    }

    return 1;
}

#ifndef RECOVERY_INIT
int load_selinux_ignored() {
    LOGI("Skipping SELinux initialization (RECOVERY_INIT not defined).");
    auto& props = PropertyManager::instance();
    props.set("ro.selinux_ignored_enabled", "true");
    return 0;
}
#endif

}  // namespace init
}  // namespace minimal_systems
