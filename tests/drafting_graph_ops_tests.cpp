// Map graph — S0 (data layout) + S1 (DraftingGraphOps).
//
// S0 cases assert the LAYOUT: the plug/connection structs exist, a fresh document
// carries an empty graph, and the fields hold their values. S1 cases assert the
// MUTATION OPS: add/remove plug, declare/undeclare connection, their validation,
// the revision bump, and the removePlug → orphan-connection cascade.
#include "drafting/DraftingDocument.h"
#include "drafting/DraftingGraphOps.h"
#include "drafting/DraftingStore.h" // removeObject — S4 cascade

#include <cassert>
#include <cstdint>
#include <string>

using namespace edi::drafting;

int main()
{
    // A fresh document carries an empty map graph — both vectors default to {}.
    {
        DraftingDocument document = makeDraftingDocument("doc-1", "Untitled");
        assert(document.plugs.empty());
        assert(document.connections.empty());
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

        assert(plug.id == "plug_0001");
        assert(plug.anchorObjectId == "room.0");
        assert(plug.name == "north_doorway");
        assert(plug.type == "door");
        assert(plug.anchor.x == 0.5 && plug.anchor.y == 0.25);
    }

    // A declared connection references two plugs by id and carries a neutral role
    // tag — and crucially nothing else (no passable / weight / direction).
    {
        DraftingDeclaredConnection connection;
        connection.id = "conn_0001";
        connection.plugA = "plug_0001";
        connection.plugB = "plug_0002";
        connection.type = "corridor";

        assert(connection.id == "conn_0001");
        assert(connection.plugA == "plug_0001");
        assert(connection.plugB == "plug_0002");
        assert(connection.type == "corridor");
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

        assert(document.plugs.size() == 1);
        assert(document.connections.size() == 1);
        assert(document.plugs.front().anchorObjectId == "room.0");
        assert(document.connections.front().plugA == "plug_0001");
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
        assert(!addPlug(document, orphan).ok);
        assert(document.plugs.empty());

        // empty id → rejected.
        DraftingPlug unnamed = plug;
        unnamed.id.clear();
        assert(!addPlug(document, unnamed).ok);

        // valid → accepted, appended, revision bumped.
        assert(addPlug(document, plug).ok);
        assert(document.plugs.size() == 1);
        assert(document.revision == before + 1);

        // duplicate id → rejected, still one plug.
        assert(!addPlug(document, plug).ok);
        assert(document.plugs.size() == 1);
    }

    // declareConnection requires both endpoints to name plugs that exist, and a
    // unique connection id.
    {
        DraftingDocument document = makeDraftingDocument("doc-conn");
        document.objects.push_back(makeDraftingObject("m.0", DraftingShapeKind::Point, PointGeometry{}));
        document.objects.push_back(makeDraftingObject("m.1", DraftingShapeKind::Point, PointGeometry{}));

        DraftingPlug a; a.id = "plug_a"; a.anchorObjectId = "m.0";
        DraftingPlug b; b.id = "plug_b"; b.anchorObjectId = "m.1";
        assert(addPlug(document, a).ok);
        assert(addPlug(document, b).ok);

        DraftingDeclaredConnection conn;
        conn.id = "conn_0001";
        conn.plugA = "plug_a";
        conn.plugB = "plug_missing";
        assert(!declareConnection(document, conn).ok); // unknown plug → rejected
        assert(document.connections.empty());

        conn.plugB = "plug_b";
        assert(declareConnection(document, conn).ok);
        assert(document.connections.size() == 1);

        // duplicate connection id → rejected.
        assert(!declareConnection(document, conn).ok);
        assert(document.connections.size() == 1);
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
        assert(addPlug(document, a).ok && addPlug(document, b).ok && addPlug(document, c).ok);

        DraftingDeclaredConnection ab; ab.id = "conn_ab"; ab.plugA = "plug_a"; ab.plugB = "plug_b";
        DraftingDeclaredConnection bc; bc.id = "conn_bc"; bc.plugA = "plug_b"; bc.plugB = "plug_c";
        assert(declareConnection(document, ab).ok && declareConnection(document, bc).ok);
        assert(document.connections.size() == 2);

        // remove plug_b → both edges (ab, bc) reference it → both dropped.
        assert(removePlug(document, "plug_b").ok);
        assert(document.plugs.size() == 2);
        assert(document.connections.empty());

        // removing a missing plug → rejected.
        assert(!removePlug(document, "plug_b").ok);
    }

    // undeclareConnection removes one specific edge; an unknown id → rejected.
    {
        DraftingDocument document = makeDraftingDocument("doc-undeclare");
        document.objects.push_back(makeDraftingObject("m.0", DraftingShapeKind::Point, PointGeometry{}));
        document.objects.push_back(makeDraftingObject("m.1", DraftingShapeKind::Point, PointGeometry{}));
        DraftingPlug a; a.id = "plug_a"; a.anchorObjectId = "m.0";
        DraftingPlug b; b.id = "plug_b"; b.anchorObjectId = "m.1";
        assert(addPlug(document, a).ok && addPlug(document, b).ok);
        DraftingDeclaredConnection ab; ab.id = "conn_ab"; ab.plugA = "plug_a"; ab.plugB = "plug_b";
        assert(declareConnection(document, ab).ok);

        assert(!undeclareConnection(document, "conn_nope").ok);
        assert(document.connections.size() == 1);
        assert(undeclareConnection(document, "conn_ab").ok);
        assert(document.connections.empty());
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
        assert(addPlug(document, a).ok && addPlug(document, b).ok);
        DraftingDeclaredConnection ab; ab.id = "conn_ab"; ab.plugA = "plug_a"; ab.plugB = "plug_b";
        assert(declareConnection(document, ab).ok);

        // delete m.2 — nothing anchored to it → graph untouched.
        assert(removeObject(document, "m.2").ok);
        assert(document.plugs.size() == 2);
        assert(document.connections.size() == 1);

        // delete m.0 — plug_a dangles → plug_a AND conn_ab pruned; plug_b survives.
        assert(removeObject(document, "m.0").ok);
        assert(document.plugs.size() == 1);
        assert(document.plugs.front().id == "plug_b");
        assert(document.connections.empty());
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
        assert(addPlug(document, a).ok && addPlug(document, b).ok);

        // Lookup on the anchored objects returns the correct plug id.
        const auto idA = plugAtAnchorObject(document, "m.0");
        assert(idA.has_value() && *idA == "plug_a");
        const auto idB = plugAtAnchorObject(document, "m.1");
        assert(idB.has_value() && *idB == "plug_b");

        // m.2 has no plug anchoring to it → nullopt.
        assert(!plugAtAnchorObject(document, "m.2").has_value());

        // An unknown object id → nullopt.
        assert(!plugAtAnchorObject(document, "nope").has_value());
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
        assert(addPlug(document, a).ok);
        const std::size_t revBefore = document.revision;

        // Happy path: update type to "window" — only type changes, revision bumped.
        assert(updatePlug(document, "plug_a", "window").ok);
        assert(document.plugs[0].type == "window");
        assert(document.plugs[0].name == "north_door"); // name untouched
        assert(document.plugs[0].anchor.x == a.anchor.x); // anchor untouched
        assert(document.revision == revBefore + 1);

        // "secret" updates correctly.
        assert(updatePlug(document, "plug_a", "secret").ok);
        assert(document.plugs[0].type == "secret");

        // Unknown id is rejected; document unchanged.
        const std::size_t revAfter = document.revision;
        assert(!updatePlug(document, "no_such_plug", "door").ok);
        assert(document.revision == revAfter); // no bump on rejection
    }

    return 0;
}
