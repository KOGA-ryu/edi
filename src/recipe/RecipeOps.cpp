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
        const char *operator()(const AddProfileMouldingOp &) const { return "AddProfileMoulding"; }
        const char *operator()(const CutFlutesOp &) const { return "CutFlutes"; }
        const char *operator()(const AddLabelOp &) const { return "AddLabel"; }
    };
    return std::visit(Namer{}, op);
}

RecipeCompileResult compileRecipeOps(const std::vector<RecipeOp> &ops)
{
    RecipeCompileResult result;
    result.ops.reserve(ops.size());
    for (const RecipeOp &op : ops) {
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

} // namespace edi::recipe
