// ueventhandler.cpp — Handles device permission rules like those in ueventd.rc

#include "ueventhandler.h"
#include "property_manager.h"  // Required for PropertyManager::instance()
#include "ueventgroups.h"

#include <fcntl.h>
#include <grp.h>
#include <pwd.h>
#include <sys/stat.h>
#include <unistd.h>
#include <algorithm>
#include <filesystem>
#include <regex>
#include <sstream>

#define LOG_TAG "ueventhandler"
#include "log_new.h"
#include "util.h"

namespace minimal_systems {
namespace init {

namespace fs = std::filesystem;

std::vector<DevicePermissionRule> device_rules;
std::vector<SubsystemPermissionRule> subsystem_rules;

static uid_t resolve_uid(const std::string& name) {
    struct passwd* pw = getpwnam(name.c_str());
    return pw ? pw->pw_uid : static_cast<uid_t>(-1);
}

static gid_t resolve_gid(const std::string& name) {
    struct group* gr = getgrnam(name.c_str());
    return gr ? gr->gr_gid : static_cast<gid_t>(-1);
}

static inline void trim(std::string& s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](int ch) { return !std::isspace(ch); }));
    s.erase(std::find_if(s.rbegin(), s.rend(), [](int ch) { return !std::isspace(ch); }).base(),
            s.end());
}

static inline bool starts_with(const std::string& s, const std::string& prefix) {
    return s.compare(0, prefix.size(), prefix) == 0;
}

std::string resolve_prop_substitutions(const std::string& line) {
    std::string result = line;
    size_t start = result.find("${");
    while (start != std::string::npos) {
        size_t end = result.find("}", start);
        if (end == std::string::npos) break;

        std::string prop_name = result.substr(start + 2, end - start - 2);
        std::string prop_value = PropertyManager::instance().get(prop_name);
        result.replace(start, end - start + 1, prop_value);
        start = result.find("${");
    }
    return result;
}

static std::regex glob_to_regex(const std::string& pattern) {
    std::string regex_str;
    for (char c : pattern) {
        switch (c) {
            case '*':
                regex_str += ".*";
                break;
            case '?':
                regex_str += ".";
                break;
            case '.':
                regex_str += "\\.";
                break;
            case '/':
                regex_str += "/";
                break;
            default:
                regex_str += std::regex_replace(std::string(1, c), std::regex(R"([\^$|()\\[\]+])"),
                                                R"(\\$&)");
                break;
        }
    }
    return std::regex(regex_str);
}

void UeventHandler::addDeviceRule(const std::string& path_pattern, mode_t mode,
                                  const std::string& user, const std::string& group) {
    uid_t uid = resolve_uid(user);
    gid_t gid = resolve_gid(group);

    if (uid == static_cast<uid_t>(-1)) {
        LOGW("Invalid user in uevent rule: %s", user.c_str());
    }

    if (gid == static_cast<gid_t>(-1)) {
        gid = resolve_known_group(group);
        if (gid == static_cast<gid_t>(-1)) {
            LOGW("Invalid user/group in uevent rule: %s:%s", user.c_str(), group.c_str());
            return;
        }
    }

    std::regex path_regex;
    try {
        path_regex = glob_to_regex(path_pattern);
    } catch (const std::regex_error& ex) {
        LOGE("Failed to compile regex from pattern '%s': %s", path_pattern.c_str(), ex.what());
        return;
    }

    device_rules.push_back({path_pattern, path_regex, mode, uid, gid});
    LOGI("Added uevent rule: %s %o %s %s", path_pattern.c_str(), mode, user.c_str(), group.c_str());
}

void UeventHandler::addSubsystemRule(const std::string& raw_line) {
    SubsystemPermissionRule rule;

    std::istringstream iss(raw_line);
    std::string token;

    while (iss >> token) {
        if (starts_with(token, "SUBSYSTEM==")) {
            rule.subsystem = token.substr(11);
            rule.subsystem.erase(std::remove(rule.subsystem.begin(), rule.subsystem.end(), '"'),
                                 rule.subsystem.end());
        } else if (starts_with(token, "KERNEL==")) {
            rule.kernel = token.substr(8);
            rule.kernel.erase(std::remove(rule.kernel.begin(), rule.kernel.end(), '"'),
                              rule.kernel.end());
        } else if (starts_with(token, "MODE=")) {
            std::string mode_str = token.substr(5);
            // Sanitize quotes and commas
            mode_str.erase(std::remove(mode_str.begin(), mode_str.end(), '"'), mode_str.end());
            mode_str.erase(std::remove(mode_str.begin(), mode_str.end(), ','), mode_str.end());
            try {
                rule.mode = static_cast<mode_t>(std::stoul(mode_str, nullptr, 8));
            } catch (...) {
                LOGW("Invalid MODE in SUBSYSTEM rule: %s", mode_str.c_str());
            }
        } else if (starts_with(token, "GROUP=")) {
            rule.group = token.substr(6);
            rule.group.erase(std::remove(rule.group.begin(), rule.group.end(), '"'),
                             rule.group.end());
            rule.group.erase(std::remove(rule.group.begin(), rule.group.end(), ','), rule.group.end());
            rule.gid = resolve_gid(rule.group);
            if (rule.gid == static_cast<gid_t>(-1)) {
                rule.gid = resolve_known_group(rule.group);
            }
        } else if (starts_with(token, "ATTR{")) {
            size_t close = token.find('}');
            if (close != std::string::npos) {
                std::string attr_key = token.substr(5, close - 5);
                std::string attr_value;
                if (token.size() > close + 2 && token[close + 1] == '=') {
                    attr_value = token.substr(close + 2);
                    attr_value.erase(std::remove(attr_value.begin(), attr_value.end(), '"'), attr_value.end());
                }
                rule.attrs[attr_key] = attr_value;
            }
        }
    }

    if (!rule.subsystem.empty()) {
        subsystem_rules.push_back(rule);
        LOGI("Parsed SUBSYSTEM rule: SUBSYSTEM=%s KERNEL=%s MODE=%o GROUP=%s",
             rule.subsystem.c_str(),
             rule.kernel.empty() ? "*" : rule.kernel.c_str(),
             rule.mode,
             rule.group.empty() ? "none" : rule.group.c_str());
    } else {
        LOGW("Invalid SUBSYSTEM rule: missing SUBSYSTEM== match");
    }
}

bool UeventHandler::parseRuleLine(const std::string& line) {
    std::string trimmed = line;
    trim(trimmed);
    if (trimmed.empty() || trimmed[0] == '#') return true;

    if (starts_with(trimmed, "SUBSYSTEM==")) {
        addSubsystemRule(trimmed);
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
        addDeviceRule(pattern, mode, user, group);
        return true;
    } catch (...) {
        LOGW("Invalid mode in uevent rule: %s", trimmed.c_str());
        return false;
    }
}

void UeventHandler::applyRulesToDevice(const std::string& device_path) {
    for (const auto& rule : device_rules) {
        if (std::regex_match(device_path, std::regex(rule.path_pattern))) {
            LOGI("Matched rule for %s: chmod %o, chown %d:%d", device_path.c_str(), rule.mode,
                 rule.uid, rule.gid);
            chmod(device_path.c_str(), rule.mode);
            chown(device_path.c_str(), rule.uid, rule.gid);
        }
    }
}

}  // namespace init
}  // namespace minimal_systems
