#include "recipe/RecipeOpsBind.h"

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
};

constexpr FieldRow<AddProfileMouldingOp> kProfileMouldingFields[] = {
    {"base_z", &AddProfileMouldingOp::baseZ},
    {"x", &AddProfileMouldingOp::x},
    {"y", &AddProfileMouldingOp::y},
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
    bool operator()(AddProfileMouldingOp &op) const { return handle(op, findMember(kProfileMouldingFields, fieldKey)); }
    bool operator()(CutFlutesOp &op) const { return handle(op, findMember(kFluteFields, fieldKey)); }
    bool operator()(AddLabelOp &op) const { return handle(op, findMember(kLabelFields, fieldKey)); }
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

} // namespace edi::recipe
