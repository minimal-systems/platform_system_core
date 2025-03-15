#ifndef MINIMAL_SYSTEMS_INIT_SELINUX_H_
#define MINIMAL_SYSTEMS_INIT_SELINUX_H_

#include <memory>
#include <string>
#include <vector>

namespace minimal_systems {
namespace init {

enum EnforcingStatus { SELINUX_PERMISSIVE, SELINUX_ENFORCING };

struct SELinuxEntry {
    std::string entry;
    std::unique_ptr<SELinuxEntry> next;
};

EnforcingStatus StatusFromProperty();
bool IsEnforcing();
int SetupSelinux(char** argv);

#ifndef RECOVERY_INIT
int load_selinux_ignored();
#endif

}  // namespace init
}  // namespace minimal_systems

#endif  // MINIMAL_SYSTEMS_INIT_SELINUX_H_
