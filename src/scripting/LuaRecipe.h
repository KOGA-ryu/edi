#pragma once

#include <string>
#include <vector>

namespace edi::scripting {

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
    std::vector<std::string> messages;
};

LuaRecipe makeLuaRecipe(std::string id, std::string name, std::string version);
LuaRecipeValidation validateLuaRecipe(const LuaRecipe &recipe);

} // namespace edi::scripting
