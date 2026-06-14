#include "io/RoomSpecStore.h"

#include "formats/TomlReader.h"

#include <cstddef>
#include <optional>
#include <set>
#include <string>

namespace edi::io {

using edi::drafting::RoomConnectionSpec;
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

// --- per-room field parsers, keyed by a prefix ------------------------------
// The single-room dialect uses prefix "room" (room.width, room.opening.0.*, …);
// the multi-room dialect uses "map.room.<i>" (map.room.0.width, …). The grammar
// is identical under the prefix, so both dialects call these same helpers — the
// behaviour-preserving refactor that lets multi-room reuse rather than fork the
// single-room parser. Each returns false and sets `message` on the first error.

bool parseRoomFields(const StaticConfig &config, const std::string &prefix, double canvasPerUnit,
                     RoomSpec &spec, std::string &message)
{
    spec.width = configDouble(config, prefix + ".width", -1.0) * canvasPerUnit;
    spec.height = configDouble(config, prefix + ".height", -1.0) * canvasPerUnit;
    if (!(spec.width > 0.0) || !(spec.height > 0.0)) {
        message = prefix + ".width and " + prefix + ".height are required and must be positive";
        return false;
    }
    spec.origin.x = configDouble(config, prefix + ".origin_x", 0.0) * canvasPerUnit;
    spec.origin.y = configDouble(config, prefix + ".origin_y", 0.0) * canvasPerUnit;
    spec.wallThickness = configDouble(config, prefix + ".wall_thickness", 1.0) * canvasPerUnit;
    spec.wallMaterial = configString(config, prefix + ".wall_material", "stone");
    return true;
}

bool parseRoomOpenings(const StaticConfig &config, const std::string &prefix, double canvasPerUnit,
                       RoomSpec &spec, std::string &message)
{
    for (int i = 0;; ++i) {
        const std::string key = prefix + ".opening." + std::to_string(i);
        if (!hasKey(config, key + ".edge")) {
            break;
        }
        const std::optional<RoomEdge> edge = edgeFromName(configString(config, key + ".edge", ""));
        if (!edge) {
            message = key + ".edge must be one of N, E, S, W";
            return false;
        }
        RoomOpening opening;
        opening.edge = *edge;
        opening.type = configString(config, key + ".type", "door");
        opening.width = configDouble(config, key + ".width", -1.0) * canvasPerUnit;
        if (!(opening.width > 0.0)) {
            message = key + ".width must be positive";
            return false;
        }
        const std::string at = configString(config, key + ".at", "center");
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
                message = key + ".at must be 'center' or a number (offset from the edge's start corner)";
                return false;
            }
            opening.center = offset * canvasPerUnit;
        }
        spec.openings.push_back(opening);
    }
    return true;
}

bool parseRoomPlugs(const StaticConfig &config, const std::string &prefix, double canvasPerUnit,
                    RoomSpec &spec, std::set<std::string> &plugNames, std::string &message)
{
    for (int i = 0;; ++i) {
        const std::string key = prefix + ".plug." + std::to_string(i);
        if (!hasKey(config, key + ".edge")) {
            break;
        }
        const std::optional<RoomEdge> edge = edgeFromName(configString(config, key + ".edge", ""));
        if (!edge) {
            message = key + ".edge must be one of N, E, S, W";
            return false;
        }
        RoomPlugSpec plug;
        plug.edge = *edge;
        plug.name = configString(config, key + ".name", "");
        if (plug.name.empty()) {
            message = key + ".name is required";
            return false;
        }
        if (!plugNames.insert(plug.name).second) {
            message = key + ".name '" + plug.name + "' is duplicated (plug names must be unique)";
            return false;
        }
        plug.type = configString(config, key + ".type", "door");
        const std::string at = configString(config, key + ".at", "center");
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
                message = key + ".at must be 'center' or a number (offset from the edge's start corner)";
                return false;
            }
            plug.at = offset * canvasPerUnit;
        }
        spec.plugs.push_back(plug);
    }
    return true;
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
    if (!parseRoomFields(config, "room", canvasPerUnit, spec, out.message)) {
        return out;
    }
    if (!parseRoomOpenings(config, "room", canvasPerUnit, spec, out.message)) {
        return out;
    }
    // The plug-name set is filled by the helper AND reused below to resolve this
    // room's connections — one shared set, so a single-room connection still binds.
    std::set<std::string> plugNames;
    if (!parseRoomPlugs(config, "room", canvasPerUnit, spec, plugNames, out.message)) {
        return out;
    }

    // Connections are edges between plugs, authored at MAP level (a connection can
    // span rooms). map.connection.<i>.{from,to,type}; stop at the first index with
    // no `.from`. In a single-room file both ends resolve within this room's plugs.
    for (int i = 0;; ++i) {
        const std::string prefix = "map.connection." + std::to_string(i);
        if (!hasKey(config, prefix + ".from")) {
            break;
        }
        RoomConnectionSpec connection;
        connection.from = configString(config, prefix + ".from", "");
        connection.to = configString(config, prefix + ".to", "");
        connection.type = configString(config, prefix + ".type", "");
        if (connection.from.empty() || connection.to.empty()) {
            out.message = prefix + " needs both .from and .to (plug names)";
            return out;
        }
        if (plugNames.find(connection.from) == plugNames.end()) {
            out.message = prefix + ".from references unknown plug '" + connection.from + "'";
            return out;
        }
        if (plugNames.find(connection.to) == plugNames.end()) {
            out.message = prefix + ".to references unknown plug '" + connection.to + "'";
            return out;
        }
        spec.connections.push_back(connection);
    }

    out.ok = true;
    out.spec = std::move(spec);
    return out;
}

} // namespace edi::io
