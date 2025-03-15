#include <algorithm> // For std::sort
#include <exception>
#include <iostream>
#include <map> // Assuming props.getAllProperties() returns a std::map
#include <unordered_map>

#include "first_stage_init.h"
#include "first_stage_mount.h"
#include "init_parser.h"
#include "property_manager.h"
#include "selinux.h"
#include "vold.h"

#define LOG_TAG "init"
#include "log_new.h"

namespace minimal_systems
{
namespace init
{

// Function prototypes
void prepare_log(char **argv);
bool FirstStageMount();
void load_loop();
bool parse_init();

} // namespace init
} // namespace minimal_systems

int main(int argc, char **argv)
{
	using namespace minimal_systems::init;

	try {
		system("clear");

		// Initialize the PropertyManager
		auto &props = PropertyManager::instance();
		auto &propertyManager = PropertyManager::instance();
		propertyManager.loadProperties("etc/prop.default");
		propertyManager.loadProperties("usr/share/etc/prop.default");

		// Step 2: Set default critical properties
		if (getprop("ro.bootmode").empty()) {
			setprop("ro.bootmode", "normal");
		}

		int first_stage_result = FirstStageMain(argc, argv);
		if (first_stage_result != 0) {
			LOGE("FirstStageMain failed with result: %d. Exiting...",
			     first_stage_result);
			return EXIT_FAILURE;
		}

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

		// Sorting and logging all properties from props
		LOGI("Loaded Properties:");

		// Collect properties into a vector for sorting
		std::vector<std::pair<std::string, std::string> >
			sortedProperties(props.getAllProperties().begin(),
					 props.getAllProperties().end());

		// Sort properties by key in alphabetical order
		std::sort(sortedProperties.begin(), sortedProperties.end(),
			  [](const auto &lhs, const auto &rhs) {
				  return lhs.first < rhs.first;
			  });

		// Log the sorted properties
		for (const auto &[key, value] : sortedProperties) {
			LOGI("  %s = %s", key.c_str(), value.c_str());
		}
		return EXIT_SUCCESS;

	} catch (const std::exception &ex) {
		LOGE("Unhandled exception: %s", ex.what());
		return EXIT_FAILURE;
	} catch (...) {
		LOGE("Unknown error occurred. Exiting...");
		return EXIT_FAILURE;
	}
}
