#pragma once

#include <string>
#include <vector>

namespace edi::scripting {

enum class ScriptResultCode {
    None,
    InvalidRecipe,
    MissingCapability,
    InvalidDomain,
    InvalidCommand,
    EmptyTargetId
};

struct ScriptDiagnostic {
    ScriptResultCode code = ScriptResultCode::None;
    std::string message;
};

struct LuaRecipe {
    std::string id;
    std::string name;
    std::string version;
    std::string description;
    std::vector<std::string> requiredCapabilities;
    std::string source;
};

struct LuaRecipeValidation {
    bool ok = false;
    std::vector<ScriptDiagnostic> diagnostics;
};

LuaRecipe makeLuaRecipe(std::string id, std::string name, std::string version);
LuaRecipeValidation validateLuaRecipe(const LuaRecipe &recipe);
const char *scriptResultCodeName(ScriptResultCode code);
bool isValidScriptTargetId(const std::string &targetId);

} // namespace edi::scripting
