// Multi-room map authoring (parser). parseMapSpecToml reads the .map.toml dialect
// — many named rooms (same per-room grammar as a single room, under map.room.<i>)
// plus cross-room connections referencing a plug as "room.plug" — and validates
// it fully in the pure parser before any geometry exists.
#include "io/RoomSpecStore.h"

#include "drafting/DraftingRoom.h"

#include <cassert>
#include <string>

using namespace edi::io;

int main()
{
    // Happy path: two rooms side by side, one cross-room connection that resolves.
    {
        const MapSpecParseResult parsed = parseMapSpecToml(
            "map.room.0.name = \"a\"\nmap.room.0.width = \"10\"\nmap.room.0.height = \"10\"\n"
            "map.room.0.plug.0.edge = \"E\"\nmap.room.0.plug.0.name = \"e\"\n"
            "map.room.1.name = \"b\"\nmap.room.1.width = \"10\"\nmap.room.1.height = \"10\"\nmap.room.1.origin_x = \"20\"\n"
            "map.room.1.plug.0.edge = \"W\"\nmap.room.1.plug.0.name = \"w\"\n"
            "map.connection.0.from = \"a.e\"\nmap.connection.0.to = \"b.w\"\nmap.connection.0.type = \"corridor\"\n",
            1.0);
        assert(parsed.ok);
        assert(parsed.spec.rooms.size() == 2);
        assert(parsed.spec.rooms[0].name == "a" && parsed.spec.rooms[1].name == "b");
        assert(parsed.spec.connections.size() == 1);
        assert(parsed.spec.connections[0].from.roomName == "a" && parsed.spec.connections[0].from.plugName == "e");
        assert(parsed.spec.connections[0].to.roomName == "b" && parsed.spec.connections[0].to.plugName == "w");
        assert(parsed.spec.connections[0].type == "corridor");
    }

    // A 3-way junction is a junction ROOM with three distinct plugs and THREE
    // separate 2-plug connections — a connection never fans to three rooms.
    {
        const MapSpecParseResult parsed = parseMapSpecToml(
            "map.room.0.name = \"j\"\nmap.room.0.width = \"10\"\nmap.room.0.height = \"10\"\nmap.room.0.origin_x = \"50\"\nmap.room.0.origin_y = \"50\"\n"
            "map.room.0.plug.0.edge = \"W\"\nmap.room.0.plug.0.name = \"to_a\"\n"
            "map.room.0.plug.1.edge = \"E\"\nmap.room.0.plug.1.name = \"to_b\"\n"
            "map.room.0.plug.2.edge = \"S\"\nmap.room.0.plug.2.name = \"to_c\"\n"
            "map.room.1.name = \"a\"\nmap.room.1.width = \"10\"\nmap.room.1.height = \"10\"\nmap.room.1.origin_x = \"10\"\nmap.room.1.origin_y = \"50\"\nmap.room.1.plug.0.edge = \"E\"\nmap.room.1.plug.0.name = \"p\"\n"
            "map.room.2.name = \"b\"\nmap.room.2.width = \"10\"\nmap.room.2.height = \"10\"\nmap.room.2.origin_x = \"90\"\nmap.room.2.origin_y = \"50\"\nmap.room.2.plug.0.edge = \"W\"\nmap.room.2.plug.0.name = \"p\"\n"
            "map.room.3.name = \"c\"\nmap.room.3.width = \"10\"\nmap.room.3.height = \"10\"\nmap.room.3.origin_x = \"50\"\nmap.room.3.origin_y = \"90\"\nmap.room.3.plug.0.edge = \"N\"\nmap.room.3.plug.0.name = \"p\"\n"
            "map.connection.0.from = \"j.to_a\"\nmap.connection.0.to = \"a.p\"\n"
            "map.connection.1.from = \"j.to_b\"\nmap.connection.1.to = \"b.p\"\n"
            "map.connection.2.from = \"j.to_c\"\nmap.connection.2.to = \"c.p\"\n",
            1.0);
        assert(parsed.ok);
        assert(parsed.spec.rooms.size() == 4);
        assert(parsed.spec.connections.size() == 3);
        // All three edges leave the junction by its three DISTINCT plugs.
        assert(parsed.spec.connections[0].from.roomName == "j" && parsed.spec.connections[0].from.plugName == "to_a");
        assert(parsed.spec.connections[1].from.plugName == "to_b");
        assert(parsed.spec.connections[2].from.plugName == "to_c");
    }

    // Two rooms may share a plug NAME — names are room-scoped, so this is accepted
    // (the S1 refactor must give each room a FRESH name set, not a shared one).
    {
        const MapSpecParseResult parsed = parseMapSpecToml(
            "map.room.0.name = \"a\"\nmap.room.0.width = \"10\"\nmap.room.0.height = \"10\"\nmap.room.0.plug.0.edge = \"E\"\nmap.room.0.plug.0.name = \"door\"\n"
            "map.room.1.name = \"b\"\nmap.room.1.width = \"10\"\nmap.room.1.height = \"10\"\nmap.room.1.origin_x = \"20\"\nmap.room.1.plug.0.edge = \"W\"\nmap.room.1.plug.0.name = \"door\"\n"
            "map.connection.0.from = \"a.door\"\nmap.connection.0.to = \"b.door\"\n",
            1.0);
        assert(parsed.ok);
        assert(parsed.spec.connections.size() == 1);
    }

    // A dead-end plug (declared, referenced by no connection) is allowed — that is
    // how a secret-door stub is authored.
    {
        const MapSpecParseResult parsed = parseMapSpecToml(
            "map.room.0.name = \"vault\"\nmap.room.0.width = \"10\"\nmap.room.0.height = \"10\"\n"
            "map.room.0.plug.0.edge = \"S\"\nmap.room.0.plug.0.name = \"secret\"\nmap.room.0.plug.0.type = \"secret\"\n",
            1.0);
        assert(parsed.ok);
        assert(parsed.spec.rooms.size() == 1);
        assert(parsed.spec.rooms[0].spec.plugs.size() == 1);
        assert(parsed.spec.connections.empty());
    }

    // --- rejections, each naming the offending key ---------------------------

    auto rejects = [](const std::string &toml, const std::string &needle) {
        const MapSpecParseResult r = parseMapSpecToml(toml, 1.0);
        assert(!r.ok);
        assert(r.message.find(needle) != std::string::npos);
    };

    // zero rooms.
    rejects("map.connection.0.from = \"a.b\"\n", "at least one room");
    // duplicate room name.
    rejects("map.room.0.name = \"a\"\nmap.room.0.width = \"10\"\nmap.room.0.height = \"10\"\n"
            "map.room.1.name = \"a\"\nmap.room.1.width = \"10\"\nmap.room.1.height = \"10\"\nmap.room.1.origin_y = \"40\"\n",
            "duplicated");
    // duplicate plug name within one room.
    rejects("map.room.0.name = \"a\"\nmap.room.0.width = \"10\"\nmap.room.0.height = \"10\"\n"
            "map.room.0.plug.0.edge = \"N\"\nmap.room.0.plug.0.name = \"x\"\n"
            "map.room.0.plug.1.edge = \"S\"\nmap.room.0.plug.1.name = \"x\"\n",
            "duplicated");
    // overlapping footprints.
    rejects("map.room.0.name = \"a\"\nmap.room.0.width = \"30\"\nmap.room.0.height = \"30\"\n"
            "map.room.1.name = \"b\"\nmap.room.1.width = \"30\"\nmap.room.1.height = \"30\"\nmap.room.1.origin_x = \"10\"\nmap.room.1.origin_y = \"10\"\n",
            "overlap");
    // connection to an unknown room.
    rejects("map.room.0.name = \"a\"\nmap.room.0.width = \"10\"\nmap.room.0.height = \"10\"\nmap.room.0.plug.0.edge = \"E\"\nmap.room.0.plug.0.name = \"e\"\n"
            "map.connection.0.from = \"a.e\"\nmap.connection.0.to = \"ghost.w\"\n",
            "unknown room");
    // connection to an unknown plug.
    rejects("map.room.0.name = \"a\"\nmap.room.0.width = \"10\"\nmap.room.0.height = \"10\"\nmap.room.0.plug.0.edge = \"E\"\nmap.room.0.plug.0.name = \"e\"\n"
            "map.connection.0.from = \"a.e\"\nmap.connection.0.to = \"a.ghost\"\n",
            "unknown plug");
    // malformed ref (no dot).
    rejects("map.room.0.name = \"a\"\nmap.room.0.width = \"10\"\nmap.room.0.height = \"10\"\nmap.room.0.plug.0.edge = \"E\"\nmap.room.0.plug.0.name = \"e\"\n"
            "map.connection.0.from = \"ae\"\nmap.connection.0.to = \"a.e\"\n",
            "must be 'room.plug'");
    // self-loop (plug joined to itself).
    rejects("map.room.0.name = \"a\"\nmap.room.0.width = \"10\"\nmap.room.0.height = \"10\"\nmap.room.0.plug.0.edge = \"E\"\nmap.room.0.plug.0.name = \"e\"\n"
            "map.connection.0.from = \"a.e\"\nmap.connection.0.to = \"a.e\"\n",
            "itself");

    return 0;
}
