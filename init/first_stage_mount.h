#ifndef FIRST_STAGE_MOUNT_H
#define FIRST_STAGE_MOUNT_H

#include <string>
#include <vector>

namespace minimal_systems {
namespace init {

/**
 * @brief Loads the appropriate fstab based on the given paths.
 *
 * This function attempts to load the fstab file from the provided list of
 * paths. If no valid fstab is found, it falls back to "/etc/fstab".
 *
 * @param fstab_paths A vector of strings containing possible fstab paths.
 * @return true if a valid fstab file is loaded, false otherwise.
 */
bool load_fstab(const std::vector<std::string>& fstab_paths);

/**
 * @brief Performs the first-stage mounting process.
 *
 * This function determines the current boot mode (normal or recovery) and loads
 * the corresponding fstab. It exits the process if no valid fstab is found.
 *
 * @return true if the first-stage mount is successful, false otherwise.
 */
bool FirstStageMount();

}  // namespace init
}  // namespace minimal_systems

#endif  // FIRST_STAGE_MOUNT_H
