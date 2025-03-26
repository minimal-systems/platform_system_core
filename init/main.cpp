#include <cstring>         // For strcmp
#include <sys/resource.h>  // For setpriority
#include <iostream>        // For logging

#include "first_stage_init.h"
#include "init.h"
#include "selinux.h"

using namespace minimal_systems::init;

int main(int argc, char** argv) {
    // Boost process priority (restored later)
    if (setpriority(PRIO_PROCESS, 0, -20) != 0) {
        std::cerr << "Failed to set priority: " << strerror(errno) << std::endl;
    }

    // Execute First Stage
    int first_stage_status = FirstStageMain(argc, argv);
    if (first_stage_status != 0) {
        std::cerr << "First stage init failed with status: " << first_stage_status << std::endl;
        return first_stage_status;
    }

    // Execute Second Stage
    int second_stage_status = SecondStageMain(argc, argv);
    if (second_stage_status != 0) {
        std::cerr << "Second stage init failed with status: " << second_stage_status << std::endl;
        return second_stage_status;
    }

    return 0; // Success
}
