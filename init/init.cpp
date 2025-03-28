// second_stage_main.cpp — Executes second-stage init logic including property loading,
// SELinux setup, and parsing init scripts

#include <algorithm>
#include <exception>
#include <filesystem>
#include <iostream>
#include <map>
#include <unordered_map>

#include "first_stage_init.h"
#include "first_stage_mount.h"
#include "init_parser.h"
#include "property_manager.h"
#include "selinux.h"
#include "vold.h"

#define LOG_TAG "init"
#include "log_new.h"
#include "action_manager.h"

namespace minimal_systems {
namespace init {

namespace fs = std::filesystem;

// Function declarations
void prepare_log(char** argv);
bool PerformFirstStageMount();
void load_loop();
bool parse_init();

/**
 * Second-stage main entry point for init.
 *
 * Responsibilities:
 * - Load system properties
 * - Initialize SELinux
 * - Detect default user from /home
 * - Parse init scripts
 *
 * @param argc Number of arguments (unused)
 * @param argv Argument values from the kernel (used for SELinux setup)
 * @return EXIT_SUCCESS on success, EXIT_FAILURE on any failure or exception
 */
int SecondStageMain(int argc, char** argv) {
    (void)argc;
    auto& am = GetActionManager();

    try {
        // Load properties from known system defaults
        auto& props = PropertyManager::instance();
        props.loadProperties("etc/prop.default");
        props.loadProperties("usr/share/etc/prop.default");

        // Initialize SELinux policy, contexts, and transitions
        SetupSelinux(argv);
        LOGI("SELinux configuration loaded.");

        // Detect and set default user from /home before parsing init
        try {
            const fs::path home_dir("/home");
            if (!fs::exists(home_dir) || !fs::is_directory(home_dir)) {
                LOGW("Home directory not found or not a directory. Skipping ro.boot.user assignment.");
            } else {
                std::vector<std::string> user_folders;
                for (const auto& entry : fs::directory_iterator(home_dir)) {
                    if (entry.is_directory()) {
                        user_folders.push_back(entry.path().filename().string());
                    }
                }

                if (user_folders.size() == 1) {
                    const std::string& username = user_folders[0];
                    props.set("ro.boot.user", username);
                    LOGI("Set ro.boot.user = %s", username.c_str());
                } else {
                    LOGW("Expected exactly one user folder in /home, found %zu. Skipping ro.boot.user.",
                         user_folders.size());
                }
            }
        } catch (const std::exception& ex) {
            LOGE("Failed to determine home user: %s", ex.what());
        }

        // Log all loaded properties (sorted for readability/debugging)
        LOGI("Loaded Properties:");

        const auto& allProps = props.getAllProperties();
        std::vector<std::pair<std::string, std::string>> sortedProperties;
        sortedProperties.reserve(allProps.size());

        for (const auto& entry : allProps) {
            sortedProperties.emplace_back(entry);
        }

        std::sort(sortedProperties.begin(), sortedProperties.end(),
                  [](const std::pair<std::string, std::string>& a,
                     const std::pair<std::string, std::string>& b) {
                      return a.first < b.first;
                  });

        for (const auto& [key, value] : sortedProperties) {
            LOGI("  %s = %s", key.c_str(), value.c_str());
        }

        // Parse init.rc or similar boot scripts
        if (!parse_init()) {
            LOGE("Parsing init configurations failed. Exiting...");
            return EXIT_FAILURE;
        }
        am.QueueBuiltinAction([]() {
            LOGI("SetupCgroups running...");
            // perform setup
        }, "SetupCgroups");
    
        am.QueueEventTrigger("early-init");
    
        am.QueueBuiltinAction([]() {
            LOGI("Post-boot lambda running...");
        }, "LateInit");
    
        // Simulate main loop
        while (true) {
            am.ExecuteNext();
            sleep(1);
        }
    
        LOGI("Initialization configurations parsed successfully.");

        // Mark init as completed for dependent services
        props.set("init.completed", "true");


        
        return EXIT_SUCCESS;

    } catch (const std::exception& ex) {
        LOGE("Unhandled exception: %s", ex.what());
        return EXIT_FAILURE;
    } catch (...) {
        LOGE("Unknown error occurred. Exiting...");
        return EXIT_FAILURE;
    }
}

}  // namespace init
}  // namespace minimal_systems
