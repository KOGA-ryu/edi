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

    // 043: the M0 props critical path — an EMPTY-blockId carrier (a MapSpec-level
    // MapBlockSpec realizes as a definition-less marker: no blockId, just an assetRef
    // + instanceId) must reach the blocks[] row UNCHANGED. The export groups on
    // instanceId and never touches blockId (verified in MapToonExport.cpp), so empty
    // blockId is transparent here — but commit 0a4df63's reviewer flagged that the
    // existing cases all carry a NON-empty blockId, so this is proven only by
    // transitivity. This case asserts the seam DIRECTLY, the way blender-lab's
    // realizer reads it. We hand-build the document (Path B): wiring Path A through
    // the controller would force a CMake link of MapToonExport into the controller
    // test target, which the brief forbids.
    {
        DraftingDocument doc = makeDraftingDocument("crypt");
        // Non-1.0 scale so origin's authored round-trip is observable: authored =
        // canvas / 0.5. 0.5 is exact in binary, so authored(canvas) is exact.
        const double scale = 0.5;
        doc.canvasPerAuthoredUnit = scale;
        doc.rooms.push_back(DraftingMapRoom{"tomb", {0.0, 0.0}, 20.0, 20.0, "stone"});

        // The authored-feet origin we expect back out: (6,8) feet -> stored as canvas
        // (3,4). The two flattened objects share an instanceId; their bounds union
        // centres at canvas (3,4) -> divides by 0.5 back to feet (6,8). The cross-dept
        // pin: the exported origin must be the AUTHORED value (6,8), not canvas (3,4).
        const auto placed = [](const std::string &id, Point2D p) {
            DraftingObject o = makeDraftingObject(id, DraftingShapeKind::Point, PointGeometry{p});
            o.bounds = computeBounds(o.geometry);
            o.metadata.blockPlacement.blockId = ""; // DEFINITION-LESS: the M0 carrier
            o.metadata.blockPlacement.assetRef = "crypt.sarcophagus";
            o.metadata.blockPlacement.instanceId = "blockinst_0043";
            o.metadata.blockPlacement.rotationDeg = 90.0; // non-identity -> proves plumbing
            o.metadata.blockPlacement.scale = 2.0;
            return o;
        };
        doc.objects.push_back(placed("instance_0043a", {2.0, 3.0}));
        doc.objects.push_back(placed("instance_0043b", {4.0, 5.0})); // centre canvas (3,4)

        const std::string toon = edi::io::exportMapToToon(doc, "crypt");
        // Exactly one block row despite empty blockId — grouping keys on instanceId.
        assert(toon.find("blocks[1]{room,asset,origin,scale,rotation}:\n") != std::string::npos);
        // room resolved by containment (tomb), asset = the raw ref, origin = AUTHORED
        // feet (6,8) NOT canvas (3,4), scale 2, rotation 90.
        assert(toon.find("  tomb,crypt.sarcophagus,\"6,8\",2,90\n") != std::string::npos);
        // The canvas value must NOT leak into the row (would be "3,4" if unscaled).
        assert(toon.find("\"3,4\"") == std::string::npos);
    }

    // P2-A1 (brief 070): nodes[] section — CONDITIONAL, canonical position after
    // connections, before blocks; header-as-truth; empty => section absent.
    {
        // --- 2-node golden: pins the exact wire bytes for two nodes. ---
        // Node 1: named "junction_a", anchor (5, 3) authored feet, type "hub".
        // Node 2: name empty -> falls back to id "node_0002"; anchor (10, 7.5); type empty.
        // canvasPerAuthoredUnit = 1.0 so authored == canvas (simplifies the golden).
        DraftingDocument nodeDoc = makeDraftingDocument("nodes-test");
        nodeDoc.canvasPerAuthoredUnit = 1.0;
        nodeDoc.rooms.push_back(DraftingMapRoom{"r", {0.0, 0.0}, 10.0, 10.0, "stone"});

        DraftingNode n1;
        n1.id     = "node_0001";
        n1.name   = "junction_a";
        n1.anchor = {5.0, 3.0};
        n1.type   = "hub";
        nodeDoc.nodes.push_back(n1);

        DraftingNode n2;
        n2.id     = "node_0002";
        n2.name   = "";          // empty name -> id is the fallback label
        n2.anchor = {10.0, 7.5};
        n2.type   = "";          // empty type -> "" via cell()
        nodeDoc.nodes.push_back(n2);

        const std::string nodeToon = edi::io::exportMapToToon(nodeDoc, "nodes-test");

        // Exact golden — pins every byte of the nodes[] section and its position.
        // Sections: rooms · plugs(0) · connections(0) · nodes(2) · blocks(0).
        // The realizer indexes by column NAME from the header, so the order inside
        // the header ({name,anchor,type}) is stable — this is the byte contract.
        const std::string expectedNodes =
            "kind: map\n"
            "title: nodes-test\n"
            "units: feet\n"
            "\n"
            "rooms[1]{name,origin,size,material}:\n"
            "  r,\"0,0\",\"10,10\",stone\n"
            "\n"
            "plugs[0]{room,name,edge,type,connected,flags}:\n"
            "\n"
            "connections[0]{from,to,type}:\n"
            "\n"
            "nodes[2]{name,anchor,type}:\n"
            "  junction_a,\"5,3\",hub\n"       // named, typed
            "  node_0002,\"10,7.5\",\"\"\n"    // id fallback; empty type -> ""
            "\n"
            "blocks[0]{room,asset,origin,scale,rotation}:\n";
        assert(nodeToon == expectedNodes);

        // Canonical position: nodes section appears AFTER connections and BEFORE blocks.
        const std::size_t connPos  = nodeToon.find("connections[");
        const std::size_t nodesPos = nodeToon.find("nodes[");
        const std::size_t blocsPos = nodeToon.find("blocks[");
        assert(connPos  != std::string::npos);
        assert(nodesPos != std::string::npos);
        assert(blocsPos != std::string::npos);
        assert(connPos  < nodesPos && nodesPos < blocsPos);

        // --- Node-LESS document: NO nodes[] section — not even a header line. ---
        // Conditional-emission invariant: empty nodes -> section ABSENT -> same
        // bytes as the pre-A1 export (legacy maps stay byte-identical).
        DraftingDocument noNodeDoc = makeDraftingDocument("no-nodes");
        noNodeDoc.canvasPerAuthoredUnit = 1.0;
        noNodeDoc.rooms.push_back(DraftingMapRoom{"r", {0.0, 0.0}, 10.0, 10.0, "stone"});
        // (nodes vector default-empty)

        const std::string noNodeToon = edi::io::exportMapToToon(noNodeDoc, "no-nodes");
        assert(noNodeToon.find("nodes[") == std::string::npos);
        // And blocks section still immediately follows the connections blank line.
        const std::size_t connEnd   = noNodeToon.find("connections[");
        const std::size_t blocksPos = noNodeToon.find("blocks[");
        assert(connEnd  != std::string::npos && blocksPos != std::string::npos);
        assert(connEnd  < blocksPos); // no nodes[] in between
    }

    return 0;
}
