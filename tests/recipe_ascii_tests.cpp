#include "recipe/RecipeAscii.h"

#include "drafting/DraftingStore.h"

#include <cassert>
#include <string>

using namespace edi::recipe;
using namespace edi::drafting;

namespace {

// Literal-only recipes never touch the drafting document, so one empty
// document and default grid serve every resolve here.
ResolvedRecipe resolveLiterals(const RecipeDocument &recipe)
{
    static const DraftingDocument empty = makeDraftingDocument("ascii_doc");
    static const DraftingGridProjection grid = projectDraftingGrid(DraftingGridSettings{});
    return resolveRecipe(recipe, empty, grid);
}

} // namespace

int main()
{
    // A 4x2 slab on an 8x4 grid: the bounding box maps exactly onto the
    // cells, so every cell center is inside — a full block of '#'.
    {
        RecipeDocument recipe;
        recipe.id = "slab";
        assert(addShaperStep(recipe, "cube").ok);
        assert(setParamLiteral(recipe, 0, "size_x", 4.0).ok);
        assert(setParamLiteral(recipe, 0, "size_y", 2.0).ok);
        const RecipeAsciiResult ascii = renderRecipeAscii(resolveLiterals(recipe), 8, 4);
        assert(ascii.ok);
        assert(ascii.text ==
            "########\n"
            "########\n"
            "########\n"
            "########\n"
            "footprint: 4 x 2\n");
    }

    // A radius-1 cylinder on a 4x4 grid: corner cell centers sit at
    // (+-0.75, +-0.75), distance^2 = 1.125 > 1 — outside; everything else in.
    {
        RecipeDocument recipe;
        recipe.id = "dowel";
        assert(addShaperStep(recipe, "cylinder").ok);
        assert(setParamLiteral(recipe, 0, "radius", 1.0).ok);
        const RecipeAsciiResult ascii = renderRecipeAscii(resolveLiterals(recipe), 4, 4);
        assert(ascii.ok);
        assert(ascii.text ==
            ".##.\n"
            "####\n"
            "####\n"
            ".##.\n"
            "footprint: 2 x 2\n");
    }

    // A lathe's footprint is the circle of its widest drafted radius: the
    // profile below peaks at physical r = 1 (0.125 on a width-8 grid), so
    // the silhouette matches the radius-1 cylinder exactly.
    {
        DraftingDocument profileDoc = makeDraftingDocument("lathe_ascii");
        PolylineGeometry profile;
        profile.vertices = {{0.125, 0.9}, {0.0625, 0.2}};
        auto built = buildDraftingObject("profile", DraftingShapeKind::Polyline, profile);
        assert(built.ok);
        assert(addObject(profileDoc, built.object).ok);
        DraftingGridSettings settings;
        settings.width = 8.0;
        settings.height = 4.0;

        RecipeDocument recipe;
        recipe.id = "turned";
        assert(addShaperStep(recipe, "lathe").ok);
        assert(setStepProfile(recipe, 0, "profile").ok);
        const ResolvedRecipe resolved = resolveRecipe(recipe, profileDoc, projectDraftingGrid(settings));
        assert(resolved.ok);
        const RecipeAsciiResult ascii = renderRecipeAscii(resolved, 4, 4);
        assert(ascii.ok);
        assert(ascii.text ==
            ".##.\n"
            "####\n"
            "####\n"
            ".##.\n"
            "footprint: 2 x 2\n");
    }

    // Array: a 1x1 block repeated 3 times at spacing 2 — three islands with
    // exact gaps, footprint 5 wide. The silhouette shows the measured
    // spacing, which is the whole point of the preview.
    {
        RecipeDocument recipe;
        recipe.id = "fence";
        assert(addShaperStep(recipe, "cube").ok);
        assert(addShaperStep(recipe, "array").ok);
        assert(setParamLiteral(recipe, 1, "count", 3.0).ok);
        assert(setParamLiteral(recipe, 1, "offset_x", 2.0).ok);
        const RecipeAsciiResult ascii = renderRecipeAscii(resolveLiterals(recipe), 10, 1);
        assert(ascii.ok);
        assert(ascii.text ==
            "##..##..##\n"
            "footprint: 5 x 1\n");
    }

    // Bevel is a surface detail: it must not change the silhouette.
    {
        RecipeDocument plain;
        plain.id = "plain";
        assert(addShaperStep(plain, "cube").ok);
        RecipeDocument beveled = plain;
        beveled.id = "beveled";
        assert(addShaperStep(beveled, "bevel").ok);
        const RecipeAsciiResult a = renderRecipeAscii(resolveLiterals(plain), 6, 3);
        const RecipeAsciiResult b = renderRecipeAscii(resolveLiterals(beveled), 6, 3);
        assert(a.ok && b.ok && a.text == b.text);
    }

    // Refusals: empty/unresolved recipes and degenerate grids say why
    // instead of rendering a lie.
    {
        const RecipeAsciiResult empty = renderRecipeAscii(ResolvedRecipe{}, 8, 4);
        assert(!empty.ok && !empty.message.empty());

        RecipeDocument recipe;
        recipe.id = "stale";
        assert(addShaperStep(recipe, "cube").ok);
        assert(bindParamToMeasurement(recipe, 0, "size_x", {"gone", "width"}).ok);
        const ResolvedRecipe broken = resolveLiterals(recipe);
        assert(!broken.ok);
        assert(!renderRecipeAscii(broken, 8, 4).ok);

        assert(!renderRecipeAscii(broken, 0, 4).ok);
    }

    return 0;
}
