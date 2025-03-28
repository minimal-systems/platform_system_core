// system/core/init/action_manager.h

#ifndef MINIMAL_SYSTEMS_INIT_ACTION_MANAGER_H
#define MINIMAL_SYSTEMS_INIT_ACTION_MANAGER_H

#include <functional>
#include <queue>
#include <string>

namespace minimal_systems {
namespace init {

class ActionManager {
public:
    void QueueBuiltinAction(const std::function<void()>& fn, const std::string& name);
    void QueueEventTrigger(const std::string& trigger_name);
    void ExecuteNext();

    /** Dispatches a command line string from an action block */
    void execute_command(const std::string& cmd);

private:
    std::queue<std::function<void()>> action_queue_;
};

ActionManager& GetActionManager();

}  // namespace init
}  // namespace minimal_systems

#endif  // MINIMAL_SYSTEMS_INIT_ACTION_MANAGER_H
