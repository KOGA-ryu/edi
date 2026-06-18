#pragma once

#include "drafting/DraftingDocument.h"
#include "drafting/DraftingGrid.h"

#include <string>
#include <vector>

namespace edi::recipe {

// The closed measurement vocabulary (width / height / length / radius) —
// extracted from pipeline A as the shared seam (R1-B03) and now the ONE
// home, full stop: A retired in R1-B06 and the op pipeline
// (resolveRecipeOps) is the sole caller. The wordings below are pipeline
// A's contract, preserved verbatim through its retirement
// (docs/recipe_binding_contract.md) and pinned by recipe_ops_resolve_tests.
//
// Physical scaling follows the binding contract
// (docs/recipe_binding_contract.md §3): width/radius scale along the grid's X
// axis, height along Y, length is a line's physical endpoint distance.
struct MeasureFieldResult {
    bool ok = false;
    double value = 0.0;
    std::string message; // B01 wordings, verbatim; empty on ok
};

// One drafted object's one measurement field -> one physical number. Field
// validity depends on the object's kind; an unanswerable field (length on a
// circle, radius on a line, an unknown name) fails with a message the UI can
// show next to the binding, as does a missing object.
MeasureFieldResult resolveMeasurementField(
    const edi::drafting::DraftingDocument &drafting,
    const edi::drafting::DraftingGridProjection &grid,
    const std::string &objectId,
    const std::string &field);

// The profile half of the seam (R1-B04, the same move as B03's
// measurements): a drafted line/polyline/arc becomes ordered PHYSICAL
// points — x = radius from the page-left spin axis, y = height above the
// page BOTTOM (z = (1 - drafted y) * grid height). The page-to-part
// convention, the deterministic arc sampling (64 segments per full circle,
// exact endpoints), and the four profile refusal wordings live HERE, once,
// for both pipelines.
struct ProfilePointsResult {
    bool ok = false;
    std::vector<edi::drafting::Point2D> points; // physical (radius, z)
    std::string message; // B01 wordings, verbatim; empty on ok
};

ProfilePointsResult resolveProfilePoints(
    const edi::drafting::DraftingDocument &drafting,
    const edi::drafting::DraftingGridProjection &grid,
    const std::string &objectId);

// The extrude twin (BL-03): the SAME point extraction + the same four refusal
// wordings as resolveProfilePoints, but projecting each drafted point to a
// physical FOOTPRINT (x = physicalWidth(x), y = physicalHeight(y)) — no radius
// reinterpretation, no 1-y z-flip. The prism rises +z by height from baseZ.
// ProfilePointsResult::points here carry physical (x, y) footprint, not (radius, z).
ProfilePointsResult resolveExtrudeProfilePoints(
    const edi::drafting::DraftingDocument &drafting,
    const edi::drafting::DraftingGridProjection &grid,
    const std::string &objectId);

// The Follow-Me sweep PATH (BL-08): the same drawing-plane footprint projection
// as the cross-section (it is the curve the footprint rides along). Same four
// refusal wordings; the >= 2 distinct-points rule is enforced by the resolve loop.
ProfilePointsResult resolveSweepPath(
    const edi::drafting::DraftingDocument &drafting,
    const edi::drafting::DraftingGridProjection &grid,
    const std::string &objectId);

} // namespace edi::recipe
