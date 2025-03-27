#ifndef MINIMAL_SYSTEMS_BOOTCFG_H_
#define MINIMAL_SYSTEMS_BOOTCFG_H_

#include <string>
#include <unordered_map>

namespace minimal_systems {
namespace bootcfg {

/**
 * @brief Initializes the bootcfg system by parsing kernel and user cmdline sources.
 *
 * Parses `/proc/cmdline` for kernel-provided boot parameters and merges any user-defined
 * overrides from a local `./.cmdline` file.
 *
 * This function is thread-safe and idempotent; it will only run once per process lifetime.
 * It must be invoked before calling Get(), IsEnabled(), or All().
 */
void Init();

/**
 * @brief Retrieves the value of a boot-time flag.
 *
 * Looks up the given key in the parsed bootcfg map and returns the corresponding value.
 * If the key does not exist, the provided default is returned instead.
 *
 * @param key Name of the boot configuration flag (e.g., "debug.boot").
 * @param def Default value to return if the flag is not present (defaults to empty string).
 * @return The string value of the flag or the default if not found.
 */
std::string Get(const std::string& key, const std::string& def = "");

/**
 * @brief Checks whether a given boot-time flag is explicitly enabled.
 *
 * A flag is considered enabled if it exists and its value is anything other than:
 * - "0"
 * - "false"
 *
 * All other values (e.g., "1", "true", "enabled", or any string) are treated as enabled.
 *
 * @param key Name of the boot configuration flag.
 * @return true if the flag is enabled, false otherwise.
 */
bool IsEnabled(const std::string& key);

/**
 * @brief Retrieves a read-only reference to all parsed boot flags.
 *
 * This provides full access to all boot-time key-value pairs loaded by Init().
 * Useful for diagnostics, testing, or full-dump logging.
 *
 * @return Const reference to the internal unordered map of flags.
 */
const std::unordered_map<std::string, std::string>& All();

}  // namespace bootcfg
}  // namespace minimal_systems

#endif  // MINIMAL_SYSTEMS_BOOTCFG_H_
