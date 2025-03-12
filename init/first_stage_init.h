#ifndef MINIMAL_SYSTEMS_INIT_FIRST_STAGE_INIT_H
#define MINIMAL_SYSTEMS_INIT_FIRST_STAGE_INIT_H

#include <string>

namespace minimal_systems
{
namespace init
{

// Enum representing the boot modes
enum class BootMode {
	NORMAL_MODE,
	RECOVERY_MODE,
	CHARGER_MODE,
};

// Returns the boot mode based on cmdline and bootconfig
BootMode GetBootMode(const std::string &cmdline, const std::string &bootconfig);

// Main entry point for the first stage bootloader
int FirstStageMain(int argc, char **argv);

} // namespace init
} // namespace minimal_systems

#endif // MINIMAL_SYSTEMS_INIT_FIRST_STAGE_INIT_H
