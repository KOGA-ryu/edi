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
    bool dryRun = true;
    std::vector<ScriptCommandRequest> requests;
    std::vector<ScriptDiagnostic> diagnostics;
};

ScriptCommandPlan planScriptCommands(const LuaRecipe &recipe, std::vector<ScriptCommandRequest> requests);

} // namespace edi::scripting
