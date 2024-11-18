#define LOG_TAG "init_parser"
#include <iostream>
#include <filesystem>
#include <vector>
#include <cstdlib>
#include <cstring>
#include <stdexcept>
#include "log_new.h"
#include "property_manager.h"



namespace minimal_systems {
namespace init {

namespace fs = std::filesystem;

// Array of directories to parse during initialization
const std::vector<std::string> init_dirs = {
    "etc/init/",
    "usr/share/etc/init/",
    "oem/etc/init/"
};

// Helper function to parse initialization files in a directory
bool parse_init_files(const std::string& dir_path) {
    try {
        if (!fs::exists(dir_path)) {
            LOGE("Unable to read config directory '%s': No such file or directory", dir_path.c_str());
            return false;
        }

        for (const auto& entry : fs::directory_iterator(dir_path)) {
            if (entry.is_regular_file()) {
                LOGD("Parsing file %s ...", entry.path().c_str());
                // Additional file-specific parsing logic can be added here
            }
        }
        return true;

    } catch (const std::exception& ex) {
        LOGE("Exception while parsing directory '%s': %s", dir_path.c_str(), ex.what());
        return false;
    }
}

// Check if a file exists
bool file_exists(const std::string& filename) {
    try {
        return fs::exists(filename);
    } catch (const std::exception& ex) {
        LOGE("Exception while checking file '%s': %s", filename.c_str(), ex.what());
        return false;
    }
}

// Recovery-specific initialization
bool recovery_init() {
    auto& props = PropertyManager::instance();
    const std::string filename = props.get("ro.recovery.init_file", "init.rc");

    if (!file_exists(filename)) {
        LOGE("Unable to read recovery init file '%s': No such file or directory", filename.c_str());
        return false;
    }

    LOGD("Executing recovery init file: %s", filename.c_str());
    try {
        std::system(("/bin/bash " + filename).c_str());
        return true;
    } catch (const std::exception& ex) {
        LOGE("Exception while executing recovery init file '%s': %s", filename.c_str(), ex.what());
        return false;
    }
}

// Main initialization entry point
bool parse_init() {
    try {
        auto& props = PropertyManager::instance();

        // Get boot mode using getprop
        std::string boot_mode = props.get("ro.boot.mode", "normal");

        if (boot_mode == "recovery") {
            LOGI("Boot mode is 'recovery'; performing recovery-specific initialization.");
            return recovery_init();
        }

        if (boot_mode == "fastboot") {
            // lets setup the minimal dev environment
            LOGI("Boot mode is 'fastboot'; skipping initialization.");
            return true;
        }

        // Parse all init directories
        for (const auto& dir : init_dirs) {
            if (!parse_init_files(dir)) {
                LOGW("Failed to parse init directory: %s", dir.c_str());
            }
        }

        // Mark initialization as completed
        props.set("ro.init.completed", "true");
        LOGI("Initialization parsing completed successfully.");
        return true;

    } catch (const std::exception& ex) {
        LOGE("Exception in parse_init: %s", ex.what());
        return false;
    }
}

} // namespace init
} // namespace minimal_systems
