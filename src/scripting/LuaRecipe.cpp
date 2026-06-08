#include "scripting/LuaRecipe.h"

#include <utility>

namespace edi::scripting {

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
        validation.messages.push_back("recipe id is required");
    }
    if (recipe.name.empty()) {
        validation.messages.push_back("recipe name is required");
    }
    if (recipe.version.empty()) {
        validation.messages.push_back("recipe version is required");
    }
    validation.ok = validation.messages.empty();
    return validation;
}

} // namespace edi::scripting
