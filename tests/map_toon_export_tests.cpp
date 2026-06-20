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

    // CONDITIONAL-ABSENCE PROOF (M3): the spec above has NO features, NO patrols, and
    // NO locked connection — so the export must carry NO markers[]/patrols[] section
    // and the connections header must stay exactly {from,to,type}. (The `expected`
    // golden above already pins this byte-for-byte; these spell out the invariant.)
    assert(toon.find("markers[") == std::string::npos);
    assert(toon.find("patrols[") == std::string::npos);
    assert(toon.find("{from,to,type}") != std::string::npos);
    assert(toon.find("locked") == std::string::npos);

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

    // P2-A2 (brief 071): level column on rooms + plugs — CONDITIONAL per-section.
    // Emitted only when some row has level != 0; appended LAST in the header.
    {
        // --- Level-bearing golden: at least one room and one plug carry non-zero level. ---
        // Two rooms: ground (level 0) and upper (level 2).
        // Two plugs: ground.south (level 0) and upper.north (level 1).
        // canvasPerAuthoredUnit = 1.0 so authored == canvas.
        DraftingDocument lvlDoc = makeDraftingDocument("level-test");
        lvlDoc.canvasPerAuthoredUnit = 1.0;
        lvlDoc.rooms.push_back(DraftingMapRoom{"ground", {0.0, 0.0}, 10.0, 10.0, "stone"});
        DraftingMapRoom upper;
        upper.name     = "upper";
        upper.origin   = {0.0, 20.0};
        upper.width    = 8.0;
        upper.height   = 8.0;
        upper.material = "stone";
        upper.level    = 2;
        lvlDoc.rooms.push_back(upper);

        DraftingPlug pGround;
        pGround.id     = "plug_0001";
        pGround.name   = "ground.south";
        pGround.anchor = {5.0, 10.0}; // S edge: y == origin.y + height = 0 + 10
        pGround.type   = "door";
        // level = 0 (default)
        lvlDoc.plugs.push_back(pGround);

        DraftingPlug pUpper;
        pUpper.id     = "plug_0002";
        pUpper.name   = "upper.north";
        pUpper.anchor = {4.0, 20.0}; // N edge: y == origin.y = 20
        pUpper.type   = "door";
        pUpper.level  = 1;
        lvlDoc.plugs.push_back(pUpper);

        const std::string lvlToon = edi::io::exportMapToToon(lvlDoc, "level-test");

        // Exact golden — pins every byte including the level column in LAST position.
        // Rooms: both rows carry level (even the level-0 row). The presence of ANY
        // non-zero level causes the column to appear for the WHOLE section.
        // Plugs: same rule — both rows carry level once any plug is non-zero.
        const std::string expectedLvl =
            "kind: map\n"
            "title: level-test\n"
            "units: feet\n"
            "\n"
            "rooms[2]{name,origin,size,material,level}:\n"
            "  ground,\"0,0\",\"10,10\",stone,0\n"
            "  upper,\"0,20\",\"8,8\",stone,2\n"
            "\n"
            "plugs[2]{room,name,edge,type,connected,flags,level}:\n"
            "  ground,south,S,door,false,\"\",0\n"
            "  upper,north,N,door,false,\"\",1\n"
            "\n"
            "connections[0]{from,to,type}:\n"
            "\n"
            "blocks[0]{room,asset,origin,scale,rotation}:\n";
        assert(lvlToon == expectedLvl);

        // Column is LAST: level appears after material in rooms, after flags in plugs.
        assert(lvlToon.find("{name,origin,size,material,level}") != std::string::npos);
        assert(lvlToon.find("{room,name,edge,type,connected,flags,level}") != std::string::npos);

        // --- All-level-0 document: rooms/plugs headers are BYTE-IDENTICAL to pre-A2. ---
        // Conditional-emission guard: zero non-default values ⇒ no level column ⇒
        // the export is unchanged from before this wire extension.
        DraftingDocument zeroDoc = makeDraftingDocument("zero-level");
        zeroDoc.canvasPerAuthoredUnit = 1.0;
        zeroDoc.rooms.push_back(DraftingMapRoom{"r", {0.0, 0.0}, 5.0, 5.0, "stone"});
        // (level defaults to 0)

        const std::string zeroToon = edi::io::exportMapToToon(zeroDoc, "zero-level");
        // No level column in rooms header.
        assert(zeroToon.find("{name,origin,size,material,level}") == std::string::npos);
        assert(zeroToon.find("{name,origin,size,material}") != std::string::npos);
        // No level column in plugs header (0 plugs, but still no level keyword).
        assert(zeroToon.find("flags,level") == std::string::npos);
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
            "nodes[2]{name,anchor,type,radius}:\n"       // A3: radius always present
            "  junction_a,\"5,3\",hub,0.5\n"             // named, typed, default radius
            "  node_0002,\"10,7.5\",\"\",0.5\n"          // id fallback; empty type; radius
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

    // P2-A3 (brief 075): inverted-model wire — nodes radius column + rooms
    // derivation + bounded_by columns (CONDITIONAL per-section).
    {
        // Build a document with:
        //   - 2 nodes with default radius (0.5) — verifies radius always appears
        //   - 1 Placed room (level 0, no boundedBy) — all columns at their defaults
        //   - 1 SpanDerived room with boundedBy = {node_A, node_B} — triggers both
        //     derivation and bounded_by columns
        // No level column (all rooms level 0).
        // scale = 1.0 so canvas == authored (keeps the golden readable).
        DraftingDocument invDoc = makeDraftingDocument("inverted-test");
        invDoc.canvasPerAuthoredUnit = 1.0;

        DraftingNode nA;
        nA.id     = "node_0001";
        nA.name   = "corner_a";
        nA.anchor = {1.0, 1.0};
        nA.type   = "hub";
        // radius = kDefaultNodeRadius (0.5) by default
        invDoc.nodes.push_back(nA);

        DraftingNode nB;
        nB.id     = "node_0002";
        nB.name   = "corner_b";
        nB.anchor = {4.0, 4.0};
        nB.type   = "hub";
        invDoc.nodes.push_back(nB);

        // Placed room — derivation = Placed (default), boundedBy = {} (default).
        invDoc.rooms.push_back(DraftingMapRoom{"placed_room", {0.0, 0.0}, 10.0, 5.0, "stone"});

        // SpanDerived room with bounding nodes.
        DraftingMapRoom spanRoom;
        spanRoom.name       = "span_room";
        spanRoom.origin     = {5.0, 5.0};
        spanRoom.width      = 3.0;
        spanRoom.height     = 3.0;
        spanRoom.material   = "stone";
        spanRoom.derivation = RoomDerivation::SpanDerived;
        spanRoom.boundedBy  = {"node_0001", "node_0002"};
        invDoc.rooms.push_back(spanRoom);

        const std::string invToon = edi::io::exportMapToToon(invDoc, "inverted-test");

        // Exact golden — pins every byte of the inverted-model wire.
        // Canonical column order:
        //   rooms: name,origin,size,material[,derivation][,bounded_by]  (no level: all 0)
        //   nodes: name,anchor,type,radius  (radius always present in this section)
        // bounded_by resolves node IDs to display names (corner_a·corner_b).
        const std::string expectedInv =
            "kind: map\n"
            "title: inverted-test\n"
            "units: feet\n"
            "\n"
            "rooms[2]{name,origin,size,material,derivation,bounded_by}:\n"
            "  placed_room,\"0,0\",\"10,5\",stone,placed,\"\"\n"
            "  span_room,\"5,5\",\"3,3\",stone,span_derived,corner_a·corner_b\n"
            "\n"
            "plugs[0]{room,name,edge,type,connected,flags}:\n"
            "\n"
            "connections[0]{from,to,type}:\n"
            "\n"
            "nodes[2]{name,anchor,type,radius}:\n"
            "  corner_a,\"1,1\",hub,0.5\n"
            "  corner_b,\"4,4\",hub,0.5\n"
            "\n"
            "blocks[0]{room,asset,origin,scale,rotation}:\n";
        assert(invToon == expectedInv);

        // bounded_by: node names joined with middle-dot, not ids.
        assert(invToon.find("corner_a·corner_b") != std::string::npos);
        assert(invToon.find("node_0001")          == std::string::npos); // id must not leak

        // --- Placed-only / node-less guard: NO derivation/bounded_by/nodes columns. ---
        DraftingDocument placedDoc = makeDraftingDocument("placed-only");
        placedDoc.canvasPerAuthoredUnit = 1.0;
        placedDoc.rooms.push_back(DraftingMapRoom{"r", {0.0, 0.0}, 5.0, 5.0, "stone"});
        // (all rooms Placed, all boundedBy empty, no nodes)

        const std::string placedToon = edi::io::exportMapToToon(placedDoc, "placed-only");
        assert(placedToon.find("derivation") == std::string::npos);
        assert(placedToon.find("bounded_by") == std::string::npos);
        assert(placedToon.find("nodes[")     == std::string::npos);
        // rooms section is byte-identical to pre-A3 (plain 4-column header).
        assert(placedToon.find("{name,origin,size,material}") != std::string::npos);
    }

    // M3 (proving-ground): the Seam B MapSpec overload carries markers[], patrols[],
    // and the conditional lock columns. Pins the EXACT bytes of all three.
    {
        MapSpec m;

        NamedRoomSpec key;
        key.name = "alcove";
        key.spec.origin = {36.0, 20.0};
        key.spec.width = 8.0;
        key.spec.height = 8.0;
        key.spec.wallMaterial = "stone";
        key.spec.plugs.push_back(RoomPlugSpec{RoomEdge::South, 0.0, "gate", "portcullis"});
        // a pickup marker, id gold_key, room-local feet (4,4), no metadata.
        RoomFeature pickup;
        pickup.type = "pickup";
        pickup.id = "gold_key";
        pickup.x = 4.0;
        pickup.y = 4.0;
        key.spec.features.push_back(pickup);
        m.rooms.push_back(key);

        NamedRoomSpec goal;
        goal.name = "goal";
        goal.spec.origin = {58.0, 44.0};
        goal.spec.width = 18.0;
        goal.spec.height = 14.0;
        goal.spec.wallMaterial = "stone";
        goal.spec.plugs.push_back(RoomPlugSpec{RoomEdge::North, 0.0, "entry", "door"});
        // a chest marker whose lock rides metadata (key_id=gold_key, locked=true).
        RoomFeature chest;
        chest.type = "chest";
        chest.id = "treasure";
        chest.x = 13.0;
        chest.y = 7.0;
        chest.metadata = {{"locked", "true"}, {"key_id", "gold_key"}};
        goal.spec.features.push_back(chest);
        m.rooms.push_back(goal);

        // the FINAL door connection: locked, key_id gold_key.
        MapConnectionSpec lockedDoor;
        lockedDoor.from = {"alcove", "gate"};
        lockedDoor.to = {"goal", "entry"};
        lockedDoor.type = "corridor";
        lockedDoor.locked = true;
        lockedDoor.keyId = "gold_key";
        m.connections.push_back(lockedDoor);

        // a closed patrol loop of 4 waypoints.
        MapPatrolPath loop;
        loop.id = "guard_loop";
        loop.closed = true;
        loop.waypoints = {{64.0, 8.0}, {74.0, 8.0}, {74.0, 16.0}, {64.0, 16.0}};
        m.patrols.push_back(loop);

        const std::string toon = edi::io::exportMapToToon(m, "pg");
        const std::string expected =
            "kind: map\n"
            "title: pg\n"
            "units: feet\n"
            "\n"
            "rooms[2]{name,origin,size,material}:\n"
            "  alcove,\"36,20\",\"8,8\",stone\n"
            "  goal,\"58,44\",\"18,14\",stone\n"
            "\n"
            "plugs[2]{room,name,edge,type,connected,flags}:\n"
            "  alcove,gate,S,portcullis,true,\"\"\n"
            "  goal,entry,N,door,true,\"\"\n"
            "\n"
            "connections[1]{from,to,type,locked,key_id}:\n"
            "  alcove.gate,goal.entry,corridor,true,gold_key\n"
            "\n"
            "markers[2]{room,id,role,x,y,meta}:\n"
            "  alcove,gold_key,pickup,\"4,4\",\"\"\n"
            "  goal,treasure,chest,\"13,7\",locked=true·key_id=gold_key\n"
            "\n"
            "patrols[1]{id,closed,points}:\n"
            "  guard_loop,true,\"64,8·74,8·74,16·64,16\"\n";
        assert(toon == expected);

        // The lock columns appear in the connections header ONLY because one is locked.
        assert(toon.find("{from,to,type,locked,key_id}") != std::string::npos);
        // meta is a middle-dot-joined key=value run.
        assert(toon.find("locked=true·key_id=gold_key") != std::string::npos);
        // patrol points are middle-dot-joined x,y pairs, quoted as one cell.
        assert(toon.find("\"64,8·74,8·74,16·64,16\"") != std::string::npos);
    }

    // M3 conditional-column guard: a connection that is NOT locked and a room WITH a
    // marker — the connections header must stay {from,to,type} (no lock columns)
    // while markers[] still appears (each section's condition is independent).
    {
        MapSpec m;
        NamedRoomSpec r;
        r.name = "spawn";
        r.spec.origin = {0.0, 0.0};
        r.spec.width = 10.0;
        r.spec.height = 10.0;
        r.spec.wallMaterial = "stone";
        r.spec.plugs.push_back(RoomPlugSpec{RoomEdge::East, 0.0, "out", "door"});
        RoomFeature sp;
        sp.type = "spawn";
        sp.id = "player_spawn";
        sp.x = 5.0;
        sp.y = 5.0;
        r.spec.features.push_back(sp);
        m.rooms.push_back(r);

        NamedRoomSpec r2;
        r2.name = "hall";
        r2.spec.origin = {20.0, 0.0};
        r2.spec.width = 10.0;
        r2.spec.height = 10.0;
        r2.spec.wallMaterial = "stone";
        r2.spec.plugs.push_back(RoomPlugSpec{RoomEdge::West, 0.0, "in", "door"});
        m.rooms.push_back(r2);

        MapConnectionSpec c;
        c.from = {"spawn", "out"};
        c.to = {"hall", "in"};
        c.type = "corridor"; // NOT locked, no key
        m.connections.push_back(c);

        const std::string toon = edi::io::exportMapToToon(m, "g");
        assert(toon.find("{from,to,type}") != std::string::npos);    // no lock columns
        assert(toon.find("locked") == std::string::npos);            // not even the word
        assert(toon.find("markers[1]{room,id,role,x,y,meta}") != std::string::npos);
        assert(toon.find("  spawn,player_spawn,spawn,\"5,5\",\"\"\n") != std::string::npos);
        assert(toon.find("patrols[") == std::string::npos);          // no patrols here
    }

    return 0;
}
