#ifndef VERIFY_H
#define VERIFY_H

#include <string>

namespace minimal_systems {
namespace init {

// Simulated AVB-like partition verification for Linux
bool verify_partition(const std::string& device, const std::string& mount_point, const std::string& filesystem);

}  // namespace init
}  // namespace minimal_systems

#endif  // VERIFY_H
