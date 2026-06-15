#include "io/MapToonExport.h"

#include <cstdio>
#include <set>
#include <sstream>
#include <string>

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
    out << "kind: map\n";
    if (!title.empty()) {
        out << "title: " << cell(title) << "\n";
    }
    if (!units.empty()) {
        out << "units: " << cell(units) << "\n";
    }
    out << "\n";

    out << "rooms[" << spec.rooms.size() << "]{name,origin,size,material}:\n";
    for (const auto &room : spec.rooms) {
        const auto &rs = room.spec;
        out << "  " << cell(room.name)
            << "," << cell(num(rs.origin.x) + "," + num(rs.origin.y))
            << "," << cell(num(rs.width) + "," + num(rs.height))
            << "," << cell(rs.wallMaterial)
            << "\n";
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
            out << "  " << cell(room.name)
                << "," << cell(plug.name)
                << "," << edgeName(plug.edge)
                << "," << cell(type)
                << "," << (connected.count(plugKey(room.name, plug.name)) > 0 ? "true" : "false")
                << "\n";
        }
    }
    out << "\n";

    out << "connections[" << spec.connections.size() << "]{from,to,type}:\n";
    for (const auto &connection : spec.connections) {
        out << "  " << cell(plugKey(connection.from.roomName, connection.from.plugName))
            << "," << cell(plugKey(connection.to.roomName, connection.to.plugName))
            << "," << cell(connection.type)
            << "\n";
    }
    return out.str();
}

} // namespace edi::io
