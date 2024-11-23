#include <string>
#include "property_manager.h"
#include "log_new.h"

namespace minimal_systems {
namespace init {

enum class BootMode {
    NORMAL_MODE,
    RECOVERY_MODE,
    CHARGER_MODE,
};

namespace {

// Helper function to get a property value
std::string GetProperty(const std::string& key) {
    return getprop(key);
}

// Determines if the device is in charger mode
bool IsChargerMode() {
    return GetProperty("ro.bootmode") == "charger";
}

// Determines if the device is in recovery mode
bool IsRecoveryMode() {
    return GetProperty("ro.boot.mode") == "recovery";
}

// Checks if a normal boot is forced
bool IsNormalBootForced() {
    return GetProperty("ro.bootmode") == "normal";
}

} // anonymous namespace

BootMode GetBootMode(const std::string& /*cmdline*/, const std::string& /*bootconfig*/) {
    if (IsChargerMode()) {
        return BootMode::CHARGER_MODE;
    }
    if (IsRecoveryMode() && !IsNormalBootForced()) {
        return BootMode::RECOVERY_MODE;
    }
    return BootMode::NORMAL_MODE;
}

int FirstStageMain(int argc, char** argv) {
    // Parse command-line arguments
    std::string boot_cmdline;
    std::string boot_config;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg.find("--cmdline=") == 0) {
            boot_cmdline = arg.substr(10);
            LOGD("Parsed boot cmdline: %s", boot_cmdline.c_str());
        } else if (arg.find("--bootconfig=") == 0) {
            boot_config = arg.substr(13);
            LOGD("Parsed boot config: %s", boot_config.c_str());
        } else {
            LOGE("Unknown argument: %s", arg.c_str());
            return 1; // Return an error code for unknown arguments
        }
    }

    LOGI("Initializing first stage...");

    // Determine boot mode
    BootMode boot_mode = GetBootMode(boot_cmdline, boot_config);
    switch (boot_mode) {
        case BootMode::CHARGER_MODE:
            LOGI("Booting into Charger Mode.");
            setprop("ro.boot.mode", "charger");
            break;
        case BootMode::RECOVERY_MODE:
            LOGI("Booting into Recovery Mode.");
            setprop("ro.boot.mode", "recovery");
            break;
        case BootMode::NORMAL_MODE:
        default:
            LOGI("Booting into Normal Mode.");
            setprop("ro.boot.mode", "normal");
            break;
    }

    // Perform first stage initialization based on boot mode
    LOGI("First stage initialization complete.");

    return 0; // Return success
}

} // namespace init
} // namespace minimal_systems
