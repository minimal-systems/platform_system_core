// second_stage_main.cpp — Executes second-stage init logic including property loading,
// SELinux setup, and parsing init scripts

#include <algorithm>
#include <exception>
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

namespace minimal_systems {
namespace init {

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
 * - Parse init scripts
 * - Report system state
 *
 * @param argc Number of arguments (unused)
 * @param argv Argument values from the kernel (used for SELinux setup)
 * @return EXIT_SUCCESS on success, EXIT_FAILURE on any failure or exception
 */
int SecondStageMain(int argc, char** argv) {
    (void)argc;

    try {
        // Load properties from known system defaults
        auto& props = PropertyManager::instance();
        props.loadProperties("etc/prop.default");
        props.loadProperties("usr/share/etc/prop.default");

        // Initialize SELinux policy, contexts, and transition
        SetupSelinux(argv);
        LOGI("SELinux configuration loaded.");

        // Parse init.rc or similar boot scripts
        if (!parse_init()) {
            LOGE("Parsing init configurations failed. Exiting...");
            return EXIT_FAILURE;
        }
        LOGI("Initialization configurations parsed successfully.");

        // Mark init as completed for dependent services
        props.set("init.completed", "true");

        // Log all loaded properties (sorted for readability/debugging)
        LOGI("Loaded Properties:");

        const auto& allProps = props.getAllProperties();
        std::vector<std::pair<std::string, std::string>> sortedProperties;
        sortedProperties.reserve(allProps.size());

        for (const auto& entry : allProps) {
            sortedProperties.emplace_back(entry);
        }

        // Sort properties lexicographically by key
        std::sort(sortedProperties.begin(), sortedProperties.end(),
                  [](const std::pair<std::string, std::string>& a,
                     const std::pair<std::string, std::string>& b) {
                      return a.first < b.first;
                  });

        // Output sorted property list to log
        for (const auto& [key, value] : sortedProperties) {
            LOGI("  %s = %s", key.c_str(), value.c_str());
        }

        return EXIT_SUCCESS;

    } catch (const std::exception& ex) {
        // Catch and log known C++ exceptions
        LOGE("Unhandled exception: %s", ex.what());
        return EXIT_FAILURE;
    } catch (...) {
        // Catch and log unknown errors
        LOGE("Unknown error occurred. Exiting...");
        return EXIT_FAILURE;
    }
}

}  // namespace init
}  // namespace minimal_systems
