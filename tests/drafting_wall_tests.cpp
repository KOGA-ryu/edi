#include "drafting/DraftingDocument.h"
#include "drafting/DraftingGeometry.h"
#include "drafting/DraftingHitTest.h"
#include "drafting/DraftingNumericEdit.h"
#include "drafting/DraftingObjectEdit.h"
#include "drafting/DraftingPlotPlan.h"
#include "drafting/DraftingStore.h"
#include "drafting/DraftingToolCreation.h"

#include <cassert>
#include <cmath>
#include <limits>
#include <string>

using namespace edi::drafting;

namespace {

bool nearlyEqual(double a, double b, double eps = 0.000001)
{
    return std::abs(a - b) <= eps;
}

// Centreline (0.2,0.5)->(0.8,0.5): a horizontal wall of thickness 0.1, so the
// band half-thickness is 0.05 and the bounds grow by exactly that on every
// side. a/b/thickness deliberately distinct so each test pins its own field.
WallGeometry sampleWall()
{
    return WallGeometry{{0.2, 0.5}, {0.8, 0.5}, 0.1};
}

DraftingObject wallObject(const WallGeometry &wall)
{
    DraftingObject object = makeDraftingObject("wall-1", DraftingShapeKind::Wall, wall);
    object.bounds = computeBounds(object.geometry);
    return object;
}

} // namespace

