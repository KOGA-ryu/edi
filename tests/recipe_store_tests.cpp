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
    // A value where the store's to_chars (shortest round-trip) and the
    // emitter's %.9g legitimately DIVERGE: the file must carry the lossless
    // form, and the exact-equality round-trip below pins it.
    assert(setParamLiteral(recipe, 0, "size_y", 0.1 + 0.2).ok);
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
    // Pin the FULL line, not a substring: shortest-form output is the
    // contract ("0.3", never "0.300000000000000").
    assert(written.text.find("step.0.param.size_x.value = \"0.3\"") != std::string::npos);

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
    // file to be pointable ("change step.0.param.loc_z.value"), and a file
    // that hides defaults makes the reader consult the C++ vocabulary to
    // know what a recipe will do. Loading still tolerates absent keys, so
    // hand-trimmed files keep working.
    assert(written.text.find("step.0.param.loc_z.value") != std::string::npos);
    // The lossless form of 0.1 + 0.2, verbatim.
    assert(written.text.find("size_y.value = \"0.30000000000000004\"") != std::string::npos);

    // Omitted keys mean the shaper spec's DEFAULT stands: a hand-trimmed
    // file keeps working, and the default's survival through load is part
    // of the binding contract the R1 migration must preserve
    // (docs/recipe_binding_contract.md). Contract pin: nothing else in the
    // suite loads a file with absent params.
    {
        const RecipeParseResult trimmed = recipeFromToml(
            "step.0.shaper = \"cube\"\n"
            "step.0.param.size_x.value = \"2\"\n", "trimmed");
        assert(trimmed.ok);
        const ShaperStep &cube = trimmed.document.steps[0];
        assert(cube.params.size() == 4);
        assert(sameParam(cube.params[0], {"size_x", 2.0, ParamSource::Literal, {}}));
        assert(sameParam(cube.params[1], {"size_y", 1.0, ParamSource::Literal, {}})); // spec default
        assert(sameParam(cube.params[2], {"size_z", 1.0, ParamSource::Literal, {}})); // spec default
        assert(sameParam(cube.params[3], {"loc_z", 0.0, ParamSource::Literal, {}}));  // spec default
    }

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

        // The sweep audits EVERY key. A plural typo must NOT load as an
        // empty recipe (which adoptDocument would then use to wipe the live
        // one); a misspelled recipe.* or alien key rejects the same way.
        const RecipeParseResult pluralTypo = recipeFromToml(
            "steps.0.shaper = \"cube\"\n", "bad");
        assert(!pluralTypo.ok);
        assert(pluralTypo.message.find("steps.0.shaper") != std::string::npos);
        const RecipeParseResult nameTypo = recipeFromToml(
            "recipe.nme = \"Doric\"\n", "bad");
        assert(!nameTypo.ok);
        const RecipeParseResult alienKey = recipeFromToml(
            "florp = \"1\"\n", "bad");
        assert(!alienKey.ok);

        // Both a literal AND a binding on one param: the file would show a
        // number the build ignores — refused, not resolved by precedence.
        const RecipeParseResult bothSources = recipeFromToml(
            "step.0.shaper = \"cube\"\n"
            "step.0.param.size_x.value = \"7\"\n"
            "step.0.param.size_x.object = \"plank\"\n"
            "step.0.param.size_x.field = \"width\"\n", "bad");
        assert(!bothSources.ok);
        assert(bothSources.message.find("both") != std::string::npos);

        // Non-finite literals never enter the document.
        const RecipeParseResult nanLiteral = recipeFromToml(
            "step.0.shaper = \"cube\"\n"
            "step.0.param.size_x.value = \"nan\"\n", "bad");
        assert(!nanLiteral.ok);
        const RecipeParseResult infLiteral = recipeFromToml(
            "step.0.shaper = \"cube\"\n"
            "step.0.param.size_x.value = \"inf\"\n", "bad");
        assert(!infLiteral.ok);

        // Gapped indices name the first MISSING step, not the orphan key.
        const RecipeParseResult gappedSteps = recipeFromToml(
            "step.0.shaper = \"cube\"\n"
            "step.2.shaper = \"cube\"\n", "bad");
        assert(!gappedSteps.ok);
        assert(gappedSteps.message.find("step.1") != std::string::npos);
    }

    // An empty file is an empty (unstarted) recipe, not an error: validity
    // is validateRecipe's question, parse only answers "is this well-formed".
    const RecipeParseResult empty = recipeFromToml("", "empty");
    assert(empty.ok);
    assert(empty.document.steps.empty());

    return 0;
}
