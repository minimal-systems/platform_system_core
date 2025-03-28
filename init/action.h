// system/core/init/action.h

#ifndef MINIMAL_SYSTEMS_INIT_ACTION_H_
#define MINIMAL_SYSTEMS_INIT_ACTION_H_

#include <string>
#include <vector>
#include <queue>

namespace minimal_systems {
namespace init {

/**
 * Represents a single condition in a trigger block.
 * Example: property:ro.bootmode=charger or simple event like boot
 */
struct TriggerCondition {
    std::string type;   // "property", "boot", etc.
    std::string key;    // For property: the key (e.g. ro.bootmode)
    std::string value;  // For property: the expected value
};

/**
 * Represents a block of actions tied to one or more trigger conditions.
 */
struct TriggerBlock {
    std::vector<TriggerCondition> conditions;
    std::vector<std::string> commands;
};

// Global trigger list and queue
extern std::vector<TriggerBlock> trigger_blocks;

/**
 * Adds a parsed "on" block's condition line.
 * Supports multiple conditions with && between them.
 */
void parse_trigger_condition_line(const std::string& line);

/**
 * Adds a command line to the most recently defined trigger block.
 * Used when parsing continuation lines inside a trigger block.
 */
void add_command_to_last_trigger(const std::string& command);

/**
 * Evaluates all trigger blocks and queues ones matching this trigger name.
 */
void queue_trigger(const std::string& trigger_name);

/**
 * Executes the next queued action block (if any).
 */
void execute_next_action();

/**
 * Checks if a given trigger block matches a specific trigger event.
 */
bool match_trigger(const TriggerBlock& block, const std::string& event);

}  // namespace init
}  // namespace minimal_systems

#endif  // MINIMAL_SYSTEMS_INIT_ACTION_H_
