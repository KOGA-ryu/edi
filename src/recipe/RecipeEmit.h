#pragma once

#include "recipe/RecipeDocument.h"

#include <string>

namespace edi::recipe {

struct RecipeEmitResult {
    bool ok = false;
    std::string script;  // Blender Python, deterministic for a given input
    std::string message; // why emission was refused
};

// The no-guesswork artifact: a resolved recipe becomes a Blender Python
// script whose numbers are the exact physical measurements — nothing for a
// human or an AI to estimate. Emission REFUSES an unresolved recipe rather
// than emitting placeholders: a script with a guess in it is precisely the
// failure mode this feature exists to kill.
RecipeEmitResult emitBlenderPython(const RecipeDocument &document, const ResolvedRecipe &resolved);

} // namespace edi::recipe
