// The craftsmen registry: parse the TOML `edi_craft.py --list-craftsmen`
// emits, and seed a Script step from a manifest. The pure half of the lab's
// custom-craftsman palette — no Qt, so the parse + factory are pinned here and
// the widget slice only wires them.
#include "recipe/RecipeCraftsmen.h"

#include "recipe/RecipeOpsStore.h"
#include "recipe/RecipeOpsValidate.h"

#include <cassert>
#include <string>

using namespace edi::recipe;

namespace {

// The exact shape `edi_craft.py --list-craftsmen` prints for the sample
// craftsman (tools/blender/craftsmen/twisted_column.py). Pinned verbatim so a
// drift in either half fails a test, not a user's palette.
const char *kTwistedRegistry =
    "craftsman.0.id = \"twisted_column\"\n"
    "craftsman.0.label = \"Twisted Column\"\n"
    "craftsman.0.param.0.key = \"radius\"\n"
    "craftsman.0.param.0.label = \"Radius\"\n"
    "craftsman.0.param.0.type = \"number\"\n"
    "craftsman.0.param.0.default = \"1.0\"\n"
    "craftsman.0.param.1.key = \"height\"\n"
    "craftsman.0.param.1.label = \"Height\"\n"
    "craftsman.0.param.1.type = \"number\"\n"
    "craftsman.0.param.1.default = \"5.0\"\n"
    "craftsman.0.param.2.key = \"sides\"\n"
    "craftsman.0.param.2.label = \"Sides\"\n"
    "craftsman.0.param.2.type = \"integer\"\n"
    "craftsman.0.param.2.default = \"4\"\n"
    "craftsman.0.param.3.key = \"turns\"\n"
    "craftsman.0.param.3.label = \"Turns\"\n"
    "craftsman.0.param.3.type = \"number\"\n"
    "craftsman.0.param.3.default = \"1.0\"\n"
    "craftsman.0.param.4.key = \"rings\"\n"
    "craftsman.0.param.4.label = \"Rings\"\n"
    "craftsman.0.param.4.type = \"integer\"\n"
    "craftsman.0.param.4.default = \"24\"\n"
    "craftsman.0.param.5.key = \"material\"\n"
    "craftsman.0.param.5.label = \"Material\"\n"
    "craftsman.0.param.5.type = \"material\"\n"
    "craftsman.0.param.5.default = \"stone\"\n";

} // namespace

int main()
{
    // ---- Parse the real registry: one craftsman, six typed params, in order.
    {
        const CraftsmanRegistryResult reg = parseCraftsmanRegistryToml(kTwistedRegistry, "registry");
        assert(reg.ok);
        assert(reg.craftsmen.size() == 1);
        const CraftsmanManifest &m = reg.craftsmen[0];
        assert(m.id == "twisted_column" && m.label == "Twisted Column");
        assert(m.params.size() == 6);
        assert(m.params[0].key == "radius" && m.params[0].label == "Radius"
               && m.params[0].type == "number" && m.params[0].defaultValue == "1.0");
        assert(m.params[2].key == "sides" && m.params[2].type == "integer"
               && m.params[2].defaultValue == "4");
        assert(m.params[5].key == "material" && m.params[5].type == "material"
               && m.params[5].defaultValue == "stone");

        // findCraftsman: hit and miss.
        assert(findCraftsman(reg.craftsmen, "twisted_column") == &m);
        assert(findCraftsman(reg.craftsmen, "nope") == nullptr);

        // ---- makeScriptOp seeds scriptId + every param at its default.
        const ScriptOp seeded = makeScriptOp(m, "twist.core");
        assert(seeded.scriptId == "twisted_column" && seeded.name == "twist.core");
        assert(seeded.params.size() == 6);
        assert(seeded.params[0].key == "radius" && seeded.params[0].value == "1.0");
        assert(seeded.params[2].key == "sides" && seeded.params[2].value == "4");
        assert(seeded.params[5].key == "material" && seeded.params[5].value == "stone");

        // An empty name falls back to the craftsman id (the store/python default).
        const ScriptOp unnamed = makeScriptOp(m, "");
        assert(unnamed.name == "twisted_column");

        // The seeded step is VALID (the factory makes nothing the pipeline
        // refuses) and round-trips through the store unchanged.
        const OpValidationReport report = validateRecipeOps({RecipeOp{seeded}});
        assert(report.ok);
        RecipeOpStream stream;
        stream.ops.push_back(seeded);
        const OpStreamTextResult written = recipeOpsToToml(stream);
        assert(written.ok);
        const OpStreamParseResult reloaded = recipeOpsFromToml(written.text, "seeded");
        assert(reloaded.ok && reloaded.stream.ops.size() == 1);
        const auto *back = std::get_if<ScriptOp>(&reloaded.stream.ops[0]);
        assert(back != nullptr && back->scriptId == "twisted_column" && back->params.size() == 6);
    }

    // ---- An empty registry is valid (no craftsmen installed): ok, empty list.
    {
        const CraftsmanRegistryResult empty = parseCraftsmanRegistryToml("", "empty");
        assert(empty.ok && empty.craftsmen.empty());
    }

    // ---- Multiple craftsmen parse in index order; a param-less craftsman is
    // fine; missing label/type fall back to the id/key and "text".
    {
        const CraftsmanRegistryResult reg = parseCraftsmanRegistryToml(
            "craftsman.0.id = \"alpha\"\n"
            "craftsman.0.label = \"Alpha\"\n"
            "craftsman.0.param.0.key = \"size\"\n"
            "craftsman.0.param.0.default = \"3\"\n" // no label, no type
            "craftsman.1.id = \"beta\"\n",          // no label, no params
            "multi");
        assert(reg.ok && reg.craftsmen.size() == 2);
        assert(reg.craftsmen[0].id == "alpha" && reg.craftsmen[0].label == "Alpha");
        assert(reg.craftsmen[0].params.size() == 1);
        assert(reg.craftsmen[0].params[0].key == "size"
               && reg.craftsmen[0].params[0].label == "size"   // fell back to the key
               && reg.craftsmen[0].params[0].type == "text"    // fell back to text
               && reg.craftsmen[0].params[0].defaultValue == "3");
        assert(reg.craftsmen[1].id == "beta" && reg.craftsmen[1].label == "beta"
               && reg.craftsmen[1].params.empty());

        // makeScriptOp on a param-less craftsman: a bare Script step, valid.
        const ScriptOp bare = makeScriptOp(reg.craftsmen[1], "b0");
        assert(bare.scriptId == "beta" && bare.params.empty());
        assert(validateRecipeOps({RecipeOp{bare}}).ok);
    }

    return 0;
}
