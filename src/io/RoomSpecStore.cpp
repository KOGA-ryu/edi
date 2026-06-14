#include "io/RoomSpecStore.h"

#include "formats/TomlReader.h"

#include <cstddef>
#include <optional>
#include <string>

namespace edi::io {

using edi::drafting::RoomEdge;
using edi::drafting::RoomOpening;
using edi::drafting::RoomPlugSpec;
using edi::drafting::RoomSpec;
using edi::formats::StaticConfig;

namespace {

// Tiny typed reads over the flat config — kept local (not SettingsStore's) so
// this parser stays Qt-free: it bridges only edi_format_core and the drafting
// RoomSpec. Same tolerant spirit: a missing/garbage value falls back.
double configDouble(const StaticConfig &config, const std::string &key, double fallback)
{
    const auto it = config.find(key);
    if (it == config.end()) {
        return fallback;
    }
    try {
        std::size_t consumed = 0;
        const double value = std::stod(it->second, &consumed);
        return consumed == it->second.size() ? value : fallback;
    } catch (...) {
        return fallback;
    }
}

std::string configString(const StaticConfig &config, const std::string &key, const std::string &fallback)
{
    const auto it = config.find(key);
    return it == config.end() ? fallback : it->second;
}

bool hasKey(const StaticConfig &config, const std::string &key)
{
    return config.find(key) != config.end();
}

std::optional<RoomEdge> edgeFromName(const std::string &name)
{
    if (name == "N" || name == "n" || name == "north") return RoomEdge::North;
    if (name == "E" || name == "e" || name == "east") return RoomEdge::East;
    if (name == "S" || name == "s" || name == "south") return RoomEdge::South;
    if (name == "W" || name == "w" || name == "west") return RoomEdge::West;
    return std::nullopt;
}

// N/S edges span the room width; E/W span its height (canvas units already).
double edgeLengthCanvas(const RoomSpec &spec, RoomEdge edge)
{
    return (edge == RoomEdge::North || edge == RoomEdge::South) ? spec.width : spec.height;
}

} // namespace

RoomSpecParseResult parseRoomSpecToml(const std::string &text, double canvasPerUnit)
{
    RoomSpecParseResult out;
    if (!(canvasPerUnit > 0.0)) {
        out.message = "canvasPerUnit must be positive";
        return out;
    }
    const auto parsed = edi::formats::readTomlStaticConfig(text);
    if (!parsed.ok || !parsed.value) {
        out.message = parsed.message.empty() ? "could not parse room TOML" : parsed.message;
        return out;
    }
    const StaticConfig &config = *parsed.value;

    RoomSpec spec;
    spec.width = configDouble(config, "room.width", -1.0) * canvasPerUnit;
    spec.height = configDouble(config, "room.height", -1.0) * canvasPerUnit;
    if (!(spec.width > 0.0) || !(spec.height > 0.0)) {
        out.message = "room.width and room.height are required and must be positive";
        return out;
    }
    spec.origin.x = configDouble(config, "room.origin_x", 0.0) * canvasPerUnit;
    spec.origin.y = configDouble(config, "room.origin_y", 0.0) * canvasPerUnit;
    spec.wallThickness = configDouble(config, "room.wall_thickness", 1.0) * canvasPerUnit;
    spec.wallMaterial = configString(config, "room.wall_material", "stone");

    // Openings are a list under indexed keys (the recipe op-stream convention):
    // read room.opening.0.*, .1.*, ... until the first index with no `.edge`.
    for (int i = 0;; ++i) {
        const std::string prefix = "room.opening." + std::to_string(i);
        if (!hasKey(config, prefix + ".edge")) {
            break;
        }
        const std::optional<RoomEdge> edge = edgeFromName(configString(config, prefix + ".edge", ""));
        if (!edge) {
            out.message = prefix + ".edge must be one of N, E, S, W";
            return out;
        }
        RoomOpening opening;
        opening.edge = *edge;
        opening.type = configString(config, prefix + ".type", "door");
        opening.width = configDouble(config, prefix + ".width", -1.0) * canvasPerUnit;
        if (!(opening.width > 0.0)) {
            out.message = prefix + ".width must be positive";
            return out;
        }
        const std::string at = configString(config, prefix + ".at", "center");
        if (at == "center" || at.empty()) {
            opening.center = edgeLengthCanvas(spec, *edge) / 2.0;
        } else {
            std::size_t consumed = 0;
            double offset = 0.0;
            try {
                offset = std::stod(at, &consumed);
            } catch (...) {
                consumed = 0;
            }
            if (consumed == 0) {
                out.message = prefix + ".at must be 'center' or a number (offset from the edge's start corner)";
                return out;
            }
            opening.center = offset * canvasPerUnit;
        }
        spec.openings.push_back(opening);
    }

    // Plugs are a second indexed list, parsed exactly like openings (same edge +
    // at grammar) but a point, not a span — so no width. room.plug.<i>.*, stop at
    // the first index with no `.edge`.
    for (int i = 0;; ++i) {
        const std::string prefix = "room.plug." + std::to_string(i);
        if (!hasKey(config, prefix + ".edge")) {
            break;
        }
        const std::optional<RoomEdge> edge = edgeFromName(configString(config, prefix + ".edge", ""));
        if (!edge) {
            out.message = prefix + ".edge must be one of N, E, S, W";
            return out;
        }
        RoomPlugSpec plug;
        plug.edge = *edge;
        plug.name = configString(config, prefix + ".name", "");
        if (plug.name.empty()) {
            // A plug must be named so a connection (room.connection.*) can refer to
            // it; a nameless plug is unreferenceable and almost certainly a typo.
            out.message = prefix + ".name is required";
            return out;
        }
        plug.type = configString(config, prefix + ".type", "door");
        const std::string at = configString(config, prefix + ".at", "center");
        if (at == "center" || at.empty()) {
            plug.at = edgeLengthCanvas(spec, *edge) / 2.0;
        } else {
            std::size_t consumed = 0;
            double offset = 0.0;
            try {
                offset = std::stod(at, &consumed);
            } catch (...) {
                consumed = 0;
            }
            if (consumed == 0) {
                out.message = prefix + ".at must be 'center' or a number (offset from the edge's start corner)";
                return out;
            }
            plug.at = offset * canvasPerUnit;
        }
        spec.plugs.push_back(plug);
    }

    out.ok = true;
    out.spec = std::move(spec);
    return out;
}

} // namespace edi::io
