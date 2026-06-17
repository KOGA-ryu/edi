#include "recipe/RecipeOpsStore.h"

#include "formats/TomlReader.h"
#include "formats/TomlWriter.h"
#include "recipe/RecipeOpsBind.h"
#include "recipe/RecipeTextNumbers.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <unordered_set>
#include <vector>

namespace edi::recipe {

namespace {

std::string opPrefix(std::size_t index)
{
    return "op." + std::to_string(index);
}

const char *booleanKindText(BooleanKind kind)
{
    switch (kind) {
    case BooleanKind::Subtract:
        return "subtract";
    case BooleanKind::Intersect:
        return "intersect";
    case BooleanKind::Union:
        break;
    }
    return "union";
}

// ---- Writing -----------------------------------------------------------

struct OpWriter {
    edi::formats::StaticConfig &config;
    const std::string &prefix;
    // Field keys of THIS op that carry a measurement binding: the binding
    // keys are emitted centrally, and the bare literal must not appear
    // beside them (a file showing a number the build ignores is the
    // ambiguity the reader refuses).
    const std::unordered_set<std::string> *boundKeys = nullptr;

    void put(const char *key, const std::string &value) const { config[prefix + "." + key] = value; }
    void put(const char *key, double value) const
    {
        if (boundKeys != nullptr && boundKeys->count(key) > 0) {
            return; // bound: .object/.field stand in for the literal
        }
        put(key, numberKeyText(value));
    }
    void put(const char *key, int value) const { put(key, std::to_string(value)); }
    void put(const char *key, bool value) const { put(key, value ? std::string("true") : std::string("false")); }
    // String literals would otherwise bind to the BOOL overload (pointer->
    // bool is a standard conversion, which beats the user-defined one to
    // std::string) and serialize as "true". Exact match intercepts them.
    void put(const char *key, const char *value) const { put(key, std::string(value)); }

    void operator()(const AddBoxOp &op) const
    {
        put("type", std::string("AddBox"));
        put("name", op.name);
        put("width", op.width);
        put("depth", op.depth);
        put("height", op.height);
        put("z", op.z);
        put("x", op.x);
        put("y", op.y);
        put("material", op.material);
        put("z_mode", std::string(op.zMode == ZMode::Center ? "center" : "base"));
    }

    void operator()(const AddCylinderOp &op) const
    {
        put("type", std::string("AddCylinder"));
        put("name", op.name);
        put("radius", op.radius);
        put("height", op.height);
        put("z", op.z);
        put("x", op.x);
        put("y", op.y);
        put("vertices", op.vertices);
        put("material", op.material);
        if (op.taperTopRadius.has_value()) {
            put("taper_top_radius", *op.taperTopRadius);
        }
        put("entasis", op.entasis);
        put("entasis_ratio", op.entasisRatio);
        put("axis", std::string(op.axis == Axis::X ? "x" : op.axis == Axis::Y ? "y" : "z"));
        put("z_mode", std::string(op.zMode == ZMode::Center ? "center" : "base"));
    }

    void operator()(const AddSphereOp &op) const
    {
        put("type", std::string("AddSphere"));
        put("name", op.name);
        put("radius", op.radius);
        put("z", op.z);
        put("x", op.x);
        put("y", op.y);
        put("vertices", op.vertices);
        put("material", op.material);
    }

    void operator()(const AddRingOp &op) const
    {
        put("type", std::string("AddRing"));
        put("name", op.name);
        put("radius", op.radius);
        put("tube_height", op.tubeHeight);
        put("z", op.z);
        put("overhang", op.overhang);
        put("x", op.x);
        put("y", op.y);
        put("vertices", op.vertices);
        put("material", op.material);
    }

    void operator()(const AddMouldingOp &op) const
    {
        put("type", std::string("AddMoulding"));
        put("name", op.name);
        put("base_z", op.baseZ);
        put("x", op.x);
        put("y", op.y);
        put("vertices", op.vertices);
        put("material", op.material);
        put("sweep_degrees", op.sweepDegrees);
        put("screw_rise", op.screwRise);
        put("screw_turns", op.screwTurns);
        for (std::size_t i = 0; i < op.profile.size(); ++i) {
            const std::string pointPrefix = prefix + ".profile." + std::to_string(i);
            config[pointPrefix + ".term"] = op.profile[i].term;
            config[pointPrefix + ".z"] = numberKeyText(op.profile[i].z);
            config[pointPrefix + ".radius"] = numberKeyText(op.profile[i].radius);
        }
    }

    void operator()(const AddPrismOp &op) const
    {
        put("type", std::string("AddPrism"));
        put("name", op.name);
        put("height", op.height);
        put("base_z", op.baseZ);
        put("x", op.x);
        put("y", op.y);
        put("material", op.material);
        put("taper_end", op.taperEnd);
        put("inset", op.inset);
        put("normal_offset", op.normalOffset);
        // The footprint is a contiguous indexed run footprint.i.{x,y}, the same
        // shape (and reader rule: read until a gap) AddMoulding uses for profile.i.
        for (std::size_t i = 0; i < op.footprint.size(); ++i) {
            const std::string pointPrefix = prefix + ".footprint." + std::to_string(i);
            config[pointPrefix + ".x"] = numberKeyText(op.footprint[i].x);
            config[pointPrefix + ".y"] = numberKeyText(op.footprint[i].y);
        }
        // BL-08: the optional sweep path, the SAME run shape. Emitted ONLY when
        // present, so an empty-path prism writes no path.* keys and its TOML/OBJ
        // stay byte-identical to the BL-04 straight-extrude golden.
        for (std::size_t i = 0; i < op.path.size(); ++i) {
            const std::string pointPrefix = prefix + ".path." + std::to_string(i);
            config[pointPrefix + ".x"] = numberKeyText(op.path[i].x);
            config[pointPrefix + ".y"] = numberKeyText(op.path[i].y);
        }
    }

