#include "io/RoomSpecStore.h"

#include "drafting/DraftingRoom.h"

#include <cassert>
#include <cmath>
#include <string>
#include <vector>

using namespace edi::io;
using edi::drafting::RoomEdge;

namespace {

bool nearlyEqual(double a, double b, double eps = 0.000001)
{
    return std::abs(a - b) <= eps;
}

} // namespace

int main()
{
    // The guard antechamber, authored in feet in the flat-TOML dialect: dotted
    // keys, openings as an indexed list (the recipe op-stream convention).
    // This dialect stores every value as a quoted string (numbers included); the
    // typed reads parse them — same convention as edi.toml / recipe files.
    const std::string toml =
        "# guard antechamber\n"
        "room.width = \"30\"\n"
        "room.height = \"20\"\n"
        "room.origin_x = \"0\"\n"
        "room.origin_y = \"0\"\n"
        "room.wall_material = \"stone\"\n"
        "room.wall_thickness = \"1\"\n"
        "room.opening.0.edge = \"N\"\n"
        "room.opening.0.type = \"corridor\"\n"
        "room.opening.0.width = \"10\"\n"
        "room.opening.0.at = \"center\"\n"
        "room.opening.1.edge = \"E\"\n"
        "room.opening.1.type = \"secret\"\n"
        "room.opening.1.width = \"3\"\n"
        "room.opening.1.at = \"12.5\"\n"
        "room.opening.2.edge = \"S\"\n"
        "room.opening.2.type = \"door\"\n"
        "room.opening.2.width = \"5\"\n"
        "room.opening.2.at = \"center\"\n";

    // 0.02 canvas per foot: 30 ft -> 0.6 canvas, 20 ft -> 0.4.
    {
        const RoomSpecParseResult parsed = parseRoomSpecToml(toml, 0.02);
        assert(parsed.ok);
        const auto &spec = parsed.spec;
        assert(nearlyEqual(spec.width, 0.6));
        assert(nearlyEqual(spec.height, 0.4));
        assert(nearlyEqual(spec.wallThickness, 0.02)); // 1 ft
        assert(spec.wallMaterial == "stone");
        assert(spec.openings.size() == 3);
        // N corridor, 10 ft wide -> 0.2 canvas, centered on the 0.6-wide N edge.
        assert(spec.openings[0].edge == RoomEdge::North);
        assert(spec.openings[0].type == "corridor");
        assert(nearlyEqual(spec.openings[0].width, 0.2));
        assert(nearlyEqual(spec.openings[0].center, 0.3)); // center of width 0.6
        // E secret at an explicit offset 12.5 ft -> 0.25 canvas from the edge start.
        assert(spec.openings[1].edge == RoomEdge::East);
        assert(nearlyEqual(spec.openings[1].center, 0.25));
        assert(nearlyEqual(spec.openings[1].width, 0.06)); // 3 ft
        // The parsed spec actually builds (geometry validation passes downstream).
        int n = 0;
        const auto plan = edi::drafting::planDraftingRoom(spec, [&n]() { return "r" + std::to_string(n++); });
        assert(plan.ok);
        assert(plan.objects.size() == 7); // 2 + 2 + 2 + 1
    }

    // Authoring directly in canvas units (canvasPerUnit = 1) leaves numbers as-is.
    {
        const RoomSpecParseResult parsed = parseRoomSpecToml("room.width = \"0.5\"\nroom.height = \"0.3\"\n", 1.0);
        assert(parsed.ok);
        assert(nearlyEqual(parsed.spec.width, 0.5) && nearlyEqual(parsed.spec.height, 0.3));
        assert(parsed.spec.openings.empty());
    }

    // Errors carry the offending key and never produce a half-spec.
    {
        assert(!parseRoomSpecToml("room.height = \"20\"\n", 1.0).ok); // missing width
        const RoomSpecParseResult badEdge = parseRoomSpecToml(
            "room.width = \"10\"\nroom.height = \"10\"\nroom.opening.0.edge = \"X\"\nroom.opening.0.width = \"2\"\n", 1.0);
        assert(!badEdge.ok);
        assert(badEdge.message.find("edge") != std::string::npos);
        const RoomSpecParseResult badWidth = parseRoomSpecToml(
            "room.width = \"10\"\nroom.height = \"10\"\nroom.opening.0.edge = \"N\"\nroom.opening.0.width = \"0\"\n", 1.0);
        assert(!badWidth.ok);
    }

    // Plugs parse as a second indexed list — same edge/at grammar, name required.
    {
        const RoomSpecParseResult parsed = parseRoomSpecToml(
            "room.width = \"10\"\nroom.height = \"8\"\n"
            "room.plug.0.edge = \"N\"\nroom.plug.0.name = \"north_door\"\nroom.plug.0.type = \"door\"\nroom.plug.0.at = \"3\"\n"
            "room.plug.1.edge = \"E\"\nroom.plug.1.name = \"east_portal\"\n", // type + at default
            1.0);
        assert(parsed.ok);
        assert(parsed.spec.plugs.size() == 2);
        assert(parsed.spec.plugs[0].name == "north_door");
        assert(parsed.spec.plugs[0].type == "door");
        assert(parsed.spec.plugs[0].edge == RoomEdge::North);
        assert(nearlyEqual(parsed.spec.plugs[0].at, 3.0)); // canvasPerUnit 1.0
        assert(parsed.spec.plugs[1].name == "east_portal");
        assert(parsed.spec.plugs[1].type == "door");          // default
        assert(nearlyEqual(parsed.spec.plugs[1].at, 4.0));    // default center of E (height/2)
        assert(parsed.spec.plugs[0].flags.empty());           // flags absent -> empty
    }

    // Plug flags: a neutral comma-separated open vocabulary. Whitespace around each
    // token is trimmed and empty tokens are dropped; absent flags stay empty.
    {
        const RoomSpecParseResult parsed = parseRoomSpecToml(
            "room.width = \"10\"\nroom.height = \"8\"\n"
            "room.plug.0.edge = \"N\"\nroom.plug.0.name = \"win\"\nroom.plug.0.flags = \"window, passes_light ,, lit\"\n"
            "room.plug.1.edge = \"S\"\nroom.plug.1.name = \"plain\"\n", // no flags key
            1.0);
        assert(parsed.ok);
        assert(parsed.spec.plugs.size() == 2);
        const std::vector<std::string> expected{"window", "passes_light", "lit"};
        assert(parsed.spec.plugs[0].flags == expected); // trimmed, empty token dropped
        assert(parsed.spec.plugs[1].flags.empty());     // absent -> empty
    }

    // A plug without a name is rejected — it would be unreferenceable.
    {
        const RoomSpecParseResult noName = parseRoomSpecToml(
            "room.width = \"10\"\nroom.height = \"8\"\nroom.plug.0.edge = \"N\"\n", 1.0);
        assert(!noName.ok);
        assert(noName.message.find("name") != std::string::npos);
    }

    // Connections parse at map level, referencing plugs by name.
    {
        const RoomSpecParseResult parsed = parseRoomSpecToml(
            "room.width = \"10\"\nroom.height = \"8\"\n"
            "room.plug.0.edge = \"N\"\nroom.plug.0.name = \"a\"\n"
            "room.plug.1.edge = \"S\"\nroom.plug.1.name = \"b\"\n"
            "map.connection.0.from = \"a\"\nmap.connection.0.to = \"b\"\nmap.connection.0.type = \"corridor\"\n",
            1.0);
        assert(parsed.ok);
        assert(parsed.spec.connections.size() == 1);
        assert(parsed.spec.connections[0].from == "a" && parsed.spec.connections[0].to == "b");
        assert(parsed.spec.connections[0].type == "corridor");
    }

    // A connection to an unknown plug name is rejected (not silently dropped), and
    // the offending name is named.
    {
        const RoomSpecParseResult bad = parseRoomSpecToml(
            "room.width = \"10\"\nroom.height = \"8\"\n"
            "room.plug.0.edge = \"N\"\nroom.plug.0.name = \"a\"\n"
            "map.connection.0.from = \"a\"\nmap.connection.0.to = \"ghost\"\n",
            1.0);
        assert(!bad.ok);
        assert(bad.message.find("ghost") != std::string::npos);
    }

    // Duplicate plug names are rejected (a connection could not resolve them).
    {
        const RoomSpecParseResult dup = parseRoomSpecToml(
            "room.width = \"10\"\nroom.height = \"8\"\n"
            "room.plug.0.edge = \"N\"\nroom.plug.0.name = \"a\"\n"
            "room.plug.1.edge = \"S\"\nroom.plug.1.name = \"a\"\n",
            1.0);
        assert(!dup.ok);
        assert(dup.message.find("duplicat") != std::string::npos);
    }

    return 0;
}
