#include "recipe/RecipeOpsBind.h"

#include <algorithm>

namespace edi::recipe {

namespace {

// One row: a TOML field key and where that number lives in the struct.
// Pointer-to-member is the data-oriented dispatch here — the registry
// stays a table, and "which field" never becomes an if-ladder in three
// different consumers.
template <typename Op>
struct FieldRow {
    const char *key;
    double Op::*member;
};

constexpr FieldRow<AddBoxOp> kBoxFields[] = {
    {"width", &AddBoxOp::width},   {"depth", &AddBoxOp::depth},
    {"height", &AddBoxOp::height}, {"z", &AddBoxOp::z},
    {"x", &AddBoxOp::x},           {"y", &AddBoxOp::y},
};

constexpr FieldRow<AddCylinderOp> kCylinderFields[] = {
    {"radius", &AddCylinderOp::radius}, {"height", &AddCylinderOp::height},
    {"z", &AddCylinderOp::z},           {"x", &AddCylinderOp::x},
    {"y", &AddCylinderOp::y},           {"entasis_ratio", &AddCylinderOp::entasisRatio},
};

constexpr FieldRow<AddSphereOp> kSphereFields[] = {
    {"radius", &AddSphereOp::radius},
    {"z", &AddSphereOp::z},
    {"x", &AddSphereOp::x},
    {"y", &AddSphereOp::y},
};

constexpr FieldRow<AddRingOp> kRingFields[] = {
    {"radius", &AddRingOp::radius}, {"tube_height", &AddRingOp::tubeHeight},
    {"z", &AddRingOp::z},           {"overhang", &AddRingOp::overhang},
    {"x", &AddRingOp::x},           {"y", &AddRingOp::y},
};

constexpr FieldRow<AddMouldingOp> kMouldingFields[] = {
    {"base_z", &AddMouldingOp::baseZ},
    {"x", &AddMouldingOp::x},
    {"y", &AddMouldingOp::y},
    // BL-06: a drafted angle could drive the partial-revolve arc, so
    // sweep_degrees is bindable like the other numerics (kept in step with the
    // store reader's bindableNumber read and the schema's Number scalar).
    {"sweep_degrees", &AddMouldingOp::sweepDegrees},
    // BL-07: screw/helix params, bindable like sweep_degrees (a drafted
    // measurement could drive the per-turn rise or the turn count).
    {"screw_rise", &AddMouldingOp::screwRise},
    {"screw_turns", &AddMouldingOp::screwTurns},
};

// BL-03: the lowered prism carrier. Its numeric fields mirror AddMoulding's
// (base_z/x/y) plus `height`, kept in step with the store reader and inspector.
constexpr FieldRow<AddPrismOp> kPrismFields[] = {
    {"height", &AddPrismOp::height},
    {"base_z", &AddPrismOp::baseZ},
    {"x", &AddPrismOp::x},
    {"y", &AddPrismOp::y},
};

constexpr FieldRow<AddProfileMouldingOp> kProfileMouldingFields[] = {
    {"base_z", &AddProfileMouldingOp::baseZ},
    {"x", &AddProfileMouldingOp::x},
    {"y", &AddProfileMouldingOp::y},
};

constexpr FieldRow<AddRevolvedProfileOp> kRevolvedProfileFields[] = {
    {"base_z", &AddRevolvedProfileOp::baseZ},
    {"x", &AddRevolvedProfileOp::x},
    {"y", &AddRevolvedProfileOp::y},
    {"sweep_degrees", &AddRevolvedProfileOp::sweepDegrees}, // BL-06, see kMouldingFields
    {"screw_rise", &AddRevolvedProfileOp::screwRise},       // BL-07
    {"screw_turns", &AddRevolvedProfileOp::screwTurns},     // BL-07
};

// BL-01: the extrude exposes the lathe's baseZ/x/y plus its own `height` —
// the new bindable field, so a drafted measurement can drive the extrude depth.
constexpr FieldRow<AddExtrudedProfileOp> kExtrudedProfileFields[] = {
    {"height", &AddExtrudedProfileOp::height},
    {"base_z", &AddExtrudedProfileOp::baseZ},
    {"x", &AddExtrudedProfileOp::x},
    {"y", &AddExtrudedProfileOp::y},
};

// BL-08: the sweep exposes baseZ/x/y; profile and path are drafted-object
// string references (the object picker owns them), never bindable doubles.
constexpr FieldRow<AddSweepProfileOp> kSweepProfileFields[] = {
    {"base_z", &AddSweepProfileOp::baseZ},
    {"x", &AddSweepProfileOp::x},
    {"y", &AddSweepProfileOp::y},
};

constexpr FieldRow<CutFlutesOp> kFluteFields[] = {
    {"depth", &CutFlutesOp::depth},
    {"width_ratio", &CutFlutesOp::widthRatio},
};

constexpr FieldRow<AddLabelOp> kLabelFields[] = {
    {"x", &AddLabelOp::x},
    {"y", &AddLabelOp::y},
    {"z", &AddLabelOp::z},
};

// A custom craftsman's PLACEMENT is bindable like every other op's (drive the
// step from a drafted measurement); its untyped param bag is NOT — bindings
// resolve to doubles, and the params are opaque strings the craftsman coerces.
constexpr FieldRow<ScriptOp> kScriptFields[] = {
    {"x", &ScriptOp::x},
    {"y", &ScriptOp::y},
    {"z", &ScriptOp::z},
};

template <typename Op, std::size_t N>
double Op::*findMember(const FieldRow<Op> (&rows)[N], const std::string &fieldKey)
{
    for (const FieldRow<Op> &row : rows) {
        if (fieldKey == row.key) {
            return row.member;
        }
    }
    return nullptr;
}

// Visitor over the variant, parameterized by what to do with a found
// member — lookup and write share the one table walk.
template <typename Handler>
struct FieldVisit {
    const std::string &fieldKey;
    Handler handle;

