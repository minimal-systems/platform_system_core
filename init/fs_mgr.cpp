#define LOG_TAG "fs_mgr"
#include "fs_mgr.h"

#include <errno.h>
#include <sys/mount.h>  // For mount system call

#include <cstdlib>  // For system() call (if needed for real fsck)
#include <cstring>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include "log_new.h"  // For LOGE, LOGI, LOGD, LOGW

namespace minimal_systems {
namespace fs_mgr {

// Helper function to read a file and return its contents
std::string ReadFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        LOGE("Failed to open '%s'.", path.c_str());
        return "";
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

// Helper function to parse key-value pairs from a string
bool ParseKeyValue(const std::string& data, const std::string& key, std::string* value) {
    std::istringstream iss(data);
    std::string token;
    while (iss >> token) {
        size_t pos = token.find('=');
        if (pos != std::string::npos) {
            std::string k = token.substr(0, pos);
            std::string v = token.substr(pos + 1);
            if (k == key) {
                *value = v;
                LOGD("Found key '%s' with value '%s'.", key.c_str(), value->c_str());
                return true;
            }
        }
    }
    LOGW("Key '%s' not found.", key.c_str());
    return false;
}

// Reads the kernel command line and searches for the specified key
bool GetKernelCmdline(const std::string& key, std::string* value) {
    std::string cmdline = ReadFile("/proc/cmdline");
    return ParseKeyValue(cmdline, key, value);
}

// Reads the bootconfig file and searches for the specified key
bool GetBootconfig(const std::string& key, std::string* value) {
    std::string bootconfig = ReadFile("/proc/bootconfig");
    return ParseKeyValue(bootconfig, key, value);
}

bool FsckPartition(const std::string& device, const std::string& filesystem) {
    std::string command;

    if (filesystem == "ext4") {
        command = "e2fsck -y " + device;
    } else if (filesystem == "fat32") {
        command = "dosfsck -a " + device;
    } else {
        LOGW("Unsupported filesystem '%s' for fsck on device '%s'. Skipping fsck.",
             filesystem.c_str(), device.c_str());
        return true;  // Skipping unsupported filesystem
    }

    // TODO: Uncomment the following lines to execute the command
    /*
    int ret = system(command.c_str());
    if (ret != 0) {
        LOGE("Filesystem check failed for device: '%s' with filesystem '%s'. Return code: %d.",
             device.c_str(), filesystem.c_str(), ret);
        return false;
    }
    LOGI("Filesystem check completed successfully for device: '%s' with filesystem '%s'.",
         device.c_str(), filesystem.c_str());
    */

    return true;
}

bool MountPartition(const std::string& device, const std::string& mount_point,
                    const std::string& filesystem, const std::string& options) {
    if (filesystem == "ext4" || filesystem == "fat32") {
        if (!FsckPartition(device, filesystem)) {
            LOGE("Filesystem check failed on %s with type %s.", device.c_str(), filesystem.c_str());
            return false;
        }
    } else if (filesystem == "overlay") {
        LOGI("Delegating overlay filesystem mounting to MountOverlayFs for %s.",
             mount_point.c_str());
        if (!MountOverlayFs(mount_point)) {
            LOGE("Failed to mount overlay filesystem at %s.", mount_point.c_str());
            return false;
        }
        LOGI("Successfully mounted overlay filesystem at %s.", mount_point.c_str());
        return true;
    } else {
        LOGW("Unsupported filesystem type %s on %s. Mount skipped.", filesystem.c_str(),
             device.c_str());
        return false;
    }

    LOGI("Attempting to mount %s at %s with filesystem type %s and options: %s.", device.c_str(),
         mount_point.c_str(), filesystem.c_str(), options.c_str());

    // TODO: Add real mount logic here when needed
    /*
    int ret = mount(device.c_str(), mount_point.c_str(), filesystem.c_str(),
        MS_RELATIME, options.c_str());
    if (ret != 0) {
    LOGE("__mount(source=%s,target=%s,type=%s)=1: Failed. Error: %s.",
    device.c_str(), mount_point.c_str(), filesystem.c_str(), strerror(errno));
    return false;
    }

    LOGI("__mount(source=%s,target=%s,type=%s)=0: Success.",
    device.c_str(), mount_point.c_str(), filesystem.c_str());
    */

    // Simulate mount success for logging purposes
    LOGI("__mount(source=%s,target=%s,type=%s)=0: Success.", device.c_str(), mount_point.c_str(),
         filesystem.c_str());
    LOGI("Mount operation completed for %s at %s.", device.c_str(), mount_point.c_str());
    return true;
}

bool MountOverlayFs(const std::string& mount_point) {
    LOGI("Preparing to mount overlay filesystem at %s.", mount_point.c_str());

    const std::vector<std::string> dirs = {"./mnt/overlay/lower", "./mnt/overlay/updater",
                                           "./mnt/overlay/upper", "./mnt/overlay/work",
                                           mount_point};

    // Create directories if they do not exist
    for (const auto& dir : dirs) {
        try {
            if (!std::filesystem::exists(dir)) {
                std::filesystem::create_directories(dir);
                LOGI("Created missing directory: %s", dir.c_str());
            }
        } catch (const std::filesystem::filesystem_error& e) {
            LOGE("Error creating directory %s: %s", dir.c_str(), e.what());
            return false;
        }
    }

    const std::string overlay_options =
            "lowerdir=" + dirs[0] + ":" + dirs[1] + ",upperdir=" + dirs[2] + ",workdir=" + dirs[3];

    LOGI("Mounting overlay with options: %s", overlay_options.c_str());

    // Perform the mount operation
    // if (mount("overlay", mount_point.c_str(), "overlay", MS_NODEV | MS_NOEXEC | MS_NOSUID,
    //          overlay_options.c_str()) != 0) {
    //    LOGE("Failed to mount overlay filesystem at %s: %s", mount_point.c_str(),
    //    strerror(errno)); return false;
    //}

    LOGI("Successfully mounted overlay filesystem at %s", mount_point.c_str());
    return true;
}

// Reads the bootconfig and extracts a value for a given key
void GetBootconfigFromString(const std::string& bootconfig, const std::string& key,
                             std::string* out_value) {
    size_t pos = bootconfig.find(key + "=");
    if (pos != std::string::npos) {
        size_t end_pos = bootconfig.find('\n', pos);
        *out_value = bootconfig.substr(pos + key.length() + 1, end_pos - pos - key.length() - 1);
    } else {
        out_value->clear();
    }
}

}  // namespace fs_mgr
}  // namespace minimal_systems
