// Map graph — S0 (data layout) + S1 (DraftingGraphOps).
//
// S0 cases assert the LAYOUT: the plug/connection structs exist, a fresh document
// carries an empty graph, and the fields hold their values. S1 cases assert the
// MUTATION OPS: add/remove plug, declare/undeclare connection, their validation,
// the revision bump, and the removePlug → orphan-connection cascade.
// S5 cases assert ANCHOR RESYNC: syncGraphForMovedObject keeps plug.anchor live
// after a move, fixing the deriveEdge drift (Phase-1 decision 7 / brief 052).
#include "drafting/DraftingDocument.h"
#include "drafting/DraftingGraphOps.h"
#include "drafting/DraftingMapQuery.h" // deriveEdge — verifies the drift fix
#include "drafting/DraftingStore.h"    // removeObject — S4 cascade

#include "EdiAssert.h"
#include <cstdint>
#include <string>

using namespace edi::drafting;

int main()
{
    // A fresh document carries an empty map graph — both vectors default to {}.
    {
        DraftingDocument document = makeDraftingDocument("doc-1", "Untitled");
        EDI_CHECK(document.plugs.empty());
        EDI_CHECK(document.connections.empty());
    }

    // A plug holds exactly its neutral fields — an id, the object it rides on, a
    // name, a free-form type tag, and a cached coordinate. Nothing rule-bearing.
    {
        DraftingPlug plug;
        plug.id = "plug_0001";
        plug.anchorObjectId = "room.0";
        plug.name = "north_doorway";
        plug.type = "door";
        plug.anchor = {0.5, 0.25};

        EDI_CHECK(plug.id == "plug_0001");
        EDI_CHECK(plug.anchorObjectId == "room.0");
        EDI_CHECK(plug.name == "north_doorway");
        EDI_CHECK(plug.type == "door");
        EDI_CHECK(plug.anchor.x == 0.5 && plug.anchor.y == 0.25);
    }

    // A declared connection references two plugs by id and carries a neutral role
    // tag — and crucially nothing else (no passable / weight / direction).
    {
        DraftingDeclaredConnection connection;
        connection.id = "conn_0001";
        connection.plugA = "plug_0001";
        connection.plugB = "plug_0002";
        connection.type = "corridor";

        EDI_CHECK(connection.id == "conn_0001");
        EDI_CHECK(connection.plugA == "plug_0001");
        EDI_CHECK(connection.plugB == "plug_0002");
        EDI_CHECK(connection.type == "corridor");
    }

    // The document holds the graph as plain data: push and read back through the
    // sibling vectors, proving they are first-class document content.
    {
        DraftingDocument document = makeDraftingDocument("doc-2");

        DraftingPlug plug;
        plug.id = "plug_0001";
        plug.anchorObjectId = "room.0";
        document.plugs.push_back(plug);

        DraftingDeclaredConnection connection;
        connection.id = "conn_0001";
        connection.plugA = "plug_0001";
        connection.plugB = "plug_0001";
        document.connections.push_back(connection);

        EDI_CHECK(document.plugs.size() == 1);
        EDI_CHECK(document.connections.size() == 1);
        EDI_CHECK(document.plugs.front().anchorObjectId == "room.0");
        EDI_CHECK(document.connections.front().plugA == "plug_0001");
    }

    // --- S1: DraftingGraphOps ------------------------------------------------

    // addPlug requires a real anchor object and a unique, non-empty id; on
    // success it appends and bumps the revision (the addObject contract).
    {
        DraftingDocument document = makeDraftingDocument("doc-ops");
        document.objects.push_back(makeDraftingObject("room.0", DraftingShapeKind::Point, PointGeometry{}));
        const std::uint64_t before = document.revision;

        DraftingPlug plug;
        plug.id = "plug_0001";
        plug.anchorObjectId = "room.0";
        plug.name = "north_doorway";
        plug.type = "door";

        // anchor missing → rejected, no mutation.
        DraftingPlug orphan = plug;
        orphan.anchorObjectId = "nope";
        EDI_CHECK(!addPlug(document, orphan).ok);
        EDI_CHECK(document.plugs.empty());

        // empty id → rejected.
        DraftingPlug unnamed = plug;
        unnamed.id.clear();
        EDI_CHECK(!addPlug(document, unnamed).ok);

        // valid → accepted, appended, revision bumped.
        EDI_CHECK(addPlug(document, plug).ok);
        EDI_CHECK(document.plugs.size() == 1);
        EDI_CHECK(document.revision == before + 1);

        // duplicate id → rejected, still one plug.
        EDI_CHECK(!addPlug(document, plug).ok);
        EDI_CHECK(document.plugs.size() == 1);
    }

    // declareConnection requires both endpoints to name plugs that exist, and a
    // unique connection id.
    {
        DraftingDocument document = makeDraftingDocument("doc-conn");
        document.objects.push_back(makeDraftingObject("m.0", DraftingShapeKind::Point, PointGeometry{}));
        document.objects.push_back(makeDraftingObject("m.1", DraftingShapeKind::Point, PointGeometry{}));

        DraftingPlug a; a.id = "plug_a"; a.anchorObjectId = "m.0";
        DraftingPlug b; b.id = "plug_b"; b.anchorObjectId = "m.1";
        EDI_CHECK(addPlug(document, a).ok);
        EDI_CHECK(addPlug(document, b).ok);

        DraftingDeclaredConnection conn;
        conn.id = "conn_0001";
        conn.plugA = "plug_a";
        conn.plugB = "plug_missing";
        EDI_CHECK(!declareConnection(document, conn).ok); // unknown plug → rejected
        EDI_CHECK(document.connections.empty());

        conn.plugB = "plug_b";
        EDI_CHECK(declareConnection(document, conn).ok);
        EDI_CHECK(document.connections.size() == 1);

        // duplicate connection id → rejected.
        EDI_CHECK(!declareConnection(document, conn).ok);
        EDI_CHECK(document.connections.size() == 1);
    }

    // removePlug cascades: every connection touching the removed plug goes too,
    // so the graph never holds a dangling edge.
    {
        DraftingDocument document = makeDraftingDocument("doc-cascade");
        for (const char *id : {"m.0", "m.1", "m.2"}) {
            document.objects.push_back(makeDraftingObject(id, DraftingShapeKind::Point, PointGeometry{}));
        }
        DraftingPlug a; a.id = "plug_a"; a.anchorObjectId = "m.0";
        DraftingPlug b; b.id = "plug_b"; b.anchorObjectId = "m.1";
        DraftingPlug c; c.id = "plug_c"; c.anchorObjectId = "m.2";
        EDI_CHECK(addPlug(document, a).ok && addPlug(document, b).ok && addPlug(document, c).ok);

        DraftingDeclaredConnection ab; ab.id = "conn_ab"; ab.plugA = "plug_a"; ab.plugB = "plug_b";
        DraftingDeclaredConnection bc; bc.id = "conn_bc"; bc.plugA = "plug_b"; bc.plugB = "plug_c";
        EDI_CHECK(declareConnection(document, ab).ok && declareConnection(document, bc).ok);
        EDI_CHECK(document.connections.size() == 2);

        // remove plug_b → both edges (ab, bc) reference it → both dropped.
        EDI_CHECK(removePlug(document, "plug_b").ok);
        EDI_CHECK(document.plugs.size() == 2);
        EDI_CHECK(document.connections.empty());

        // removing a missing plug → rejected.
        EDI_CHECK(!removePlug(document, "plug_b").ok);
    }

    // undeclareConnection removes one specific edge; an unknown id → rejected.
    {
        DraftingDocument document = makeDraftingDocument("doc-undeclare");
        document.objects.push_back(makeDraftingObject("m.0", DraftingShapeKind::Point, PointGeometry{}));
        document.objects.push_back(makeDraftingObject("m.1", DraftingShapeKind::Point, PointGeometry{}));
        DraftingPlug a; a.id = "plug_a"; a.anchorObjectId = "m.0";
        DraftingPlug b; b.id = "plug_b"; b.anchorObjectId = "m.1";
        EDI_CHECK(addPlug(document, a).ok && addPlug(document, b).ok);
        DraftingDeclaredConnection ab; ab.id = "conn_ab"; ab.plugA = "plug_a"; ab.plugB = "plug_b";
        EDI_CHECK(declareConnection(document, ab).ok);

        EDI_CHECK(!undeclareConnection(document, "conn_nope").ok);
        EDI_CHECK(document.connections.size() == 1);
        EDI_CHECK(undeclareConnection(document, "conn_ab").ok);
        EDI_CHECK(document.connections.empty());
    }

    // --- S4: removing an anchor object cascades into the graph ----------------
    // A plug anchored to an object that is deleted is now dangling — removeObject
    // (and so DeleteObjectCommand) drops the plug and its edges, leaving the rest.
    {
        DraftingDocument document = makeDraftingDocument("doc-objdelete");
        for (const char *id : {"m.0", "m.1", "m.2"}) {
            document.objects.push_back(makeDraftingObject(id, DraftingShapeKind::Point, PointGeometry{}));
        }
        DraftingPlug a; a.id = "plug_a"; a.anchorObjectId = "m.0";
        DraftingPlug b; b.id = "plug_b"; b.anchorObjectId = "m.1";
        EDI_CHECK(addPlug(document, a).ok && addPlug(document, b).ok);
        DraftingDeclaredConnection ab; ab.id = "conn_ab"; ab.plugA = "plug_a"; ab.plugB = "plug_b";
        EDI_CHECK(declareConnection(document, ab).ok);

        // delete m.2 — nothing anchored to it → graph untouched.
        EDI_CHECK(removeObject(document, "m.2").ok);
        EDI_CHECK(document.plugs.size() == 2);
        EDI_CHECK(document.connections.size() == 1);

        // delete m.0 — plug_a dangles → plug_a AND conn_ab pruned; plug_b survives.
        EDI_CHECK(removeObject(document, "m.0").ok);
        EDI_CHECK(document.plugs.size() == 1);
        EDI_CHECK(document.plugs.front().id == "plug_b");
        EDI_CHECK(document.connections.empty());
    }

    // --- plugAtAnchorObject (B2-2 interactive authoring helper) ----------------
    // plugAtAnchorObject maps a document objectId back to the plug that anchors to
    // it — the connection tool uses this to resolve a canvas hit-test result to a
    // plug id. Returns nullopt when no plug anchors to the given objectId.
    {
        DraftingDocument document = makeDraftingDocument("doc-anchor-lookup");
        // Two Point markers, one plug anchored to each.
        document.objects.push_back(makeDraftingObject("m.0", DraftingShapeKind::Point, PointGeometry{}));
        document.objects.push_back(makeDraftingObject("m.1", DraftingShapeKind::Point, PointGeometry{}));
        document.objects.push_back(makeDraftingObject("m.2", DraftingShapeKind::Point, PointGeometry{}));

        DraftingPlug a; a.id = "plug_a"; a.anchorObjectId = "m.0";
        DraftingPlug b; b.id = "plug_b"; b.anchorObjectId = "m.1";
        EDI_CHECK(addPlug(document, a).ok && addPlug(document, b).ok);

        // Lookup on the anchored objects returns the correct plug id.
        const auto idA = plugAtAnchorObject(document, "m.0");
        EDI_CHECK(idA.has_value() && *idA == "plug_a");
        const auto idB = plugAtAnchorObject(document, "m.1");
        EDI_CHECK(idB.has_value() && *idB == "plug_b");

        // m.2 has no plug anchoring to it → nullopt.
        EDI_CHECK(!plugAtAnchorObject(document, "m.2").has_value());

        // An unknown object id → nullopt.
        EDI_CHECK(!plugAtAnchorObject(document, "nope").has_value());
    }

    // --- updatePlug (B2-3): mutate plug.type, touch nothing else ---------------
    {
        DraftingDocument document = makeDraftingDocument("doc-updateplug");
        document.objects.push_back(makeDraftingObject("m.0", DraftingShapeKind::Point, PointGeometry{}));
        DraftingPlug a;
        a.id             = "plug_a";
        a.anchorObjectId = "m.0";
        a.name           = "north_door";
        a.type           = "door";
        a.anchor         = {0.3, 0.3};
        EDI_CHECK(addPlug(document, a).ok);
        const std::size_t revBefore = document.revision;

        // Happy path: update type to "window" — only type changes, revision bumped.
        EDI_CHECK(updatePlug(document, "plug_a", "window").ok);
        EDI_CHECK(document.plugs[0].type == "window");
        EDI_CHECK(document.plugs[0].name == "north_door"); // name untouched
        EDI_CHECK(document.plugs[0].anchor.x == a.anchor.x); // anchor untouched
        EDI_CHECK(document.revision == revBefore + 1);

        // "secret" updates correctly.
        EDI_CHECK(updatePlug(document, "plug_a", "secret").ok);
        EDI_CHECK(document.plugs[0].type == "secret");

        // Unknown id is rejected; document unchanged.
        const std::size_t revAfter = document.revision;
        EDI_CHECK(!updatePlug(document, "no_such_plug", "door").ok);
        EDI_CHECK(document.revision == revAfter); // no bump on rejection
    }

    // --- S5: syncGraphForMovedObject — anchor resync (Phase-1 decision 7) -----
    //
    // PROBLEM solved: DraftingPlug::anchor was seeded at creation time and NOT
    // updated when the anchor object moved, so deriveEdge() (Seam C export) would
    // return the wrong wall edge after an interactive reposition.  This section
    // verifies the fix in isolation — pure ops, no Qt needed.
    //
    // Geometry: a 10×10 room at origin (0, 0).
    //   N wall midpoint  = (5, 0)   → deriveEdge → "N"
    //   E wall midpoint  = (10, 5)  → deriveEdge → "E"
    //   S wall midpoint  = (5, 10)  → deriveEdge → "S"
    //   W wall midpoint  = (0, 5)   → deriveEdge → "W"
    {
        DraftingDocument document = makeDraftingDocument("doc-sync");

        // Room footprint: 10×10 at origin (0, 0).
        DraftingMapRoom room;
        room.name     = "test_room";
        room.origin   = {0.0, 0.0};
        room.width    = 10.0;
        room.height   = 10.0;
        room.material = "stone";

        // Two Point markers: one that will be the plug anchor, one unrelated.
        // Initial anchor position: North wall midpoint (5, 0).
        DraftingObject markerA = makeDraftingObject(
            "anchor.A", DraftingShapeKind::Point, PointGeometry{Point2D{5.0, 0.0}});
        DraftingObject markerB = makeDraftingObject(
            "anchor.B", DraftingShapeKind::Point, PointGeometry{Point2D{0.0, 5.0}});
        document.objects.push_back(markerA);
        document.objects.push_back(markerB);

        // Plug A rides on markerA; plug B on markerB (second, independent plug).
        DraftingPlug plugA;
        plugA.id             = "plug_a";
        plugA.anchorObjectId = "anchor.A";
        plugA.name           = "north_door";
        plugA.type           = "door";
        plugA.anchor         = {5.0, 0.0}; // matches marker's initial position

        DraftingPlug plugB;
        plugB.id             = "plug_b";
        plugB.anchorObjectId = "anchor.B";
        plugB.name           = "west_door";
        plugB.type           = "door";
        plugB.anchor         = {0.0, 5.0};

        EDI_CHECK(addPlug(document, plugA).ok);
        EDI_CHECK(addPlug(document, plugB).ok);

        // Before any move: syncGraphForMovedObject is a no-op for an object
        // with no matching plug and returns false for an unknown id.
        EDI_CHECK(!syncGraphForMovedObject(document, "nope"));
        // Before move, anchor is already correct — still returns true (updated to
        // same value) because plug.anchorObjectId matches.
        // (We use markerB here — which HAS a plug — to show the "match but same
        // value" path is indistinguishable from a real move.)
        EDI_CHECK(syncGraphForMovedObject(document, "anchor.B")); // plug_b matched → true
        EDI_CHECK(document.plugs[1].anchor.x == 0.0);             // anchor.B is at (0,5)
        EDI_CHECK(document.plugs[1].anchor.y == 5.0);

        // Verify BEFORE the move: deriveEdge(room, plug_a.anchor) → "N" (stale check).
        EDI_CHECK(deriveEdge(room, document.plugs[0].anchor) == "N");

        // "MOVE" markerA to East wall midpoint (10, 5) by directly updating its
        // PointGeometry.  In the controller, this happens inside applyDraftingCommand;
        // here we update it directly to test syncGraphForMovedObject in isolation.
        DraftingObject *marker = findObject(document, "anchor.A");
        EDI_CHECK(marker != nullptr);
        marker->geometry = DraftingGeometry{PointGeometry{Point2D{10.0, 5.0}}};
        // anchor.A is now at (10, 5) but plug_a.anchor still reads (5, 0) — STALE.

        // Before sync: deriveEdge sees the stale anchor — still "N" (the drift bug).
        EDI_CHECK(deriveEdge(room, document.plugs[0].anchor) == "N");

        // Sync: syncGraphForMovedObject re-reads the live marker position and
        // updates plug_a.anchor to (10, 5).
        const std::uint64_t revBefore2 = document.revision;
        EDI_CHECK(syncGraphForMovedObject(document, "anchor.A")); // plug_a matched → true
        EDI_CHECK(document.revision == revBefore2); // NO revision bump (command owns it)

        // plug_a.anchor now matches the marker's live position.
        EDI_CHECK(document.plugs[0].anchor.x == 10.0);
        EDI_CHECK(document.plugs[0].anchor.y == 5.0);

        // After sync: deriveEdge sees the live anchor — "E" (the drift fix).
        EDI_CHECK(deriveEdge(room, document.plugs[0].anchor) == "E");

        // plug_b is anchored to a DIFFERENT object (anchor.B) — its anchor is untouched.
        EDI_CHECK(document.plugs[1].anchor.x == 0.0);
        EDI_CHECK(document.plugs[1].anchor.y == 5.0);

        // Sync on an object with no plug → returns false (common case: a wall).
        document.objects.push_back(
            makeDraftingObject("wall.0", DraftingShapeKind::Rectangle, PointGeometry{}));
        EDI_CHECK(!syncGraphForMovedObject(document, "wall.0"));
    }

    return 0;
}