    bool operator()(AddBoxOp &op) const { return handle(op, findMember(kBoxFields, fieldKey)); }
    bool operator()(AddCylinderOp &op) const { return handle(op, findMember(kCylinderFields, fieldKey)); }
    bool operator()(AddSphereOp &op) const { return handle(op, findMember(kSphereFields, fieldKey)); }
    bool operator()(AddRingOp &op) const { return handle(op, findMember(kRingFields, fieldKey)); }
    bool operator()(AddMouldingOp &op) const { return handle(op, findMember(kMouldingFields, fieldKey)); }
    bool operator()(AddPrismOp &op) const { return handle(op, findMember(kPrismFields, fieldKey)); }
    bool operator()(AddProfileMouldingOp &op) const { return handle(op, findMember(kProfileMouldingFields, fieldKey)); }
    bool operator()(AddRevolvedProfileOp &op) const { return handle(op, findMember(kRevolvedProfileFields, fieldKey)); }
    bool operator()(AddExtrudedProfileOp &op) const { return handle(op, findMember(kExtrudedProfileFields, fieldKey)); }
    bool operator()(AddSweepProfileOp &op) const { return handle(op, findMember(kSweepProfileFields, fieldKey)); }
    bool operator()(CutFlutesOp &op) const { return handle(op, findMember(kFluteFields, fieldKey)); }
    bool operator()(AddLabelOp &op) const { return handle(op, findMember(kLabelFields, fieldKey)); }
    bool operator()(ScriptOp &op) const { return handle(op, findMember(kScriptFields, fieldKey)); }
};

// Lists every {key, value} of an op by walking its registry table — the read
// twin of setOpFieldValue's write, over the same rows.
struct FieldList {
    template <typename Op, std::size_t N>
    static std::vector<RecipeOpField> of(const Op &op, const FieldRow<Op> (&rows)[N])
    {
        std::vector<RecipeOpField> fields;
        fields.reserve(N);
        for (const FieldRow<Op> &row : rows) {
            fields.push_back({row.key, op.*(row.member)});
        }
        return fields;
    }
    std::vector<RecipeOpField> operator()(const AddBoxOp &op) const { return of(op, kBoxFields); }
    std::vector<RecipeOpField> operator()(const AddCylinderOp &op) const { return of(op, kCylinderFields); }
    std::vector<RecipeOpField> operator()(const AddSphereOp &op) const { return of(op, kSphereFields); }
    std::vector<RecipeOpField> operator()(const AddRingOp &op) const { return of(op, kRingFields); }
    std::vector<RecipeOpField> operator()(const AddMouldingOp &op) const { return of(op, kMouldingFields); }
    std::vector<RecipeOpField> operator()(const AddPrismOp &op) const { return of(op, kPrismFields); }
    std::vector<RecipeOpField> operator()(const AddProfileMouldingOp &op) const { return of(op, kProfileMouldingFields); }
    std::vector<RecipeOpField> operator()(const AddRevolvedProfileOp &op) const { return of(op, kRevolvedProfileFields); }
    std::vector<RecipeOpField> operator()(const AddExtrudedProfileOp &op) const { return of(op, kExtrudedProfileFields); }
    std::vector<RecipeOpField> operator()(const AddSweepProfileOp &op) const { return of(op, kSweepProfileFields); }
    std::vector<RecipeOpField> operator()(const CutFlutesOp &op) const { return of(op, kFluteFields); }
    std::vector<RecipeOpField> operator()(const AddLabelOp &op) const { return of(op, kLabelFields); }
    std::vector<RecipeOpField> operator()(const ScriptOp &op) const { return of(op, kScriptFields); }
};

} // namespace

bool opFieldBindable(const RecipeOp &op, const std::string &fieldKey)
{
    const auto exists = [](auto &, auto member) { return member != nullptr; };
    // The visit only reads, but the visitor shape is shared with the
    // writing path — const_cast localizes the compromise to one line.
    return std::visit(FieldVisit<decltype(exists)>{fieldKey, exists},
                      const_cast<RecipeOp &>(op));
}

bool setOpFieldValue(RecipeOp &op, const std::string &fieldKey, double value)
{
    const auto write = [value](auto &concreteOp, auto member) {
        if (member == nullptr) {
            return false;
        }
        concreteOp.*member = value;
        return true;
    };
    return std::visit(FieldVisit<decltype(write)>{fieldKey, write}, op);
}

std::vector<RecipeOpField> opFields(const RecipeOp &op)
{
    return std::visit(FieldList{}, op);
}

void clearRecipeBinding(RecipeOpStream &stream, std::size_t opIndex, const std::string &fieldKey)
{
    auto &bindings = stream.bindings;
    bindings.erase(std::remove_if(bindings.begin(), bindings.end(),
                                  [&](const RecipeFieldBinding &binding) {
                                      return binding.opIndex == opIndex && binding.fieldKey == fieldKey;
                                  }),
                   bindings.end());
}

bool addRecipeBinding(RecipeOpStream &stream, std::size_t opIndex, const std::string &fieldKey,
                      const std::string &objectId, const std::string &field)
{
    if (opIndex >= stream.ops.size() || !opFieldBindable(stream.ops[opIndex], fieldKey)) {
        return false;
    }
    clearRecipeBinding(stream, opIndex, fieldKey); // one binding per field
    stream.bindings.push_back({opIndex, fieldKey, objectId, field});
    return true;
}

const RecipeFieldBinding *findRecipeBinding(const RecipeOpStream &stream, std::size_t opIndex,
                                            const std::string &fieldKey)
{
    for (const RecipeFieldBinding &binding : stream.bindings) {
        if (binding.opIndex == opIndex && binding.fieldKey == fieldKey) {
            return &binding;
        }
    }
    return nullptr;
}

} // namespace edi::recipe
