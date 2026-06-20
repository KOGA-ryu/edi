#include "io/MapToonExport.h"

#include "drafting/DraftingGeometry.h" // includeBounds
#include "drafting/DraftingMapQuery.h" // deriveEdge, plugIsConnected (shared with the map browser)

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <set>
#include <sstream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

// CANONICAL TOON SECTION ORDER — owned here, the single source of truth.
//
// Document export (DraftingDocument overload) emits sections in this order:
//   rooms · plugs · connections · nodes · blocks  [future: vertical after blocks]
//
// TWO INVARIANTS the realizer must obey:
//   (a) INDEX BY NAME: the realizer reads every cell by its column name from the
//       section header (e.g. {name,anchor,type}), never by zero-based index.  This
//       means new columns can be appended, prepended, or reordered as long as the
//       header is updated — backward-compatible for any reader that keys on names.
//   (b) CONDITIONAL EMISSION: every NEW section (and every NEW column) is emitted
//       ONLY when non-default data exists for it.  A section/column with all-default
//       values is SILENTLY ABSENT.  This is the mechanism that lets the wire grow
//       without ever breaking a legacy byte-identical export: an all-default map
//       produces the same byte string before and after the wire extension.  The
//       engine reader must therefore treat an absent section as "no entries of that
//       kind", and must NOT expect every section to be present.
//
// ONE CANONICAL-ORDER OWNER: this file.  No other code declares the section order.
// Concurrent builders adding new sections must coordinate here to avoid collision.