    void operator()(const AddProfileMouldingOp &op) const
    {
        put("type", std::string("AddProfileMoulding"));
        put("name", op.name);
        put("base_z", op.baseZ);
        put("x", op.x);
        put("y", op.y);
        put("vertices", op.vertices);
        put("material", op.material);
        for (std::size_t i = 0; i < op.sequence.size(); ++i) {
            const MouldingSegment &segment = op.sequence[i];
            const std::string segmentPrefix = prefix + ".seq." + std::to_string(i);
            config[segmentPrefix + ".term"] = segment.term;
            config[segmentPrefix + ".height"] = numberKeyText(segment.height);
            if (segment.startRadius.has_value()) {
                config[segmentPrefix + ".start_radius"] = numberKeyText(*segment.startRadius);
            }
            if (segment.endRadius.has_value()) {
                config[segmentPrefix + ".end_radius"] = numberKeyText(*segment.endRadius);
            }
            if (segment.radiusDelta.has_value()) {
                config[segmentPrefix + ".radius_delta"] = numberKeyText(*segment.radiusDelta);
            }
            if (segment.steps.has_value()) {
                config[segmentPrefix + ".steps"] = std::to_string(*segment.steps);
            }
        }
    }

    void operator()(const AddRevolvedProfileOp &op) const
    {
        put("type", std::string("AddRevolvedProfile"));
        put("name", op.name);
        put("profile", op.profile);
        put("base_z", op.baseZ);
        put("x", op.x);
        put("y", op.y);
        put("vertices", op.vertices);
        put("material", op.material);
        put("sweep_degrees", op.sweepDegrees);
        put("screw_rise", op.screwRise);
        put("screw_turns", op.screwTurns);
    }

    void operator()(const AddExtrudedProfileOp &op) const
    {
        put("type", std::string("AddExtrudedProfile"));
        put("name", op.name);
        put("profile", op.profile);
        put("height", op.height);
        put("base_z", op.baseZ);
        put("x", op.x);
        put("y", op.y);
        put("material", op.material);
    }

    void operator()(const AddSweepProfileOp &op) const
    {
        put("type", std::string("AddSweepProfile"));
        put("name", op.name);
        put("profile", op.profile);
        put("path", op.path);
        put("base_z", op.baseZ);
        put("x", op.x);
        put("y", op.y);
        put("material", op.material);
        put("taper_end", op.taperEnd);
    }

    void operator()(const CutFlutesOp &op) const
    {
        put("type", std::string("CutFlutes"));
        put("target", op.target);
        put("count", op.count);
        put("depth", op.depth);
        // Explicit cutter geometry XOR the width_ratio derivation (R1-B04b):
        // emit exactly ONE cutter source so the file never shows a number the
        // build ignores. The pair is present-together by struct invariant (the
        // reader enforces it on load); width_ratio keeps its slot — and its
        // bindability through put() — when the pair is absent.
        if (op.cutterRadius.has_value() && op.atRadius.has_value()) {
            put("cutter_radius", *op.cutterRadius);
            put("at_radius", *op.atRadius);
        } else {
            put("width_ratio", op.widthRatio);
        }
        if (op.startZ.has_value()) {
            put("start_z", *op.startZ);
        }
        if (op.endZ.has_value()) {
            put("end_z", *op.endZ);
        }
    }

    void operator()(const AddBooleanOp &op) const
    {
        put("type", std::string("AddBoolean"));
        put("name", op.name);
        put("a", op.a);
        put("b", op.b);
        put("kind", std::string(booleanKindText(op.kind)));
    }

    void operator()(const AddLabelOp &op) const
    {
        put("type", std::string("AddLabel"));
        put("name", op.name);
        put("text", op.text);
        put("x", op.x);
        put("y", op.y);
        put("z", op.z);
    }

    void operator()(const ScriptOp &op) const
    {
        put("type", std::string("Script"));
        put("script", op.scriptId);
        put("name", op.name);
        // x/y/z go through the bound-aware put(): a bound coordinate is omitted
        // and its .object/.field stand in, exactly like every other op.
        put("x", op.x);
        put("y", op.y);
        put("z", op.z);
        // The free param bag, verbatim strings. recipeOpsToToml already refused
        // any param key that collides with a built-in above or isn't a bare key
        // (the pre-serialization pass), so each lands in its own slot here.
        for (const ScriptParam &param : op.params) {
            put(param.key.c_str(), param.value);
        }
    }
};

// ---- Reading -----------------------------------------------------------

// The reader tracks every key it consumes; the final sweep rejects anything
// left over. Tracking-by-consumption audits arbitrary nested shapes
// (seq.K.*, profile.K.*) without reconstructing a legal-key list.
struct OpReader {
    const edi::formats::StaticConfig &config;
    std::unordered_set<std::string> &consumed;
    std::string error;
    std::vector<RecipeFieldBinding> *bindings = nullptr;
    std::size_t opIndex = 0;

