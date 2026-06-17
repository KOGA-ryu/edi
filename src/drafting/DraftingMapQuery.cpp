#include "drafting/DraftingMapQuery.h"

#include <cmath>

namespace edi::drafting {

std::string deriveEdge(const DraftingMapRoom &room, Point2D anchor)
{
    double best = std::abs(anchor.y - room.origin.y);
    std::string edge = "N";
    if (const double d = std::abs(anchor.y - (room.origin.y + room.height)); d < best) { best = d; edge = "S"; }
    if (const double d = std::abs(anchor.x - room.origin.x); d < best) { best = d; edge = "W"; }
    if (const double d = std::abs(anchor.x - (room.origin.x + room.width)); d < best) { best = d; edge = "E"; }
    return edge;
}

bool plugIsConnected(const std::vector<DraftingDeclaredConnection> &connections,
                     const DraftingPlugId &plugId)
{
    for (const DraftingDeclaredConnection &connection : connections) {
        if (connection.plugA == plugId || connection.plugB == plugId) {
            return true;
        }
    }
    return false;
}

} // namespace edi::drafting
