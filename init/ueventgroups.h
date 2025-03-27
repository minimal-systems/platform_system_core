// ueventgroups.h — Provides fallback GID values for common Android group names

#ifndef MINIMAL_SYSTEMS_INIT_UEVENTGROUPS_H_
#define MINIMAL_SYSTEMS_INIT_UEVENTGROUPS_H_

#include <string>
#include <unordered_map>
#include <sys/types.h>

namespace minimal_systems {
namespace init {

/**
 * Resolve a group name to a fallback GID.
 * This is used when getgrnam() fails to resolve the group.
 *
 * @param group The group name to look up
 * @return gid_t value if known, otherwise -1
 */
gid_t resolve_known_group(const std::string& group);

}  // namespace init
}  // namespace minimal_systems

#endif  // MINIMAL_SYSTEMS_INIT_UEVENTGROUPS_H_
