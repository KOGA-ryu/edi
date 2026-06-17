#include "recipe/RecipeOps.h"

namespace edi::recipe {

const std::vector<std::string> &recipeMaterialTable()
{
    // v0 shipped stone + limestone; its README names the planned passes.
    // One table, strictly checked — a typo'd material must be a finding,
    // not a silent repaint to stone.
    static const std::vector<std::string> table = {
        "stone", "limestone", "marble", "sandstone", "aged_stone", "iron", "glass",
    };
    return table;
}

bool recipeMaterialSupported(const std::string &material)
{
    for (const std::string &known : recipeMaterialTable()) {
        if (material == known) {
            return true;
        }
    }
    return false;
}

const char *recipeOpTypeName(const RecipeOp &op)
{
    struct Namer {
        const char *operator()(const AddBoxOp &) const { return "AddBox"; }
        const char *operator()(const AddCylinderOp &) const { return "AddCylinder"; }
        const char *operator()(const AddSphereOp &) const { return "AddSphere"; }
        const char *operator()(const AddRingOp &) const { return "AddRing"; }
        const char *operator()(const AddMouldingOp &) const { return "AddMoulding"; }
        const char *operator()(const AddPrismOp &) const { return "AddPrism"; }
        const char *operator()(const AddProfileMouldingOp &) const { return "AddProfileMoulding"; }
        const char *operator()(const AddRevolvedProfileOp &) const { return "AddRevolvedProfile"; }
        const char *operator()(const AddExtrudedProfileOp &) const { return "AddExtrudedProfile"; }
        const char *operator()(const CutFlutesOp &) const { return "CutFlutes"; }
        const char *operator()(const AddLabelOp &) const { return "AddLabel"; }
        const char *operator()(const ScriptOp &) const { return "Script"; }
    };
    return std::visit(Namer{}, op);
}

std::string recipeScriptParamKeyProblem(const std::string &key)
{
    if (key.empty()) {
        return "param key must not be empty";
    }
    // The keys the Script op writes itself: a param of the same name would
    // overwrite that field on write (name/x/…) or, for "type", make the op
    // unreadable on reload. The python parse_ops shadows these too.
    static const char *const reserved[] = {"type", "script", "name", "x", "y", "z"};
    for (const char *const taken : reserved) {
        if (key == taken) {
            return "param key '" + key + "' collides with the built-in field '" + key + "'";
        }
    }
    // TOML bare-key charset MINUS '.' ('.' is our prefix separator and nests a
    // table under tomllib): letters, digits, '_' and '-'. A char outside it
    // either breaks the emitted line or diverges between the two readers.
    for (const char ch : key) {
        const bool bare = (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z')
            || (ch >= '0' && ch <= '9') || ch == '_' || ch == '-';
        if (!bare) {
            return "param key '" + key + "' must be letters, digits, '_' or '-'";
        }
    }
    return "";
}

RecipeCompileResult compileRecipeOps(const std::vector<RecipeOp> &ops)
{
    RecipeCompileResult result;
    result.ops.reserve(ops.size());
    for (const RecipeOp &op : ops) {
        // A revolved profile reaching compile means the resolve pass never
        // ran: refuse by name. Compile cannot read the drafting document,
        // and inventing the points would be guesswork wearing a default.
        if (const auto *revolved = std::get_if<AddRevolvedProfileOp>(&op)) {
            result.message =
                "AddRevolvedProfile must be resolved before compiling: " + revolved->name;
            result.ops.clear();
            return result;
        }
        // Same contract for the extrude (BL-01): a profile reference that
        // reached compile means resolve never lowered it. Refuse by name.
        if (const auto *extruded = std::get_if<AddExtrudedProfileOp>(&op)) {
            result.message =
                "AddExtrudedProfile must be resolved before compiling: " + extruded->name;
            result.ops.clear();
            return result;
        }
        const auto *profileMoulding = std::get_if<AddProfileMouldingOp>(&op);
        if (profileMoulding == nullptr) {
            result.ops.push_back(op);
            continue;
        }
        const MouldingCompileResult compiled =
            compileMouldingSequence(profileMoulding->name, profileMoulding->sequence);
        if (!compiled.ok) {
            result.message = compiled.message;
            result.ops.clear();
            return result;
        }
        AddMouldingOp lowered;
        lowered.name = profileMoulding->name;
        lowered.baseZ = profileMoulding->baseZ;
        lowered.profile = compiled.points;
        lowered.x = profileMoulding->x;
        lowered.y = profileMoulding->y;
        lowered.vertices = profileMoulding->vertices;
        lowered.material = profileMoulding->material;
        result.ops.push_back(std::move(lowered));
    }
    result.ok = true;
    return result;
}

const std::vector<std::string> &recipePaletteOpTypes()
{
    // The primitives a single click can append as a VALID step. The mouldings,
    // the lathe, and flutes need a sequence/reference/target to be valid, so
    // they are authored elsewhere, not one-click-added.
    static const std::vector<std::string> kTypes = {
        "AddBox", "AddCylinder", "AddSphere", "AddRing"};
    return kTypes;
}

std::optional<RecipeOp> makeRecipeOp(const std::string &typeName, const std::string &name)
{
    // Unit starter dimensions so a freshly clicked step is immediately visible
    // in the proof; the human then tunes it through the Steps inspector.
    if (typeName == "AddBox") {
        AddBoxOp op;
        op.name = name;
        op.width = op.depth = op.height = 1.0;
        return RecipeOp{op};
    }
    if (typeName == "AddCylinder") {
        AddCylinderOp op;
        op.name = name;
        op.radius = 0.5;
        op.height = 1.0;
        return RecipeOp{op};
    }
    if (typeName == "AddSphere") {
        AddSphereOp op;
        op.name = name;
        op.radius = 0.5;
        return RecipeOp{op};
    }
    if (typeName == "AddRing") {
        AddRingOp op;
        op.name = name;
        op.radius = 0.5;
        op.tubeHeight = 0.25;
        return RecipeOp{op};
    }
    return std::nullopt;
}

void removeRecipeOp(RecipeOpStream &stream, std::size_t index)
{
    if (index >= stream.ops.size()) {
        return;
    }
    stream.ops.erase(stream.ops.begin() + static_cast<std::ptrdiff_t>(index));
    std::vector<RecipeFieldBinding> kept;
    kept.reserve(stream.bindings.size());
    for (RecipeFieldBinding binding : stream.bindings) {
        if (binding.opIndex == index) {
            continue; // its op is gone
        }
        if (binding.opIndex > index) {
            --binding.opIndex; // everything after slid down one
        }
        kept.push_back(binding);
    }
    stream.bindings = std::move(kept);
}

void moveRecipeOp(RecipeOpStream &stream, std::size_t from, std::size_t to)
{
    if (from >= stream.ops.size() || to >= stream.ops.size() || from == to) {
        return;
    }
    RecipeOp moved = std::move(stream.ops[from]);
    stream.ops.erase(stream.ops.begin() + static_cast<std::ptrdiff_t>(from));
    stream.ops.insert(stream.ops.begin() + static_cast<std::ptrdiff_t>(to), std::move(moved));
    // Remap binding indices through the same erase+insert: the moved op carries
    // its bindings to `to`, and the ops that slid over shift by one.
    for (RecipeFieldBinding &binding : stream.bindings) {
        if (binding.opIndex == from) {
            binding.opIndex = to;
        } else if (from < to && binding.opIndex > from && binding.opIndex <= to) {
            --binding.opIndex;
        } else if (to < from && binding.opIndex >= to && binding.opIndex < from) {
            ++binding.opIndex;
        }
    }
}

} // namespace edi::recipe
