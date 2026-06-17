#include "recipe/RecipeOpSchema.h"

#include "recipe/RecipeOpsBind.h"

namespace edi::recipe {

namespace {

// "z_mode" -> "Z Mode" (mirrors the inspector's own prettifier, kept here so
// the schema carries its labels and the UI never re-derives them).
std::string labelFor(const std::string &key)
{
    std::string label;
    bool capNext = true;
    for (const char ch : key) {
        if (ch == '_') {
            label.push_back(' ');
            capNext = true;
        } else if (capNext && ch >= 'a' && ch <= 'z') {
            label.push_back(static_cast<char>(ch - 'a' + 'A'));
            capNext = false;
        } else {
            label.push_back(ch);
            capNext = false;
        }
    }
    return label;
}

std::string zModeText(ZMode mode) { return mode == ZMode::Base ? "base" : "center"; }
std::string axisText(Axis axis) { return axis == Axis::X ? "x" : axis == Axis::Y ? "y" : "z"; }
ZMode parseZMode(const std::string &text) { return text == "base" ? ZMode::Base : ZMode::Center; }
Axis parseAxis(const std::string &text) { return text == "x" ? Axis::X : text == "y" ? Axis::Y : Axis::Z; }

RecipeOpScalar intField(const std::string &key, int value)
{
    RecipeOpScalar scalar;
    scalar.key = key;
    scalar.label = labelFor(key);
    scalar.kind = RecipeFieldKind::Integer;
    scalar.integer = value;
    return scalar;
}

RecipeOpScalar boolField(const std::string &key, bool value)
{
    RecipeOpScalar scalar;
    scalar.key = key;
    scalar.label = labelFor(key);
    scalar.kind = RecipeFieldKind::Boolean;
    scalar.boolean = value;
    return scalar;
}

RecipeOpScalar textField(const std::string &key, const std::string &value, bool editable = true)
{
    RecipeOpScalar scalar;
    scalar.key = key;
    scalar.label = labelFor(key);
    scalar.kind = RecipeFieldKind::Text;
    scalar.text = value;
    scalar.editable = editable;
    return scalar;
}

RecipeOpScalar choiceField(const std::string &key, const std::string &value, std::vector<std::string> choices)
{
    RecipeOpScalar scalar;
    scalar.key = key;
    scalar.label = labelFor(key);
    scalar.kind = RecipeFieldKind::Choice;
    scalar.text = value;
    scalar.choices = std::move(choices);
    return scalar;
}

RecipeOpScalar materialField(const std::string &value)
{
    return choiceField("material", value, recipeMaterialTable());
}

const std::vector<std::string> kZModes = {"center", "base"};
const std::vector<std::string> kAxes = {"x", "y", "z"};

// The non-double scalars of each op, in display order (the doubles come first,
// from opFields). One overload per op type — the variant visit picks it.
void appendExtras(std::vector<RecipeOpScalar> &out, const AddBoxOp &op)
{
    out.push_back(textField("name", op.name));
    out.push_back(materialField(op.material));
    out.push_back(choiceField("z_mode", zModeText(op.zMode), kZModes));
}
void appendExtras(std::vector<RecipeOpScalar> &out, const AddCylinderOp &op)
{
    out.push_back(textField("name", op.name));
    out.push_back(intField("vertices", op.vertices));
    out.push_back(boolField("entasis", op.entasis));
    out.push_back(choiceField("axis", axisText(op.axis), kAxes));
    out.push_back(choiceField("z_mode", zModeText(op.zMode), kZModes));
    out.push_back(materialField(op.material));
}
void appendExtras(std::vector<RecipeOpScalar> &out, const AddSphereOp &op)
{
    out.push_back(textField("name", op.name));
    out.push_back(intField("vertices", op.vertices));
    out.push_back(materialField(op.material));
}
void appendExtras(std::vector<RecipeOpScalar> &out, const AddRingOp &op)
{
    out.push_back(textField("name", op.name));
    out.push_back(intField("vertices", op.vertices));
    out.push_back(materialField(op.material));
}
void appendExtras(std::vector<RecipeOpScalar> &out, const AddMouldingOp &op)
{
    out.push_back(textField("name", op.name));
    out.push_back(intField("vertices", op.vertices));
    out.push_back(materialField(op.material));
}
void appendExtras(std::vector<RecipeOpScalar> &out, const AddPrismOp &op)
{
    // The doubles (height, base_z, x, y) come from opFields; the extras are the
    // non-numeric scalars. No `vertices` (a prism has a footprint, not a facet
    // count) and the footprint itself is geometry, not an inspector scalar.
    out.push_back(textField("name", op.name));
    out.push_back(materialField(op.material));
}
void appendExtras(std::vector<RecipeOpScalar> &out, const AddProfileMouldingOp &op)
{
    out.push_back(textField("name", op.name));
    out.push_back(intField("vertices", op.vertices));
    out.push_back(materialField(op.material));
}
void appendExtras(std::vector<RecipeOpScalar> &out, const AddRevolvedProfileOp &op)
{
    out.push_back(textField("name", op.name));
    out.push_back(textField("profile", op.profile, /*editable=*/false)); // a drafted object id, picked elsewhere
    out.push_back(intField("vertices", op.vertices));
    out.push_back(materialField(op.material));
}
void appendExtras(std::vector<RecipeOpScalar> &out, const AddExtrudedProfileOp &op)
{
    // The doubles (height, base_z, x, y) come from opFields; the extras are the
    // non-numeric scalars. No `vertices` — the extrude has no facet count
    // (unlike the lathe, which revolves N segments).
    out.push_back(textField("name", op.name));
    out.push_back(textField("profile", op.profile, /*editable=*/false)); // a drafted object id, picked elsewhere
    out.push_back(materialField(op.material));
}
void appendExtras(std::vector<RecipeOpScalar> &out, const AddSweepProfileOp &op)
{
    // BL-08: base_z/x/y come from opFields; profile and path are read-only
    // drafted-object references (the object picker owns them).
    out.push_back(textField("name", op.name));
    out.push_back(textField("profile", op.profile, /*editable=*/false));
    out.push_back(textField("path", op.path, /*editable=*/false));
    out.push_back(materialField(op.material));
}
void appendExtras(std::vector<RecipeOpScalar> &out, const CutFlutesOp &op)
{
    out.push_back(textField("target", op.target));
    out.push_back(intField("count", op.count));
}
void appendExtras(std::vector<RecipeOpScalar> &out, const AddLabelOp &op)
{
    out.push_back(textField("name", op.name));
    out.push_back(textField("text", op.text));
}
void appendExtras(std::vector<RecipeOpScalar> &out, const ScriptOp &op)
{
    out.push_back(textField("name", op.name));
    // The craftsman id is the dispatch target; it is chosen in the palette
    // (it decides the whole param schema), so it is a read-only reference here.
    out.push_back(textField("script", op.scriptId, /*editable=*/false));
    // Every param as an editable string. The values ARE strings in the bag, so
    // a text field edits any of them; the manifest-typed inspector overlay
    // (number/integer/material widgets) lands in the palette/inspector slice.
    for (const ScriptParam &param : op.params) {
        out.push_back(textField(param.key, param.value));
    }
}

// --- setters: one overload per op type, by key, from a typed value ---

bool asInt(const RecipeScalarValue &value, int &out)
{
    if (const int *i = std::get_if<int>(&value)) { out = *i; return true; }
    return false;
}
bool asBool(const RecipeScalarValue &value, bool &out)
{
    if (const bool *b = std::get_if<bool>(&value)) { out = *b; return true; }
    return false;
}
bool asText(const RecipeScalarValue &value, std::string &out)
{
    if (const std::string *s = std::get_if<std::string>(&value)) { out = *s; return true; }
    return false;
}

bool setExtra(AddBoxOp &op, const std::string &key, const RecipeScalarValue &value)
{
    if (key == "name") return asText(value, op.name);
    if (key == "material") return asText(value, op.material);
    if (key == "z_mode") { std::string s; if (asText(value, s)) { op.zMode = parseZMode(s); return true; } }
    return false;
}
bool setExtra(AddCylinderOp &op, const std::string &key, const RecipeScalarValue &value)
{
    if (key == "name") return asText(value, op.name);
    if (key == "material") return asText(value, op.material);
    if (key == "vertices") return asInt(value, op.vertices);
    if (key == "entasis") return asBool(value, op.entasis);
    if (key == "axis") { std::string s; if (asText(value, s)) { op.axis = parseAxis(s); return true; } }
    if (key == "z_mode") { std::string s; if (asText(value, s)) { op.zMode = parseZMode(s); return true; } }
    return false;
}
bool setExtra(AddSphereOp &op, const std::string &key, const RecipeScalarValue &value)
{
    if (key == "name") return asText(value, op.name);
    if (key == "material") return asText(value, op.material);
    if (key == "vertices") return asInt(value, op.vertices);
    return false;
}
bool setExtra(AddRingOp &op, const std::string &key, const RecipeScalarValue &value)
{
    if (key == "name") return asText(value, op.name);
    if (key == "material") return asText(value, op.material);
    if (key == "vertices") return asInt(value, op.vertices);
    return false;
}
bool setExtra(AddMouldingOp &op, const std::string &key, const RecipeScalarValue &value)
{
    if (key == "name") return asText(value, op.name);
    if (key == "material") return asText(value, op.material);
    if (key == "vertices") return asInt(value, op.vertices);
    return false;
}
bool setExtra(AddPrismOp &op, const std::string &key, const RecipeScalarValue &value)
{
    if (key == "name") return asText(value, op.name);
    if (key == "material") return asText(value, op.material);
    return false;
}
bool setExtra(AddProfileMouldingOp &op, const std::string &key, const RecipeScalarValue &value)
{
    if (key == "name") return asText(value, op.name);
    if (key == "material") return asText(value, op.material);
    if (key == "vertices") return asInt(value, op.vertices);
    return false;
}
bool setExtra(AddRevolvedProfileOp &op, const std::string &key, const RecipeScalarValue &value)
{
    if (key == "name") return asText(value, op.name);
    if (key == "material") return asText(value, op.material);
    if (key == "vertices") return asInt(value, op.vertices);
    // `profile` is a read-only reference here; the binding/object picker owns it.
    return false;
}
bool setExtra(AddExtrudedProfileOp &op, const std::string &key, const RecipeScalarValue &value)
{
    if (key == "name") return asText(value, op.name);
    if (key == "material") return asText(value, op.material);
    // `profile` is a read-only reference here; the binding/object picker owns it.
    return false;
}
bool setExtra(AddSweepProfileOp &op, const std::string &key, const RecipeScalarValue &value)
{
    if (key == "name") return asText(value, op.name);
    if (key == "material") return asText(value, op.material);
    // `profile` and `path` are read-only references; the object picker owns them.
    return false;
}
bool setExtra(CutFlutesOp &op, const std::string &key, const RecipeScalarValue &value)
{
    if (key == "target") return asText(value, op.target);
    if (key == "count") return asInt(value, op.count);
    return false;
}
bool setExtra(AddLabelOp &op, const std::string &key, const RecipeScalarValue &value)
{
    if (key == "name") return asText(value, op.name);
    if (key == "text") return asText(value, op.text);
    return false;
}
bool setExtra(ScriptOp &op, const std::string &key, const RecipeScalarValue &value)
{
    if (key == "name") return asText(value, op.name);
    if (key == "script") return false; // read-only reference; the palette owns craftsman choice
    for (ScriptParam &param : op.params) {
        if (param.key == key) return asText(value, param.value);
    }
    return false;
}

} // namespace

std::vector<RecipeOpScalar> opEditableScalars(const RecipeOp &op)
{
    std::vector<RecipeOpScalar> out;
    // The doubles, from the binding registry (its single source for them).
    for (const RecipeOpField &field : opFields(op)) {
        RecipeOpScalar scalar;
        scalar.key = field.key;
        scalar.label = labelFor(field.key);
        scalar.kind = RecipeFieldKind::Number;
        scalar.number = field.value;
        out.push_back(std::move(scalar));
    }
    std::visit([&out](const auto &concrete) { appendExtras(out, concrete); }, op);
    return out;
}

bool setOpScalar(RecipeOp &op, const std::string &key, const RecipeScalarValue &value)
{
    // A double goes through the binding registry first (the one writer for the
    // bindable numbers); anything else dispatches to the per-type extras.
    if (const double *number = std::get_if<double>(&value)) {
        if (setOpFieldValue(op, key, *number)) {
            return true;
        }
    }
    return std::visit([&](auto &concrete) { return setExtra(concrete, key, value); }, op);
}

} // namespace edi::recipe
