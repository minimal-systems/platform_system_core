// ueventhandler.h — Header for ueventd.rc rule processing

#ifndef MINIMAL_SYSTEMS_INIT_UEVENTHANDLER_H_
#define MINIMAL_SYSTEMS_INIT_UEVENTHANDLER_H_

#include <sys/types.h>
#include <string>
#include <unordered_map>
#include <vector>
#include <regex>

namespace minimal_systems {
namespace init {

struct DevicePermissionRule {
    std::string path_pattern;
    std::regex path_regex;
    mode_t mode;
    uid_t uid;
    gid_t gid;

    DevicePermissionRule(const std::string& pattern, const std::regex& regex, mode_t m, uid_t u,
                         gid_t g)
        : path_pattern(pattern), path_regex(regex), mode(m), uid(u), gid(g) {}
};

/**
 * Represents a subsystem rule like:
 * SUBSYSTEM=="block", KERNEL=="sd[a-z]", MODE="0660", GROUP="disk"
 */
struct SubsystemPermissionRule {
    std::string subsystem;
    std::string kernel;
    std::string group;
    gid_t gid = static_cast<gid_t>(-1);
    mode_t mode = 0660;
    std::unordered_map<std::string, std::string> attrs;  // e.g., ATTR{foo}="bar"
};

class UeventHandler {
  public:
    static void addDeviceRule(const std::string& path_pattern, mode_t mode, const std::string& user,
                              const std::string& group);

    static void addSubsystemRule(const std::string& raw_line);

    static bool parseRuleLine(const std::string& line);

    static void applyRulesToDevice(const std::string& device_path);
};

}  // namespace init
}  // namespace minimal_systems

#endif  // MINIMAL_SYSTEMS_INIT_UEVENTHANDLER_H_