    const std::string *take(const std::string &key)
    {
        const auto found = config.find(key);
        if (found == config.end()) {
            return nullptr;
        }
        consumed.insert(key);
        return &found->second;
    }

    // True when a key exists in the file, WITHOUT consuming it: the cutter
    // XOR (R1-B04b) must spot a width_ratio beside an explicit pair before
    // the reader would otherwise read it.
    bool present(const std::string &key) const { return config.find(key) != config.end(); }

    bool requireText(const std::string &key, std::string &out)
    {
        const std::string *value = take(key);
        if (value == nullptr) {
            error = "missing required key: " + key;
            return false;
        }
        out = *value;
        return true;
    }

    bool requireNumber(const std::string &key, double &out)
    {
        std::string text;
        if (!requireText(key, text)) {
            return false;
        }
        if (!parseNumberText(text, out)) {
            error = key + ": not a number: " + text;
            return false;
        }
        return true;
    }

    bool optionalNumber(const std::string &key, double &out, bool &present)
    {
        const std::string *value = take(key);
        present = value != nullptr;
        if (!present) {
            return true;
        }
        if (!parseNumberText(*value, out)) {
            error = key + ": not a number: " + *value;
            return false;
        }
        return true;
    }

    bool optionalInt(const std::string &key, int &out, bool &present)
    {
        const std::string *value = take(key);
        present = value != nullptr;
        if (!present) {
            return true;
        }
        if (!parseIntText(*value, out)) {
            error = key + ": not an integer: " + *value;
            return false;
        }
        return true;
    }

    bool requireInt(const std::string &key, int &out)
    {
        std::string text;
        if (!requireText(key, text)) {
            return false;
        }
        if (!parseIntText(text, out)) {
            error = key + ": not an integer: " + text;
            return false;
        }
        return true;
    }

    bool optionalIntDefault(const std::string &key, int &out)
    {
        bool present = false;
        int value = 0;
        if (!optionalInt(key, value, present)) {
            return false;
        }
        if (present) {
            out = value;
        }
        return true;
    }

    bool optionalNumberDefault(const std::string &key, double &out)
    {
        bool present = false;
        double value = 0.0;
        if (!optionalNumber(key, value, present)) {
            return false;
        }
        if (present) {
            out = value;
        }
        return true;
    }

    bool optionalTextDefault(const std::string &key, std::string &out)
    {
        if (const std::string *value = take(key)) {
            out = *value;
        }
        return true;
    }

    // Binding-aware read of a bindable double field: pipeline A's
    // .object/.field shape REPLACES the literal (docs/
    // recipe_binding_contract.md). Divergence from A, documented: the op
    // dialect's literal is the BARE key (A's carried a .value suffix), so
    // the has-both wording names no ".value". Check order mirrors A's
    // loader: half a binding refuses before the literal clash does.
    bool bindableNumber(const std::string &prefix, const std::string &fieldKey,
                        double &out, bool required)
    {
        const std::string key = prefix + "." + fieldKey;
        const bool literalPresent = config.find(key) != config.end();
        const std::string *object = take(key + ".object");
        const std::string *field = take(key + ".field");
        if (object != nullptr || field != nullptr) {
            if (object == nullptr || field == nullptr) {
                error = key + ": a measurement binding needs both .object and .field";
                return false;
            }
            if (literalPresent) {
                error = key + ": has both a literal and a measurement binding (.object/.field)";
                return false;
            }
            if (object->empty() || field->empty()) {
                error = key + ": a binding names an object and a field";
                return false;
            }
            bindings->push_back({opIndex, fieldKey, *object, *field});
            return true; // the struct default stands until resolve (B03)
        }
        if (required) {
            return requireNumber(key, out);
        }
        return optionalNumberDefault(key, out);
    }

    bool readZMode(const std::string &key, ZMode &out)
    {
        const std::string *value = take(key);
        if (value == nullptr) {
            return true; // keep default
        }
        if (*value == "center") {
            out = ZMode::Center;
        } else if (*value == "base") {
            out = ZMode::Base;
        } else {
            error = key + ": z_mode must be center or base, got '" + *value + "'";
            return false;
        }
        return true;
    }

    bool readAxis(const std::string &key, Axis &out)
    {
        const std::string *value = take(key);
        if (value == nullptr) {
            return true;
        }
        if (*value == "x") {
            out = Axis::X;
        } else if (*value == "y") {
            out = Axis::Y;
        } else if (*value == "z") {
            out = Axis::Z;
        } else {
            error = key + ": axis must be x, y, or z, got '" + *value + "'";
            return false;
        }
        return true;
    }

    bool readBooleanKind(const std::string &key, BooleanKind &out)
    {
        const std::string *value = take(key);
        if (value == nullptr) {
            return true; // keep default (Union)
        }
        if (*value == "union") {
            out = BooleanKind::Union;
        } else if (*value == "subtract") {
            out = BooleanKind::Subtract;
        } else if (*value == "intersect") {
            out = BooleanKind::Intersect;
        } else {
            error = key + ": kind must be union, subtract, or intersect, got '" + *value + "'";
            return false;
        }
        return true;
    }

