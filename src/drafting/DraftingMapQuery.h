#pragma once

#include "drafting/DraftingTypes.h"
#include "drafting/DraftingDocument.h" // DraftingMapRoom, DraftingDeclaredConnection

#include <string>
#include <vector>

namespace edi::drafting {

// Shared map-graph queries the Seam C TOON export (io/MapToonExport) AND the live
// map-browser panel (widgets) both read — a CORE-side, NO-Qt home so the two views
// derive the SAME edge/connected facts from the same function and cannot drift
// (DM-11). Free functions over plain structs, no state.

// Which edge of a room footprint an anchor sits on — the document plug carries no
// edge, so derive it as the nearest of the four sides (canvas units; origin = NW,
// +y south). Returns "N"/"E"/"S"/"W".
std::string deriveEdge(const DraftingMapRoom &room, Point2D anchor);

// A plug is `connected` iff some declared connection names it (by plug id). Pure
// derivation over the document's connection list — no stored flag.
bool plugIsConnected(const std::vector<DraftingDeclaredConnection> &connections,
                     const DraftingPlugId &plugId);

} // namespace edi::drafting
