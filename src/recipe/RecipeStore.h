#pragma once

#include "recipe/RecipeDocument.h"

#include <string>

namespace edi::recipe {

// Recipe <-> TOML text. The recipe is THE pointable document of the Blender
// pipeline — the user (or an AI under instruction) edits it by naming an
// exact key: "step.4.param.count.value = 20". TOML because it is the
// project's configurable-text format (never JSON), and flat numbered keys
// because that is the house persistence style (panel_content.N.*) and the
// flat shape survives the StaticConfig codec unchanged.

struct RecipeTextResult {
    bool ok = false;
    std::string text;
    std::string message;
};

RecipeTextResult recipeToToml(const RecipeDocument &document);

struct RecipeParseResult {
    bool ok = false;
    RecipeDocument document;
    std::string message;
};

// STRICT: an unknown shaper, an unknown param key, or a modifier-first step
// list rejects the whole file with a message naming the offender. Silently
// dropping a misspelled number would be guesswork by omission — the one
// failure mode this pipeline exists to kill.
RecipeParseResult recipeFromToml(const std::string &text, const std::string &source = {});

} // namespace edi::recipe
