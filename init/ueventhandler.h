#ifndef MINIMAL_SYSTEMS_INIT_UEVENTHANDLER_H_
#define MINIMAL_SYSTEMS_INIT_UEVENTHANDLER_H_

#include <string>
#include <vector>

namespace minimal_systems {
namespace init {

struct DevicePermissionRule {
    std::string path_pattern;
    mode_t mode;
    uid_t uid;
    gid_t gid;
};

class UeventHandler {
  public:
    static void addDeviceRule(const std::string& path_pattern, mode_t mode,
                              const std::string& user, const std::string& group);

    static void applyRulesToDevice(const std::string& device_path);
    static bool parseRuleLine(const std::string& line);

};

}  // namespace init
}  // namespace minimal_systems

#endif  // MINIMAL_SYSTEMS_INIT_UEVENTHANDLER_H_
