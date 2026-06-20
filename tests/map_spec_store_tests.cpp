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

    // --- M1/M2: marker id+metadata, patrols, connection lock, prop blocks ----
    // Round-trip the new neutral fields from the .map.toml dialect into the structs.
    {
        const MapSpecParseResult parsed = parseMapSpecToml(
            // room a: a pickup marker id="gold_key", plus an npc with metadata patrol=loop.
            "map.room.0.name = \"a\"\nmap.room.0.width = \"20\"\nmap.room.0.height = \"20\"\n"
            "map.room.0.plug.0.edge = \"E\"\nmap.room.0.plug.0.name = \"e\"\n"
            "map.room.0.feature.0.type = \"pickup\"\nmap.room.0.feature.0.id = \"gold_key\"\n"
            "map.room.0.feature.0.x = \"5\"\nmap.room.0.feature.0.y = \"5\"\n"
            "map.room.0.feature.1.type = \"npc\"\nmap.room.0.feature.1.id = \"guard\"\n"
            "map.room.0.feature.1.meta.0.key = \"patrol\"\nmap.room.0.feature.1.meta.0.value = \"loop\"\n"
            // room b: a chest whose lock rides metadata key_id=gold_key.
            "map.room.1.name = \"b\"\nmap.room.1.width = \"20\"\nmap.room.1.height = \"20\"\nmap.room.1.origin_x = \"40\"\n"
            "map.room.1.plug.0.edge = \"W\"\nmap.room.1.plug.0.name = \"w\"\n"
            "map.room.1.feature.0.type = \"chest\"\nmap.room.1.feature.0.id = \"treasure\"\n"
            "map.room.1.feature.0.meta.0.key = \"key_id\"\nmap.room.1.feature.0.meta.0.value = \"gold_key\"\n"
            // a LOCKED connection keyed to gold_key.
            "map.connection.0.from = \"a.e\"\nmap.connection.0.to = \"b.w\"\nmap.connection.0.type = \"corridor\"\n"
            "map.connection.0.locked = \"true\"\nmap.connection.0.key_id = \"gold_key\"\n"
            // a closed patrol loop of 4 waypoints + a prop block.
            "map.patrol.0.id = \"loop\"\nmap.patrol.0.closed = \"true\"\n"
            "map.patrol.0.point.0.x = \"1\"\nmap.patrol.0.point.0.y = \"1\"\n"
            "map.patrol.0.point.1.x = \"9\"\nmap.patrol.0.point.1.y = \"1\"\n"
            "map.patrol.0.point.2.x = \"9\"\nmap.patrol.0.point.2.y = \"9\"\n"
            "map.patrol.0.point.3.x = \"1\"\nmap.patrol.0.point.3.y = \"9\"\n"
            "map.block.0.asset_ref = \"dungeon.barrel\"\nmap.block.0.x = \"3\"\nmap.block.0.y = \"4\"\n",
            2.0); // canvasPerUnit = 2.0 to prove waypoint scaling (feet*2) vs feature/block (unscaled)
        assert(parsed.ok);
        // feature id + metadata round-trip (features stay in authored feet).
        assert(parsed.spec.rooms[0].spec.features[0].id == "gold_key");
        assert(parsed.spec.rooms[0].spec.features[0].x == 5.0); // NOT scaled
        assert(parsed.spec.rooms[0].spec.features[1].metadata.size() == 1);
        assert(parsed.spec.rooms[0].spec.features[1].metadata[0].first == "patrol");
        assert(parsed.spec.rooms[0].spec.features[1].metadata[0].second == "loop");
        assert(parsed.spec.rooms[1].spec.features[0].metadata[0].first == "key_id");
        assert(parsed.spec.rooms[1].spec.features[0].metadata[0].second == "gold_key");
        // connection lock tags round-trip.
        assert(parsed.spec.connections[0].locked);
        assert(parsed.spec.connections[0].keyId == "gold_key");
        // patrol round-trips: 4 waypoints, closed, SCALED to canvas (feet * 2).
        assert(parsed.spec.patrols.size() == 1);
        assert(parsed.spec.patrols[0].id == "loop");
        assert(parsed.spec.patrols[0].closed);
        assert(parsed.spec.patrols[0].waypoints.size() == 4);
        assert(parsed.spec.patrols[0].waypoints[1].x == 18.0); // 9 ft * 2.0 canvasPerUnit
        // prop block round-trips: position stays AUTHORED FEET (the controller scales it).
        assert(parsed.spec.blocks.size() == 1);
        assert(parsed.spec.blocks[0].assetRef == "dungeon.barrel");
        assert(parsed.spec.blocks[0].position.x == 3.0); // NOT scaled
    }

    // closed defaults to true; an explicit "false" opens the loop.
    {
        const MapSpecParseResult parsed = parseMapSpecToml(
            "map.room.0.name = \"a\"\nmap.room.0.width = \"10\"\nmap.room.0.height = \"10\"\n"
            "map.patrol.0.id = \"open\"\nmap.patrol.0.closed = \"false\"\n"
            "map.patrol.0.point.0.x = \"1\"\nmap.patrol.0.point.0.y = \"1\"\n"
            "map.patrol.0.point.1.x = \"2\"\nmap.patrol.0.point.1.y = \"2\"\n",
            1.0);
        assert(parsed.ok);
        assert(!parsed.spec.patrols[0].closed);
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

    // --- M2 referential-integrity rejections (guardrail #4) ------------------
    // These are AUTHORED-ID checks, NOT a lock rule — edi enforces no gate.

    // duplicate marker id across rooms.
    rejects("map.room.0.name = \"a\"\nmap.room.0.width = \"10\"\nmap.room.0.height = \"10\"\n"
            "map.room.0.feature.0.type = \"pickup\"\nmap.room.0.feature.0.id = \"dup\"\n"
            "map.room.1.name = \"b\"\nmap.room.1.width = \"10\"\nmap.room.1.height = \"10\"\nmap.room.1.origin_x = \"20\"\n"
            "map.room.1.feature.0.type = \"pickup\"\nmap.room.1.feature.0.id = \"dup\"\n",
            "marker id 'dup' is duplicated");

    // duplicate patrol id.
    rejects("map.room.0.name = \"a\"\nmap.room.0.width = \"10\"\nmap.room.0.height = \"10\"\n"
            "map.patrol.0.id = \"p\"\nmap.patrol.0.point.0.x = \"1\"\nmap.patrol.0.point.0.y = \"1\"\nmap.patrol.0.point.1.x = \"2\"\nmap.patrol.0.point.1.y = \"2\"\n"
            "map.patrol.1.id = \"p\"\nmap.patrol.1.point.0.x = \"1\"\nmap.patrol.1.point.0.y = \"1\"\nmap.patrol.1.point.1.x = \"2\"\nmap.patrol.1.point.1.y = \"2\"\n",
            "patrol id 'p' is duplicated");

    // npc metadata patrol=<id> references a missing patrol.
    rejects("map.room.0.name = \"a\"\nmap.room.0.width = \"10\"\nmap.room.0.height = \"10\"\n"
            "map.room.0.feature.0.type = \"npc\"\nmap.room.0.feature.0.id = \"g\"\n"
            "map.room.0.feature.0.meta.0.key = \"patrol\"\nmap.room.0.feature.0.meta.0.value = \"ghost\"\n",
            "unknown patrol 'ghost'");

    // connection key_id with no matching pickup id.
    rejects("map.room.0.name = \"a\"\nmap.room.0.width = \"10\"\nmap.room.0.height = \"10\"\nmap.room.0.plug.0.edge = \"E\"\nmap.room.0.plug.0.name = \"e\"\n"
            "map.room.1.name = \"b\"\nmap.room.1.width = \"10\"\nmap.room.1.height = \"10\"\nmap.room.1.origin_x = \"20\"\nmap.room.1.plug.0.edge = \"W\"\nmap.room.1.plug.0.name = \"w\"\n"
            "map.connection.0.from = \"a.e\"\nmap.connection.0.to = \"b.w\"\nmap.connection.0.key_id = \"nokey\"\n",
            "no matching pickup id");

    // chest marker key_id with no matching pickup id.
    rejects("map.room.0.name = \"a\"\nmap.room.0.width = \"10\"\nmap.room.0.height = \"10\"\n"
            "map.room.0.feature.0.type = \"chest\"\nmap.room.0.feature.0.id = \"c\"\n"
            "map.room.0.feature.0.meta.0.key = \"key_id\"\nmap.room.0.feature.0.meta.0.value = \"missing\"\n",
            "no matching pickup id");

    // a key_id that DOES match a pickup id is accepted (the happy referential case).
    {
        const MapSpecParseResult ok = parseMapSpecToml(
            "map.room.0.name = \"a\"\nmap.room.0.width = \"10\"\nmap.room.0.height = \"10\"\n"
            "map.room.0.feature.0.type = \"pickup\"\nmap.room.0.feature.0.id = \"k\"\n"
            "map.room.0.feature.1.type = \"chest\"\nmap.room.0.feature.1.id = \"c\"\n"
            "map.room.0.feature.1.meta.0.key = \"key_id\"\nmap.room.0.feature.1.meta.0.value = \"k\"\n",
            1.0);
        assert(ok.ok);
    }

    return 0;
}
