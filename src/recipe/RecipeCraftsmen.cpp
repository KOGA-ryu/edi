#include "recipe/RecipeCraftsmen.h"

#include "formats/TomlReader.h"

namespace edi::recipe {

CraftsmanRegistryResult parseCraftsmanRegistryToml(const std::string &toml, const std::string &source)
{
    CraftsmanRegistryResult result;
    const auto parsed = edi::formats::readTomlStaticConfig(toml, source);
    if (!parsed.ok) {
        result.message = parsed.message;
        return result;
    }
    const edi::formats::StaticConfig &config = *parsed.value;
    const auto get = [&config](const std::string &key) -> const std::string * {
        const auto found = config.find(key);
        return found == config.end() ? nullptr : &found->second;
    };

    // Walk craftsman.0, craftsman.1, … until an id is missing; for each, walk
    // its param.0, param.1, … until a key is missing. The index-run shape is
    // the same the op store reader uses for op.N — the file's own ordering is
    // the registry's ordering, so a palette button row is a manifest row.
    for (std::size_t c = 0;; ++c) {
        const std::string cp = "craftsman." + std::to_string(c);
        const std::string *id = get(cp + ".id");
        if (id == nullptr) {
            break;
        }
        CraftsmanManifest manifest;
        manifest.id = *id;
        const std::string *label = get(cp + ".label");
        manifest.label = label != nullptr ? *label : *id; // label falls back to the id

        for (std::size_t p = 0;; ++p) {
            const std::string pp = cp + ".param." + std::to_string(p);
            const std::string *key = get(pp + ".key");
            if (key == nullptr) {
                break;
            }
            CraftsmanParam param;
            param.key = *key;
            const std::string *plabel = get(pp + ".label");
            param.label = plabel != nullptr ? *plabel : *key;
            const std::string *ptype = get(pp + ".type");
            param.type = ptype != nullptr ? *ptype : "text"; // unspecified ⇒ a plain string
            if (const std::string *def = get(pp + ".default")) {
                param.defaultValue = *def;
            }
            manifest.params.push_back(std::move(param));
        }
        result.craftsmen.push_back(std::move(manifest));
    }

    result.ok = true;
    return result;
}

const CraftsmanManifest *findCraftsman(const std::vector<CraftsmanManifest> &craftsmen,
                                       const std::string &id)
{
    for (const CraftsmanManifest &manifest : craftsmen) {
        if (manifest.id == id) {
            return &manifest;
        }
    }
    return nullptr;
}

ScriptOp makeScriptOp(const CraftsmanManifest &manifest, const std::string &name)
{
    ScriptOp op;
    op.scriptId = manifest.id;
    op.name = name.empty() ? manifest.id : name;
    op.params.reserve(manifest.params.size());
    for (const CraftsmanParam &param : manifest.params) {
        // Seed each param at its declared default. The keys come from the
        // manifest, so a malformed one (a built-in collision, a non-bare key)
        // is the author's bug — surfaced by the same named refusal the store
        // and validator give, not silently dropped here.
        op.params.push_back({param.key, param.defaultValue});
    }
    return op;
}

} // namespace edi::recipe
