#include "io/RoomSpecStore.h"

#include "drafting/DraftingRoom.h"

#include "EdiAssert.h"
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
        EDI_CHECK(parsed.ok);
        const auto &spec = parsed.spec;
        EDI_CHECK(nearlyEqual(spec.width, 0.6));
        EDI_CHECK(nearlyEqual(spec.height, 0.4));
        EDI_CHECK(nearlyEqual(spec.wallThickness, 0.02)); // 1 ft
        EDI_CHECK(spec.wallMaterial == "stone");
        EDI_CHECK(spec.openings.size() == 3);
        // N corridor, 10 ft wide -> 0.2 canvas, centered on the 0.6-wide N edge.
        EDI_CHECK(spec.openings[0].edge == RoomEdge::North);
        EDI_CHECK(spec.openings[0].type == "corridor");
        EDI_CHECK(nearlyEqual(spec.openings[0].width, 0.2));
        EDI_CHECK(nearlyEqual(spec.openings[0].center, 0.3)); // center of width 0.6
        // E secret at an explicit offset 12.5 ft -> 0.25 canvas from the edge start.
        EDI_CHECK(spec.openings[1].edge == RoomEdge::East);
        EDI_CHECK(nearlyEqual(spec.openings[1].center, 0.25));
        EDI_CHECK(nearlyEqual(spec.openings[1].width, 0.06)); // 3 ft
        // The parsed spec actually builds (geometry validation passes downstream).
        int n = 0;
        const auto plan = edi::drafting::planDraftingRoom(spec, [&n]() { return "r" + std::to_string(n++); });
        EDI_CHECK(plan.ok);
        EDI_CHECK(plan.objects.size() == 7); // 2 + 2 + 2 + 1
    }

    // Authoring directly in canvas units (canvasPerUnit = 1) leaves numbers as-is.
    {
        const RoomSpecParseResult parsed = parseRoomSpecToml("room.width = \"0.5\"\nroom.height = \"0.3\"\n", 1.0);
        EDI_CHECK(parsed.ok);
        EDI_CHECK(nearlyEqual(parsed.spec.width, 0.5) && nearlyEqual(parsed.spec.height, 0.3));
        EDI_CHECK(parsed.spec.openings.empty());
    }

    // Errors carry the offending key and never produce a half-spec.
    {
        EDI_CHECK(!parseRoomSpecToml("room.height = \"20\"\n", 1.0).ok); // missing width
        const RoomSpecParseResult badEdge = parseRoomSpecToml(
            "room.width = \"10\"\nroom.height = \"10\"\nroom.opening.0.edge = \"X\"\nroom.opening.0.width = \"2\"\n", 1.0);
        EDI_CHECK(!badEdge.ok);
        EDI_CHECK(badEdge.message.find("edge") != std::string::npos);
        const RoomSpecParseResult badWidth = parseRoomSpecToml(
            "room.width = \"10\"\nroom.height = \"10\"\nroom.opening.0.edge = \"N\"\nroom.opening.0.width = \"0\"\n", 1.0);
        EDI_CHECK(!badWidth.ok);
    }

    // Plugs parse as a second indexed list — same edge/at grammar, name required.
    {
        const RoomSpecParseResult parsed = parseRoomSpecToml(
            "room.width = \"10\"\nroom.height = \"8\"\n"
            "room.plug.0.edge = \"N\"\nroom.plug.0.name = \"north_door\"\nroom.plug.0.type = \"door\"\nroom.plug.0.at = \"3\"\n"
            "room.plug.1.edge = \"E\"\nroom.plug.1.name = \"east_portal\"\n", // type + at default
            1.0);
        EDI_CHECK(parsed.ok);
        EDI_CHECK(parsed.spec.plugs.size() == 2);
        EDI_CHECK(parsed.spec.plugs[0].name == "north_door");
        EDI_CHECK(parsed.spec.plugs[0].type == "door");
        EDI_CHECK(parsed.spec.plugs[0].edge == RoomEdge::North);
        EDI_CHECK(nearlyEqual(parsed.spec.plugs[0].at, 3.0)); // canvasPerUnit 1.0
        EDI_CHECK(parsed.spec.plugs[1].name == "east_portal");
        EDI_CHECK(parsed.spec.plugs[1].type == "door");          // default
        EDI_CHECK(nearlyEqual(parsed.spec.plugs[1].at, 4.0));    // default center of E (height/2)
        EDI_CHECK(parsed.spec.plugs[0].flags.empty());           // flags absent -> empty
    }

    // Plug flags: a neutral comma-separated open vocabulary. Whitespace around each
    // token is trimmed and empty tokens are dropped; absent flags stay empty.
    {
        const RoomSpecParseResult parsed = parseRoomSpecToml(
            "room.width = \"10\"\nroom.height = \"8\"\n"
            "room.plug.0.edge = \"N\"\nroom.plug.0.name = \"win\"\nroom.plug.0.flags = \"window, passes_light ,, lit\"\n"
            "room.plug.1.edge = \"S\"\nroom.plug.1.name = \"plain\"\n", // no flags key
            1.0);
        EDI_CHECK(parsed.ok);
        EDI_CHECK(parsed.spec.plugs.size() == 2);
        const std::vector<std::string> expected{"window", "passes_light", "lit"};
        EDI_CHECK(parsed.spec.plugs[0].flags == expected); // trimmed, empty token dropped
        EDI_CHECK(parsed.spec.plugs[1].flags.empty());     // absent -> empty
    }

    // Interior features parse as a contiguous indexed list keyed on `.type`. x/y
    // stay in AUTHORED FEET (NOT scaled to canvas), room-local from the NW origin;
    // contiguous indexing stops at the first gap; absent feature keys -> empty.
    {
        const RoomSpecParseResult parsed = parseRoomSpecToml(
            "room.width = \"10\"\nroom.height = \"8\"\n"
            "room.feature.0.x = \"3\"\nroom.feature.0.y = \"4\"\nroom.feature.0.type = \"rubble\"\nroom.feature.0.name = \"cave_in\"\n"
            "room.feature.1.x = \"5\"\nroom.feature.1.type = \"statue\"\n" // y + name default
            "room.feature.3.type = \"orphan\"\n",                          // index 2 absent -> 3 not reached
            0.02); // canvasPerUnit 0.02: proves x/y are NOT scaled by it
        EDI_CHECK(parsed.ok);
        EDI_CHECK(parsed.spec.features.size() == 2); // stops at the gap before index 3
        EDI_CHECK(nearlyEqual(parsed.spec.features[0].x, 3.0)); // authored feet, unscaled
        EDI_CHECK(nearlyEqual(parsed.spec.features[0].y, 4.0));
        EDI_CHECK(parsed.spec.features[0].type == "rubble");
        EDI_CHECK(parsed.spec.features[0].name == "cave_in");
        EDI_CHECK(nearlyEqual(parsed.spec.features[1].x, 5.0));
        EDI_CHECK(nearlyEqual(parsed.spec.features[1].y, 0.0)); // default
        EDI_CHECK(parsed.spec.features[1].type == "statue");
        EDI_CHECK(parsed.spec.features[1].name.empty());        // default
    }

    // A room with no feature keys -> empty features (behavior unchanged).
    {
        const RoomSpecParseResult parsed = parseRoomSpecToml("room.width = \"10\"\nroom.height = \"8\"\n", 1.0);
        EDI_CHECK(parsed.ok);
        EDI_CHECK(parsed.spec.features.empty());
    }

    // A plug without a name is rejected — it would be unreferenceable.
    {
        const RoomSpecParseResult noName = parseRoomSpecToml(
            "room.width = \"10\"\nroom.height = \"8\"\nroom.plug.0.edge = \"N\"\n", 1.0);
        EDI_CHECK(!noName.ok);
        EDI_CHECK(noName.message.find("name") != std::string::npos);
    }

    // Connections parse at map level, referencing plugs by name.
    {
        const RoomSpecParseResult parsed = parseRoomSpecToml(
            "room.width = \"10\"\nroom.height = \"8\"\n"
            "room.plug.0.edge = \"N\"\nroom.plug.0.name = \"a\"\n"
            "room.plug.1.edge = \"S\"\nroom.plug.1.name = \"b\"\n"
            "map.connection.0.from = \"a\"\nmap.connection.0.to = \"b\"\nmap.connection.0.type = \"corridor\"\n",
            1.0);
        EDI_CHECK(parsed.ok);
        EDI_CHECK(parsed.spec.connections.size() == 1);
        EDI_CHECK(parsed.spec.connections[0].from == "a" && parsed.spec.connections[0].to == "b");
        EDI_CHECK(parsed.spec.connections[0].type == "corridor");
    }

    // A connection to an unknown plug name is rejected (not silently dropped), and
    // the offending name is named.
    {
        const RoomSpecParseResult bad = parseRoomSpecToml(
            "room.width = \"10\"\nroom.height = \"8\"\n"
            "room.plug.0.edge = \"N\"\nroom.plug.0.name = \"a\"\n"
            "map.connection.0.from = \"a\"\nmap.connection.0.to = \"ghost\"\n",
            1.0);
        EDI_CHECK(!bad.ok);
        EDI_CHECK(bad.message.find("ghost") != std::string::npos);
    }

    // Duplicate plug names are rejected (a connection could not resolve them).
    {
        const RoomSpecParseResult dup = parseRoomSpecToml(
            "room.width = \"10\"\nroom.height = \"8\"\n"
            "room.plug.0.edge = \"N\"\nroom.plug.0.name = \"a\"\n"
            "room.plug.1.edge = \"S\"\nroom.plug.1.name = \"a\"\n",
            1.0);
        EDI_CHECK(!dup.ok);
        EDI_CHECK(dup.message.find("duplicat") != std::string::npos);
    }

    return 0;
}
