#define LOG_TAG "bootcfg"

#include "bootcfg.h"
#include "log_new.h"

#include <fstream>
#include <sstream>
#include <regex>
#include <unordered_map>
#include <mutex>

namespace minimal_systems {
namespace bootcfg {

// Stores key=value pairs parsed from /proc/cmdline and ./.cmdline
static std::unordered_map<std::string, std::string> flags;

// Ensures one-time initialization of the config state
static std::once_flag once;

/**
 * Parses a space-separated command-line argument string.
 *
 * Each token is split by the first `=`:
 * - `key=value` becomes {key → value}
 * - `key` alone is stored as {key → "true"} to represent boolean switches
 *
 * @param line Raw command-line string with arguments separated by spaces.
 */
static void ParseLine(const std::string& line) {
    std::stringstream ss(line);
    std::string token;
    while (ss >> token) {
        size_t eq = token.find('=');
        if (eq != std::string::npos) {
            flags[token.substr(0, eq)] = token.substr(eq + 1);
        } else {
            flags[token] = "true";
        }
    }
}

/**
 * Reads and cleans a file containing kernel-like command-line arguments.
 *
 * Behavior:
 * - Removes comments starting with `#`
 * - Strips leading/trailing whitespace
 * - Flattens all content into a single space-separated line
 *
 * Used for both `/proc/cmdline` and user overrides (`./.cmdline`).
 *
 * @param path File path to read.
 * @return A cleaned, space-delimited string of flags.
 */
static std::string ReadAndClean(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) return "";

    std::stringstream out;
    std::string line;
    while (std::getline(file, line)) {
        // Remove comments
        size_t comment = line.find('#');
        if (comment != std::string::npos) {
            line = line.substr(0, comment);
        }

        // Strip whitespace from both ends
        line = std::regex_replace(line, std::regex(R"(^\s+|\s+$)"), "");

        if (!line.empty()) {
            out << line << ' ';
        }
    }
    return out.str();
}

static void ParsePiConfigTxt(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) return;

    std::string line;
    while (std::getline(file, line)) {
        size_t comment = line.find('#');
        if (comment != std::string::npos) {
            line = line.substr(0, comment);
        }
        line = std::regex_replace(line, std::regex(R"(^\s+|\s+$)"), "");

        if (line.empty()) continue;
        size_t eq = line.find('=');
        if (eq != std::string::npos) {
            flags[line.substr(0, eq)] = line.substr(eq + 1);
        } else {
            flags[line] = "true";
        }
    }
}

/**
 * Initializes the bootcfg flag map by parsing:
 * 1. `/proc/cmdline` - kernel arguments
 * 2. `./.cmdline` - user override file (optional)
 *
 * This function is safe to call multiple times; initialization runs only once.
 */
void Init() {
    std::call_once(once, []() {
        std::string base = ReadAndClean("/proc/cmdline");
        std::string local = ReadAndClean("./.cmdline");
        std::string pi_cmdline = ReadAndClean("/boot/cmdline.txt");
        std::string pi_config = "/boot/config.txt";

        if (!base.empty()) {
            LOGI("bootcfg: parsing /proc/cmdline...");
            ParseLine(base);
        }

        if (!pi_cmdline.empty()) {
            LOGI("bootcfg: merging /boot/cmdline.txt...");
            ParseLine(pi_cmdline);
        }

        std::ifstream pi_file(pi_config);
        if (pi_file.is_open()) {
            LOGI("bootcfg: merging /boot/config.txt...");
            ParsePiConfigTxt(pi_config);
        }

        if (!local.empty()) {
            LOGI("bootcfg: merging ./.cmdline overrides...");
            ParseLine(local);
        }

        LOGI("bootcfg initialized: %zu keys", flags.size());
    });
}

/**
 * Retrieves a boot-time configuration property.
 *
 * If the key is not found, a default value is returned instead.
 *
 * @param key Property name.
 * @param def Default value to return if key is not present.
 * @return The corresponding value or the default.
 */
std::string Get(const std::string& key, const std::string& def) {
    Init();  // Ensure flags are parsed
    auto it = flags.find(key);
    return (it != flags.end()) ? it->second : def;
}

/**
 * Evaluates whether a given flag is explicitly enabled.
 *
 * Recognized false values: "0", "false" (case-sensitive)
 * All other values (including "true", "1", or any non-zero string) are treated as enabled.
 *
 * @param key Flag name.
 * @return true if the flag is present and not explicitly disabled.
 */
bool IsEnabled(const std::string& key) {
    std::string val = Get(key, "false");
    return val != "0" && val != "false";
}

/**
 * Returns the entire parsed flag map.
 *
 * Use this for inspection, debugging, or enumeration of all bootcfg keys.
 *
 * @return Const reference to the internal flags map.
 */
const std::unordered_map<std::string, std::string>& All() {
    Init();
    return flags;
}

}  // namespace bootcfg
}  // namespace minimal_systems
