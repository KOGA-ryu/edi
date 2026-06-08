#pragma once

#include "scripting/LuaRecipe.h"

#include <string>
#include <vector>

namespace edi::scripting {

struct ScriptCommandRequest {
    std::string domain;
    std::string command;
    std::string targetId;
};

struct ScriptCommandPlan {
    bool ok = false;
    ScriptResultCode code = ScriptResultCode::None;
    std::string message;
    bool dryRun = true;
    std::vector<ScriptCommandRequest> requests;
    std::vector<ScriptDiagnostic> diagnostics;

    static ScriptCommandPlan accepted(std::vector<ScriptCommandRequest> requests);
    static ScriptCommandPlan rejected(ScriptResultCode code, std::string message);
};

ScriptCommandPlan planScriptCommands(const LuaRecipe &recipe, std::vector<ScriptCommandRequest> requests);

} // namespace edi::scripting
