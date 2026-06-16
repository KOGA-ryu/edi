#pragma once

#include "recipe/RecipeOps.h"

#include <string>
#include <vector>

namespace edi::recipe {

// The C++ mirror of the python craftsmen registry. edi_craft.py scans
// tools/blender/craftsmen/, reads each script's MANIFEST, and emits the
// registry as TOML (`--list-craftsmen`); this module parses that TOML so the
// lab's palette can offer each craftsman and the inspector can render its
// params. The schema lives in the python script — C++ never re-derives it,
// it carries it as DATA (mirroring how RecipeOpField/RecipeOpScalar carry the
// built-in ops' fields). See [[edi-blender-feature-vision]].

// One craftsman parameter as the MANIFEST declares it. `type` is the RAW
// manifest string ("number"/"integer"/"material"/…) — the widget maps it to an
// editor; the pure core stays Qt-free and keeps the string. `defaultValue` is
// the param's starting string (the craftsman coerces it).
struct CraftsmanParam {
    std::string key;
    std::string label;
    std::string type;
    std::string defaultValue;
};

// One registered craftsman: its dispatch id (the ScriptOp.scriptId), a human
// label for the palette button, and the param schema the inspector renders.
struct CraftsmanManifest {
    std::string id;
    std::string label;
    std::vector<CraftsmanParam> params;
};

struct CraftsmanRegistryResult {
    bool ok = false;
    std::string message;
    std::vector<CraftsmanManifest> craftsmen;
};

// Parse the registry TOML `edi_craft.py --list-craftsmen` emits:
//   craftsman.N.id / .label
//   craftsman.N.param.M.key / .label / .type / .default
// Tolerant by design (a machine-generated file, not user-authored): missing
// label/type default sensibly, a craftsman/param run ends at the first absent
// index, and extra keys are ignored. An empty file is a valid empty registry.
CraftsmanRegistryResult parseCraftsmanRegistryToml(const std::string &toml,
                                                   const std::string &source = {});

// The craftsman with this id, or nullptr — the palette/inspector look-up.
const CraftsmanManifest *findCraftsman(const std::vector<CraftsmanManifest> &craftsmen,
                                       const std::string &id);

// A fresh Script step seeded from a manifest: scriptId = the craftsman id,
// name (falls back to the id when empty, as the store/python reader do), and
// each param at its declared default. The craftsman twin of makeRecipeOp —
// the palette's click-to-append factory.
ScriptOp makeScriptOp(const CraftsmanManifest &manifest, const std::string &name);

} // namespace edi::recipe
