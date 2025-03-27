// ueventgroups.cpp — Maps fallback Android-style group names to hardcoded GIDs

#include "ueventgroups.h"

#include <unordered_map>

namespace minimal_systems {
namespace init {

gid_t resolve_known_group(const std::string& group) {
    static const std::unordered_map<std::string, gid_t> known_groups = {
        {"root", 0},
        {"system", 1000},
        {"radio", 1001},
        {"bluetooth", 1002},
        {"graphics", 1003},
        {"input", 1004},
        {"audio", 1041},
        {"camera", 1006},
        {"log", 1007},
        {"compass", 1008},
        {"mount", 1009},
        {"wifi", 1010},
        {"adb", 1011},
        {"install", 1012},
        {"media", 1013},
        {"dhcp", 1014},
        {"sdcard_rw", 1015},
        {"vpn", 1016},
        {"keystore", 1017},
        {"usb", 1018},
        {"cache", 2001},
        {"diag", 2500},
        {"net_bt", 3001},
        {"net_bt_admin", 3002},
        {"gps", 3003},
        {"nfc", 3004},
        {"media_rw", 1023},
        {"shell", 2000},
        {"i2c", 1019},
        {"tty", 5},
        {"disk", 6},
        {"video", 44},
        {"network", 3005},
        {"dialout", 20},
        {"gpio", 27},
        {"net", 3006},
        {"net_admin", 3007},
        {"net_raw", 3008},
        {"netlink", 3009},
        {"netlink_route", 3010},
        {"netlink_audit", 3011},
        {"netlink_diag", 3012},
        {"netlink_ipsec", 3013},
        {"netlink_xfrm", 3014},
        {"netlink_nlmsg", 3015},
        {"netlink_nlmon", 3016}
    };

    auto it = known_groups.find(group);
    return it != known_groups.end() ? it->second : static_cast<gid_t>(-1);
}

}  // namespace init
}  // namespace minimal_systems
