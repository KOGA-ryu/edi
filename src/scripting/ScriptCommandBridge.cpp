#include "scripting/ScriptCommandBridge.h"

#include <algorithm>
#include <utility>

namespace edi::scripting {

ScriptCommandPlan ScriptCommandPlan::accepted(std::vector<ScriptCommandRequest> requests)
{
    ScriptCommandPlan plan;
    plan.ok = true;
    plan.code = ScriptResultCode::None;
    plan.dryRun = true;
    plan.requests = std::move(requests);
    return plan;
}

ScriptCommandPlan ScriptCommandPlan::rejected(ScriptResultCode code, std::string message)
{
    ScriptCommandPlan plan;
    plan.ok = false;
    plan.code = code;
    plan.message = std::move(message);
    plan.dryRun = true;
    plan.diagnostics.push_back({plan.code, plan.message});
    return plan;
}

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
        plan.code = validation.diagnostics.empty() ? ScriptResultCode::InvalidRecipe : validation.diagnostics.front().code;
        plan.message = validation.diagnostics.empty() ? "recipe is invalid" : validation.diagnostics.front().message;
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
        if (!isValidScriptTargetId(request.targetId)) {
            addDiagnostic(plan, ScriptResultCode::EmptyTargetId, "script command target id is required");
        }
    }

    if (!plan.diagnostics.empty()) {
        plan.code = plan.diagnostics.front().code;
        plan.message = plan.diagnostics.front().message;
        return plan;
    }
    return ScriptCommandPlan::accepted(std::move(requests));
}

} // namespace edi::scripting