int main()
{
    const WallGeometry wall = sampleWall();

    // validate: finite endpoints + non-negative thickness pass; a non-finite
    // coordinate or a negative thickness is rejected (mirrors line + circle).
    {
        assert(validateGeometry(wall).ok);
        WallGeometry bad = wall;
        bad.thickness = -0.1;
        assert(!validateGeometry(bad).ok); // MUTATION PIN: drop the thickness check and this fails
        bad = wall;
        bad.b.x = std::numeric_limits<double>::quiet_NaN();
        assert(!validateGeometry(bad).ok);
        WallGeometry zero = wall;
        zero.thickness = 0.0;
        assert(validateGeometry(zero).ok); // a zero band is degenerate but legal, like a==b on a line
    }

    // kind round-trips through the variant and the name maps.
    assert(geometryKind(wall) == DraftingShapeKind::Wall);
    assert(std::string(shapeKindName(DraftingShapeKind::Wall)) == "wall");
    assert(shapeKindFromName("wall") == DraftingShapeKind::Wall);

    // bounds: the segment bbox EXPANDED by thickness/2 on every side. The
    // centreline spans x[0.2,0.8] at y=0.5; half-thickness 0.05 fattens it to
    // x[0.15,0.85] y[0.45,0.55]. A dropped half-thickness (the likely bug) fails.
    {
        const Bounds2D b = computeBounds(DraftingGeometry{wall});
        assert(nearlyEqual(b.x, 0.15));
        assert(nearlyEqual(b.y, 0.45));
        assert(nearlyEqual(b.width, 0.7));
        assert(nearlyEqual(b.height, 0.1));
    }

    // a wall, like a line, encloses no region: area is 0.
    assert(nearlyEqual(area(DraftingGeometry{wall}), 0.0));

    // translate moves both endpoints; thickness unchanged.
    {
        const auto moved = std::get<WallGeometry>(translateGeometry(DraftingGeometry{wall}, 0.1, -0.2));
        assert(nearlyEqual(moved.a.x, 0.3) && nearlyEqual(moved.a.y, 0.3));
        assert(nearlyEqual(moved.b.x, 0.9) && nearlyEqual(moved.b.y, 0.3));
        assert(nearlyEqual(moved.thickness, 0.1));
    }

    // hit: distance to the centreline SEGMENT minus half-thickness, clamped to
    // >= 0 — a click anywhere on the band registers as ~0. The centreline is
    // inside (0), the band edge is ~0, a far point is (offset - half-thickness).
    {
        const DraftingObject object = wallObject(wall);
        assert(nearlyEqual(hitDistance(object.geometry, {0.5, 0.5}), 0.0));           // on the centreline
        assert(nearlyEqual(hitDistance(object.geometry, {0.5, 0.55}), 0.0, 0.0001));  // band edge
        assert(nearlyEqual(hitDistance(object.geometry, {0.5, 0.7}), 0.15, 0.0001));  // 0.2 off - 0.05
    }

    // handles: two endpoint handles at a and b, plus a thickness handle at the
    // segment midpoint pushed perpendicular by half-thickness.
    {
        const DraftingObject object = wallObject(wall);
        const auto handles = draftingHandlesForObject(object);
        assert(handles.size() == 3);
        assert(handles[0].id == "wall_start");
        assert(handles[1].id == "wall_end");
        assert(handles[2].id == "wall_thickness");
        assert(nearlyEqual(handles[0].point.x, 0.2) && nearlyEqual(handles[0].point.y, 0.5));
        assert(nearlyEqual(handles[1].point.x, 0.8) && nearlyEqual(handles[1].point.y, 0.5));
        // midpoint (0.5,0.5) offset perpendicular by 0.05; the horizontal wall's
        // perpendicular is +/- y, so the handle sits half-thickness off the line.
        assert(nearlyEqual(handles[2].point.x, 0.5));
        assert(nearlyEqual(std::abs(handles[2].point.y - 0.5), 0.05, 0.0001));
    }

    // handle edits: dragging an endpoint moves a/b; dragging the thickness
    // handle resets thickness to twice the perpendicular offset (the inverse of
    // the handle placement above).
    {
        const DraftingObject object = wallObject(wall);
        const DraftingHandleEditPlan startPlan = handleEditPlan(object, "wall_start", {0.1, 0.4});
        assert(startPlan.ok);
        const DraftingObjectEditResult startEdit = applyObjectEdit(object, startPlan.edit);
        assert(startEdit.ok);
        assert(nearlyEqual(std::get<WallGeometry>(startEdit.geometry).a.x, 0.1));
        assert(nearlyEqual(std::get<WallGeometry>(startEdit.geometry).a.y, 0.4));

        // Drag the thickness handle to 0.15 off the midpoint -> thickness 0.3.
        const DraftingHandleEditPlan thickPlan = handleEditPlan(object, "wall_thickness", {0.5, 0.65});
        assert(thickPlan.ok);
        const DraftingObjectEditResult thickEdit = applyObjectEdit(object, thickPlan.edit);
        assert(thickEdit.ok);
        assert(nearlyEqual(std::get<WallGeometry>(thickEdit.geometry).thickness, 0.3, 0.0001));
    }

    // numeric edit: every field id round-trips; an unknown field is rejected.
    {
        const DraftingObject object = wallObject(wall);
        assert(nearlyEqual(std::get<WallGeometry>(applyNumericGeometryEdit(object, "ax", 0.05).geometry).a.x, 0.05));
        assert(nearlyEqual(std::get<WallGeometry>(applyNumericGeometryEdit(object, "ay", 0.06).geometry).a.y, 0.06));
        assert(nearlyEqual(std::get<WallGeometry>(applyNumericGeometryEdit(object, "bx", 0.95).geometry).b.x, 0.95));
        assert(nearlyEqual(std::get<WallGeometry>(applyNumericGeometryEdit(object, "by", 0.96).geometry).b.y, 0.96));
        assert(nearlyEqual(std::get<WallGeometry>(applyNumericGeometryEdit(object, "thickness", 0.2).geometry).thickness, 0.2));
        assert(!applyNumericGeometryEdit(object, "radius", 0.2).ok); // not a wall field
    }

    // tool creation: two clicks (a, then b); thickness comes from the tool
    // option, defaulting to 0.1 so a fresh wall is a visible band.
    {
        DraftingToolCreationRequest request;
        request.tool = DraftingToolKind::Wall;
        request.objectId = "w2";
        request.start = {0.3, 0.3};
        request.end = {0.7, 0.3};
        assert(draftingToolKindFromId("wall_tool") == DraftingToolKind::Wall);
        assert(std::string(draftingToolKindName(DraftingToolKind::Wall)) == "wall");
        const DraftingObjectBuildResult built = buildDraftingObjectForTool(request);
        assert(built.ok && built.object.kind == DraftingShapeKind::Wall);
        const auto &geo = std::get<WallGeometry>(built.object.geometry);
        assert(nearlyEqual(geo.a.x, 0.3) && nearlyEqual(geo.a.y, 0.3)); // start -> a
        assert(nearlyEqual(geo.b.x, 0.7) && nearlyEqual(geo.b.y, 0.3)); // end -> b
        assert(nearlyEqual(geo.thickness, 0.1));                        // default option

        // A custom thickness option is honoured; the endpoints are untouched.
        request.wallThickness = 0.25;
        const auto thick = buildDraftingObjectForTool(request);
        assert(nearlyEqual(std::get<WallGeometry>(thick.object.geometry).thickness, 0.25));
        assert(nearlyEqual(std::get<WallGeometry>(thick.object.geometry).a.x, 0.3));
    }

    // plot: v1 emits the CENTRELINE as a single segment (a->b), NOT the band
    // outline — the honest minimal choice, matching LineGeometry. A missing
    // arm here is SILENT (the visit has no static_assert), so this pins it.
    {
        DraftingDocument document = makeDraftingDocument("wall_plot_doc");
        const auto built = buildDraftingObject("plot_wall", DraftingShapeKind::Wall,
                                               WallGeometry{{0.2, 0.5}, {0.8, 0.5}, 0.1});
        assert(built.ok);
        assert(addObject(document, built.object).ok);
        const DraftingPlotPlan plan = buildDraftingPlotPlan(document, projectDraftingGrid(defaultDraftingGridSettings()));
        assert(plan.segments.size() == 1);
        assert(nearlyEqual(plan.segments[0].rawA.x, 0.2) && nearlyEqual(plan.segments[0].rawA.y, 0.5));
        assert(nearlyEqual(plan.segments[0].rawB.x, 0.8) && nearlyEqual(plan.segments[0].rawB.y, 0.5));
    }

    return 0;
}
