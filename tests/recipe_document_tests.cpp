#include "recipe/RecipeDocument.h"

#include "drafting/DraftingStore.h"

#include <cassert>
#include <cmath>

using namespace edi::recipe;
using namespace edi::drafting;

namespace {

bool near(double a, double b, double tolerance = 1e-9)
{
    return std::abs(a - b) <= tolerance;
}

DraftingObject object(DraftingObjectId id, DraftingShapeKind kind, DraftingGeometry geometry)
{
    auto built = buildDraftingObject(std::move(id), kind, std::move(geometry));
    assert(built.ok);
    return built.object;
}

const ResolvedParam &paramOf(const ResolvedStep &step, const std::string &id)
{
    for (const ResolvedParam &param : step.params) {
        if (param.id == id) {
            return param;
        }
    }
    assert(false);
    static ResolvedParam none;
    return none;
}

} // namespace

int main()
{
    // The shaper vocabulary: known ids resolve, unknown ones do not, and the
    // table distinguishes stock (primitives) from shapers (modifiers).
    assert(findShaper("cube") != nullptr && findShaper("cube")->primitive);
    assert(findShaper("bevel") != nullptr && !findShaper("bevel")->primitive);
    assert(findShaper("router") == nullptr);

    // Grammar: a recipe starts with a primitive — nothing to bevel on an
    // empty bench. Steps arrive with the spec's default params.
    RecipeDocument recipe;
    recipe.id = "lab_recipe";
    assert(!validateRecipe(recipe).ok);             // empty
    assert(!addShaperStep(recipe, "bevel").ok);     // modifier first: rejected
    assert(!addShaperStep(recipe, "router").ok);    // unknown shaper
    assert(addShaperStep(recipe, "cube").ok);
    assert(addShaperStep(recipe, "bevel").ok);
    assert(validateRecipe(recipe).ok);
    assert(recipe.steps.size() == 2);
    assert(recipe.steps[0].params.size() == 3);     // size_x/y/z defaults
    assert(near(recipe.steps[0].params[0].value, 1.0));
    const int revisionAfterAdds = recipe.revision;
    assert(revisionAfterAdds > 0);

    // Edits are plan-style: rejected ops leave the document untouched.
    assert(!setParamLiteral(recipe, 0, "no_such", 2.0).ok);
    assert(recipe.revision == revisionAfterAdds);
    assert(setParamLiteral(recipe, 0, "size_x", 2.5).ok);
    assert(near(recipe.steps[0].params[0].value, 2.5));

    // The grammar rule holds across edits, not only at append time: you
    // cannot remove the stock from under its modifiers, and you cannot
    // reorder a modifier to the front.
    assert(!removeShaperStep(recipe, 0).ok);
    assert(!moveShaperStep(recipe, 1, 0).ok);
    assert(addShaperStep(recipe, "cylinder").ok);   // cube, bevel, cylinder
    assert(moveShaperStep(recipe, 2, 1).ok);        // cube, cylinder, bevel
    assert(recipe.steps[1].shaperId == "cylinder");
    assert(removeShaperStep(recipe, 2).ok);         // bevel off the end is fine
    assert(recipe.steps.size() == 2);

    // Bindings: a parameter points at a canvas measurement instead of a
    // typed literal. Malformed bindings are rejected.
    assert(!bindParamToMeasurement(recipe, 0, "size_x", {"", "width"}).ok);
    assert(bindParamToMeasurement(recipe, 0, "size_x", {"plank", "width"}).ok);
    assert(bindParamToMeasurement(recipe, 0, "size_y", {"plank", "height"}).ok);
    assert(bindParamToMeasurement(recipe, 1, "radius", {"hole", "radius"}).ok);
    assert(bindParamToMeasurement(recipe, 1, "depth", {"cut", "length"}).ok);

    // Resolution against a real drafting document: a 12x8 (physical) grid,
    // so normalized sizes scale by 12 horizontally and 8 vertically.
    DraftingDocument drafting = makeDraftingDocument("lab_doc");
    assert(addObject(drafting, object("plank", DraftingShapeKind::Rectangle,
        RectangleGeometry{{0.1, 0.1}, 0.5, 0.25})).ok);
    assert(addObject(drafting, object("hole", DraftingShapeKind::Circle,
        CircleGeometry{{0.5, 0.5}, 0.1})).ok);
    assert(addObject(drafting, object("cut", DraftingShapeKind::Line,
        LineGeometry{{0.2, 0.2}, {0.5, 0.6}})).ok);

    DraftingGridSettings settings;
    settings.width = 12.0;
    settings.height = 8.0;
    const DraftingGridProjection grid = projectDraftingGrid(settings);

    const ResolvedRecipe resolved = resolveRecipe(recipe, drafting, grid);
    assert(resolved.ok);
    assert(resolved.steps.size() == 2);
    const ResolvedStep &cube = resolved.steps[0];
    assert(paramOf(cube, "size_x").ok && paramOf(cube, "size_x").fromMeasurement);
    assert(near(paramOf(cube, "size_x").value, 0.5 * 12.0));  // plank width
    assert(near(paramOf(cube, "size_y").value, 0.25 * 8.0));  // plank height
    assert(paramOf(cube, "size_z").ok && !paramOf(cube, "size_z").fromMeasurement);
    assert(near(paramOf(cube, "size_z").value, 1.0));         // untouched literal
    const ResolvedStep &cylinder = resolved.steps[1];
    assert(near(paramOf(cylinder, "radius").value, 0.1 * 12.0)); // X-axis convention
    assert(near(paramOf(cylinder, "depth").value, std::hypot(0.3 * 12.0, 0.4 * 8.0)));

    // A stale binding fails ITS parameter with a message, not the recipe's
    // other parameters — the UI can point at exactly what broke.
    RecipeDocument stale = recipe;
    assert(bindParamToMeasurement(stale, 0, "size_z", {"gone", "width"}).ok);
    assert(bindParamToMeasurement(stale, 1, "depth", {"hole", "length"}).ok); // wrong kind
    const ResolvedRecipe broken = resolveRecipe(stale, drafting, grid);
    assert(!broken.ok);
    assert(!paramOf(broken.steps[0], "size_z").ok);
    assert(!paramOf(broken.steps[0], "size_z").message.empty());
    assert(paramOf(broken.steps[0], "size_x").ok);   // healthy bindings unaffected
    assert(!paramOf(broken.steps[1], "depth").ok);   // length on a circle: refused
    assert(paramOf(broken.steps[1], "radius").ok);

    return 0;
}
