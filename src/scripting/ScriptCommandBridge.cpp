#include "scripting/ScriptCommandBridge.h"

#include <algorithm>
#include <utility>

namespace edi::scripting {

namespace {

bool isSupportedDomain(const std::string &domain)
{
    static constexpr const char *supportedDomains[] = {"drafting", "text", "project"};
    return std::find(std::begin(supportedDomains), std::end(supportedDomains), domain) != std::end(supportedDomains);
}

void addDiagnostic(ScriptCommandPlan &plan, ScriptResultCode code, std::string message)
{
    plan.diagnostics.push_back({code, std::move(message)});
}

} // namespace

ScriptCommandPlan planScriptCommands(const LuaRecipe &recipe, std::vector<ScriptCommandRequest> requests)
{
    ScriptCommandPlan plan;
    auto validation = validateLuaRecipe(recipe);
    if (!validation.ok) {
        plan.diagnostics = std::move(validation.diagnostics);
        return plan;
    }

    for (const ScriptCommandRequest &request : requests) {
        if (!isSupportedDomain(request.domain)) {
            addDiagnostic(plan, ScriptResultCode::InvalidDomain, "script command domain is unsupported");
        }
        if (request.command.empty()) {
            addDiagnostic(plan, ScriptResultCode::InvalidCommand, "script command name is required");
        }
        if (request.targetId.empty()) {
            addDiagnostic(plan, ScriptResultCode::EmptyTargetId, "script command target id is required");
        }
    }

    plan.ok = plan.diagnostics.empty();
    plan.dryRun = true;
    if (plan.ok) {
        plan.requests = std::move(requests);
    }
    return plan;
}

} // namespace edi::scripting
