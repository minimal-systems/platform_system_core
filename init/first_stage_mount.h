#ifndef FIRST_STAGE_MOUNT_H
#define FIRST_STAGE_MOUNT_H

#include <memory>
#include <string>
#include <vector>
#include "result_ext.h"  // Ensure this file exists and provides `Result<T>`

namespace minimal_systems {
namespace init {

/**
 * @brief Abstract class representing the first stage mount process.
 *
 * This class provides a factory method to create a FirstStageMount instance and
 * defines virtual methods for device creation and fstab-based mounting.
 */
class FirstStageMount {
  public:
    virtual ~FirstStageMount() = default;

    /**
     * @brief Factory method to create a FirstStageMount instance.
     *
     * @param cmdline The bootloader command line.
     * @return A Result containing a unique pointer to a FirstStageMount instance or an error.
     */
    static Result<std::unique_ptr<FirstStageMount>> Create(const std::string& cmdline);

    /**
     * @brief Creates devices and logical partitions from storage devices.
     *
     * @return True if device creation is successful, false otherwise.
     */
    virtual bool DoCreateDevices() = 0;

    /**
     * @brief Mounts fstab entries read from the device tree.
     *
     * @return True if the first-stage mount is successful, false otherwise.
     */
    virtual bool DoFirstStageMount() = 0;

  protected:
    FirstStageMount() = default;
};

/**
 * @brief Sets the Init AVB version when in recovery mode.
 */
void SetInitAvbVersionInRecovery();

/**
 * @brief Loads the appropriate fstab based on the given paths.
 *
 * This function attempts to load the fstab file from the provided list of
 * paths. If no valid fstab is found, it falls back to "/etc/fstab".
 *
 * @param fstab_paths A vector of strings containing possible fstab paths.
 * @return True if a valid fstab file is loaded, false otherwise.
 */
bool LoadFstab(const std::vector<std::string>& fstab_paths);

/**
 * @brief Performs the first-stage mounting process.
 *
 * This function determines the current boot mode (normal or recovery) and loads
 * the corresponding fstab. It exits the process if no valid fstab is found.
 *
 * @return True if the first-stage mount is successful, false otherwise.
 */
bool PerformFirstStageMount();  // Renamed to prevent conflict with class

}  // namespace init
}  // namespace minimal_systems

#endif  // FIRST_STAGE_MOUNT_H
