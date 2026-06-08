#include "scripting/LuaRecipe.h"

#include <utility>

namespace edi::scripting {

namespace {

void addDiagnostic(LuaRecipeValidation &validation, ScriptResultCode code, std::string message)
{
    validation.diagnostics.push_back({code, std::move(message)});
}

} // namespace

LuaRecipe makeLuaRecipe(std::string id, std::string name, std::string version)
{
    LuaRecipe recipe;
    recipe.id = std::move(id);
    recipe.name = std::move(name);
    recipe.version = std::move(version);
    return recipe;
}

LuaRecipeValidation validateLuaRecipe(const LuaRecipe &recipe)
{
    LuaRecipeValidation validation;
    if (recipe.id.empty()) {
        addDiagnostic(validation, ScriptResultCode::InvalidRecipe, "recipe id is required");
    }
    if (recipe.name.empty()) {
        addDiagnostic(validation, ScriptResultCode::InvalidRecipe, "recipe name is required");
    }
    if (recipe.version.empty()) {
        addDiagnostic(validation, ScriptResultCode::InvalidRecipe, "recipe version is required");
    }
    for (const std::string &capability : recipe.requiredCapabilities) {
        if (capability.empty()) {
            addDiagnostic(validation, ScriptResultCode::MissingCapability, "recipe capability name is required");
        }
    }
    validation.ok = validation.diagnostics.empty();
    return validation;
}

const char *scriptResultCodeName(ScriptResultCode code)
{
    switch (code) {
    case ScriptResultCode::None:
        return "none";
    case ScriptResultCode::InvalidRecipe:
        return "invalid_recipe";
    case ScriptResultCode::MissingCapability:
        return "missing_capability";
    case ScriptResultCode::InvalidDomain:
        return "invalid_domain";
    case ScriptResultCode::InvalidCommand:
        return "invalid_command";
    case ScriptResultCode::EmptyTargetId:
        return "empty_target_id";
    }
    return "unknown";
}

} // namespace edi::scripting
