// service.cpp — Parses, stores, and starts service block definitions from init.rc

#include "service.h"

#include <sstream>
#include <algorithm>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <pwd.h>
#include <grp.h>

#define LOG_TAG "service"
#include "log_new.h"
#include "property_manager.h"

namespace minimal_systems {
namespace init {

std::vector<ServiceDefinition> service_list;

static inline void trim(std::string& s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](int ch) {
        return !std::isspace(ch);
    }));
    s.erase(std::find_if(s.rbegin(), s.rend(), [](int ch) {
        return !std::isspace(ch);
    }).base(), s.end());
}

void parse_service_block(const std::string& first_line, std::istream& in) {
    std::istringstream header_stream(first_line);
    std::string keyword, name, exec_path;
    header_stream >> keyword >> name >> exec_path;

    std::vector<std::string> args;
    std::string arg;
    while (header_stream >> arg) args.push_back(arg);

    ServiceDefinition service;
    service.name = name;
    service.exec = exec_path;
    service.args = args;

    std::string line;
    while (std::getline(in, line)) {
        trim(line);
        if (line.empty()) break;

        std::istringstream iss(line);
        std::string token;
        iss >> token;

        if (token == "class") {
            iss >> service.service_class;
        } else if (token == "user") {
            iss >> service.user;
        } else if (token == "group") {
            iss >> service.group;
        } else if (token == "disabled") {
            service.disabled = true;
        } else if (token == "oneshot") {
            service.oneshot = true;
        } else {
            LOGW("Unknown service option: %s", token.c_str());
        }
    }

    // Register service status property
    std::string svc_prop = "init.svc." + service.name;
    PropertyManager::instance().set(svc_prop, service.disabled ? "stopped" : "running");

    LOGI("Parsed service: %s -> %s", service.name.c_str(), service.exec.c_str());
    service_list.push_back(service);
}

void start_service_by_name(const std::string& name) {
    for (const auto& service : service_list) {
        if (service.name == name) {
            if (!service.disabled) {
                LOGI("Requested to start already-enabled service '%s'", name.c_str());
            } else {
                LOGI("Requested to start disabled service '%s'", name.c_str());
            }
            start_service(service);
            return;
        }
    }
    LOGW("Service not found: %s", name.c_str());
}

void start_service(const ServiceDefinition& service) {
    pid_t pid = fork();
    if (pid == 0) {
        // Child
        if (!service.user.empty()) {
            struct passwd* pw = getpwnam(service.user.c_str());
            if (pw) setuid(pw->pw_uid);
        }
        if (!service.group.empty()) {
            struct group* gr = getgrnam(service.group.c_str());
            if (gr) setgid(gr->gr_gid);
        }

        std::vector<const char*> exec_args;
        exec_args.push_back(service.exec.c_str());
        for (const auto& arg : service.args) exec_args.push_back(arg.c_str());
        exec_args.push_back(nullptr);

        execvp(service.exec.c_str(), const_cast<char* const*>(exec_args.data()));
        _exit(127); // exec failed
    } else if (pid > 0) {
        LOGI("Started service '%s' with pid %d", service.name.c_str(), pid);
        std::string prop = "init.svc." + service.name;
        PropertyManager::instance().set(prop, "running");
    } else {
        LOGE("Failed to fork service '%s'", service.name.c_str());
    }
}

const std::vector<ServiceDefinition>& get_services() {
    return service_list;
}

}  // namespace init
}  // namespace minimal_systems