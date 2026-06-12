#include "recipe/RecipeOpsStore.h"

#include "formats/TomlReader.h"
#include "formats/TomlWriter.h"
#include "recipe/RecipeOpsBind.h"
#include "recipe/RecipeTextNumbers.h"

#include <string>
#include <unordered_set>
#include <vector>

namespace edi::recipe {

namespace {

std::string opPrefix(std::size_t index)
{
    return "op." + std::to_string(index);
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
        for (std::size_t i = 0; i < op.profile.size(); ++i) {
            const std::string pointPrefix = prefix + ".profile." + std::to_string(i);
            config[pointPrefix + ".term"] = op.profile[i].term;
            config[pointPrefix + ".z"] = numberKeyText(op.profile[i].z);
            config[pointPrefix + ".radius"] = numberKeyText(op.profile[i].radius);
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

    void operator()(const AddLabelOp &op) const
    {
        put("type", std::string("AddLabel"));
        put("name", op.name);
        put("text", op.text);
        put("x", op.x);
        put("y", op.y);
        put("z", op.z);
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
                || !readMouldingPoints(reader, prefix, op.profile)) {
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
                || !reader.optionalTextDefault(prefix + ".material", op.material)) {
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

} // namespace edi::recipe
