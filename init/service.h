// system/core/init/service.h

#ifndef MINIMAL_SYSTEMS_INIT_SERVICE_H_
#define MINIMAL_SYSTEMS_INIT_SERVICE_H_

#include <string>
#include <vector>
#include <istream>

namespace minimal_systems {
namespace init {

/**
 * Represents a parsed 'service' block from init.rc.
 */
struct ServiceDefinition {
    std::string name;
    std::string exec;
    std::vector<std::string> args;
    std::string user;
    std::string group;
    std::string service_class;
    bool disabled = false;
    bool oneshot = false;
};

/**
 * Parses a full service block starting from the first line.
 *
 * @param first_line  The 'service' declaration line.
 * @param in          The input stream positioned at the next line.
 */
void parse_service_block(const std::string& first_line, std::istream& in);

/**
 * Starts a parsed service definition.
 *
 * @param service  The service to start.
 */
void start_service(const ServiceDefinition& service);

/**
 * Starts a service by its name.
 *
 * @param name  The name of the service to start.
 */
void start_service_by_name(const std::string& name);

/**
 * Returns the global service list.
 */
const std::vector<ServiceDefinition>& get_services();

}  // namespace init
}  // namespace minimal_systems

#endif  // MINIMAL_SYSTEMS_INIT_SERVICE_H_
