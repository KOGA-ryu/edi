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

// Render-only mapping: a plug's neutral type string → WallType for the door leaf
// it renders. "window" → Window; "secret" → Secret; anything else (door/portal/
// threshold/…) → Door. The set is intentionally open on the left (new neutral
// type tags remain valid — they just render as a door until the painter gains a
// new branch). "Render-only" means this function decides visual appearance only;
// it never assigns game rules (Seam B owns those).
// Promoted from a local lambda in createMapFromSpec (B2-3) so setPlugType can
// share the IDENTICAL mapping — one place to change if a new visual variant lands.
WallType wallTypeForPlugType(const std::string &type);

// A plug is `connected` iff some declared connection names it (by plug id). Pure
// derivation over the document's connection list — no stored flag.
bool plugIsConnected(const std::vector<DraftingDeclaredConnection> &connections,
                     const DraftingPlugId &plugId);

} // namespace edi::drafting
