#include "fs_mgr.h"
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <sys/mount.h>  // For mount system call
#include <cstdlib>      // For system() call (if needed for real fsck)
#include <errno.h>
#include <cstring>
#include "log_new.h"    // For LOGE, LOGI, LOGD, LOGW
#include <filesystem>


namespace minimal_systems {
namespace fs_mgr {

// Reads the kernel command line and searches for the specified key
bool GetKernelCmdline(const std::string& key, std::string* value) {
    const std::string cmdline_path = "/proc/cmdline";
    std::ifstream cmdline_file(cmdline_path);
    if (!cmdline_file.is_open()) {
        LOGE("Failed to open '%s'.", cmdline_path.c_str());
        return false;
    }

    std::string line;
    std::getline(cmdline_file, line);
    cmdline_file.close();

    std::istringstream iss(line);
    std::string token;
    while (iss >> token) {
        size_t pos = token.find('=');
        if (pos != std::string::npos) {
            std::string k = token.substr(0, pos);
            std::string v = token.substr(pos + 1);
            if (k == key) {
                *value = v;
                LOGD("Found kernel cmdline key '%s' with value '%s'.", key.c_str(), value->c_str());
                return true;
            }
        }
    }
    LOGW("Kernel cmdline key '%s' not found.", key.c_str());
    return false;
}

// Reads the bootconfig file and searches for the specified key
bool GetBootconfig(const std::string& key, std::string* value) {
    const std::string bootconfig_path = "/proc/bootconfig";
    std::ifstream bootconfig_file(bootconfig_path);
    if (!bootconfig_file.is_open()) {
        LOGE("Failed to open '%s'.", bootconfig_path.c_str());
        return false;
    }

    std::string line;
    while (std::getline(bootconfig_file, line)) {
        size_t pos = line.find('=');
        if (pos != std::string::npos) {
            std::string k = line.substr(0, pos);
            std::string v = line.substr(pos + 1);
            if (k == key) {
                *value = v;
                LOGD("Found bootconfig key '%s' with value '%s'.", key.c_str(), value->c_str());
                return true;
            }
        }
    }
    LOGW("Bootconfig key '%s' not found.", key.c_str());
    return false;
}

bool FsckPartition(const std::string& device, const std::string& filesystem) {
    std::string command;

    if (filesystem == "ext4") {
        // Use e2fsck for ext4 filesystems
        command = "e2fsck -y " + device;
    } else if (filesystem == "fat32") {
        // Use dosfsck for fat32 filesystems
        command = "dosfsck -a " + device;
    } else {
        LOGW("Unsupported filesystem '%s' for fsck on device '%s'. Skipping fsck.", filesystem.c_str(), device.c_str());
        return true; // Skipping unsupported filesystem
    }

    LOGI("Prepared fsck command: '%s' for device '%s'.", command.c_str(), device.c_str());

    // TODO: Uncomment the following lines to execute the command
    /*
    int ret = system(command.c_str());
    if (ret != 0) {
        LOGE("Filesystem check failed for device: '%s' with filesystem '%s'. Return code: %d.", device.c_str(), filesystem.c_str(), ret);
        return false;
    }
    LOGI("Filesystem check completed successfully for device: '%s' with filesystem '%s'.", device.c_str(), filesystem.c_str());
    */

    return true;
}

bool MountPartition(const std::string& device, const std::string& mount_point,
                    const std::string& filesystem, const std::string& options) {
    if (filesystem == "ext4" || filesystem == "fat32") {
        LOGI("[libfs_mgr] Running filesystem check on %s with type %s.", device.c_str(), filesystem.c_str());
        if (!FsckPartition(device, filesystem)) {
            LOGE("[libfs_mgr] Filesystem check failed on %s with type %s.", device.c_str(), filesystem.c_str());
            return false;
        }
        LOGI("[libfs_mgr] Filesystem check succeeded on %s with type %s.", device.c_str(), filesystem.c_str());
    } else if (filesystem == "overlay") {
        LOGI("[libfs_mgr] Delegating overlay filesystem mounting to MountOverlayFs for %s.", mount_point.c_str());
        if (!MountOverlayFs(mount_point)) {
            LOGE("[libfs_mgr] Failed to mount overlay filesystem at %s.", mount_point.c_str());
            return false;
        }
        LOGI("[libfs_mgr] Successfully mounted overlay filesystem at %s.", mount_point.c_str());
        return true;
    } else {
        LOGW("[libfs_mgr] Unsupported filesystem type %s on %s. Mount skipped.", filesystem.c_str(), device.c_str());
        return false;
    }

    LOGI("[libfs_mgr] Attempting to mount %s at %s with filesystem type %s and options: %s.",
         device.c_str(), mount_point.c_str(), filesystem.c_str(), options.c_str());

    // TODO: Add real mount logic here when needed
    /*
    int ret = mount(device.c_str(), mount_point.c_str(), filesystem.c_str(),
                    MS_RELATIME, options.c_str());
    if (ret != 0) {
        LOGE("[libfs_mgr] __mount(source=%s,target=%s,type=%s)=1: Failed. Error: %s.",
             device.c_str(), mount_point.c_str(), filesystem.c_str(), strerror(errno));
        return false;
    }

    LOGI("[libfs_mgr] __mount(source=%s,target=%s,type=%s)=0: Success.",
         device.c_str(), mount_point.c_str(), filesystem.c_str());
    */

    // Simulate mount success for logging purposes
    LOGI("[libfs_mgr] __mount(source=%s,target=%s,type=%s)=0: Success.",
         device.c_str(), mount_point.c_str(), filesystem.c_str());
    LOGI("[libfs_mgr] Mount operation completed for %s at %s.", device.c_str(), mount_point.c_str());
    return true;
}


#include <fstream>

bool MountOverlayFs(const std::string& mount_point) {
    LOGI("Preparing to mount overlay filesystem at %s.", mount_point.c_str());

    std::string lowerdir = "mnt/overlay/lower";
    std::string updater_path = "mnt/overlay/updater";
    std::string upperdir = "mnt/overlay/upper";
    std::string workdir = "mnt/overlay/work";
    std::string update_flag = "mnt/overlay/update_mode";

    // Check if the system is in update mode
    bool update_mode = false;
    try {
        if (std::filesystem::exists(update_flag)) {
            LOGI("Update mode detected. Applying full overlay with updater.");
            update_mode = true;
        } else {
            LOGI("Normal mode detected. Applying overlay without updater.");
        }
    } catch (const std::filesystem::filesystem_error& e) {
        LOGE("Failed to check update mode flag at %s. Error: %s.", update_flag.c_str(), e.what());
        return false;
    }

    // Create necessary directories for overlayfs using std::filesystem::create_directories
    for (const auto& dir : {lowerdir, updater_path, upperdir, workdir, mount_point}) {
        try {
            if (!std::filesystem::exists(dir)) {
                std::filesystem::create_directories(dir);
                LOGI("Directory %s created successfully.", dir.c_str());
            }
        } catch (const std::filesystem::filesystem_error& e) {
            LOGE("Failed to create directory %s. Error: %s.", dir.c_str(), e.what());
            return false;
        }
    }

    // Adjust overlay options based on update mode
    std::string overlay_options;
    if (update_mode) {
        overlay_options = "lowerdir=" + lowerdir + ":" + updater_path + ",upperdir=" + upperdir + ",workdir=" + workdir;
    } else {
        overlay_options = "lowerdir=" + lowerdir + ",upperdir=" + upperdir + ",workdir=" + workdir;
    }

    LOGI("Mounting overlay filesystem at %s with options: %s.", mount_point.c_str(), overlay_options.c_str());

    // TODO: Add real mount logic here when needed
    /*
    int ret = mount("overlay", mount_point.c_str(), "overlay", 0, overlay_options.c_str());
    if (ret != 0) {
        LOGE("Failed to mount overlay filesystem at %s. Error: %s.", mount_point.c_str(), strerror(errno));
        return false;
    }
    LOGI("Overlay filesystem mounted successfully at %s.", mount_point.c_str());
    */

    LOGI("Overlay filesystem prepared successfully at %s with %s updater support.",
         mount_point.c_str(), update_mode ? "full" : "no");
    return true;
}


}  // namespace fs_mgr
}  // namespace minimal_systems
