#include "recipe/RecipeOpsStore.h"

#include "formats/TomlReader.h"
#include "formats/TomlWriter.h"
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

    void put(const char *key, const std::string &value) const { config[prefix + "." + key] = value; }
    void put(const char *key, double value) const { put(key, numberKeyText(value)); }
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

    void operator()(const CutFlutesOp &op) const
    {
        put("type", std::string("CutFlutes"));
        put("target", op.target);
        put("count", op.count);
        put("depth", op.depth);
        put("width_ratio", op.widthRatio);
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

    const std::string *take(const std::string &key)
    {
        const auto found = config.find(key);
        if (found == config.end()) {
            return nullptr;
        }
        consumed.insert(key);
        return &found->second;
    }

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
    edi::formats::StaticConfig config;
    config["recipe.id"] = stream.id;
    config["recipe.name"] = stream.name;
    for (std::size_t i = 0; i < stream.ops.size(); ++i) {
        const std::string prefix = opPrefix(i);
        std::visit(OpWriter{config, prefix}, stream.ops[i]);
    }

    const auto written = edi::formats::writeTomlStaticConfig(config, "recipe_ops");
    OpStreamTextResult result;
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

    if (const std::string *id = reader.take("recipe.id")) {
        result.stream.id = *id;
    }
    if (const std::string *name = reader.take("recipe.name")) {
        result.stream.name = *name;
    }

    for (std::size_t i = 0;; ++i) {
        const std::string prefix = opPrefix(i);
        const std::string *type = reader.take(prefix + ".type");
        if (type == nullptr) {
            break;
        }

        if (*type == "AddBox") {
            AddBoxOp op;
            if (!reader.requireText(prefix + ".name", op.name)
                || !reader.requireNumber(prefix + ".width", op.width)
                || !reader.requireNumber(prefix + ".depth", op.depth)
                || !reader.requireNumber(prefix + ".height", op.height)
                || !reader.requireNumber(prefix + ".z", op.z)
                || !reader.optionalNumberDefault(prefix + ".x", op.x)
                || !reader.optionalNumberDefault(prefix + ".y", op.y)
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
                || !reader.requireNumber(prefix + ".radius", op.radius)
                || !reader.requireNumber(prefix + ".height", op.height)
                || !reader.requireNumber(prefix + ".z", op.z)
                || !reader.optionalNumberDefault(prefix + ".x", op.x)
                || !reader.optionalNumberDefault(prefix + ".y", op.y)
                || !reader.optionalIntDefault(prefix + ".vertices", op.vertices)
                || !reader.optionalTextDefault(prefix + ".material", op.material)
                || !reader.optionalNumber(prefix + ".taper_top_radius", taper, present)
                || !reader.readBool(prefix + ".entasis", op.entasis)
                || !reader.optionalNumberDefault(prefix + ".entasis_ratio", op.entasisRatio)
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
                || !reader.requireNumber(prefix + ".radius", op.radius)
                || !reader.requireNumber(prefix + ".z", op.z)
                || !reader.optionalNumberDefault(prefix + ".x", op.x)
                || !reader.optionalNumberDefault(prefix + ".y", op.y)
                || !reader.optionalIntDefault(prefix + ".vertices", op.vertices)
                || !reader.optionalTextDefault(prefix + ".material", op.material)) {
                result.message = reader.error;
                return result;
            }
            result.stream.ops.push_back(std::move(op));
        } else if (*type == "AddRing") {
            AddRingOp op;
            if (!reader.requireText(prefix + ".name", op.name)
                || !reader.requireNumber(prefix + ".radius", op.radius)
                || !reader.requireNumber(prefix + ".tube_height", op.tubeHeight)
                || !reader.requireNumber(prefix + ".z", op.z)
                || !reader.optionalNumberDefault(prefix + ".overhang", op.overhang)
                || !reader.optionalNumberDefault(prefix + ".x", op.x)
                || !reader.optionalNumberDefault(prefix + ".y", op.y)
                || !reader.optionalIntDefault(prefix + ".vertices", op.vertices)
                || !reader.optionalTextDefault(prefix + ".material", op.material)) {
                result.message = reader.error;
                return result;
            }
            result.stream.ops.push_back(std::move(op));
        } else if (*type == "AddMoulding") {
            AddMouldingOp op;
            if (!reader.requireText(prefix + ".name", op.name)
                || !reader.requireNumber(prefix + ".base_z", op.baseZ)
                || !reader.optionalNumberDefault(prefix + ".x", op.x)
                || !reader.optionalNumberDefault(prefix + ".y", op.y)
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
                || !reader.requireNumber(prefix + ".base_z", op.baseZ)
                || !reader.optionalNumberDefault(prefix + ".x", op.x)
                || !reader.optionalNumberDefault(prefix + ".y", op.y)
                || !reader.optionalIntDefault(prefix + ".vertices", op.vertices)
                || !reader.optionalTextDefault(prefix + ".material", op.material)
                || !readMouldingSegments(reader, prefix, op.sequence)) {
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
                || !reader.requireNumber(prefix + ".depth", op.depth)
                || !reader.optionalNumberDefault(prefix + ".width_ratio", op.widthRatio)) {
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
                || !reader.requireNumber(prefix + ".x", op.x)
                || !reader.requireNumber(prefix + ".y", op.y)
                || !reader.requireNumber(prefix + ".z", op.z)) {
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
