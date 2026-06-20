// DM-11: the shared map-graph queries (DraftingMapQuery) the TOON export and the map
// browser both read. Pin deriveEdge (nearest room side) + plugIsConnected.
#include "drafting/DraftingMapQuery.h"
#include "drafting/DraftingDocument.h"

#include "EdiAssert.h"
#include <vector>

using namespace edi::drafting;

int main()
{
    // deriveEdge: a 10x6 room at origin (1,2) spans x[1,11], y[2,8]. An anchor near
    // each side resolves to that side (origin = NW, +y south).
    {
        const DraftingMapRoom room{"r", {1.0, 2.0}, 10.0, 6.0, "stone"};
        EDI_CHECK(deriveEdge(room, {6.0, 2.0}) == "N");  // y == top
        EDI_CHECK(deriveEdge(room, {6.0, 8.0}) == "S");  // y == bottom
        EDI_CHECK(deriveEdge(room, {1.0, 5.0}) == "W");  // x == left
        EDI_CHECK(deriveEdge(room, {11.0, 5.0}) == "E"); // x == right
        // A point just inside the top-left resolves to the NEAREST side (N: dy 0.1 <
        // dx 0.3), pinning the nearest-of-four contract.
        EDI_CHECK(deriveEdge(room, {1.3, 2.1}) == "N");
    }

    // plugIsConnected: a plug id is connected iff some declared connection names it
    // (as plugA OR plugB); an unreferenced plug is not.
    {
        std::vector<DraftingDeclaredConnection> connections;
        DraftingDeclaredConnection c;
        c.id = "conn_0001";
        c.plugA = "plug_0001";
        c.plugB = "plug_0002";
        connections.push_back(c);

        EDI_CHECK(plugIsConnected(connections, "plug_0001"));  // named as plugA
        EDI_CHECK(plugIsConnected(connections, "plug_0002"));  // named as plugB
        EDI_CHECK(!plugIsConnected(connections, "plug_0003")); // unreferenced -> false
        EDI_CHECK(!plugIsConnected({}, "plug_0001"));          // no connections -> false
    }

    return 0;
}
