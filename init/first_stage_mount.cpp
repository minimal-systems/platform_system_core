// first_stage_mount.cpp — Handles first-stage mounting during init

#define LOG_TAG "init"

#include "first_stage_mount.h"

#include <sys/stat.h>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "fs_mgr.h"
#include "log_new.h"
#include "property_manager.h"
#include "verify.h"

namespace minimal_systems {
namespace init {

/**
 * Normalize a file path by removing duplicate slashes.
 *
 * @param path The raw input path string.
 * @return A cleaned-up path string with redundant '/' removed.
 */
std::string normalize_path(const std::string& path) {
    std::string result;
    std::unique_copy(path.begin(), path.end(), std::back_inserter(result),
                     [](char a, char b) { return a == '/' && b == '/'; });
    return result;
}

/**
 * Parse a given fstab file and mount entries marked with 'verify' or 'overlay'.
 *
 * Supports overlayfs mounts explicitly. Logs all issues and aborts on fatal errors.
 *
 * @param filepath The absolute path to the fstab file.
 */
void parse_fstab_file(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        LOGE("Unable to open fstab file: '%s'. Check file path or permissions.", filepath.c_str());
        return;
    }

    LOGI("Parsing fstab file: '%s'", filepath.c_str());
    std::string line;
    int line_number = 0;

    while (std::getline(file, line)) {
        ++line_number;

        // Remove leading whitespace
        line.erase(0, line.find_first_not_of(" \t"));
        if (line.empty() || line[0] == '#') {
            continue;  // Skip comments and blank lines
        }

        // Parse expected fields from fstab line
        std::istringstream iss(line);
        std::string device, mount_point, filesystem, options;

        if (!(iss >> device >> mount_point >> filesystem >> options)) {
            LOGE("Failed to parse fstab entry on line %d. Skipping.", line_number);
            continue;
        }

        // Only process entries marked with 'verify' unless it's an overlay
        if (options.find("verify") == std::string::npos && filesystem != "overlay") {
            continue;
        }

        if (filesystem == "overlay") {
            // Mount overlayfs partition
            if (!minimal_systems::fs_mgr::MountOverlayFs(mount_point)) {
                LOGE("Overlay mount failed for mount point '%s' on line %d. Aborting.",
                     mount_point.c_str(), line_number);
                std::exit(-1);
            }
            continue;  // Skip further handling for overlay entries
        }

        // Warn if the device node does not exist
        struct stat buffer;
        if (stat(device.c_str(), &buffer) != 0) {
            LOGW("Device '%s' does not exist or cannot be opened. Proceeding with mount attempt.",
                 device.c_str());
        }

        // Attempt to mount the partition
        if (!minimal_systems::fs_mgr::MountPartition(device, mount_point, filesystem, options)) {
            LOGE("Mount operation failed for device '%s' at '%s' on line %d. Aborting.",
                 device.c_str(), mount_point.c_str(), line_number);
            std::exit(-1);
        }
    }

    LOGI("Fstab file parsing completed.");
}

/**
 * Try to load a valid fstab file from a list of candidate paths.
 *
 * Attempts normalized and unnormalized paths. Falls back to /etc/fstab if all else fails.
 *
 * @param fstab_paths A list of possible fstab file locations.
 * @return True if a valid fstab was found and parsed successfully; otherwise, exits.
 */
bool load_fstab(const std::vector<std::string>& fstab_paths) {
    auto& props = PropertyManager::instance();
    (void)props;
    std::string hardware = getprop("ro.boot.hardware");

    for (const auto& path : fstab_paths) {
        // Try original path first
        std::ifstream file(path);
        if (file) {
            LOGI("fstab '%s' found for hardware '%s' without normalization", path.c_str(),
                 hardware.c_str());
            parse_fstab_file(path);
            return true;
        }

        // Try normalized version of the path
        std::string normalized_path = normalize_path(path);
        file.open(normalized_path);
        if (file) {
            LOGI("fstab '%s' found for hardware '%s' with normalization", normalized_path.c_str(),
                 hardware.c_str());
            parse_fstab_file(normalized_path);
            return true;
        }

        LOGW("fstab '%s' not found; continuing to next option", path.c_str());
    }

    // Try fallback location: /etc/fstab
    std::ifstream fallback_file("/etc/fstab");
    if (fallback_file) {
        LOGI("Using fallback fstab '/etc/fstab' for hardware '%s'", hardware.c_str());
        parse_fstab_file("/etc/fstab");
        return true;
    }

    LOGE("No valid fstab found; device will reboot to bootloader.");
    std::exit(-1);
}

/**
 * Main entry for first-stage mount operations.
 *
 * Selects fstab file(s) based on boot mode (normal or recovery) and mounts
 * partitions flagged with 'verify' or overlayfs entries.
 *
 * @return True on successful mounting; exits on error.
 */
bool PerformFirstStageMount() {
    auto& props = PropertyManager::instance();
    (void)props;
    std::string boot_mode = getprop("ro.boot.mode");

    const std::vector<std::string> fstab_paths = {
        "etc/fstab",
        "usr/share/etc/fstab",
        "/etc/fstab"
    };

    if (boot_mode == "recovery") {
        LOGI("Boot mode: 'recovery'; attempting to load recovery fstab paths.");
        return load_fstab(fstab_paths);
    } else {
        LOGI("Boot mode: 'normal'; attempting to load normal fstab paths.");
        return load_fstab(fstab_paths);
    }

    // After overlayfs is mounted, system will continue to next init phase.
}

}  // namespace init
}  // namespace minimal_systems
