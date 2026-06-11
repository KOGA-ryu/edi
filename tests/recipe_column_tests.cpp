// The doric column, end to end: drafted profiles with exact coordinates ->
// recipe of shapers in order -> resolved physical numbers -> Blender python.
// This is the project's thesis exercised whole: every number below is
// pointable — change a drafted vertex or a recipe key, and exactly one known
// thing changes downstream. (The committed sample under samples/doric_column
// is generated from THIS construction; see its README.)
#include "recipe/RecipeEmit.h"
#include "recipe/RecipeStore.h"

#include "drafting/DraftingStore.h"

#include <cassert>
#include <string>
#include <utility>

using namespace edi::recipe;
using namespace edi::drafting;

namespace {

DraftingObject polylineObject(DraftingObjectId id, std::vector<Point2D> vertices)
{
    PolylineGeometry geometry;
    geometry.vertices = std::move(vertices);
    auto built = buildDraftingObject(std::move(id), DraftingShapeKind::Polyline, geometry);
    assert(built.ok);
    return built.object;
}

} // namespace

int main()
{
    // ---- The drafted profiles (document coordinates on the default 12x12
    // inch board; x = radius from the page-left axis, z = (1 - y) * 12).
    // A 9.6 inch column maquette.
    DraftingDocument drafting = makeDraftingDocument("doric_column_profiles");

    // Base cove: plinth top (r 1.32, z 0.84) easing into the shaft foot.
    assert(addObject(drafting, polylineObject("base_cove", {
        {0.110, 0.930},   // r 1.32,  z 0.84
        {0.096, 0.924},   // r 1.152, z 0.912
        {0.088, 0.916},   // r 1.056, z 1.008
    })).ok);

    // Shaft with entasis: foot r 1.056 tapering to neck r 0.792.
    assert(addObject(drafting, polylineObject("shaft", {
        {0.088, 0.916},   // r 1.056, z 1.008 (continuous with the cove)
        {0.086, 0.700},   // r 1.032, z 3.6
        {0.080, 0.500},   // r 0.96,  z 6.0
        {0.070, 0.350},   // r 0.84,  z 7.8
        {0.066, 0.330},   // r 0.792, z 8.04
    })).ok);

    // Echinus: the neck flaring out to carry the abacus.
    assert(addObject(drafting, polylineObject("echinus", {
        {0.066, 0.330},   // r 0.792, z 8.04 (continuous with the shaft)
        {0.072, 0.315},   // r 0.864, z 8.22
        {0.090, 0.305},   // r 1.08,  z 8.34
        {0.110, 0.300},   // r 1.32,  z 8.4
    })).ok);

    const DraftingGridProjection grid = projectDraftingGrid(DraftingGridSettings{});

    // ---- The recipe: shapers in order, bottom to top.
    RecipeDocument recipe;
    recipe.id = "doric_column";
    recipe.name = "Doric Column";

    assert(addShaperStep(recipe, "cube").ok);                       // 0: plinth
    assert(setParamLiteral(recipe, 0, "size_x", 3.0).ok);
    assert(setParamLiteral(recipe, 0, "size_y", 3.0).ok);
    assert(setParamLiteral(recipe, 0, "size_z", 0.48).ok);

    assert(addShaperStep(recipe, "cube").ok);                       // 1: plinth step
    assert(setParamLiteral(recipe, 1, "size_x", 2.64).ok);
    assert(setParamLiteral(recipe, 1, "size_y", 2.64).ok);
    assert(setParamLiteral(recipe, 1, "size_z", 0.36).ok);
    assert(setParamLiteral(recipe, 1, "loc_z", 0.48).ok);

    assert(addShaperStep(recipe, "lathe").ok);                      // 2: base cove
    assert(setStepProfile(recipe, 2, "base_cove").ok);

    assert(addShaperStep(recipe, "lathe").ok);                      // 3: shaft
    assert(setStepProfile(recipe, 3, "shaft").ok);

    assert(addShaperStep(recipe, "radial_groove").ok);              // 4: flutes
    assert(setParamLiteral(recipe, 4, "count", 20.0).ok);
    assert(setParamLiteral(recipe, 4, "cutter_radius", 0.16).ok);
    assert(setParamLiteral(recipe, 4, "depth", 0.12).ok);
    assert(setParamLiteral(recipe, 4, "at_radius", 1.056).ok);
    assert(setParamLiteral(recipe, 4, "z_from", 1.2).ok);
    assert(setParamLiteral(recipe, 4, "z_to", 6.0).ok);

    assert(addShaperStep(recipe, "lathe").ok);                      // 5: echinus
    assert(setStepProfile(recipe, 5, "echinus").ok);

    assert(addShaperStep(recipe, "cube").ok);                       // 6: abacus step
    assert(setParamLiteral(recipe, 6, "size_x", 2.64).ok);
    assert(setParamLiteral(recipe, 6, "size_y", 2.64).ok);
    assert(setParamLiteral(recipe, 6, "size_z", 0.24).ok);
    assert(setParamLiteral(recipe, 6, "loc_z", 8.4).ok);

    assert(addShaperStep(recipe, "cube").ok);                       // 7: abacus
    assert(setParamLiteral(recipe, 7, "size_x", 3.36).ok);
    assert(setParamLiteral(recipe, 7, "size_y", 3.36).ok);
    assert(setParamLiteral(recipe, 7, "size_z", 0.96).ok);
    assert(setParamLiteral(recipe, 7, "loc_z", 8.64).ok);

    assert(validateRecipe(recipe).ok);

    // ---- The recipe survives its document form (the pointable artifact).
    const RecipeTextResult toml = recipeToToml(recipe);
    assert(toml.ok);
    assert(toml.text.find("step.4.param.count.value = \"20\"") != std::string::npos);
    assert(toml.text.find("step.3.profile = \"shaft\"") != std::string::npos);
    const RecipeParseResult reloaded = recipeFromToml(toml.text, "column");
    assert(reloaded.ok);
    assert(reloaded.document.steps.size() == recipe.steps.size());

    // ---- Resolution: drafted coordinates become physical inches.
    const ResolvedRecipe resolved = resolveRecipe(reloaded.document, drafting, grid);
    assert(resolved.ok);
    const ResolvedStep &shaft = resolved.steps[3];
    assert(shaft.profileOk);
    assert(shaft.profilePoints.size() == 5);

    // ---- Emission: the script carries the exact numbers, end to end.
    const RecipeEmitResult emitted = emitBlenderPython(reloaded.document, resolved);
    assert(emitted.ok);
    const std::string &script = emitted.script;

    // Plinth and abacus stack by pointable bases (centre arithmetic shown).
    assert(script.find("obj.location[2] = 0.24  # base z = 0") != std::string::npos);
    assert(script.find("obj.location[2] = 0.66  # base z = 0.48") != std::string::npos);
    assert(script.find("obj.location[2] = 8.52  # base z = 8.4") != std::string::npos);
    assert(script.find("obj.location[2] = 9.12  # base z = 8.64") != std::string::npos);

    // The shaft profile lands verbatim, physical: 0.088*12 = 1.056 at
    // z (1-0.916)*12 = 1.008, through to the neck at 0.792 / 8.04.
    assert(script.find("(1.056, 0.0, 1.008)") != std::string::npos);
    assert(script.find("(1.032, 0.0, 3.6)") != std::string::npos);
    assert(script.find("(0.96, 0.0, 6)") != std::string::npos);
    assert(script.find("(0.792, 0.0, 8.04)") != std::string::npos);

    // The echinus flare and the cove read straight off the drafted page.
    assert(script.find("(1.32, 0.0, 8.4)") != std::string::npos);
    assert(script.find("(1.152, 0.0, 0.912)") != std::string::npos);

    // Twenty flutes, cutter centre arithmetic shown: 1.056 + 0.16 - 0.12.
    assert(script.find("for _i in range(20):") != std::string::npos);
    assert(script.find("_d = 1.096  # at_radius 1.056 + cutter 0.16 - depth 0.12") != std::string::npos);
    assert(script.find("primitive_cylinder_add(radius=0.16, depth=4.8)") != std::string::npos);

    // The whole column is eight steps; nothing was guessed anywhere: the
    // script must contain no placeholder vocabulary at all.
    assert(script.find("TODO") == std::string::npos);
    assert(script.find("approx") == std::string::npos);

    return 0;
}