    bool readBool(const std::string &key, bool &out)
    {
        const std::string *value = take(key);
        if (value == nullptr) {
            return true;
        }
        if (*value == "true") {
            out = true;
        } else if (*value == "false") {
            out = false;
        } else {
            error = key + ": expected true or false, got '" + *value + "'";
            return false;
        }
        return true;
    }
};

bool readMouldingSegments(OpReader &reader, const std::string &prefix,
                          std::vector<MouldingSegment> &out)
{
    for (std::size_t i = 0;; ++i) {
        const std::string segmentPrefix = prefix + ".seq." + std::to_string(i);
        if (reader.config.find(segmentPrefix + ".term") == reader.config.end()) {
            return true;
        }
        MouldingSegment segment;
        if (!reader.requireText(segmentPrefix + ".term", segment.term)
            || !reader.requireNumber(segmentPrefix + ".height", segment.height)) {
            return false;
        }
        bool present = false;
        double value = 0.0;
        if (!reader.optionalNumber(segmentPrefix + ".start_radius", value, present)) {
            return false;
        }
        if (present) {
            segment.startRadius = value;
        }
        if (!reader.optionalNumber(segmentPrefix + ".end_radius", value, present)) {
            return false;
        }
        const bool hasEnd = present;
        if (present) {
            segment.endRadius = value;
        }
        if (!reader.optionalNumber(segmentPrefix + ".radius_delta", value, present)) {
            return false;
        }
        if (present && hasEnd) {
            // House ambiguity rule (same as literal-vs-binding): a file
            // showing a delta the compiler ignores is a lie in waiting.
            reader.error = segmentPrefix + ": has both end_radius and radius_delta";
            return false;
        }
        if (present) {
            segment.radiusDelta = value;
        }
        int steps = 0;
        if (!reader.optionalInt(segmentPrefix + ".steps", steps, present)) {
            return false;
        }
        if (present) {
            segment.steps = steps;
        }
        out.push_back(std::move(segment));
    }
}

bool readMouldingPoints(OpReader &reader, const std::string &prefix,
                        std::vector<MouldingPoint> &out)
{
    for (std::size_t i = 0;; ++i) {
        const std::string pointPrefix = prefix + ".profile." + std::to_string(i);
        if (reader.config.find(pointPrefix + ".term") == reader.config.end()) {
            return true;
        }
        MouldingPoint point;
        if (!reader.requireText(pointPrefix + ".term", point.term)
            || !reader.requireNumber(pointPrefix + ".z", point.z)
            || !reader.requireNumber(pointPrefix + ".radius", point.radius)) {
            return false;
        }
        out.push_back(std::move(point));
    }
}

// Reads a contiguous <run>.i.{x,y} point run until a gap (a missing .x ends the
// run), the same shape readMouldingPoints uses for the profile run. Shared by
// AddPrism's `footprint` and its optional BL-08 `path` (an absent run reads as
// an empty vector — exactly the empty-path = straight-extrude default).
bool readPrismPointRun(OpReader &reader, const std::string &prefix, const std::string &run,
                       std::vector<PrismPoint> &out)
{
    for (std::size_t i = 0;; ++i) {
        const std::string pointPrefix = prefix + "." + run + "." + std::to_string(i);
        if (reader.config.find(pointPrefix + ".x") == reader.config.end()) {
            return true;
        }
        PrismPoint point;
        if (!reader.requireNumber(pointPrefix + ".x", point.x)
            || !reader.requireNumber(pointPrefix + ".y", point.y)) {
            return false;
        }
        out.push_back(std::move(point));
    }
}

} // namespace

OpStreamTextResult recipeOpsToToml(const RecipeOpStream &stream)
{
    OpStreamTextResult result;
    edi::formats::StaticConfig config;
    config["recipe.id"] = stream.id;
    config["recipe.name"] = stream.name;

    // Bindings are validated at WRITE as well as read: a stream carrying a
    // bogus binding (no such op, unbindable field, empty names, the same
    // field bound twice) must refuse to serialize, not produce a file the
    // reader will bounce later.
    std::vector<std::unordered_set<std::string>> boundByOp(stream.ops.size());
    for (const RecipeFieldBinding &binding : stream.bindings) {
        if (binding.opIndex >= stream.ops.size()) {
            result.message = "binding for op." + std::to_string(binding.opIndex) + "."
                + binding.fieldKey + ": no such op";
            return result;
        }
        const std::string key = opPrefix(binding.opIndex) + "." + binding.fieldKey;
        if (!opFieldBindable(stream.ops[binding.opIndex], binding.fieldKey)) {
            result.message = key + ": not a bindable field";
            return result;
        }
        if (binding.objectId.empty() || binding.field.empty()) {
            result.message = key + ": a binding names an object and a field";
            return result;
        }
        if (!boundByOp[binding.opIndex].insert(binding.fieldKey).second) {
            result.message = key + ": bound more than once";
            return result;
        }
        config[key + ".object"] = binding.objectId;
        config[key + ".field"] = binding.field;
    }

    // Struct invariants the reader enforces on load get enforced at WRITE
    // too (the B02 rule: never serialize a file the reader bounces). Today
    // that is one invariant: a CutFlutes explicit cutter is both-or-neither
    // — a half pair in memory would otherwise silently emit width_ratio and
    // DROP the lone cutter value (planner ruling on the B04b builder's
    // flagged decision #4).
    for (std::size_t i = 0; i < stream.ops.size(); ++i) {
        if (const auto *flutes = std::get_if<CutFlutesOp>(&stream.ops[i])) {
            if (flutes->cutterRadius.has_value() != flutes->atRadius.has_value()) {
                result.message =
                    opPrefix(i) + ": a cutter needs both .cutter_radius and .at_radius";
                return result;
            }
        }
        // Same B02 rule for a craftsman's param bag: a key that collides with a
        // built-in (it would overwrite that field in the flat map) or isn't a
        // bare key (it would emit an unreadable line, or nest under tomllib)
        // must refuse here, not silently corrupt the file.
        if (const auto *script = std::get_if<ScriptOp>(&stream.ops[i])) {
            for (const ScriptParam &param : script->params) {
                const std::string problem = recipeScriptParamKeyProblem(param.key);
                if (!problem.empty()) {
                    result.message = opPrefix(i) + ": " + problem;
                    return result;
                }
            }
        }
    }

    for (std::size_t i = 0; i < stream.ops.size(); ++i) {
        const std::string prefix = opPrefix(i);
        std::visit(OpWriter{config, prefix, &boundByOp[i]}, stream.ops[i]);
    }

    const auto written = edi::formats::writeTomlStaticConfig(config, "recipe_ops");
    if (!written.ok) {
        result.message = written.message;
        return result;
    }
    result.ok = true;
    result.text = *written.value;
    return result;
}

OpStreamParseResult recipeOpsFromToml(const std::string &text, const std::string &source)
{
    // TRIPWIRE — the store READER below is a string-keyed if/else ladder over
    // the `type` key, NOT a std::visit over RecipeOp. Every WRITER-side
    // interpreter is compiler-exhaustive (add a RecipeOp arm or it won't
    // compile — docs/architecture/edi-blender-lab.md §2), but a reader keyed by
    // a runtime string can't be: growing RecipeOp does NOT break this
    // function's compile, so a forgotten reader branch would only fail at
    // runtime (the refusing "unknown op type" default at the bottom). This
    // static_assert is the deliberate guard that DOES break: bump the variant
    // and this fires, reminding the next author to add a reader branch HERE and
    // a matching `edi_craft.parse_ops` arm in Python, key-for-key — the
    // cross-language TOML contract is a two-sided obligation (§3).
    static_assert(std::variant_size_v<RecipeOp> == 14,
                  "RecipeOp grew: add a reader branch in recipeOpsFromToml (this "
                  "ladder is string-keyed, not a std::visit, so the compiler "
                  "won't force it) AND a matching parse_ops arm in edi_craft.py.");

    OpStreamParseResult result;
    const auto parsed = edi::formats::readTomlStaticConfig(text, source);
    if (!parsed.ok) {
        result.message = parsed.message;
        return result;
    }
    const edi::formats::StaticConfig &config = *parsed.value;

    std::unordered_set<std::string> consumed;
    OpReader reader{config, consumed, {}};
    reader.bindings = &result.stream.bindings;

    if (const std::string *id = reader.take("recipe.id")) {
        result.stream.id = *id;
    }
    if (const std::string *name = reader.take("recipe.name")) {
        result.stream.name = *name;
    }

    for (std::size_t i = 0;; ++i) {
        const std::string prefix = opPrefix(i);
        reader.opIndex = i;
        const std::string *type = reader.take(prefix + ".type");
        if (type == nullptr) {
            break;
        }

        if (*type == "AddBox") {
            AddBoxOp op;
            if (!reader.requireText(prefix + ".name", op.name)
                || !reader.bindableNumber(prefix, "width", op.width, true)
                || !reader.bindableNumber(prefix, "depth", op.depth, true)
                || !reader.bindableNumber(prefix, "height", op.height, true)
                || !reader.bindableNumber(prefix, "z", op.z, true)
                || !reader.bindableNumber(prefix, "x", op.x, false)
                || !reader.bindableNumber(prefix, "y", op.y, false)
                || !reader.optionalTextDefault(prefix + ".material", op.material)
                || !reader.readZMode(prefix + ".z_mode", op.zMode)) {
                result.message = reader.error;
                return result;
            }
            result.stream.ops.push_back(std::move(op));
        } else if (*type == "AddCylinder") {
            AddCylinderOp op;
            bool present = false;
            double taper = 0.0;
            if (!reader.requireText(prefix + ".name", op.name)
                || !reader.bindableNumber(prefix, "radius", op.radius, true)
                || !reader.bindableNumber(prefix, "height", op.height, true)
                || !reader.bindableNumber(prefix, "z", op.z, true)
                || !reader.bindableNumber(prefix, "x", op.x, false)
                || !reader.bindableNumber(prefix, "y", op.y, false)
                || !reader.optionalIntDefault(prefix + ".vertices", op.vertices)
                || !reader.optionalTextDefault(prefix + ".material", op.material)
                || !reader.optionalNumber(prefix + ".taper_top_radius", taper, present)
                || !reader.readBool(prefix + ".entasis", op.entasis)
                || !reader.bindableNumber(prefix, "entasis_ratio", op.entasisRatio, false)
                || !reader.readAxis(prefix + ".axis", op.axis)
                || !reader.readZMode(prefix + ".z_mode", op.zMode)) {
                result.message = reader.error;
                return result;
            }
            if (present) {
                op.taperTopRadius = taper;
            }
            result.stream.ops.push_back(std::move(op));
        } else if (*type == "AddSphere") {
            AddSphereOp op;
            if (!reader.requireText(prefix + ".name", op.name)
                || !reader.bindableNumber(prefix, "radius", op.radius, true)
                || !reader.bindableNumber(prefix, "z", op.z, true)
                || !reader.bindableNumber(prefix, "x", op.x, false)
                || !reader.bindableNumber(prefix, "y", op.y, false)
                || !reader.optionalIntDefault(prefix + ".vertices", op.vertices)
                || !reader.optionalTextDefault(prefix + ".material", op.material)) {
                result.message = reader.error;
                return result;
            }
            result.stream.ops.push_back(std::move(op));
        } else if (*type == "AddRing") {
            AddRingOp op;
            if (!reader.requireText(prefix + ".name", op.name)
                || !reader.bindableNumber(prefix, "radius", op.radius, true)
                || !reader.bindableNumber(prefix, "tube_height", op.tubeHeight, true)
                || !reader.bindableNumber(prefix, "z", op.z, true)
                || !reader.bindableNumber(prefix, "overhang", op.overhang, false)
                || !reader.bindableNumber(prefix, "x", op.x, false)
                || !reader.bindableNumber(prefix, "y", op.y, false)
                || !reader.optionalIntDefault(prefix + ".vertices", op.vertices)
                || !reader.optionalTextDefault(prefix + ".material", op.material)) {
                result.message = reader.error;
                return result;
            }
            result.stream.ops.push_back(std::move(op));
        } else if (*type == "AddMoulding") {
            AddMouldingOp op;
            if (!reader.requireText(prefix + ".name", op.name)
                || !reader.bindableNumber(prefix, "base_z", op.baseZ, true)
                || !reader.bindableNumber(prefix, "x", op.x, false)
                || !reader.bindableNumber(prefix, "y", op.y, false)
                || !reader.optionalIntDefault(prefix + ".vertices", op.vertices)
                || !reader.optionalTextDefault(prefix + ".material", op.material)
                || !reader.bindableNumber(prefix, "sweep_degrees", op.sweepDegrees, false)
                || !reader.bindableNumber(prefix, "screw_rise", op.screwRise, false)
                || !reader.bindableNumber(prefix, "screw_turns", op.screwTurns, false)
                || !readMouldingPoints(reader, prefix, op.profile)) {
                result.message = reader.error;
                return result;
            }
            result.stream.ops.push_back(std::move(op));
        } else if (*type == "AddPrism") {
            // The lowered prism carrier (BL-03). Read with bindableNumber so the
            // reader, the bind registry, and the inspector agree on which fields
            // are numeric (mirroring AddMoulding). In practice a prism is born
            // from lowering with no bindings — its source AddExtrudedProfile's
            // bindings were already applied pre-lowering — so the binding path
            // here is unexercised, but keeping the shapes in step avoids drift.
            // footprint.i.{x,y} reads as a contiguous run until a gap.
            AddPrismOp op;
            if (!reader.requireText(prefix + ".name", op.name)
                || !reader.bindableNumber(prefix, "height", op.height, true)
                || !reader.bindableNumber(prefix, "base_z", op.baseZ, true)
                || !reader.bindableNumber(prefix, "x", op.x, false)
                || !reader.bindableNumber(prefix, "y", op.y, false)
                || !reader.optionalTextDefault(prefix + ".material", op.material)
                || !reader.bindableNumber(prefix, "taper_end", op.taperEnd, false) // BL-09; default 1
                || !reader.bindableNumber(prefix, "inset", op.inset, false)          // BL-10; default 0
                || !reader.bindableNumber(prefix, "normal_offset", op.normalOffset, false)
                || !readPrismPointRun(reader, prefix, "footprint", op.footprint)
                || !readPrismPointRun(reader, prefix, "path", op.path)) { // BL-08; absent = empty
                result.message = reader.error;
                return result;
            }
            result.stream.ops.push_back(std::move(op));
        } else if (*type == "AddProfileMoulding") {
            AddProfileMouldingOp op;
            if (!reader.requireText(prefix + ".name", op.name)
                || !reader.bindableNumber(prefix, "base_z", op.baseZ, true)
                || !reader.bindableNumber(prefix, "x", op.x, false)
                || !reader.bindableNumber(prefix, "y", op.y, false)
                || !reader.optionalIntDefault(prefix + ".vertices", op.vertices)
                || !reader.optionalTextDefault(prefix + ".material", op.material)
                || !readMouldingSegments(reader, prefix, op.sequence)) {
                result.message = reader.error;
                return result;
            }
            result.stream.ops.push_back(std::move(op));
        } else if (*type == "AddRevolvedProfile") {
            AddRevolvedProfileOp op;
            if (!reader.requireText(prefix + ".name", op.name)
                || !reader.requireText(prefix + ".profile", op.profile)
                || !reader.bindableNumber(prefix, "base_z", op.baseZ, true)
                || !reader.bindableNumber(prefix, "x", op.x, false)
                || !reader.bindableNumber(prefix, "y", op.y, false)
                || !reader.optionalIntDefault(prefix + ".vertices", op.vertices)
                || !reader.optionalTextDefault(prefix + ".material", op.material)
                || !reader.bindableNumber(prefix, "sweep_degrees", op.sweepDegrees, false)
                || !reader.bindableNumber(prefix, "screw_rise", op.screwRise, false)
                || !reader.bindableNumber(prefix, "screw_turns", op.screwTurns, false)) {
                result.message = reader.error;
                return result;
            }
            result.stream.ops.push_back(std::move(op));
        } else if (*type == "AddExtrudedProfile") {
            // Key-for-key the OpWriter's AddExtrudedProfile arm — the
            // cross-language contract (§3) is what this branch upholds, and
            // a forgotten key here is a runtime "unknown" not a compile error
            // (the static_assert above is the only compile-time guard the
            // string-keyed reader gets). height/base_z/x/y are bindable like
            // the lathe's numeric fields; profile is the drafted-object id.
            AddExtrudedProfileOp op;
            if (!reader.requireText(prefix + ".name", op.name)
                || !reader.requireText(prefix + ".profile", op.profile)
                || !reader.bindableNumber(prefix, "height", op.height, true)
                || !reader.bindableNumber(prefix, "base_z", op.baseZ, true)
                || !reader.bindableNumber(prefix, "x", op.x, false)
                || !reader.bindableNumber(prefix, "y", op.y, false)
                || !reader.optionalTextDefault(prefix + ".material", op.material)) {
                result.message = reader.error;
                return result;
            }
            result.stream.ops.push_back(std::move(op));
        } else if (*type == "AddSweepProfile") {
            // BL-08: two drafted-object refs (profile + path) read key-for-key
            // with the OpWriter; base_z/x/y bindable like the extrude's.
            AddSweepProfileOp op;
            if (!reader.requireText(prefix + ".name", op.name)
                || !reader.requireText(prefix + ".profile", op.profile)
                || !reader.requireText(prefix + ".path", op.path)
                || !reader.bindableNumber(prefix, "base_z", op.baseZ, true)
                || !reader.bindableNumber(prefix, "x", op.x, false)
                || !reader.bindableNumber(prefix, "y", op.y, false)
                || !reader.optionalTextDefault(prefix + ".material", op.material)
                || !reader.bindableNumber(prefix, "taper_end", op.taperEnd, false)) { // BL-09
                result.message = reader.error;
                return result;
            }
            result.stream.ops.push_back(std::move(op));
        } else if (*type == "CutFlutes") {
            CutFlutesOp op;
            bool present = false;
            double value = 0.0;
            if (!reader.requireText(prefix + ".target", op.target)
                || !reader.requireInt(prefix + ".count", op.count)
                || !reader.bindableNumber(prefix, "depth", op.depth, true)) {
                result.message = reader.error;
                return result;
            }
            // Explicit cutter geometry (R1-B04b): optional, present-together,
            // XOR width_ratio. Literal-only (not bindable), like start_z/end_z.
            bool hasCutter = false;
            bool hasAt = false;
            double cutterRadius = 0.0;
            double atRadius = 0.0;
            if (!reader.optionalNumber(prefix + ".cutter_radius", cutterRadius, hasCutter)
                || !reader.optionalNumber(prefix + ".at_radius", atRadius, hasAt)) {
                result.message = reader.error;
                return result;
            }
            if (hasCutter != hasAt) {
                result.message = prefix + ": a cutter needs both .cutter_radius and .at_radius";
                return result;
            }
            if (hasCutter) {
                // A width_ratio beside an explicit cutter is the both-sources
                // lie — refuse before width_ratio would be read and consumed.
                if (reader.present(prefix + ".width_ratio")) {
                    result.message = prefix
                        + ": has both an explicit cutter (.cutter_radius/.at_radius) and a .width_ratio";
                    return result;
                }
                op.cutterRadius = cutterRadius;
                op.atRadius = atRadius;
                // width_ratio keeps its struct default (0.28), inert and unwritten.
            } else if (!reader.bindableNumber(prefix, "width_ratio", op.widthRatio, false)) {
                result.message = reader.error;
                return result;
            }
            if (!reader.optionalNumber(prefix + ".start_z", value, present)) {
                result.message = reader.error;
                return result;
            }
            if (present) {
                op.startZ = value;
            }
            if (!reader.optionalNumber(prefix + ".end_z", value, present)) {
                result.message = reader.error;
                return result;
            }
            if (present) {
                op.endZ = value;
            }
            result.stream.ops.push_back(std::move(op));
        } else if (*type == "AddBoolean") {
            // BL-11: the general composer. a/b name earlier ops (validated in
            // order, like CutFlutes.target); kind via readBooleanKind.
            AddBooleanOp op;
            if (!reader.requireText(prefix + ".name", op.name)
                || !reader.requireText(prefix + ".a", op.a)
                || !reader.requireText(prefix + ".b", op.b)
                || !reader.readBooleanKind(prefix + ".kind", op.kind)) {
                result.message = reader.error;
                return result;
            }
            result.stream.ops.push_back(std::move(op));
        } else if (*type == "AddLabel") {
            AddLabelOp op;
            if (!reader.requireText(prefix + ".name", op.name)
                || !reader.requireText(prefix + ".text", op.text)
                || !reader.bindableNumber(prefix, "x", op.x, true)
                || !reader.bindableNumber(prefix, "y", op.y, true)
                || !reader.bindableNumber(prefix, "z", op.z, true)) {
                result.message = reader.error;
                return result;
            }
            result.stream.ops.push_back(std::move(op));
        } else if (*type == "Script") {
            // A custom-craftsman step (matches edi_craft.parse_ops): a script
            // id, a placement, and a free param bag. The position is bindable;
            // everything else under this op's prefix is an untyped param.
            ScriptOp op;
            if (!reader.requireText(prefix + ".script", op.scriptId)) {
                result.message = reader.error;
                return result;
            }
            op.name = op.scriptId; // the python default: name falls back to the id
            if (!reader.optionalTextDefault(prefix + ".name", op.name)
                || !reader.bindableNumber(prefix, "x", op.x, false)
                || !reader.bindableNumber(prefix, "y", op.y, false)
                || !reader.bindableNumber(prefix, "z", op.z, false)) {
                result.message = reader.error;
                return result;
            }
            // Sweep every still-unconsumed key under this op's prefix into the
            // param bag and MARK it consumed, so the trailing unknown-key audit
            // still guards typos in the built-in fields above. There is no
            // schema to check the params against here — that is the craftsman's
            // MANIFEST, read at proof/build — so the bag accepts any key. The
            // trailing dot in `opDot` keeps op.1 from swallowing op.10's keys.
            const std::string opDot = prefix + ".";
            for (const auto &[key, value] : config) {
                if (key.rfind(opDot, 0) != 0 || consumed.find(key) != consumed.end()) {
                    continue;
                }
                // The strict reader holds the same param-key contract the writer
                // does: a dotted/illegal key would round-trip differently here
                // than under the python half's tomllib, so refuse it by name
                // rather than load a file the two languages read apart.
                const std::string paramKey = key.substr(opDot.size());
                const std::string problem = recipeScriptParamKeyProblem(paramKey);
                if (!problem.empty()) {
                    result.message = prefix + ": " + problem;
                    return result;
                }
                op.params.push_back({paramKey, value});
                consumed.insert(key);
            }
            result.stream.ops.push_back(std::move(op));
        } else {
            result.message = prefix + ".type: unknown op type: '" + *type + "'";
            return result;
        }
    }

    // The audit: every key in the file must have been consumed. Catches
    // misspelled fields, "ops.0" plural typos, gapped indices (op.5 with
    // only 3 ops parsed), and trailing junk — each named verbatim.
    for (const auto &[key, unused] : config) {
        (void)unused;
        if (consumed.find(key) == consumed.end()) {
            result.message = "unknown recipe key: " + key;
            return result;
        }
    }

    result.ok = true;
    return result;
}

namespace {

// The library file stem: the recipe's name (or id when name is empty),
// sanitized to bare TOML/path-safe chars — letters, digits, '_' and '-'; any
// other char becomes '_'. Deterministic, so the same recipe always lands on the
// same file; collapses to "recipe" if nothing usable remains.
std::string libraryStem(const RecipeOpStream &stream)
{
    const std::string &source = !stream.name.empty() ? stream.name : stream.id;
    std::string stem;
    for (const char ch : source) {
        const bool bare = (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z')
            || (ch >= '0' && ch <= '9') || ch == '_' || ch == '-';
        stem.push_back(bare ? ch : '_');
    }
    return stem.empty() ? std::string("recipe") : stem;
}

const char *const kLibrarySuffix = ".ops.toml";

} // namespace

OpStreamTextResult saveLibraryRecipe(const std::string &dirPath, const RecipeOpStream &stream)
{
    OpStreamTextResult result = recipeOpsToToml(stream);
    if (!result.ok) {
        return result; // a stream that won't serialize is named by recipeOpsToToml
    }
    std::error_code ec;
    std::filesystem::create_directories(dirPath, ec); // idempotent; ec ignored, the write reports
    const std::filesystem::path path =
        std::filesystem::path(dirPath) / (libraryStem(stream) + kLibrarySuffix);
    std::ofstream out(path, std::ios::binary);
    if (!out) {
        result.ok = false;
        result.message = "cannot write recipe file: " + path.string();
        return result;
    }
    out << result.text;
    return result;
}

OpStreamParseResult loadLibraryRecipe(const std::string &dirPath, const std::string &name)
{
    OpStreamParseResult result;
    const std::filesystem::path path =
        std::filesystem::path(dirPath) / (name + kLibrarySuffix);
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        result.message = "recipe not found: " + path.string();
        return result;
    }
    const std::string text((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    return recipeOpsFromToml(text, path.string());
}

std::vector<std::string> listLibraryRecipes(const std::string &dirPath)
{
    std::vector<std::string> names;
    std::error_code ec;
    std::filesystem::directory_iterator it(dirPath, ec);
    if (ec) {
        return names; // a missing directory is an empty library, not an error
    }
    const std::string suffix = kLibrarySuffix;
    for (const auto &entry : it) {
        if (!entry.is_regular_file()) {
            continue;
        }
        const std::string filename = entry.path().filename().string();
        if (filename.size() > suffix.size()
            && filename.compare(filename.size() - suffix.size(), suffix.size(), suffix) == 0) {
            names.push_back(filename.substr(0, filename.size() - suffix.size()));
        }
    }
    std::sort(names.begin(), names.end());
    return names;
}

} // namespace edi::recipe
