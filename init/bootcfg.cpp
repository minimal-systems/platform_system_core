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

static std::unordered_map<std::string, std::string> flags;
static std::once_flag once;

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

static std::string ReadAndClean(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) return "";

    std::stringstream out;
    std::string line;
    while (std::getline(file, line)) {
        size_t comment = line.find('#');
        if (comment != std::string::npos) line = line.substr(0, comment);
        line = std::regex_replace(line, std::regex(R"(^\s+|\s+$)"), "");
        if (!line.empty()) out << line << ' ';
    }
    return out.str();
}

void Init() {
    std::call_once(once, []() {
        std::string base = ReadAndClean("/proc/cmdline");
        std::string local = ReadAndClean("./.cmdline");

        if (!base.empty()) {
            LOGI("bootcfg: parsing /proc/cmdline...");
            ParseLine(base);
        }

        if (!local.empty()) {
            LOGI("bootcfg: merging ./.cmdline overrides...");
            ParseLine(local);
        }

        LOGI("bootcfg initialized: %zu keys", flags.size());
    });
}

std::string Get(const std::string& key, const std::string& def) {
    Init();
    auto it = flags.find(key);
    return (it != flags.end()) ? it->second : def;
}

bool IsEnabled(const std::string& key) {
    std::string val = Get(key, "false");
    return val != "0" && val != "false";
}

const std::unordered_map<std::string, std::string>& All() {
    Init();
    return flags;
}

}  // namespace bootcfg
}  // namespace minimal_systems
