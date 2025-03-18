#ifndef FS_MGR_H
#define FS_MGR_H

#include <string>

namespace minimal_systems {
namespace fs_mgr {

// Reads the kernel command line and searches for the specified key
bool GetKernelCmdline(const std::string& key, std::string* value);

// Reads the bootconfig file and searches for the specified key
bool GetBootconfig(const std::string& key, std::string* value);

/**
 * @brief Runs a filesystem check on a specified device based on its filesystem
 * type.
 *
 * @param device The path to the device to be checked.
 * @param filesystem The filesystem type (e.g., ext4, fat32).
 * @return True if the filesystem check is successful, false otherwise.
 */
bool FsckPartition(const std::string& device, const std::string& filesystem);

// Handles mounting a partition
bool MountPartition(const std::string& device, const std::string& mount_point,
                    const std::string& filesystem, const std::string& options);

/**
 * @brief Prepares and mounts an overlay filesystem.
 *
 * @param mount_point The target mount point for the overlay filesystem.
 * @return True if the overlay filesystem was successfully mounted (or
 * simulated), false otherwise.
 */
bool MountOverlayFs(const std::string& mount_point);

/**
 * Extracts a key-value pair from a boot configuration string.
 * @param bootconfig The full boot configuration string.
 * @param key The key to search for.
 * @param out_value Pointer to store the extracted value.
 */
void GetBootconfigFromString(const std::string& bootconfig, const std::string& key,
                             std::string* out_value);

}  // namespace fs_mgr
}  // namespace minimal_systems

#endif  // FS_MGR_H
