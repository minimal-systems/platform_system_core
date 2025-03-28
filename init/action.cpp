// system/core/init/action.cpp — Handles trigger-based actions from init.rc

#include <algorithm>
#include <sstream>
#include <queue>
#include <unistd.h>

#define LOG_TAG "action"

#include "action.h"
#include "log_new.h"
#include "property_manager.h"

namespace minimal_systems {
namespace init {

std::vector<TriggerBlock> trigger_blocks;
std::queue<const TriggerBlock*> action_queue;

static inline void trim(std::string& s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](int ch) {
        return !std::isspace(ch);
    }));
    s.erase(std::find_if(s.rbegin(), s.rend(), [](int ch) {
        return !std::isspace(ch);
    }).base(), s.end());
}

static TriggerCondition parse_condition(const std::string& token) {
    TriggerCondition cond;

    if (token.find("property:") == 0) {
        size_t eq = token.find('=');
        if (eq != std::string::npos) {
            cond.type = "property";
            cond.key = token.substr(9, eq - 9);
            cond.value = token.substr(eq + 1);
        }
    } else {
        cond.type = token;
    }

    return cond;
}

bool match_trigger(const TriggerBlock& block, const std::string& event) {
    for (const auto& cond : block.conditions) {
        if (cond.type == "property") {
            std::string actual = PropertyManager::instance().get(cond.key);
            if (actual != cond.value) {
                LOGD("Condition failed: property:%s != %s (actual: %s)",
                     cond.key.c_str(), cond.value.c_str(), actual.c_str());
                return false;
            }
        } else if (cond.type != event) {
            LOGD("Event mismatch: expected '%s', got '%s'",
                 cond.type.c_str(), event.c_str());
            return false;
        }
    }
    return true;
}

void queue_trigger(const std::string& trigger_name) {
    LOGI("Checking trigger blocks for event: %s", trigger_name.c_str());

    for (const auto& block : trigger_blocks) {
        if (match_trigger(block, trigger_name)) {
            LOGI("Queued trigger block (%zu commands)", block.commands.size());
            action_queue.push(&block);
        }
    }
}

void execute_next_action() {
    if (action_queue.empty()) return;

    const TriggerBlock* block = action_queue.front();
    action_queue.pop();

    LOGI("Executing trigger block with %zu command(s)", block->commands.size());
    for (const auto& cmd : block->commands) {
        LOGI("  → %s", cmd.c_str());

        std::istringstream iss(cmd);
        std::string keyword;
        iss >> keyword;

        if (keyword == "setprop") {
            std::string key, value;
            iss >> key >> value;
            PropertyManager::instance().set(key, value);
            LOGI("Action: setprop %s = %s", key.c_str(), value.c_str());
        } else {
            LOGW("Unknown or unhandled command: %s", cmd.c_str());
        }
    }
}

void parse_trigger_condition_line(const std::string& line) {
    std::string condition_expr = line.substr(3);  // strip "on "
    std::istringstream iss(condition_expr);
    std::string token;

    TriggerBlock block;

    while (std::getline(iss, token, '&')) {
        trim(token);
        if (token == "") continue;

        // Handle duplicate &&
        if (!token.empty() && token.back() == '&') token.pop_back();

        auto cond = parse_condition(token);
        block.conditions.push_back(cond);
    }

    LOGI("Registered trigger block: %s", condition_expr.c_str());
    trigger_blocks.push_back(block);
}

void add_command_to_last_trigger(const std::string& command) {
    if (!trigger_blocks.empty()) {
        trigger_blocks.back().commands.push_back(command);
        LOGD("Queued command for trigger: %s", command.c_str());
    } else {
        LOGW("Orphan command not within 'on' trigger block: %s", command.c_str());
    }
}

}  // namespace init
}  // namespace minimal_systems
