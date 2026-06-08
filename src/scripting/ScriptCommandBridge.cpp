#include "scripting/ScriptCommandBridge.h"

#include <utility>

namespace edi::scripting {

ScriptCommandPlan planScriptCommands(const LuaRecipe &recipe, std::vector<ScriptCommandRequest> requests)
{
    ScriptCommandPlan plan;
    auto validation = validateLuaRecipe(recipe);
    if (!validation.ok) {
        plan.messages = std::move(validation.messages);
        return plan;
    }

    for (const ScriptCommandRequest &request : requests) {
        if (request.domain.empty() || request.command.empty()) {
            plan.messages.push_back("script command request requires domain and command");
        }
    }

    plan.ok = plan.messages.empty();
    plan.dryRun = true;
    plan.requests = std::move(requests);
    return plan;
}

} // namespace edi::scripting
