// ueventhandler.cpp — Handles device permission rules like those in ueventd.rc

#include "ueventhandler.h"

#include <fcntl.h>
#include <pwd.h>
#include <grp.h>
#include <regex>
#include <sys/stat.h>
#include <unistd.h>
#include <filesystem>

#define LOG_TAG "ueventhandler"
#include "log_new.h"
#include "util.h"



namespace minimal_systems {
namespace init {

namespace fs = std::filesystem;


std::vector<DevicePermissionRule> device_rules;

static uid_t resolve_uid(const std::string& name) {
    struct passwd* pw = getpwnam(name.c_str());
    return pw ? pw->pw_uid : static_cast<uid_t>(-1);
}

static gid_t resolve_gid(const std::string& name) {
    struct group* gr = getgrnam(name.c_str());
    return gr ? gr->gr_gid : static_cast<gid_t>(-1);
}


static inline void trim(std::string& s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](int ch) {
        return !std::isspace(ch);
    }));
    s.erase(std::find_if(s.rbegin(), s.rend(), [](int ch) {
        return !std::isspace(ch);
    }).base(), s.end());
}

static inline bool starts_with(const std::string& s, const std::string& prefix) {
    return s.compare(0, prefix.size(), prefix) == 0;
}

void UeventHandler::addDeviceRule(const std::string& path_pattern, mode_t mode,
                                   const std::string& user, const std::string& group) {
    uid_t uid = resolve_uid(user);
    gid_t gid = resolve_gid(group);
    if (uid == static_cast<uid_t>(-1) || gid == static_cast<gid_t>(-1)) {
        LOGW("Invalid user/group in uevent rule: %s:%s", user.c_str(), group.c_str());
        return;
    }

    device_rules.push_back({path_pattern, mode, uid, gid});
    LOGI("Added uevent rule: %s %o %s %s", path_pattern.c_str(), mode, user.c_str(), group.c_str());
}

bool UeventHandler::parseRuleLine(const std::string& line) {
    std::string trimmed = line;
    trim(trimmed);
    if (trimmed.empty() || trimmed[0] == '#') return true;

    if (starts_with(trimmed, "SUBSYSTEM==")) {
        // TODO: implement subsystem matching later
        LOGI("Skipping SUBSYSTEM rule (not implemented): %s", trimmed.c_str());
        return true;
    }

    std::istringstream iss(trimmed);
    std::string pattern, mode_str, user, group;
    iss >> pattern >> mode_str >> user >> group;

    if (pattern.empty() || mode_str.empty() || user.empty() || group.empty()) {
        LOGW("Malformed uevent rule: %s", trimmed.c_str());
        return false;
    }

    try {
        mode_t mode = static_cast<mode_t>(std::stoul(mode_str, nullptr, 8));
        UeventHandler::addDeviceRule(pattern, mode, user, group);
        return true;
    } catch (...) {
        LOGW("Invalid mode in uevent rule: %s", trimmed.c_str());
        return false;
    }
}


void UeventHandler::applyRulesToDevice(const std::string& device_path) {
    for (const auto& rule : device_rules) {
        // Wildcard match
        if (std::regex_match(device_path, std::regex(rule.path_pattern))) {
            LOGI("Matched rule for %s: chmod %o, chown %d:%d",
                 device_path.c_str(), rule.mode, rule.uid, rule.gid);

            chmod(device_path.c_str(), rule.mode);
            chown(device_path.c_str(), rule.uid, rule.gid);
        }
    }
}

}  // namespace init
}  // namespace minimal_systems
