#include "scripting/ScriptCommandBridge.h"

#include <cassert>
#include <string>

using namespace edi::scripting;

int main()
{
    LuaRecipe recipe = makeLuaRecipe("recipe_1", "Apply preset", "1");
    recipe.requiredCapabilities = {"drafting.commands"};

    auto validation = validateLuaRecipe(recipe);
    assert(validation.ok);
    assert(validation.diagnostics.empty());
    assert(scriptResultCodeName(ScriptResultCode::InvalidDomain) == std::string("invalid_domain"));

    ScriptCommandRequest request;
    request.domain = "drafting";
    request.command = "select";
    request.targetId = "object_1";

    auto plan = planScriptCommands(recipe, {request});
    assert(plan.ok);
    assert(plan.dryRun);
    assert(plan.requests.size() == 1);
    assert(plan.diagnostics.empty());

    LuaRecipe invalidRecipe = makeLuaRecipe("", "", "");
    auto invalidValidation = validateLuaRecipe(invalidRecipe);
    assert(!invalidValidation.ok);
    assert(invalidValidation.diagnostics.front().code == ScriptResultCode::InvalidRecipe);

    LuaRecipe missingCapability = recipe;
    missingCapability.requiredCapabilities.push_back("");
    auto capabilityValidation = validateLuaRecipe(missingCapability);
    assert(!capabilityValidation.ok);
    assert(capabilityValidation.diagnostics.back().code == ScriptResultCode::MissingCapability);

    auto invalidRecipePlan = planScriptCommands(invalidRecipe, {request});
    assert(!invalidRecipePlan.ok);
    assert(invalidRecipePlan.diagnostics.front().code == ScriptResultCode::InvalidRecipe);
    assert(invalidRecipePlan.requests.empty());

    ScriptCommandRequest badDomain = request;
    badDomain.domain = "filesystem";
    auto badDomainPlan = planScriptCommands(recipe, {badDomain});
    assert(!badDomainPlan.ok);
    assert(badDomainPlan.diagnostics.front().code == ScriptResultCode::InvalidDomain);
    assert(badDomainPlan.requests.empty());

    ScriptCommandRequest emptyCommand = request;
    emptyCommand.command.clear();
    auto emptyCommandPlan = planScriptCommands(recipe, {emptyCommand});
    assert(!emptyCommandPlan.ok);
    assert(emptyCommandPlan.diagnostics.front().code == ScriptResultCode::InvalidCommand);

    ScriptCommandRequest emptyTarget = request;
    emptyTarget.targetId.clear();
    auto emptyTargetPlan = planScriptCommands(recipe, {emptyTarget});
    assert(!emptyTargetPlan.ok);
    assert(emptyTargetPlan.diagnostics.front().code == ScriptResultCode::EmptyTargetId);

    return 0;
}
