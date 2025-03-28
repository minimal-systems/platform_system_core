// system/core/init/action_manager.cpp
#define LOG_TAG "action_manager"
#include "action_manager.h"
#include "action.h"
#include "service.h"
#include "log_new.h"
#include "property_manager.h"

#include <sstream>

namespace minimal_systems {
namespace init {

static ActionManager singleton;

ActionManager& GetActionManager() {
    return singleton;
}

void ActionManager::QueueBuiltinAction(const std::function<void()>& fn, const std::string& name) {
    action_queue_.emplace([fn, name]() {
        LOGI("Executing builtin: %s", name.c_str());
        fn();
    });
}

void ActionManager::QueueEventTrigger(const std::string& trigger_name) {
    LOGI("Queueing event trigger: %s", trigger_name.c_str());

    for (const auto& block : trigger_blocks) {
        if (match_trigger(block, trigger_name)) {
            action_queue_.emplace([this, block]() {
                LOGI("Executing trigger block with %zu command(s)", block.commands.size());
                for (const auto& cmd : block.commands) {
                    LOGI("  -> Running: %s", cmd.c_str());
                    this->execute_command(cmd);
                }
            });
        }
    }
}

void ActionManager::ExecuteNext() {
    if (action_queue_.empty()) return;
    std::function<void()> fn = action_queue_.front();
    action_queue_.pop();
    fn();
}

/**
 * Evaluates and dispatches a single command string.
 * Extendable command handler.
 */
void ActionManager::execute_command(const std::string& cmd) {
    std::istringstream iss(cmd);
    std::string keyword;
    iss >> keyword;

    if (keyword == "setprop") {
        std::string key, value;
        iss >> key >> value;
        PropertyManager::instance().set(key, value);
        LOGI("setprop %s = %s", key.c_str(), value.c_str());
    } else if (keyword == "start") {
        std::string svc_name;
        iss >> svc_name;
        if (!svc_name.empty()) {
            LOGI("start service: %s", svc_name.c_str());
            start_service_by_name(svc_name);
        }
    } else {
        LOGW("Unhandled command: %s", cmd.c_str());
    }
}

}  // namespace init
}  // namespace minimal_systems
