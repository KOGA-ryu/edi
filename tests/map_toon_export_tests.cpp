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
    a.spec.plugs.push_back(RoomPlugSpec{RoomEdge::North, 0.0, "door1", "", {"window", "passes_light"}}); // type defaults -> door, carries flags
    a.spec.plugs.push_back(RoomPlugSpec{RoomEdge::South, 0.0, "secret1", "secret"}); // dead-end, no flags
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
        "plugs[3]{room,name,edge,type,connected,flags}:\n"
        "  a,door1,N,door,true,window·passes_light\n"
        "  a,secret1,S,secret,false,\"\"\n"
        "  b,in,N,door,true,\"\"\n"
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
        door.flags = {"locked", "iron"}; // DM-06: flags column on the document overload
        door.anchor = {3.0, 2.0}; // on the N edge (y == origin.y)
        DraftingPlug exit;
        exit.id = "plug_0002";
        exit.name = "a.exit";
        exit.anchor = {5.0, 5.0}; // on the E edge (x == origin.x + width), no flags
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
            "plugs[2]{room,name,edge,type,connected,flags}:\n"
            "  a,door,N,door,true,locked·iron\n"
            "  a,exit,E,door,true,\"\"\n"
            "\n"
            "connections[1]{from,to,type}:\n"
            "  a.door,a.exit,corridor\n"
            "\n"
            "blocks[1]{room,asset,origin,scale,rotation}:\n"
            "  a,recipe.chair,\"6,8\",1,0\n";
        assert(docToon == expectedDoc);
    }

    // DM-08 export-fidelity (Seam C is not lossy vs Seam B for rooms). Build the SAME
    // room two ways — an authored MapSpec (Seam B), and a DraftingDocument whose
    // canvas coords divide by the stored scale back to those authored numbers
    // (Seam C) — and assert the `rooms[...]` SECTION is byte-identical between the
    // two exports. This is the edit-then-export fidelity contract: the document path
    // (loaded → edited → saved → reloaded → exported) projects rooms with the same
    // name + footprint + material the authored path emits. scale 0.5 is exact in
    // binary, so authored(canvas) = canvas/0.5 recovers the integers exactly.
    {
        const double scale = 0.5;

        MapSpec mspec;
        NamedRoomSpec rr;
        rr.name = "hall";
        rr.spec.origin = {3.0, 4.0}; // authored feet (MapSpec export emits these directly)
        rr.spec.width = 10.0;
        rr.spec.height = 6.0;
        rr.spec.wallMaterial = "stone";
        mspec.rooms.push_back(rr);

        DraftingDocument rdoc = makeDraftingDocument("rt");
        rdoc.canvasPerAuthoredUnit = scale; // document stores CANVAS; export divides back
        rdoc.rooms.push_back(DraftingMapRoom{
            "hall", {3.0 * scale, 4.0 * scale}, 10.0 * scale, 6.0 * scale, "stone"});

        // Slice out the `rooms[...]:` header + rows, up to the blank line that ends it.
        const auto roomsSection = [](const std::string &toon) {
            const std::size_t start = toon.find("rooms[");
            assert(start != std::string::npos);
            const std::size_t end = toon.find("\n\n", start);
            return toon.substr(start, end == std::string::npos ? std::string::npos : end - start);
        };

        const std::string specRooms = roomsSection(edi::io::exportMapToToon(mspec));
        const std::string docRooms = roomsSection(edi::io::exportMapToToon(rdoc));
        // Seam C agrees with Seam B for the rooms section, byte-for-byte.
        assert(specRooms == docRooms);
        // …and it actually carries the name + footprint + material (would fail if a
        // field stopped projecting).
        assert(docRooms.find("hall") != std::string::npos);
        assert(docRooms.find("\"3,4\"") != std::string::npos);   // origin, authored
        assert(docRooms.find("\"10,6\"") != std::string::npos);  // size, authored
        assert(docRooms.find("stone") != std::string::npos);     // material
    }

    // DM-13: the blocks[] scale/rotation columns read the placement's real
    // rotationDeg/scale (not the old 1/0 literals). A non-identity instance must
    // surface its exact values in the exported row.
    {
        DraftingDocument doc = makeDraftingDocument("spun");
        doc.canvasPerAuthoredUnit = 1.0;
        doc.rooms.push_back(DraftingMapRoom{"a", {0.0, 0.0}, 10.0, 10.0, "stone"});

        // One placed instance = two flattened objects sharing instanceId AND the same
        // stamped placement transform (rot 45, scale 1.5).
        const auto placed = [](const std::string &id, Point2D p) {
            DraftingObject o = makeDraftingObject(id, DraftingShapeKind::Point, PointGeometry{p});
            o.bounds = computeBounds(o.geometry);
            o.metadata.blockPlacement.blockId = "block_0001";
            o.metadata.blockPlacement.assetRef = "recipe.urn";
            o.metadata.blockPlacement.instanceId = "blockinst_0009";
            o.metadata.blockPlacement.rotationDeg = 45.0;
            o.metadata.blockPlacement.scale = 1.5;
            return o;
        };
        doc.objects.push_back(placed("instance_0001", {4.0, 4.0}));
        doc.objects.push_back(placed("instance_0002", {6.0, 6.0})); // centre (5,5) -> room a

        const std::string toon = edi::io::exportMapToToon(doc, "spun");
        // The column order is {room,asset,origin,scale,rotation}: scale then rotation.
        assert(toon.find("blocks[1]{room,asset,origin,scale,rotation}:\n") != std::string::npos);
        assert(toon.find("  a,recipe.urn,\"5,5\",1.5,45\n") != std::string::npos);
    }

    return 0;
}
