#pragma once

#include "drafting/DraftingTypes.h"
#include "drafting/DraftingDocument.h" // DraftingMapRoom

#include <string>
#include <vector>

namespace edi::drafting {

// The result of locating the region to fill around a seed click. A pure plan
// struct (ok + payload), the same shape as planDraftingRoom / the other map plans —
// the controller does the resolve→plan→apply→emit orchestration around it.
struct RegionFillPlan {
    bool ok = false;
    DraftingResultCode code = DraftingResultCode::None; // miss ⇒ InvalidSelectionTarget
    std::string message;
    std::vector<Point2D> ring;                          // closed footprint corners; empty when !ok
};

// v1 (algorithm a — room-footprint lookup): the enclosing region is the footprint
// of the FIRST DraftingMapRoom (document order) whose rectangle contains `seed`
// (inclusive on edges, via boundsContainsPoint). The ring is that footprint's four
// corners, consistently wound, in canvas units. A seed in open space (no room
// contains it) ⇒ !ok with an empty ring and a reason code — the no-op refusal the
// surface wants.
//
// PARKED extension point (algorithm b — general wall-loop trace): a later version
// adds `const std::vector<DraftingObject> &walls` to this signature and returns an
// arbitrary traced ring for wall-bounded areas with no room record. That is a
// behavior-preserving signature extension; v1 stays rooms-only (YAGNI), which is
// also what keeps DM-09/10 in its own lane, distinct from drafting's DR-15.
RegionFillPlan planRegionFill(Point2D seed, const std::vector<DraftingMapRoom> &rooms);

} // namespace edi::drafting
