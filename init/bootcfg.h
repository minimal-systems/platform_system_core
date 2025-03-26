#ifndef MINIMAL_SYSTEMS_BOOTCFG_H_
#define MINIMAL_SYSTEMS_BOOTCFG_H_

#include <string>
#include <unordered_map>

namespace minimal_systems {
namespace bootcfg {

/**
 * Initializes and merges /proc/cmdline and ./.cmdline.
 * Must be called once at early boot.
 */
void Init();

/**
 * Get the value of a boot-time flag, or return default if not found.
 */
std::string Get(const std::string& key, const std::string& def = "");

/**
 * Returns true if the flag exists and is not "0"/"false".
 */
bool IsEnabled(const std::string& key);

/**
 * Access all parsed flags for diagnostics or testing.
 */
const std::unordered_map<std::string, std::string>& All();

}  // namespace bootcfg
}  // namespace minimal_systems

#endif  // MINIMAL_SYSTEMS_BOOTCFG_H_
