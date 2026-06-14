#include "io/RoomSpecStore.h"

#include "drafting/DraftingRoom.h"

#include <cassert>
#include <cmath>
#include <string>

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

    return 0;
}
