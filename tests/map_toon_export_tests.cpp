// Seam B export (Phase D / D1): exportMapToToon projects a typed MapSpec to the
// TOON map document the game engine reads. This pins the exact wire format —
// three tabular arrays, rooms keyed by name, coordinate cells quoted, the derived
// `connected` flag, and the empty-type -> "door" default — so the engine's reader
// can rely on it (a golden contract).
#include "drafting/DraftingRoom.h"
#include "io/MapToonExport.h"

#include <cassert>
#include <string>

using namespace edi::drafting;

int main()
{
    // Two rooms, a normal door, a secret dead-end (no connection), one corridor.
    MapSpec spec;

    NamedRoomSpec a;
    a.name = "a";
    a.spec.origin = {1.0, 2.0};
    a.spec.width = 10.0;
    a.spec.height = 5.0;
    a.spec.wallMaterial = "stone";
    a.spec.plugs.push_back(RoomPlugSpec{RoomEdge::North, 0.0, "door1", ""});       // type defaults -> door
    a.spec.plugs.push_back(RoomPlugSpec{RoomEdge::South, 0.0, "secret1", "secret"}); // dead-end
    spec.rooms.push_back(a);

    NamedRoomSpec b;
    b.name = "b";
    b.spec.origin = {1.0, 10.0};
    b.spec.width = 8.0;
    b.spec.height = 6.0;
    b.spec.wallMaterial = "wood";
    b.spec.plugs.push_back(RoomPlugSpec{RoomEdge::North, 0.0, "in", ""});
    spec.rooms.push_back(b);

    MapConnectionSpec corridor;
    corridor.from = {"a", "door1"};
    corridor.to = {"b", "in"};
    corridor.type = "corridor";
    spec.connections.push_back(corridor);

    const std::string toon = edi::io::exportMapToToon(spec, "test");

    const std::string expected =
        "kind: map\n"
        "title: test\n"
        "units: feet\n"
        "\n"
        "rooms[2]{name,origin,size,material}:\n"
        "  a,\"1,2\",\"10,5\",stone\n"
        "  b,\"1,10\",\"8,6\",wood\n"
        "\n"
        "plugs[3]{room,name,edge,type,connected}:\n"
        "  a,door1,N,door,true\n"
        "  a,secret1,S,secret,false\n"
        "  b,in,N,door,true\n"
        "\n"
        "connections[1]{from,to,type}:\n"
        "  a.door1,b.in,corridor\n";

    assert(toon == expected);

    // Title omitted -> no title line; units still present.
    const std::string noTitle = edi::io::exportMapToToon(spec);
    assert(noTitle.find("title:") == std::string::npos);
    assert(noTitle.find("kind: map\nunits: feet\n") == 0);

    // A coordinate cell with a comma is quoted; a bare token is not.
    assert(toon.find("\"1,2\"") != std::string::npos);
    assert(toon.find(",stone\n") != std::string::npos);

    return 0;
}
