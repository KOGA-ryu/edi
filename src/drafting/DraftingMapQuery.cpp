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

WallType wallTypeForPlugType(const std::string &type)
{
    // Render-only: "window" and "secret" get their own visual; every other neutral
    // type tag (door/portal/threshold/…) falls through to Door.  The intentionally
    // open vocabulary means adding a new neutral type is a no-op here — it renders
    // as a door (the most-common default) until a new visual branch is wanted.
    if (type == "window") {
        return WallType::Window;
    }
    if (type == "secret") {
        return WallType::Secret;
    }
    return WallType::Door;
}

} // namespace edi::drafting
