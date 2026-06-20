#include "io/RoomSpecStore.h"

#include "drafting/DraftingOverlap.h"

#include "formats/TomlReader.h"

#include <cctype>
#include <cstddef>
#include <optional>
#include <set>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace edi::io {

using edi::drafting::MapConnectionSpec;
using edi::drafting::MapPlugRef;
using edi::drafting::MapSpec;
using edi::drafting::NamedRoomSpec;
using edi::drafting::Point2D;
using edi::drafting::RoomConnectionSpec;
using edi::drafting::RoomEdge;
using edi::drafting::RoomFeature;
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

// Split a neutral comma-separated tag list ("window, passes_light") into trimmed
// tokens, dropping empties. Open vocabulary — no token is interpreted (mandate).
std::vector<std::string> splitCommaTokens(const std::string &value)
{
    std::vector<std::string> tokens;
    std::size_t start = 0;
    while (start <= value.size()) {
        const std::size_t comma = value.find(',', start);
        const std::size_t end = comma == std::string::npos ? value.size() : comma;
        std::size_t a = start;
        std::size_t b = end;
        while (a < b && std::isspace(static_cast<unsigned char>(value[a]))) ++a;
        while (b > a && std::isspace(static_cast<unsigned char>(value[b - 1]))) --b;
        if (b > a) {
            tokens.push_back(value.substr(a, b - a));
        }
        if (comma == std::string::npos) {
            break;
        }
        start = comma + 1;
    }
    return tokens;
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
        plug.flags = splitCommaTokens(configString(config, key + ".flags", ""));
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

// Interior point features: a contiguous indexed list, exactly the plug/opening
// dialect (room.feature.0.*, .1.*, … until the first absent index). The
// discriminating key is `.type` — a feature is a neutral typed marker — so an
// index with no `.type` ends the list. Unlike plugs/openings, x/y are NOT scaled
// to canvas here: features are stored in AUTHORED FEET (room-local), and the
// authoring controller applies the authored→canvas scale to the offset (see
// RoomFeature's coordinate-frame note). edi assigns the type no meaning (mandate).
bool parseRoomFeatures(const StaticConfig &config, const std::string &prefix, RoomSpec &spec)
{
    for (int i = 0;; ++i) {
        const std::string key = prefix + ".feature." + std::to_string(i);
        if (!hasKey(config, key + ".type")) {
            break;
        }
        RoomFeature feature;
        feature.x = configDouble(config, key + ".x", 0.0);    // authored feet, NOT scaled
        feature.y = configDouble(config, key + ".y", 0.0);    // (room-local offset from origin)
        feature.type = configString(config, key + ".type", "");
        feature.name = configString(config, key + ".name", "");
        // NEUTRAL identity + free-form bag — the engine reads these; edi only records.
        // `id` defaults "" (most features are anonymous decor); a pickup/npc/chest gives
        // one so a door's key_id / an npc's patrol can cross-reference it.
        feature.id = configString(config, key + ".id", "");
        // metadata is its OWN contiguous indexed sublist, keyed by `.key` (the
        // discriminator): feature.<i>.meta.<j>.{key,value}, read until the first
        // absent `.key`. Same op-stream dialect as openings/plugs, nested one level.
        for (int j = 0;; ++j) {
            const std::string mkey = key + ".meta." + std::to_string(j);
            if (!hasKey(config, mkey + ".key")) {
                break;
            }
            feature.metadata.emplace_back(configString(config, mkey + ".key", ""),
                                          configString(config, mkey + ".value", ""));
        }
        spec.features.push_back(feature);
    }
    return true;
}

// Patrol paths authored at MAP level: map.patrol.<i>.{id, closed, point.<j>.{x,y}}.
// Discriminator is `.id` (every patrol needs one — an npc references it by id).
// Waypoints are a nested contiguous sublist read until the first absent `.x`, and
// — unlike features — they ARE scaled authored-feet -> canvas here, matching how
// room/connection/plug coords are stored (the rest of MapSpec is already-scaled).
// `closed` defaults "true" (a patrol is a loop unless told otherwise).
bool parseMapPatrols(const StaticConfig &config, double canvasPerUnit, MapSpec &map,
                     std::string &message)
{
    for (int i = 0;; ++i) {
        const std::string key = "map.patrol." + std::to_string(i);
        if (!hasKey(config, key + ".id")) {
            break;
        }
        edi::drafting::MapPatrolPath patrol;
        patrol.id = configString(config, key + ".id", "");
        if (patrol.id.empty()) {
            message = key + ".id is required";
            return false;
        }
        // "true"/anything-else: only an explicit "false" opens the loop. The engine
        // owns the meaning; edi just records the flag.
        patrol.closed = configString(config, key + ".closed", "true") != "false";
        for (int j = 0;; ++j) {
            const std::string pkey = key + ".point." + std::to_string(j);
            if (!hasKey(config, pkey + ".x")) {
                break;
            }
            patrol.waypoints.push_back(Point2D{
                configDouble(config, pkey + ".x", 0.0) * canvasPerUnit,
                configDouble(config, pkey + ".y", 0.0) * canvasPerUnit});
        }
        if (patrol.waypoints.size() < 2) {
            message = key + " needs at least two points";
            return false;
        }
        map.patrols.push_back(std::move(patrol));
    }
    return true;
}

// MapSpec-level prop instances: map.block.<i>.{asset_ref, x, y, rotation, scale, name}.
// Discriminator is `.asset_ref`. CRITICAL coordinate-frame note: MapBlockSpec.position
// is documented as ABSOLUTE AUTHORED FEET — createMapFromSpec multiplies it by
// canvasPerAuthoredUnit when it mints the marker. So we DELIBERATELY do NOT scale here
// (scaling would double-apply). This mirrors RoomFeature: both stay in authored feet,
// with the controller owning the authored->canvas scale. rotation/scale are identity
// defaults (0/1).
bool parseMapBlocks(const StaticConfig &config, MapSpec &map)
{
    for (int i = 0;; ++i) {
        const std::string key = "map.block." + std::to_string(i);
        if (!hasKey(config, key + ".asset_ref")) {
            break;
        }
        edi::drafting::MapBlockSpec block;
        block.assetRef = configString(config, key + ".asset_ref", "");
        block.position = Point2D{configDouble(config, key + ".x", 0.0),   // authored feet, NOT scaled
                                 configDouble(config, key + ".y", 0.0)};
        block.rotationDeg = configDouble(config, key + ".rotation", 0.0);
        block.scale = configDouble(config, key + ".scale", 1.0);
        block.name = configString(config, key + ".name", "");
        map.blocks.push_back(std::move(block));
    }
    return true;
}

// Split a cross-room ref "room.plug" on the FIRST dot. nullopt if there is no dot
// or either half is empty. (Names are dotless by convention; a stray dot would
// mis-split, but the room/plug existence check below then rejects it loudly.)
std::optional<std::pair<std::string, std::string>> splitRoomPlug(const std::string &ref)
{
    const auto dot = ref.find('.');
    if (dot == std::string::npos) {
        return std::nullopt;
    }
    std::string room = ref.substr(0, dot);
    std::string plug = ref.substr(dot + 1);
    if (room.empty() || plug.empty()) {
        return std::nullopt;
    }
    return std::make_pair(std::move(room), std::move(plug));
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
    parseRoomFeatures(config, "room", spec);

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

MapSpecParseResult parseMapSpecToml(const std::string &text, double canvasPerUnit)
{
    MapSpecParseResult out;
    if (!(canvasPerUnit > 0.0)) {
        out.message = "canvasPerUnit must be positive";
        return out;
    }
    const auto parsed = edi::formats::readTomlStaticConfig(text);
    if (!parsed.ok || !parsed.value) {
        out.message = parsed.message.empty() ? "could not parse map TOML" : parsed.message;
        return out;
    }
    const StaticConfig &config = *parsed.value;

    MapSpec map;
    std::set<std::string> roomNames;
    std::unordered_map<std::string, std::set<std::string>> plugsByRoom; // room name -> its plug names

    // Rooms: map.room.<i>.*, same per-room grammar as a single room (shared
    // helpers), each carrying a unique name. Stop at the first index with no name.
    for (int i = 0;; ++i) {
        const std::string prefix = "map.room." + std::to_string(i);
        if (!hasKey(config, prefix + ".name")) {
            break;
        }
        const std::string name = configString(config, prefix + ".name", "");
        if (name.empty()) {
            out.message = prefix + ".name is required";
            return out;
        }
        if (!roomNames.insert(name).second) {
            out.message = prefix + ".name '" + name + "' is duplicated (room names must be unique)";
            return out;
        }

        RoomSpec spec;
        if (!parseRoomFields(config, prefix, canvasPerUnit, spec, out.message)) {
            return out;
        }
        if (!parseRoomOpenings(config, prefix, canvasPerUnit, spec, out.message)) {
            return out;
        }
        std::set<std::string> plugNames; // fresh per room — plug names are room-scoped
        if (!parseRoomPlugs(config, prefix, canvasPerUnit, spec, plugNames, out.message)) {
            return out;
        }
        parseRoomFeatures(config, prefix, spec);
        plugsByRoom.emplace(name, std::move(plugNames));
        map.rooms.push_back({name, std::move(spec)});
    }
    if (map.rooms.empty()) {
        out.message = "a map needs at least one room (map.room.0.name)";
        return out;
    }

    // Footprints must not overlap — pure spec-level AABB, before any geometry.
    for (std::size_t a = 0; a < map.rooms.size(); ++a) {
        for (std::size_t b = a + 1; b < map.rooms.size(); ++b) {
            if (edi::drafting::footprintsOverlap(
                    map.rooms[a].spec.origin, map.rooms[a].spec.width, map.rooms[a].spec.height,
                    map.rooms[b].spec.origin, map.rooms[b].spec.width, map.rooms[b].spec.height)) {
                out.message = "rooms '" + map.rooms[a].name + "' and '" + map.rooms[b].name + "' overlap";
                return out;
            }
        }
    }

    // Cross-room connections: from/to are "room.plug"; both halves must resolve to
    // a live (room, plug), and a connection may not join a plug to itself.
    for (int k = 0;; ++k) {
        const std::string prefix = "map.connection." + std::to_string(k);
        if (!hasKey(config, prefix + ".from")) {
            break;
        }
        const std::string fromStr = configString(config, prefix + ".from", "");
        const std::string toStr = configString(config, prefix + ".to", "");
        if (fromStr.empty() || toStr.empty()) {
            out.message = prefix + " needs both .from and .to (\"room.plug\")";
            return out;
        }
        if (fromStr == toStr) {
            out.message = prefix + " joins a plug to itself ('" + fromStr + "')";
            return out;
        }
        const auto from = splitRoomPlug(fromStr);
        const auto to = splitRoomPlug(toStr);
        if (!from) {
            out.message = prefix + ".from must be 'room.plug' (got '" + fromStr + "')";
            return out;
        }
        if (!to) {
            out.message = prefix + ".to must be 'room.plug' (got '" + toStr + "')";
            return out;
        }
        const auto resolve = [&](const std::pair<std::string, std::string> &ref, const std::string &side) -> bool {
            const auto room = plugsByRoom.find(ref.first);
            if (room == plugsByRoom.end()) {
                out.message = prefix + "." + side + " references unknown room '" + ref.first + "'";
                return false;
            }
            if (room->second.find(ref.second) == room->second.end()) {
                out.message = prefix + "." + side + " references unknown plug '" + ref.second + "' in room '" + ref.first + "'";
                return false;
            }
            return true;
        };
        if (!resolve(*from, "from") || !resolve(*to, "to")) {
            return out;
        }

        MapConnectionSpec connection;
        connection.from = MapPlugRef{from->first, from->second};
        connection.to = MapPlugRef{to->first, to->second};
        connection.type = configString(config, prefix + ".type", "");
        // NEUTRAL engine tags (M1): lock STATE + which key. `locked` defaults "false";
        // only an explicit "true" sets it. edi records; the engine owns the gate rule.
        connection.locked = configString(config, prefix + ".locked", "false") == "true";
        connection.keyId = configString(config, prefix + ".key_id", "");
        map.connections.push_back(connection);
    }

    // Patrol paths + prop blocks at MAP level (after rooms/connections so the
    // referential checks below can see everything they reference).
    if (!parseMapPatrols(config, canvasPerUnit, map, out.message)) {
        return out;
    }
    if (!parseMapBlocks(config, map)) {
        return out;
    }

    // --- Referential integrity of AUTHORED IDS (guardrail #4) -----------------
    // This is NOT a lock rule — edi enforces NO gate. It is the same loud
    // dangling-ref rejection the room/plug resolution above does: an authored id
    // that points at nothing is an authoring MISTAKE, caught in the pure parser
    // before any geometry exists. The engine still owns all behaviour.
    //
    // Collect every marker id (feature.id), index features by id, and collect the
    // set of patrol ids. Reject: duplicate marker ids, duplicate patrol ids, an npc
    // marker whose metadata patrol=<id> names no patrol, and a connection/marker
    // key_id with no matching pickup id.
    std::set<std::string> markerIds;          // every non-empty feature.id, for uniqueness
    std::set<std::string> pickupIds;          // ids that a key_id may match (any marker's id)
    std::vector<std::pair<std::string, std::string>> patrolRefs; // (roomName, patrol id) to check
    std::vector<std::pair<std::string, std::string>> keyRefs;    // (where, key_id) to check
    for (const auto &room : map.rooms) {
        for (const auto &feature : room.spec.features) {
            if (!feature.id.empty()) {
                if (!markerIds.insert(feature.id).second) {
                    out.message = "marker id '" + feature.id + "' is duplicated (marker ids must be unique)";
                    return out;
                }
                pickupIds.insert(feature.id); // any marker's id may be a key target
            }
            for (const auto &kv : feature.metadata) {
                if (kv.first == "patrol") {
                    patrolRefs.emplace_back(room.name, kv.second);
                } else if (kv.first == "key_id") {
                    keyRefs.emplace_back("marker '" + room.name + "'", kv.second);
                }
            }
        }
    }
    std::set<std::string> patrolIds;
    for (const auto &patrol : map.patrols) {
        if (!patrolIds.insert(patrol.id).second) {
            out.message = "patrol id '" + patrol.id + "' is duplicated (patrol ids must be unique)";
            return out;
        }
    }
    for (const auto &connection : map.connections) {
        if (!connection.keyId.empty()) {
            keyRefs.emplace_back("connection '" + connection.from.roomName + "." + connection.from.plugName + "'",
                                 connection.keyId);
        }
    }
    for (const auto &ref : patrolRefs) {
        if (patrolIds.find(ref.second) == patrolIds.end()) {
            out.message = "npc in room '" + ref.first + "' references unknown patrol '" + ref.second + "'";
            return out;
        }
    }
    for (const auto &ref : keyRefs) {
        if (pickupIds.find(ref.second) == pickupIds.end()) {
            out.message = ref.first + " references key_id '" + ref.second + "' with no matching pickup id";
            return out;
        }
    }

    out.ok = true;
    out.spec = std::move(map);
    return out;
}

} // namespace edi::io
