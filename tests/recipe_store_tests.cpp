#include "recipe/RecipeStore.h"

#include <cassert>
#include <string>

using namespace edi::recipe;

namespace {

bool sameParam(const RecipeParam &a, const RecipeParam &b)
{
    if (a.id != b.id || a.source != b.source) {
        return false;
    }
    if (a.source == ParamSource::Measurement) {
        return a.measurement.objectId == b.measurement.objectId
            && a.measurement.field == b.measurement.field;
    }
    return a.value == b.value;
}

} // namespace

int main()
{
    // A column-shaped recipe: stacked primitives, a lathe with a profile
    // binding, grooves, and one measurement binding — every feature the
    // format must carry.
    RecipeDocument recipe;
    recipe.id = "doric_column";
    recipe.name = "Doric Column";
    assert(addShaperStep(recipe, "cube").ok);
    assert(setParamLiteral(recipe, 0, "size_x", 0.3).ok);
    assert(setParamLiteral(recipe, 0, "size_z", 0.045).ok);
    assert(addShaperStep(recipe, "lathe").ok);
    assert(setStepProfile(recipe, 1, "shaft_profile").ok);
    assert(setParamLiteral(recipe, 1, "segments", 48.0).ok);
    assert(addShaperStep(recipe, "radial_groove").ok);
    assert(setParamLiteral(recipe, 2, "count", 20.0).ok);
    assert(bindParamToMeasurement(recipe, 2, "at_radius", {"shaft_top", "radius"}).ok);

    const RecipeTextResult written = recipeToToml(recipe);
    assert(written.ok);
    // The keys are the pointable surface: a person (or an AI under
    // instruction) names the exact line to change.
    assert(written.text.find("step.1.profile = \"shaft_profile\"") != std::string::npos);
    assert(written.text.find("0.3") != std::string::npos);     // shortest round-trip text
    assert(written.text.find("0.29999") == std::string::npos); // never binary noise

    const RecipeParseResult reloaded = recipeFromToml(written.text, "round_trip");
    assert(reloaded.ok);
    const RecipeDocument &loaded = reloaded.document;
    assert(loaded.id == recipe.id);
    assert(loaded.name == recipe.name);
    assert(loaded.steps.size() == recipe.steps.size());
    for (std::size_t i = 0; i < recipe.steps.size(); ++i) {
        assert(loaded.steps[i].shaperId == recipe.steps[i].shaperId);
        assert(loaded.steps[i].profileObjectId == recipe.steps[i].profileObjectId);
        assert(loaded.steps[i].params.size() == recipe.steps[i].params.size());
        for (std::size_t p = 0; p < recipe.steps[i].params.size(); ++p) {
            assert(sameParam(loaded.steps[i].params[p], recipe.steps[i].params[p]));
        }
    }

    // Every parameter is written, defaults included: a key must EXIST in the
    // file to be pointable ("change step.0.param.size_y.value"), and a file
    // that hides defaults makes the reader consult the C++ vocabulary to
    // know what a recipe will do. Loading still tolerates absent keys, so
    // hand-trimmed files keep working.
    assert(written.text.find("size_y") != std::string::npos);

    // STRICT loading: every failure names its offender instead of guessing.
    {
        const RecipeParseResult unknownShaper = recipeFromToml(
            "step.0.shaper = \"router\"\n", "bad");
        assert(!unknownShaper.ok);
        assert(unknownShaper.message.find("router") != std::string::npos);

        const RecipeParseResult modifierFirst = recipeFromToml(
            "step.0.shaper = \"bevel\"\n", "bad");
        assert(!modifierFirst.ok);

        const RecipeParseResult typoKey = recipeFromToml(
            "step.0.shaper = \"cube\"\n"
            "step.0.param.size_zz.value = \"2\"\n", "bad");
        assert(!typoKey.ok);
        assert(typoKey.message.find("size_zz") != std::string::npos);

        const RecipeParseResult badNumber = recipeFromToml(
            "step.0.shaper = \"cube\"\n"
            "step.0.param.size_x.value = \"wide\"\n", "bad");
        assert(!badNumber.ok);

        const RecipeParseResult halfBinding = recipeFromToml(
            "step.0.shaper = \"cube\"\n"
            "step.0.param.size_x.object = \"plank\"\n", "bad");
        assert(!halfBinding.ok);
        assert(halfBinding.message.find("both") != std::string::npos);

        const RecipeParseResult profileOnCube = recipeFromToml(
            "step.0.shaper = \"cube\"\n"
            "step.0.profile = \"shaft\"\n", "bad");
        assert(!profileOnCube.ok);
    }

    // An empty file is an empty (unstarted) recipe, not an error: validity
    // is validateRecipe's question, parse only answers "is this well-formed".
    const RecipeParseResult empty = recipeFromToml("", "empty");
    assert(empty.ok);
    assert(empty.document.steps.empty());

    return 0;
}
