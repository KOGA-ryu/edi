#pragma once

#include "recipe/RecipeDocument.h"

#include <string>

namespace edi::recipe {

struct RecipeAsciiResult {
    bool ok = false;
    std::string text;    // rows of '#' (inside) and '.' (outside) + a footer
    std::string message; // why rendering was refused
};

// The terminal's preview (user decision 2026-06-10): a top-down orthographic
// silhouette of the resolved recipe on a character grid. Cube -> rectangle,
// cylinder -> circle, array -> repeats at the exact measured spacing; bevel
// is a surface detail with no footprint, so the silhouette ignores it.
//
// The footprint's bounding box maps EXACTLY onto columns x rows — no margin,
// no aspect correction here. Characters are ~2x taller than wide, so a
// caller that wants true proportions passes rows ~= columns * (boxHeight /
// boxWidth) / 2; keeping that in the widget keeps this function trivially
// predictable (and golden-testable cell by cell).
RecipeAsciiResult renderRecipeAscii(const ResolvedRecipe &resolved, int columns, int rows);

} // namespace edi::recipe
