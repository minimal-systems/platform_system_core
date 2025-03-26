#include <algorithm>  // For std::sort
#include <exception>
#include <iostream>
#include <map>  // Assuming props.getAllProperties() returns a std::map
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

// Function prototypes
void prepare_log(char** argv);
bool PerformFirstStageMount();
void load_loop();
bool parse_init();


using namespace minimal_systems::init;

int SecondStageMain(int argc, char** argv) {
    (void)argc;
    try {
        // Initialize the PropertyManager
        auto& props = PropertyManager::instance();
        auto& propertyManager = PropertyManager::instance();
        propertyManager.loadProperties("etc/prop.default");
        propertyManager.loadProperties("usr/share/etc/prop.default");

        // Load SELinux configuration
        SetupSelinux(argv);
        LOGI("SELinux configuration loaded.");

        // Parse init configurations
        if (!parse_init()) {
            LOGE("Parsing init configurations failed. Exiting...");
            return EXIT_FAILURE;
        }
        LOGI("Initialization configurations parsed successfully.");

        // Set final property indicating success
        props.set("init.completed", "true");

        // Sorting and logging all properties from props
        LOGI("Loaded Properties:");

        // Reserve upfront to avoid unnecessary reallocations
        const auto& allProps = props.getAllProperties();
        std::vector<std::pair<std::string, std::string>> sortedProperties;
        sortedProperties.reserve(allProps.size());

        for (const auto& entry : allProps) {
            sortedProperties.emplace_back(entry);
        }

        // Sort using fast lexicographic comparison
        std::sort(sortedProperties.begin(), sortedProperties.end(),
                  [](const std::pair<std::string, std::string>& a,
                     const std::pair<std::string, std::string>& b) {
                      return a.first < b.first;
                  });

        // Log sorted output
        for (const auto& [key, value] : sortedProperties) {
            LOGI("  %s = %s", key.c_str(), value.c_str());
        }

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
