// init/init_parser.cpp
//
// Entry-point and utilities for parsing Android-style init.rc files
// and initialization directories. Supports import directives, block
// parsing, property substitution, and boot mode detection.

#define LOG_TAG "init_parser"

#include <grp.h>
#include <pwd.h>
#include <sys/stat.h>
#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include "log_new.h"
#include "property_manager.h"
#include "service.h"
#include "ueventhandler.h"
#include "util.h"
#include "action.h"

namespace minimal_systems {
namespace init {

namespace fs = std::filesystem;

const std::vector<std::string> init_dirs = {"etc/init/", "usr/share/etc/init/", "oem/etc/init/"};

// Forward declarations
bool parse_rc_file(const std::string& filepath);
void trim(std::string& str);
bool starts_with(const std::string& line, const std::string& prefix);
void substitute_props(std::string& str);

/**
 * Check if a file exists.
 */
bool file_exists(const std::string& filename) {
    try {
        return fs::exists(filename);
    } catch (const std::exception& ex) {
        LOGE("Exception while checking file '%s': %s", filename.c_str(), ex.what());
        return false;
    }
}

/**
 * Expand all ${prop_name} tokens with values from PropertyManager.
 */
void substitute_props(std::string& str) {
    auto& props = PropertyManager::instance();
    size_t start = str.find("${");

    while (start != std::string::npos) {
        size_t end = str.find('}', start);
        if (end == std::string::npos) break;

        std::string key = str.substr(start + 2, end - start - 2);
        std::string value = props.get(key, "");
        str.replace(start, end - start + 1, value);
        start = str.find("${", start + value.length());
    }
}

/**
 * Trim whitespace from both ends of the string.
 */
void trim(std::string& str) {
    auto not_space = [](int ch) { return !std::isspace(ch); };
    str.erase(str.begin(), std::find_if(str.begin(), str.end(), not_space));
    str.erase(std::find_if(str.rbegin(), str.rend(), not_space).base(), str.end());
}

/**
 * Check if a string starts with a given prefix.
 */
bool starts_with(const std::string& line, const std::string& prefix) {
    return line.rfind(prefix, 0) == 0;
}

/**
 * Parse a single .rc file and apply configuration.
 *
 * Supports:
 * - `import <path>` (recursive)
 * - `setprop <key> <value>`
 * - `on <trigger>` (stub)
 * - `service <name> <exec>` (stub)
 *
 * @param filepath Path to the .rc script
 * @return true if the file was parsed successfully, false otherwise
 */
bool parse_rc_file(const std::string& filepath) {
    std::ifstream file(filepath);
    std::string line;
    std::string current_block;
    if (!file.is_open()) {
        LOGE("Failed to open rc file '%s'", filepath.c_str());
        return false;
    }

    LOGD("Parsing init RC file: %s", filepath.c_str());

    bool is_ueventd_rc = filepath.find("ueventd") != std::string::npos;

    while (std::getline(file, line)) {
        trim(line);
        if (line.empty() || line[0] == '#') continue;

        substitute_props(line);
        bool inside_quote = false;
        for (size_t i = 0; i < line.size(); ++i) {
            if (line[i] == '"') {
                inside_quote = !inside_quote;
            } else if (line[i] == '#' && !inside_quote) {
                line = line.substr(0, i);
                trim(line);
                break;
            }
        }

        if (is_ueventd_rc) {
            UeventHandler::parseRuleLine(line);
            continue;
        }

        if (starts_with(line, "import ")) {
            std::string import_path = trim_copy(line.substr(7));
            import_path = resolve_prop_substitutions(import_path);
            substitute_props(import_path);
            if (!parse_rc_file(import_path)) {
                LOGW("Failed to import RC file: %s", import_path.c_str());
            }
            continue;
        }

        if (starts_with(line, "on ")) {
            current_block = "on";

            // Extract the full condition string after "on"
            std::string condition_str = trim_copy(line.substr(3));
            LOGI("Parsing 'on' trigger line: %s", condition_str.c_str());

            std::istringstream iss(condition_str);
            std::string token;
            std::vector<TriggerCondition> conditions;

            // Split on "&&" (with optional whitespace)
            while (std::getline(iss, token, '&')) {
                trim(token);

                // Skip empty tokens (from consecutive '&&' or trailing '&')
                if (token.empty() || token == "&") {
                    LOGW("Skipped empty condition token in trigger: '%s'", condition_str.c_str());
                    continue;
                }

                // Handle property-based condition
                if (starts_with(token, "property:")) {
                    std::string prop_expr = token.substr(9);  // Remove "property:"
                    size_t eq_pos = prop_expr.find('=');

                    if (eq_pos != std::string::npos) {
                        std::string prop_key = trim_copy(prop_expr.substr(0, eq_pos));
                        std::string prop_val = trim_copy(prop_expr.substr(eq_pos + 1));
                        LOGI("Detected property condition: [%s = %s]", prop_key.c_str(),
                             prop_val.c_str());

                        conditions.push_back(TriggerCondition{
                                .type = "property", .key = prop_key, .value = prop_val});
                    } else {
                        LOGW("Malformed property trigger (missing '=' symbol): %s", token.c_str());
                    }

                } else {
                    // Treat as a generic named trigger, e.g., "boot", "post-fs", "early-init"
                    LOGI("Detected generic trigger condition: [%s]", token.c_str());

                    conditions.push_back(TriggerCondition{.type = token, .key = "", .value = ""});
                }
            }

            if (!conditions.empty()) {
                // Register the trigger block for deferred execution
                trigger_blocks.push_back(TriggerBlock{.conditions = conditions, .commands = {}});

                LOGI("Registered 'on' trigger block with %zu condition(s): %s", conditions.size(),
                     condition_str.c_str());

            } else {
                LOGW("No valid conditions found in 'on' block: %s", condition_str.c_str());
            }

            continue;
        }

        if (current_block == "on" && !line.empty()) {
            if (!trigger_blocks.empty()) {
                trigger_blocks.back().commands.push_back(line);
            }
            continue;
        }

        if (starts_with(line, "mkdir ")) {
            LOGI("Processing mkdir: %s", line.c_str());

            std::istringstream iss(trim_copy(line.substr(6)));
            std::string path, mode_str, user, group;
            iss >> path >> mode_str >> user >> group;

            std::string dir_path = "." + path;
            mode_t mode = 0755;  // Default fallback

            if (!mode_str.empty()) {
                try {
                    mode = static_cast<mode_t>(std::stoul(mode_str, nullptr, 8));
                } catch (...) {
                    LOGW("Invalid mode '%s', defaulting to 0755", mode_str.c_str());
                }
            }

            try {
                if (!fs::create_directories(dir_path)) {
                    LOGW("Directory exists or failed to create: %s", dir_path.c_str());
                } else {
                    chmod(dir_path.c_str(), mode);
                    LOGI("Directory created: %s (mode %o)", dir_path.c_str(), mode);
                }

                if (!user.empty() && !group.empty()) {
                    struct passwd* pw = getpwnam(user.c_str());
                    struct group* gr = getgrnam(group.c_str());
                    if (pw && gr) {
                        if (chown(dir_path.c_str(), pw->pw_uid, gr->gr_gid) == 0) {
                            LOGI("Set owner of %s to %s:%s", dir_path.c_str(), user.c_str(),
                                 group.c_str());
                        } else {
                            LOGW("Failed to chown %s: %s", dir_path.c_str(), strerror(errno));
                        }
                    } else {
                        LOGW("Invalid user/group: %s:%s", user.c_str(), group.c_str());
                    }
                }

            } catch (const std::exception& ex) {
                LOGE("Exception while creating directory '%s': %s", dir_path.c_str(), ex.what());
            }

            continue;
        }

        if (starts_with(line, "write ")) {
            std::istringstream iss(trim_copy(line.substr(6)));
            std::string raw_path;
            iss >> raw_path;

            std::string full_path = "." + raw_path;

            size_t content_pos = line.find(raw_path) + raw_path.length();
            std::string content = trim_copy(line.substr(content_pos));

            if (!content.empty() && content.front() == '"' && content.back() == '"') {
                content = content.substr(1, content.length() - 2);
            }

            LOGI("Writing to file: %s", full_path.c_str());
            try {
                std::ofstream out(full_path, std::ios::out | std::ios::trunc);
                if (!out) {
                    LOGW("Failed to open file for writing: %s", full_path.c_str());
                    continue;
                }
                out << content;
                out.close();
                LOGI("Wrote to %s: %s", full_path.c_str(), content.c_str());
            } catch (const std::exception& ex) {
                LOGE("Exception while writing to '%s': %s", full_path.c_str(), ex.what());
            }

            continue;
        }

        if (starts_with(line, "setprop ")) {
            std::istringstream iss(line);
            std::string command, key, value;

            iss >> command >> key;
            std::getline(iss, value);
            trim(value);

            if (!key.empty() && !value.empty()) {
                PropertyManager::instance().set(key, value);
                LOGI("Property set: %s = %s", key.c_str(), value.c_str());
            } else {
                LOGW("Malformed setprop line: %s", line.c_str());
            }
            continue;
        }

        if (starts_with(line, "service ")) {
            current_block = "service";
            LOGI("Service block: %s", line.c_str());
            parse_service_block(line, file);  // This consumes the whole block
            continue;
        }
        if (starts_with(line, "start ")) {
            std::istringstream iss(line);
            std::string command, service_name;
            iss >> command >> service_name;
            trim(service_name);
            if (service_name.empty()) {
                LOGW("Malformed start line: %s", line.c_str());
                continue;
            }
            LOGI("Starting service: %s", service_name.c_str());
            start_service_by_name(service_name);  // Correct function call
            continue;
        }

        LOGD("Command: %s", line.c_str());
        // TODO: Store or execute command depending on current_block
    }

    return true;
}

/**
 * Scan a directory for .rc files and parse each.
 */
bool parse_init_files(const std::string& dir_path) {
    try {
        if (!fs::exists(dir_path)) {
            LOGW("Init config directory not found: %s", dir_path.c_str());
            return false;
        }

        for (const auto& entry : fs::directory_iterator(dir_path)) {
            if (entry.is_regular_file() && entry.path().extension() == ".rc") {
                if (!parse_rc_file(entry.path().string())) {
                    LOGW("Failed to parse file: %s", entry.path().c_str());
                }
            }
        }

        return true;
    } catch (const std::exception& ex) {
        LOGE("Exception parsing init dir '%s': %s", dir_path.c_str(), ex.what());
        return false;
    }
}

/**
 * Execute a recovery init script.
 */
bool recovery_init() {
    auto& props = PropertyManager::instance();
    const std::string filename = props.get("ro.recovery.init_file", "init.rc");

    if (!file_exists(filename)) {
        LOGE("Recovery init file not found: '%s'", filename.c_str());
        return false;
    }

    LOGD("Executing recovery init script: %s", filename.c_str());
    try {
        std::system(("/bin/bash " + filename).c_str());
        return true;
    } catch (const std::exception& ex) {
        LOGE("Error running recovery init: %s", ex.what());
        return false;
    }
}

/**
 * Main entry point for init.rc parsing.
 */
bool parse_init() {
    try {
        auto& props = PropertyManager::instance();
        std::string boot_mode = props.get("ro.boot.mode", "");

        if (boot_mode == "recovery") {
            LOGI("Boot mode: recovery. Starting recovery init.");
            return recovery_init();
        }

        if (boot_mode == "fastboot") {
            LOGI("Boot mode: fastboot. Skipping init parsing.");
            return true;
        }

        for (const auto& dir : init_dirs) {
            if (!parse_init_files(dir)) {
                LOGW("Skipping failed init directory: %s", dir.c_str());
            }
        }

        props.set("ro.init.completed", "true");
        LOGI("Init parsing complete.");
        return true;

    } catch (const std::exception& ex) {
        LOGE("Exception in parse_init(): %s", ex.what());
        return false;
    }
}

}  // namespace init
}  // namespace minimal_systems
