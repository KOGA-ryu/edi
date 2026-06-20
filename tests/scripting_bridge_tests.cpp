#include "scripting/ScriptCommandBridge.h"

#include "EdiAssert.h"
#include <string>

using namespace edi::scripting;

int main()
{
    LuaRecipe recipe = makeLuaRecipe("recipe_1", "Apply preset", "1");
    recipe.requiredCapabilities = {"drafting.commands"};
    EDI_CHECK(isValidScriptTargetId("object_1"));
    EDI_CHECK(!isValidScriptTargetId(""));
    EDI_CHECK(isValidLuaRecipeName("Apply preset"));
    EDI_CHECK(!isValidLuaRecipeName(""));
    EDI_CHECK(isValidLuaRecipeVersion("1"));
    EDI_CHECK(!isValidLuaRecipeVersion(""));

    auto validation = validateLuaRecipe(recipe);
    EDI_CHECK(validation.ok);
    EDI_CHECK(validation.diagnostics.empty());
    EDI_CHECK(scriptResultCodeName(ScriptResultCode::InvalidDomain) == std::string("invalid_domain"));

    ScriptCommandRequest request;
    request.domain = "drafting";
    request.command = "select";
    request.targetId = "object_1";

    auto plan = planScriptCommands(recipe, {request});
    EDI_CHECK(plan.ok);
    EDI_CHECK(plan.code == ScriptResultCode::None);
    EDI_CHECK(plan.dryRun);
    EDI_CHECK(plan.requests.size() == 1);
    EDI_CHECK(plan.diagnostics.empty());

    LuaRecipe invalidRecipe = makeLuaRecipe("", "", "");
    auto invalidValidation = validateLuaRecipe(invalidRecipe);
    EDI_CHECK(!invalidValidation.ok);
    EDI_CHECK(invalidValidation.diagnostics.front().code == ScriptResultCode::InvalidRecipe);

    LuaRecipe missingCapability = recipe;
    missingCapability.requiredCapabilities.push_back("");
    auto capabilityValidation = validateLuaRecipe(missingCapability);
    EDI_CHECK(!capabilityValidation.ok);
    EDI_CHECK(capabilityValidation.diagnostics.back().code == ScriptResultCode::MissingCapability);

    auto invalidRecipePlan = planScriptCommands(invalidRecipe, {request});
    EDI_CHECK(!invalidRecipePlan.ok);
    EDI_CHECK(invalidRecipePlan.code == ScriptResultCode::InvalidRecipe);
    EDI_CHECK(invalidRecipePlan.diagnostics.front().code == ScriptResultCode::InvalidRecipe);
    EDI_CHECK(invalidRecipePlan.requests.empty());

    ScriptCommandRequest badDomain = request;
    badDomain.domain = "filesystem";
    auto badDomainPlan = planScriptCommands(recipe, {badDomain});
    EDI_CHECK(!badDomainPlan.ok);
    EDI_CHECK(badDomainPlan.code == ScriptResultCode::InvalidDomain);
    EDI_CHECK(badDomainPlan.diagnostics.front().code == ScriptResultCode::InvalidDomain);
    EDI_CHECK(badDomainPlan.requests.empty());

    ScriptCommandRequest emptyCommand = request;
    emptyCommand.command.clear();
    auto emptyCommandPlan = planScriptCommands(recipe, {emptyCommand});
    EDI_CHECK(!emptyCommandPlan.ok);
    EDI_CHECK(emptyCommandPlan.code == ScriptResultCode::InvalidCommand);
    EDI_CHECK(emptyCommandPlan.diagnostics.front().code == ScriptResultCode::InvalidCommand);

    ScriptCommandRequest emptyTarget = request;
    emptyTarget.targetId.clear();
    auto emptyTargetPlan = planScriptCommands(recipe, {emptyTarget});
    EDI_CHECK(!emptyTargetPlan.ok);
    EDI_CHECK(emptyTargetPlan.code == ScriptResultCode::EmptyTargetId);
    EDI_CHECK(emptyTargetPlan.diagnostics.front().code == ScriptResultCode::EmptyTargetId);

    auto acceptedPlan = ScriptCommandPlan::accepted({request});
    EDI_CHECK(acceptedPlan.ok);
    EDI_CHECK(acceptedPlan.code == ScriptResultCode::None);
    auto rejectedPlan = ScriptCommandPlan::rejected(ScriptResultCode::InvalidCommand, "bad command");
    EDI_CHECK(!rejectedPlan.ok);
    EDI_CHECK(rejectedPlan.code == ScriptResultCode::InvalidCommand);

    return 0;
}
