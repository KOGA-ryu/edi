#include "io/MapToonExport.h"

#include "drafting/DraftingGeometry.h" // includeBounds

#include <cmath>
#include <cstdio>
#include <set>
#include <sstream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace edi::io {
namespace {

using edi::drafting::MapSpec;
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

void writeMapHeader(std::ostringstream &out, const std::string &title, const std::string &units)
{
    out << "kind: map\n";
    if (!title.empty()) {
        out << "title: " << cell(title) << "\n";
    }
    if (!units.empty()) {
        out << "units: " << cell(units) << "\n";
    }
    out << "\n";
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

void writePlugRow(std::ostringstream &out, const std::string &room,
                  const std::string &name, const std::string &edge,
                  const std::string &type, bool connected)
{
    out << "  " << cell(room)
        << "," << cell(name)
        << "," << edge // bare: edge tokens are always N/E/S/W/? — no delimiters
        << "," << cell(type)
        << "," << (connected ? "true" : "false")
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
    out << "plugs[" << plugCount << "]{room,name,edge,type,connected}:\n";
    for (const auto &room : spec.rooms) {
        for (const auto &plug : room.spec.plugs) {
            const std::string type = plug.type.empty() ? "door" : plug.type;
            writePlugRow(out, room.name, plug.name, edgeName(plug.edge), type,
                         connected.count(plugKey(room.name, plug.name)) > 0);
        }
    }
    out << "\n";

    out << "connections[" << spec.connections.size() << "]{from,to,type}:\n";
    for (const auto &connection : spec.connections) {
        writeConnectionRow(out,
                           plugKey(connection.from.roomName, connection.from.plugName),
                           plugKey(connection.to.roomName, connection.to.plugName),
                           connection.type);
    }
    return out.str();
}

namespace {

using edi::drafting::DraftingMapRoom;
using edi::drafting::Point2D;

// Split a globally-unique plug handle "room.plug" on its first '.'.
std::pair<std::string, std::string> splitRoomPlug(const std::string &full)
{
    const std::size_t dot = full.find('.');
    if (dot == std::string::npos) {
        return {full, std::string{}};
    }
    return {full.substr(0, dot), full.substr(dot + 1)};
}

// Which edge of a room footprint an anchor sits on — the document plug carries no
// edge, so derive it as the nearest of the four sides (canvas units; origin = NW,
// +y south).
std::string deriveEdge(const DraftingMapRoom &room, Point2D anchor)
{
    double best = std::abs(anchor.y - room.origin.y);
    std::string edge = "N";
    if (const double d = std::abs(anchor.y - (room.origin.y + room.height)); d < best) { best = d; edge = "S"; }
    if (const double d = std::abs(anchor.x - room.origin.x); d < best) { best = d; edge = "W"; }
    if (const double d = std::abs(anchor.x - (room.origin.x + room.width)); d < best) { best = d; edge = "E"; }
    return edge;
}

} // namespace

std::string exportMapToToon(const edi::drafting::DraftingDocument &document,
                            const std::string &title, const std::string &units)
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
    writeMapHeader(out, title, units);

    out << "rooms[" << document.rooms.size() << "]{name,origin,size,material}:\n";
    for (const auto &room : document.rooms) {
        writeRoomRow(out, room.name,
                     authored(room.origin.x) + "," + authored(room.origin.y),
                     authored(room.width) + "," + authored(room.height),
                     room.material);
    }
    out << "\n";

    std::set<std::string> connectedPlugIds;
    for (const auto &connection : document.connections) {
        connectedPlugIds.insert(connection.plugA);
        connectedPlugIds.insert(connection.plugB);
    }
    out << "plugs[" << document.plugs.size() << "]{room,name,edge,type,connected}:\n";
    for (const auto &plug : document.plugs) {
        const auto [roomName, plugName] = splitRoomPlug(plug.name);
        std::string edge = "?";
        if (const DraftingMapRoom *room = findRoom(roomName)) {
            edge = deriveEdge(*room, plug.anchor);
        }
        const std::string type = plug.type.empty() ? "door" : plug.type;
        writePlugRow(out, roomName, plugName, edge, type,
                     connectedPlugIds.count(plug.id) > 0);
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
    out << "connections[" << document.connections.size() << "]{from,to,type}:\n";
    for (const auto &connection : document.connections) {
        writeConnectionRow(out, plugNameById(connection.plugA),
                           plugNameById(connection.plugB), connection.type);
    }
    out << "\n";

    // Block instances: FLATTEN scattered each placement into N independent objects,
    // so re-form them by grouping on the BlockPlacementMetadata instanceId. The
    // placement centre is the union of the group's bounds; the room is whichever
    // footprint contains it (canvas units). scale/rotation are 1/0 (translate-only).
    struct Accum {
        std::string asset;
        edi::drafting::Bounds2D bounds;
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
            << "," << "1"  // translate-only placement: unit scale
            << "," << "0"  // and no rotation
            << "\n";
    }
    return out.str();
}

} // namespace edi::io
