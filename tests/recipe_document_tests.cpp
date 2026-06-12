#include "recipe/RecipeDocument.h"

#include "drafting/DraftingStore.h"

#include <cassert>
#include <limits>
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
    assert(recipe.steps[0].params.size() == 4);     // size_x/y/z + loc_z defaults
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
    //
    // CONTRACT PINS (R1 migration): the exact refusal wordings below are
    // pipeline A's binding contract, recorded in
    // docs/recipe_binding_contract.md. When resolution moves to the shared
    // seam (resolveRecipeOps, R1-B03) these asserts must keep passing
    // unchanged — they are the proof the move preserved behavior.
    RecipeDocument stale = recipe;
    assert(bindParamToMeasurement(stale, 0, "size_z", {"gone", "width"}).ok);
    assert(bindParamToMeasurement(stale, 0, "loc_z", {"plank", "girth"}).ok); // unknown field
    assert(bindParamToMeasurement(stale, 1, "depth", {"hole", "length"}).ok); // wrong kind
    assert(bindParamToMeasurement(stale, 1, "loc_z", {"cut", "radius"}).ok);  // radius on a line
    const ResolvedRecipe broken = resolveRecipe(stale, drafting, grid);
    assert(!broken.ok);
    assert(!paramOf(broken.steps[0], "size_z").ok);
    assert(paramOf(broken.steps[0], "size_z").message == "object not found: gone");
    assert(!paramOf(broken.steps[0], "loc_z").ok);
    assert(paramOf(broken.steps[0], "loc_z").message == "unknown measurement field: girth");
    assert(paramOf(broken.steps[0], "size_x").ok);   // healthy bindings unaffected
    assert(!paramOf(broken.steps[1], "depth").ok);   // length on a circle: refused
    assert(paramOf(broken.steps[1], "depth").message == "length needs a line");
    assert(!paramOf(broken.steps[1], "loc_z").ok);
    assert(paramOf(broken.steps[1], "loc_z").message == "radius needs a circle or arc");
    assert(paramOf(broken.steps[1], "radius").ok);

    // ---- Lathe + profiles (column phase C2) --------------------------------
    // The lathe takes a PROFILE: a drafted object whose exact coordinates
    // become physical (radius, height) pairs. Page-left is the spin axis,
    // page-bottom is z = 0 — drafted x scales along grid X like every radius,
    // drafted height stands the part up (z = (1 - y) * grid height).
    {
        assert(findShaper("lathe") != nullptr);
        assert(findShaper("lathe")->primitive);
        assert(findShaper("lathe")->needsProfile);
        assert(!findShaper("cube")->needsProfile);

        DraftingDocument profileDoc = makeDraftingDocument("profiles");
        PolylineGeometry shaft;
        shaft.vertices = {{0.105, 0.90}, {0.085, 0.20}};
        assert(addObject(profileDoc, object("shaft", DraftingShapeKind::Polyline, shaft)).ok);
        assert(addObject(profileDoc, object("flare", DraftingShapeKind::Arc,
            ArcGeometry{{0.5, 0.5}, 0.25, 0.0, 90.0})).ok);
        assert(addObject(profileDoc, object("disc", DraftingShapeKind::Circle,
            CircleGeometry{{0.5, 0.5}, 0.1})).ok);

        RecipeDocument lathed;
        lathed.id = "turned";
        assert(addShaperStep(lathed, "lathe").ok);
        // Binding rules: only profile-taking shapers accept one.
        assert(addShaperStep(lathed, "cube").ok);
        assert(!setStepProfile(lathed, 1, "shaft").ok);
        assert(!setStepProfile(lathed, 0, "").ok);
        assert(setStepProfile(lathed, 0, "shaft").ok);

        const ResolvedRecipe turned = resolveRecipe(lathed, profileDoc, grid);
        assert(turned.ok);
        const ResolvedStep &lathe = turned.steps[0];
        assert(lathe.profileOk);
        assert(lathe.profilePoints.size() == 2);
        assert(near(lathe.profilePoints[0].x, 0.105 * 12.0)); // r = 1.26
        assert(near(lathe.profilePoints[0].y, 0.10 * 8.0));   // z = 0.8 (page bottom up)
        assert(near(lathe.profilePoints[1].x, 0.085 * 12.0)); // r = 1.02
        assert(near(lathe.profilePoints[1].y, 0.80 * 8.0));   // z = 6.4

        // An arc samples deterministically: 64 segments per full circle, so
        // a quarter sweep is 16 segments = 17 points with exact endpoints.
        RecipeDocument arced;
        assert(addShaperStep(arced, "lathe").ok);
        assert(setStepProfile(arced, 0, "flare").ok);
        const ResolvedRecipe flared = resolveRecipe(arced, profileDoc, grid);
        assert(flared.ok);
        const ResolvedStep &flareStep = flared.steps[0];
        assert(flareStep.profilePoints.size() == 17);
        assert(near(flareStep.profilePoints.front().x, 0.75 * 12.0)); // arc start (0.75, 0.5)
        assert(near(flareStep.profilePoints.front().y, 0.50 * 8.0));
        assert(near(flareStep.profilePoints.back().x, 0.50 * 12.0)); // arc end (0.5, 0.75)
        assert(near(flareStep.profilePoints.back().y, 0.25 * 8.0));

        // Failure modes fail the STEP with a message, like stale bindings:
        // nothing bound, object gone, or a kind that is not a profile.
        RecipeDocument unbound;
        assert(addShaperStep(unbound, "lathe").ok);
        const ResolvedRecipe unresolvedLathe = resolveRecipe(unbound, profileDoc, grid);
        assert(!unresolvedLathe.ok);
        assert(!unresolvedLathe.steps[0].profileOk);
        assert(unresolvedLathe.steps[0].profileMessage == "no profile bound");

        RecipeDocument missing;
        assert(addShaperStep(missing, "lathe").ok);
        assert(setStepProfile(missing, 0, "gone").ok);
        const ResolvedRecipe missingResolved = resolveRecipe(missing, profileDoc, grid);
        assert(!missingResolved.steps[0].profileOk);
        // Contract pin (R1 migration) — see docs/recipe_binding_contract.md.
        assert(missingResolved.steps[0].profileMessage == "profile object not found: gone");

        RecipeDocument wrongKind;
        assert(addShaperStep(wrongKind, "lathe").ok);
        assert(setStepProfile(wrongKind, 0, "disc").ok);
        const ResolvedRecipe disced = resolveRecipe(wrongKind, profileDoc, grid);
        assert(!disced.steps[0].profileOk);
        assert(disced.steps[0].profileMessage == "profile needs a line, polyline, or arc");

        // Non-finite literals are rejected at the op — the one gate every
        // entry path (UI, TOML loader) shares.
        RecipeDocument finiteOnly;
        assert(addShaperStep(finiteOnly, "cube").ok);
        assert(!setParamLiteral(finiteOnly, 0, "size_x", std::numeric_limits<double>::quiet_NaN()).ok);
        assert(!setParamLiteral(finiteOnly, 0, "size_x", std::numeric_limits<double>::infinity()).ok);

        // A one-vertex polyline cannot be built through the ops, but the
        // .edidraw LOAD path does not re-validate geometry — hand-assemble
        // the corrupt object the way a damaged file would deliver it.
        DraftingObject corrupt;
        corrupt.id = "stub";
        corrupt.kind = DraftingShapeKind::Polyline;
        PolylineGeometry oneVertex;
        oneVertex.vertices = {{0.5, 0.5}};
        corrupt.geometry = oneVertex;
        DraftingDocument corruptDoc = makeDraftingDocument("corrupt");
        corruptDoc.objects.push_back(corrupt);
        RecipeDocument stubbed;
        assert(addShaperStep(stubbed, "lathe").ok);
        assert(setStepProfile(stubbed, 0, "stub").ok);
        const ResolvedRecipe stubResolved = resolveRecipe(stubbed, corruptDoc, grid);
        assert(!stubResolved.steps[0].profileOk);
        assert(stubResolved.steps[0].profileMessage == "profile polyline needs at least two vertices");
    }

    return 0;
}
