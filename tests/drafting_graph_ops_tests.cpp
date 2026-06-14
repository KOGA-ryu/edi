// Map graph — S0 (data layout only).
//
// This slice adds the plug and declared-connection structs plus the two sibling
// vectors on DraftingDocument; it has NO ops, commands, or serialization yet
// (those are S1+). So these assertions are deliberately about LAYOUT: the structs
// exist, a fresh document carries an empty graph, and the fields hold their
// values. This file grows to hold the DraftingGraphOps tests in the next slice.
#include "drafting/DraftingDocument.h"

#include <cassert>
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

    return 0;
}
