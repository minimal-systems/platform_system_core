#include <iostream>
#include <exception>
#include <unordered_map>
#include "init_parser.h"
#include "selinux.h"
#include "vold.h"
#include "first_stage_mount.h"
#include "property_manager.h"
#define LOG_TAG "init"
#include "log_new.h"



namespace minimal_systems {
namespace init {

// Function prototypes
void prepare_log(char** argv);
bool FirstStageMount();
void load_loop();
bool parse_init();

}  // namespace init
}  // namespace minimal_systems

int main(int argc, char** argv) {
    using namespace minimal_systems::init;

    try {

        // Initialize the PropertyManager
        auto& props = PropertyManager::instance();
        props.loadProperties("etc/prop.default");
        props.set("init.completed", "false");

        // Perform first-stage mounting
        if (!FirstStageMount()) {
            LOGE("FirstStageMount failed. Exiting...");
            return EXIT_FAILURE;
        }
        LOGI("First stage mount completed.");

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

        LOGI("Loaded Properties:");
        for (const auto& [key, value] : props.getAllProperties()) {
            LOGI("  %s = %s", key.c_str(), value.c_str());
        }

        LOGI("Initialization completed successfully.");
        return EXIT_SUCCESS;

    } catch (const std::exception& ex) {
        LOGE("Unhandled exception: %s", ex.what());
        return EXIT_FAILURE;
    } catch (...) {
        LOGE("Unknown error occurred. Exiting...");
        return EXIT_FAILURE;
    }
}
