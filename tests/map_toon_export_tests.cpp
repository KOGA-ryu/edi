// Seam B export (Phase D / D1): exportMapToToon projects a typed MapSpec to the
// TOON map document the game engine reads. This pins the exact wire format —
// three tabular arrays, rooms keyed by name, coordinate cells quoted, the derived
// `connected` flag, and the empty-type -> "door" default — so the engine's reader
// can rely on it (a golden contract).
#include "drafting/DraftingDocument.h"
#include "drafting/DraftingGeometry.h"
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

    // Seam C: the DOCUMENT-based export — same three arrays PLUS placed block
    // instances, with canvas coordinates converted to authored units via the stored
    // scale, plug edges DERIVED from the footprint, and connections resolved by id.
    {
        DraftingDocument doc = makeDraftingDocument("doc");
        doc.canvasPerAuthoredUnit = 0.5; // 0.5 canvas per foot -> divide to recover feet
        doc.rooms.push_back(DraftingMapRoom{"a", {1.0, 2.0}, 4.0, 6.0, "stone"});

        DraftingPlug door;
        door.id = "plug_0001";
        door.name = "a.door";
        door.anchor = {3.0, 2.0}; // on the N edge (y == origin.y)
        DraftingPlug exit;
        exit.id = "plug_0002";
        exit.name = "a.exit";
        exit.anchor = {5.0, 5.0}; // on the E edge (x == origin.x + width)
        doc.plugs.push_back(door);
        doc.plugs.push_back(exit);

        DraftingDeclaredConnection conn;
        conn.id = "conn_0001";
        conn.plugA = "plug_0001";
        conn.plugB = "plug_0002";
        conn.type = "corridor";
        doc.connections.push_back(conn);

        // One placed block instance = two flattened objects sharing an instanceId;
        // their bounds union centres at canvas (3,4) -> inside room a -> feet (6,8).
        const auto placed = [](const std::string &id, Point2D p) {
            DraftingObject o = makeDraftingObject(id, DraftingShapeKind::Point, PointGeometry{p});
            o.bounds = computeBounds(o.geometry);
            o.metadata.blockPlacement.blockId = "block_0001";
            o.metadata.blockPlacement.assetRef = "recipe.chair";
            o.metadata.blockPlacement.instanceId = "blockinst_0005";
            return o;
        };
        doc.objects.push_back(placed("instance_0006", {2.0, 3.0}));
        doc.objects.push_back(placed("instance_0007", {4.0, 5.0}));

        const std::string docToon = edi::io::exportMapToToon(doc, "doc");
        const std::string expectedDoc =
            "kind: map\n"
            "title: doc\n"
            "units: feet\n"
            "\n"
            "rooms[1]{name,origin,size,material}:\n"
            "  a,\"2,4\",\"8,12\",stone\n"
            "\n"
            "plugs[2]{room,name,edge,type,connected}:\n"
            "  a,door,N,door,true\n"
            "  a,exit,E,door,true\n"
            "\n"
            "connections[1]{from,to,type}:\n"
            "  a.door,a.exit,corridor\n"
            "\n"
            "blocks[1]{room,asset,origin,scale,rotation}:\n"
            "  a,recipe.chair,\"6,8\",1,0\n";
        assert(docToon == expectedDoc);
    }

    return 0;
}