namespace edi::io {
namespace {

using edi::drafting::MapConnectionSpec;
using edi::drafting::MapSpec;
using edi::drafting::NamedRoomSpec;
using edi::drafting::RoomEdge;

// Minimal numeric form: "%g" prints 21, 47.5, 6 — never 21.000000 — matching how
// the authored .map.toml reads, so the round-trip stays human-legible.
std::string num(double v)
{
    char buffer[32];
    std::snprintf(buffer, sizeof(buffer), "%g", v);
    return std::string(buffer);
}

std::string edgeName(RoomEdge edge)
{
    switch (edge) {
    case RoomEdge::North: return "N";
    case RoomEdge::East:  return "E";
    case RoomEdge::South: return "S";
    case RoomEdge::West:  return "W";
    }
    return "?"; // unreachable (the switch is exhaustive); satisfies the compiler
}

// Quote a TOON cell only when it carries a delimiter/space (so simple tokens like
// `stone` or `N` stay bare, while a coordinate pair `21,47.5` is quoted).
std::string cell(const std::string &value)
{
    bool quote = value.empty();
    for (const char c : value) {
        if (c == ',' || c == '"' || c == ' ' || c == '\t' || c == '\n') {
            quote = true;
            break;
        }
    }
    if (!quote) {
        return value;
    }
    std::string out = "\"";
    for (const char c : value) {
        if (c == '"') {
            out += '\\';
        }
        out += c;
    }
    out += '"';
    return out;
}

std::string plugKey(const std::string &room, const std::string &plug)
{
    return room + "." + plug; // the globally-unique handle plugs/connections use
}

// --- Shared TOON shape ----------------------------------------------------
// Both exportMapToToon overloads (MapSpec vs DraftingDocument) emit the SAME
// document grammar; only the field SOURCES differ (authored num() vs derived
// edge, etc.). These helpers own the byte shape — the preamble and the three
// row forms — so the two callers stay character-for-character identical and
// the grammar lives in one place. Callers pass already-resolved strings; the
// helpers decide what gets cell()-quoted (note: edge is emitted bare, like the
// MapSpec path always did).

// `sceneScale` emits an advisory `scale: <S>` line after `units:`, ONLY when
// sceneScale != 1.0.  Omitting it at S=1 keeps all existing TOON outputs byte-
// identical (the realizer treats missing => 1).  THE FENCE: the exported FEET are
// already scaled; the meta is for the realizer's greybox constants, not room dims.
void writeMapHeader(std::ostringstream &out, const std::string &title,
                    const std::string &units, double sceneScale = 1.0)
{
    out << "kind: map\n";
    if (!title.empty()) {
        out << "title: " << cell(title) << "\n";
    }
    if (!units.empty()) {
        out << "units: " << cell(units) << "\n";
    }
    if (sceneScale != 1.0) {
        // advisory only — the reader scales its own greybox constants; it must
        // NOT divide the room-feet back by S (they are already scaled feet).
        out << "scale: " << num(sceneScale) << "\n";
    }
    out << "\n"; // blank section separator (always present, after scale: if any)
}

void writeRoomRow(std::ostringstream &out, const std::string &name,
                  const std::string &origin, const std::string &size,
                  const std::string &material)
{
    out << "  " << cell(name)
        << "," << cell(origin)
        << "," << cell(size)
        << "," << cell(material)
        << "\n";
}

// Join a plug's neutral flag tokens into the single `·`-separated run the TOON
// `flags` column carries (Fork 2 ratified shape). Empty list -> empty string,
// which the caller passes through cell() like any other cell. The middle dot is
// chosen so the run is ONE cell with no comma/space, staying bare under cell().
std::string joinFlags(const std::vector<std::string> &flags)
{
    std::string out;
    for (std::size_t i = 0; i < flags.size(); ++i) {
        if (i != 0) {
            out += "·"; // U+00B7 MIDDLE DOT
        }
        out += flags[i];
    }
    return out;
}

void writePlugRow(std::ostringstream &out, const std::string &room,
                  const std::string &name, const std::string &edge,
                  const std::string &type, bool connected, const std::string &flags)
{
    out << "  " << cell(room)
        << "," << cell(name)
        << "," << edge // bare: edge tokens are always N/E/S/W/? — no delimiters
        << "," << cell(type)
        << "," << (connected ? "true" : "false")
        << "," << cell(flags) // ·-joined neutral tags; empty -> "" via cell()
        << "\n";
}

void writeConnectionRow(std::ostringstream &out, const std::string &from,
                        const std::string &to, const std::string &type)
{
    out << "  " << cell(from)
        << "," << cell(to)
        << "," << cell(type)
        << "\n";
}

// Like writeConnectionRow but with the NEUTRAL lock TAG columns (locked,key_id)
// APPENDED LAST. Called only when at least one connection in the document carries
// a lock (conditional-emission rule, invariant b) — so an unlocked map stays
// byte-identical to the legacy {from,to,type}. Mirrors the Seam B emission shape
// exactly: `locked` as bare true/false, `key_id` via cell() (empty -> "").
void writeConnectionRowWithLock(std::ostringstream &out, const std::string &from,
                                const std::string &to, const std::string &type,
                                bool locked, const std::string &keyId)
{
    out << "  " << cell(from)
        << "," << cell(to)
        << "," << cell(type)
        << "," << (locked ? "true" : "false")
        << "," << cell(keyId) // empty -> "" via cell()
        << "\n";
}

// Like writeRoomRow but with a `level` integer column APPENDED LAST.
// Called only when at least one room has level != 0 (conditional-emission rule,
// invariant b). Level is a plain integer — never contains a comma or quote, so
// no cell() quoting is needed; stream it directly the way `blocks` streams scale.
void writeRoomRowWithLevel(std::ostringstream &out, const std::string &name,
                           const std::string &origin, const std::string &size,
                           const std::string &material, int level)
{
    out << "  " << cell(name)
        << "," << cell(origin)
        << "," << cell(size)
        << "," << cell(material)
        << "," << level
        << "\n";
}

// Like writePlugRow but with a `level` integer column APPENDED LAST.
// Called only when at least one plug has level != 0 (conditional-emission rule).
void writePlugRowWithLevel(std::ostringstream &out, const std::string &room,
                           const std::string &name, const std::string &edge,
                           const std::string &type, bool connected,
                           const std::string &flags, int level)
{
    out << "  " << cell(room)
        << "," << cell(name)
        << "," << edge // bare: N/E/S/W/? never contain delimiters
        << "," << cell(type)
        << "," << (connected ? "true" : "false")
        << "," << cell(flags)
        << "," << level
        << "\n";
}

// Node connector row: name (fall back to id if empty), anchor in authored feet
// (quoted because x,y always carries a comma), type (empty -> "" via cell()),
// radius via num() (always present whenever the nodes[] section is emitted —
// NOT a conditional column; every node has an authored radius, default 0.5).
void writeNodeRow(std::ostringstream &out, const std::string &name,
                  const std::string &anchor, const std::string &type, double radius)
{
    out << "  " << cell(name)
        << "," << cell(anchor)
        << "," << cell(type)
        << "," << num(radius)
        << "\n";
}

} // namespace

std::string exportMapToToon(const MapSpec &spec, const std::string &title, const std::string &units)
{
    // A plug is `connected` iff some connection names it — so a secret dead-end
    // (a plug with no edge) reads false. Derived here rather than stored.
    std::set<std::string> connected;
    for (const auto &connection : spec.connections) {
        connected.insert(plugKey(connection.from.roomName, connection.from.plugName));
        connected.insert(plugKey(connection.to.roomName, connection.to.plugName));
    }

    std::ostringstream out;
    writeMapHeader(out, title, units);

    out << "rooms[" << spec.rooms.size() << "]{name,origin,size,material}:\n";
    for (const auto &room : spec.rooms) {
        const auto &rs = room.spec;
        writeRoomRow(out, room.name,
                     num(rs.origin.x) + "," + num(rs.origin.y),
                     num(rs.width) + "," + num(rs.height),
                     rs.wallMaterial);
    }
    out << "\n";

    std::size_t plugCount = 0;
    for (const auto &room : spec.rooms) {
        plugCount += room.spec.plugs.size();
    }
    out << "plugs[" << plugCount << "]{room,name,edge,type,connected,flags}:\n";
    for (const auto &room : spec.rooms) {
        for (const auto &plug : room.spec.plugs) {
            const std::string type = plug.type.empty() ? "door" : plug.type;
            writePlugRow(out, room.name, plug.name, edgeName(plug.edge), type,
                         connected.count(plugKey(room.name, plug.name)) > 0,
                         joinFlags(plug.flags));
        }
    }
    out << "\n";

    // Connections — the lock columns (locked,key_id) are CONDITIONAL (invariant b):
    // appended ONLY when at least one connection carries a lock tag. Pre-scan once so
    // the header and every row stay consistent (a fixed-width table: once the columns
    // are "on", every row emits them, including the unlocked rows). When NO connection
    // is locked the header is byte-identical to the legacy {from,to,type} — that is
    // the conditional-absence proof the reference golden pins.
    const bool hasLock = std::any_of(spec.connections.begin(), spec.connections.end(),
        [](const edi::drafting::MapConnectionSpec &c) { return c.locked || !c.keyId.empty(); });
    out << "connections[" << spec.connections.size() << "]{from,to,type";
    if (hasLock) out << ",locked,key_id";
    out << "}:\n";
    for (const auto &connection : spec.connections) {
        out << "  " << cell(plugKey(connection.from.roomName, connection.from.plugName))
            << "," << cell(plugKey(connection.to.roomName, connection.to.plugName))
            << "," << cell(connection.type);
        if (hasLock) {
            out << "," << (connection.locked ? "true" : "false")
                << "," << cell(connection.keyId); // empty -> "" via cell()
        }
        out << "\n";
    }

    // markers[] section — the neutral entity layer (spawn/pickup/npc/goal/chest/…).
    // CONDITIONAL (invariant b): emitted only when SOME room has a feature; an
    // entity-less map (every existing fixture) stays byte-identical. Columns:
    // {room,id,role,x,y,meta}. `role` is the feature's neutral `type`; `x,y` are the
    // ROOM-LOCAL authored-feet offset (the same frame the parser stored — features are
    // not canvas-scaled); `meta` is the metadata projected as ONE `·`-joined run of
    // `key=value` (reusing the middle-dot separator the flags/bounded_by columns use,
    // so the engine splits on the dot then on the first '='). The x,y cell carries a
    // comma so cell() quotes it; the meta run has no comma (key=value pairs joined by
    // the dot) so it stays bare unless empty.
    const bool hasMarkers = std::any_of(spec.rooms.begin(), spec.rooms.end(),
        [](const edi::drafting::NamedRoomSpec &r) { return !r.spec.features.empty(); });
    if (hasMarkers) {
        std::size_t markerCount = 0;
        for (const auto &room : spec.rooms) markerCount += room.spec.features.size();
        out << "\n";
        out << "markers[" << markerCount << "]{room,id,role,x,y,meta}:\n";
        for (const auto &room : spec.rooms) {
            for (const auto &feature : room.spec.features) {
                std::string metaRun;
                for (std::size_t i = 0; i < feature.metadata.size(); ++i) {
                    if (i != 0) metaRun += "·"; // U+00B7 MIDDLE DOT
                    metaRun += feature.metadata[i].first + "=" + feature.metadata[i].second;
                }
                out << "  " << cell(room.name)
                    << "," << cell(feature.id)        // empty -> "" via cell()
                    << "," << cell(feature.type)      // the role
                    << "," << cell(num(feature.x) + "," + num(feature.y)) // room-local feet, quoted
                    << "," << cell(metaRun)
                    << "\n";
            }
        }
    }

    // patrols[] section — neutral patrol paths. CONDITIONAL: emitted only when
    // spec.patrols is non-empty. Columns: {id,closed,points}. `points` is a single
    // `·`-joined run of `x,y` pairs (each pair has a comma, so the WHOLE run carries
    // commas ⇒ cell() quotes it). closed is the literal true/false. The waypoints are
    // in CANVAS units as stored; the MapSpec overload emits authored numbers directly
    // (canvasPerUnit=1.0 on this path), matching how rooms emit their coords here.
    if (!spec.patrols.empty()) {
        out << "\n";
        out << "patrols[" << spec.patrols.size() << "]{id,closed,points}:\n";
        for (const auto &patrol : spec.patrols) {
            std::string pts;
            for (std::size_t i = 0; i < patrol.waypoints.size(); ++i) {
                if (i != 0) pts += "·"; // U+00B7 MIDDLE DOT between waypoints
                pts += num(patrol.waypoints[i].x) + "," + num(patrol.waypoints[i].y);
            }
            out << "  " << cell(patrol.id)
                << "," << (patrol.closed ? "true" : "false")
                << "," << cell(pts) // quoted: the x,y pairs carry commas
                << "\n";
        }
    }

    return out.str();
}

namespace {

using edi::drafting::DraftingMapRoom;
using edi::drafting::DraftingPlug;      // needed for hasPlugLevel lambda + plug loop
using edi::drafting::Point2D;
using edi::drafting::RoomDerivation;    // needed for hasRoomDerivation lambda
using edi::drafting::roomDerivationName; // needed for derivation column value

// Split a globally-unique plug handle "room.plug" on its first '.'.
std::pair<std::string, std::string> splitRoomPlug(const std::string &full)
{
    const std::size_t dot = full.find('.');
    if (dot == std::string::npos) {
        return {full, std::string{}};
    }
    return {full.substr(0, dot), full.substr(dot + 1)};
}

} // namespace

std::string exportMapToToon(const edi::drafting::DraftingDocument &document,
                            const std::string &title, const std::string &units,
                            double sceneScale)
{
    // Authored units = canvas / scale. Guard a zero/invalid scale (treat as 1:1).
    const double scale = document.canvasPerAuthoredUnit > 0.0 ? document.canvasPerAuthoredUnit : 1.0;
    const auto authored = [scale](double canvas) { return num(canvas / scale); };
    const auto findRoom = [&document](const std::string &name) -> const DraftingMapRoom * {
        for (const auto &room : document.rooms) {
            if (room.name == name) {
                return &room;
            }
        }
        return nullptr;
    };

    std::ostringstream out;
    // Pass sceneScale so the header emits `scale: S` only when S != 1.0.
    // The MapSpec overload calls writeMapHeader without sceneScale → default 1.0
    // → no scale: line → all existing map_toon_export_tests stay byte-identical.
    writeMapHeader(out, title, units, sceneScale);

    // Pre-scan: which optional columns are needed for rooms and plugs?
    // An optional column is emitted only when at least one row carries a
    // non-default value (conditional-emission, invariant b from A1). Scanning
    // once up-front keeps the header and every row consistent: once a column is
    // "on", ALL rows emit it — including the default-valued rows — because TOON
    // is a fixed-width table (every row must have the same number of cells).
    const bool hasRoomLevel      = std::any_of(document.rooms.begin(), document.rooms.end(),
        [](const DraftingMapRoom &r) { return r.level != 0; });
    const bool hasRoomDerivation = std::any_of(document.rooms.begin(), document.rooms.end(),
        [](const DraftingMapRoom &r) { return r.derivation != RoomDerivation::Placed; });
    const bool hasRoomBoundedBy  = std::any_of(document.rooms.begin(), document.rooms.end(),
        [](const DraftingMapRoom &r) { return !r.boundedBy.empty(); });
    const bool hasPlugLevel      = std::any_of(document.plugs.begin(), document.plugs.end(),
        [](const DraftingPlug &p) { return p.level != 0; });

    // Node id → display name: used to resolve boundedBy ids to human-readable names
    // for the `bounded_by` column.  The engine reads by name (header-as-truth); ids
    // are edi's internal opaque handles, never surfaced in the TOON wire.
    // Fall back to the id itself when the node has no authored name.
    std::unordered_map<std::string, std::string> nodeDisplayName;
    for (const auto &node : document.nodes) {
        nodeDisplayName.emplace(node.id, node.name.empty() ? node.id : node.name);
    }
    // Resolve a room's boundedBy id vector to a middle-dot-joined name string —
    // the same `·` separator joinFlags uses, so the TOON reader splits on dots.
    const auto resolveBoundedBy = [&nodeDisplayName](const DraftingMapRoom &room) {
        std::string joined;
        for (std::size_t i = 0; i < room.boundedBy.size(); ++i) {
            if (i != 0) joined += "·"; // U+00B7 MIDDLE DOT
            const auto it = nodeDisplayName.find(room.boundedBy[i]);
            joined += (it != nodeDisplayName.end()) ? it->second : room.boundedBy[i];
        }
        return joined;
    };

    // Rooms section — unified conditional header + rows.
    // Canonical column order: name,origin,size,material[,level][,derivation][,bounded_by].
    // Each optional column appears only when its pre-scan flag is true.
    out << "rooms[" << document.rooms.size() << "]{name,origin,size,material";
    if (hasRoomLevel)      out << ",level";
    if (hasRoomDerivation) out << ",derivation";
    if (hasRoomBoundedBy)  out << ",bounded_by";
    out << "}:\n";
    for (const auto &room : document.rooms) {
        out << "  " << cell(room.name)
            << "," << cell(authored(room.origin.x) + "," + authored(room.origin.y))
            << "," << cell(authored(room.width) + "," + authored(room.height))
            << "," << cell(room.material);
        if (hasRoomLevel)      out << "," << room.level;
        if (hasRoomDerivation) out << "," << roomDerivationName(room.derivation);
        if (hasRoomBoundedBy)  out << "," << cell(resolveBoundedBy(room));
        out << "\n";
    }
    out << "\n";

    if (hasPlugLevel) {
        out << "plugs[" << document.plugs.size() << "]{room,name,edge,type,connected,flags,level}:\n";
        for (const auto &plug : document.plugs) {
            const auto [roomName, plugName] = splitRoomPlug(plug.name);
            std::string edge = "?";
            if (const DraftingMapRoom *room = findRoom(roomName)) {
                edge = edi::drafting::deriveEdge(*room, plug.anchor);
            }
            const std::string type = plug.type.empty() ? "door" : plug.type;
            writePlugRowWithLevel(out, roomName, plugName, edge, type,
                                  edi::drafting::plugIsConnected(document.connections, plug.id),
                                  joinFlags(plug.flags), plug.level);
        }
    } else {
        out << "plugs[" << document.plugs.size() << "]{room,name,edge,type,connected,flags}:\n";
        for (const auto &plug : document.plugs) {
            const auto [roomName, plugName] = splitRoomPlug(plug.name);
            std::string edge = "?";
            if (const DraftingMapRoom *room = findRoom(roomName)) {
                edge = edi::drafting::deriveEdge(*room, plug.anchor);
            }
            const std::string type = plug.type.empty() ? "door" : plug.type;
            // Shared with the map browser: a plug is connected iff a declared connection
            // names it (DraftingMapQuery — the single source so the two views never drift).
            writePlugRow(out, roomName, plugName, edge, type,
                         edi::drafting::plugIsConnected(document.connections, plug.id),
                         joinFlags(plug.flags));
        }
    }
    out << "\n";

    // Resolve a plug id back to its "room.plug" name (the engine reads names).
    const auto plugNameById = [&document](const std::string &id) -> std::string {
        for (const auto &plug : document.plugs) {
            if (plug.id == id) {
                return plug.name;
            }
        }
        return id; // fall back to the raw id if unresolved
    };
    // Lock columns (locked,key_id) are CONDITIONAL (invariant b), mirroring the Seam B
    // overload: appended ONLY when at least one connection carries a lock tag. Pre-scan
    // once so header and rows stay consistent. When NO connection is locked the header
    // is byte-identical to the legacy {from,to,type} — the conditional-absence the new
    // reference Seam C golden pins. (Pure emit of a NEUTRAL tag — no rule interpreted.)
    const bool hasLock = std::any_of(document.connections.begin(), document.connections.end(),
        [](const edi::drafting::DraftingDeclaredConnection &c) { return c.locked || !c.keyId.empty(); });
    out << "connections[" << document.connections.size() << "]{from,to,type";
    if (hasLock) out << ",locked,key_id";
    out << "}:\n";
    for (const auto &connection : document.connections) {
        if (hasLock) {
            writeConnectionRowWithLock(out, plugNameById(connection.plugA),
                                       plugNameById(connection.plugB), connection.type,
                                       connection.locked, connection.keyId);
        } else {
            writeConnectionRow(out, plugNameById(connection.plugA),
                               plugNameById(connection.plugB), connection.type);
        }
    }
    out << "\n";

    // nodes[] section (canonical position: after connections, before blocks).
    //
    // CONDITIONAL EMISSION (invariant b): the section is written ONLY when
    // document.nodes is non-empty.  An empty vector ⇒ no header, no blank line,
    // nothing — so every legacy map (no nodes) stays byte-identical through this
    // wire extension.  The engine reader must treat an absent `nodes[]` section
    // as "zero nodes", not as an error.
    //
    // Column shape: {name,anchor,type,radius}.  `name` falls back to `id` if the
    // authored name is empty, so every row has a non-empty identity key.
    // `anchor` is in AUTHORED FEET (canvas / canvasPerAuthoredUnit), quoted
    // because the "x,y" form always contains a comma.  `type` is an open
    // vocabulary tag edi does not interpret — empty is a valid value.
    // `radius` is the authored footprint radius via num() — NOT a conditional
    // column; it is ALWAYS present when the section is emitted (every node has
    // one, default 0.5).
    if (!document.nodes.empty()) {
        out << "nodes[" << document.nodes.size() << "]{name,anchor,type,radius}:\n";
        for (const auto &node : document.nodes) {
            const std::string label = node.name.empty() ? node.id : node.name;
            writeNodeRow(out, label,
                         authored(node.anchor.x) + "," + authored(node.anchor.y),
                         node.type, node.radius);
        }
        out << "\n";
    }

    // Block instances: FLATTEN scattered each placement into N independent objects,
    // so re-form them by grouping on the BlockPlacementMetadata instanceId. The
    // placement centre is the union of the group's bounds; the room is whichever
    // footprint contains it (canvas units). scale/rotation come from the placement's
    // metadata (FLATTEN stamps one instance's objects with the same values); they are
    // identity (1/0) until DM-14 writes real values.
    struct Accum {
        std::string asset;
        edi::drafting::Bounds2D bounds;
        double scale = 1.0;       // from BlockPlacementMetadata.scale (identity 1)
        double rotationDeg = 0.0; // from BlockPlacementMetadata.rotationDeg (identity 0)
        bool init = false;
    };
    std::vector<std::string> order;
    std::unordered_map<std::string, Accum> instances;
    for (const auto &object : document.objects) {
        const auto &bp = object.metadata.blockPlacement;
        if (bp.instanceId.empty()) {
            continue; // an ordinary object, not a placement
        }
        Accum &acc = instances[bp.instanceId];
        if (!acc.init) {
            acc.init = true;
            acc.asset = bp.assetRef;
            acc.bounds = object.bounds;
            acc.scale = bp.scale;             // the instance's stamped placement transform
            acc.rotationDeg = bp.rotationDeg; // (shared across the group; read off the first)
            order.push_back(bp.instanceId);
        } else {
            acc.bounds = edi::drafting::includeBounds(acc.bounds, object.bounds);
        }
    }
    const auto roomAt = [&document](Point2D point) -> std::string {
        for (const auto &room : document.rooms) {
            if (point.x >= room.origin.x && point.x <= room.origin.x + room.width
                && point.y >= room.origin.y && point.y <= room.origin.y + room.height) {
                return room.name;
            }
        }
        return std::string{};
    };
    out << "blocks[" << order.size() << "]{room,asset,origin,scale,rotation}:\n";
    for (const std::string &id : order) {
        const Accum &acc = instances[id];
        const Point2D centre{acc.bounds.x + acc.bounds.width / 2.0, acc.bounds.y + acc.bounds.height / 2.0};
        out << "  " << cell(roomAt(centre))
            << "," << cell(acc.asset)
            << "," << cell(authored(centre.x) + "," + authored(centre.y))
            << "," << num(acc.scale)        // scale column (identity 1 -> "1")
            << "," << num(acc.rotationDeg)  // rotation column (identity 0 -> "0")
            << "\n";
    }
    return out.str();
}

} // namespace edi::io
