#include "drafting/DraftingDocument.h"
#include "drafting/DraftingGeometry.h"
#include "drafting/DraftingHitTest.h"
#include "drafting/DraftingNumericEdit.h"
#include "drafting/DraftingObjectEdit.h"
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

// Centre (0.5, 0.5), rx 0.3, ry 0.2 — deliberately rx != ry so the tests pin
// that rx maps to the x axis / width and ry to the y axis / height.
EllipseGeometry sampleEllipseGeometry()
{
    return EllipseGeometry{{0.5, 0.5}, 0.3, 0.2};
}

DraftingObject ellipseObject(const EllipseGeometry &ellipse)
{
    DraftingObject object = makeDraftingObject("ellipse-1", DraftingShapeKind::Ellipse, ellipse);
    object.bounds = computeBounds(object.geometry);
    return object;
}

} // namespace

int main()
{
    const EllipseGeometry ellipse = sampleEllipseGeometry();

    // validate: well-formed accepted; negative radius / non-finite rejected.
    {
        assert(validateGeometry(ellipse).ok);
        EllipseGeometry bad = ellipse;
        bad.rx = -1.0;
        assert(!validateGeometry(bad).ok);
        bad = ellipse;
        bad.ry = std::numeric_limits<double>::quiet_NaN();
        assert(!validateGeometry(bad).ok);
    }

    // shape kind round-trips through the variant and the name maps.
    assert(geometryKind(ellipse) == DraftingShapeKind::Ellipse);
    assert(std::string(shapeKindName(DraftingShapeKind::Ellipse)) == "ellipse");
    assert(shapeKindFromName("ellipse") == DraftingShapeKind::Ellipse);

    // bounds: axis-aligned box centre +/- (rx, ry). rx != ry, so a swap fails.
    {
        const Bounds2D b = computeBounds(DraftingGeometry{ellipse});
        assert(nearlyEqual(b.x, 0.2));
        assert(nearlyEqual(b.y, 0.3));
        assert(nearlyEqual(b.width, 0.6));
        assert(nearlyEqual(b.height, 0.4));
    }

    // area = pi * rx * ry.
    assert(nearlyEqual(area(DraftingGeometry{ellipse}), 3.14159265358979323846 * 0.3 * 0.2, 0.0001));

    // translate moves the centre; radii unchanged.
    {
        const auto moved = std::get<EllipseGeometry>(translateGeometry(DraftingGeometry{ellipse}, 0.1, -0.2));
        assert(nearlyEqual(moved.center.x, 0.6));
        assert(nearlyEqual(moved.center.y, 0.3));
        assert(nearlyEqual(moved.rx, 0.3));
        assert(nearlyEqual(moved.ry, 0.2));
    }

    // flatten: a closed perimeter loop; first point at angle 0 is (cx+rx, cy);
    // every sample lies on the ellipse.
    {
        const auto points = sampleEllipse(ellipse);
        assert(points.size() == 64);
        assert(nearlyEqual(points.front().x, 0.8)); // cx + rx
        assert(nearlyEqual(points.front().y, 0.5));
        for (const Point2D &p : points) {
            const double nx = (p.x - ellipse.center.x) / ellipse.rx;
            const double ny = (p.y - ellipse.center.y) / ellipse.ry;
            assert(nearlyEqual(nx * nx + ny * ny, 1.0, 0.0001));
        }
    }

    // hit: a point on the perimeter is near; the centre is ~ry (the minor axis)
    // from the nearest perimeter point.
    {
        const DraftingObject object = ellipseObject(ellipse);
        assert(hitDistance(object.geometry, {0.8, 0.5}) < 0.01); // on the perimeter
        assert(nearlyEqual(hitDistance(object.geometry, ellipse.center), 0.2, 0.01));
    }

    // handles: centre, rx (on +x), ry (on +y).
    {
        const DraftingObject object = ellipseObject(ellipse);
        const auto handles = draftingHandlesForObject(object);
        assert(handles.size() == 3);
        assert(handles[0].id == "ellipse_center");
        assert(handles[1].id == "ellipse_rx");
        assert(handles[2].id == "ellipse_ry");
        assert(nearlyEqual(handles[1].point.x, 0.8) && nearlyEqual(handles[1].point.y, 0.5));
        assert(nearlyEqual(handles[2].point.x, 0.5) && nearlyEqual(handles[2].point.y, 0.7));
    }

    // handle edits: dragging the rx / ry handles sets the radii to the offset.
    {
        const DraftingObject object = ellipseObject(ellipse);
        const DraftingHandleEditPlan rxPlan = handleEditPlan(object, "ellipse_rx", {0.9, 0.5}); // dx 0.4
        assert(rxPlan.ok);
        const DraftingObjectEditResult rxEdit = applyObjectEdit(object, rxPlan.edit);
        assert(rxEdit.ok);
        assert(nearlyEqual(std::get<EllipseGeometry>(rxEdit.geometry).rx, 0.4));

        const DraftingHandleEditPlan ryPlan = handleEditPlan(object, "ellipse_ry", {0.5, 0.95}); // dy 0.45
        assert(ryPlan.ok);
        const DraftingObjectEditResult ryEdit = applyObjectEdit(object, ryPlan.edit);
        assert(ryEdit.ok);
        assert(nearlyEqual(std::get<EllipseGeometry>(ryEdit.geometry).ry, 0.45));
    }

    // numeric edit: setting rx by field id.
    {
        const DraftingObject object = ellipseObject(ellipse);
        const DraftingNumericEditResult edit = applyNumericGeometryEdit(object, "rx", 0.42);
        assert(edit.ok);
        assert(nearlyEqual(std::get<EllipseGeometry>(edit.geometry).rx, 0.42));
    }

    // tool creation: two clicks (centre, then a corner) give rx/ry from the
    // independent x and y spans (not a single radius like circle).
    {
        DraftingToolCreationRequest request;
        request.tool = DraftingToolKind::Ellipse;
        request.objectId = "e2";
        request.start = {0.3, 0.3};
        request.end = {0.5, 0.45};
        const DraftingObjectBuildResult built = buildDraftingObjectForTool(request);
        assert(built.ok);
        assert(built.object.kind == DraftingShapeKind::Ellipse);
        const auto geo = std::get<EllipseGeometry>(built.object.geometry);
        assert(nearlyEqual(geo.center.x, 0.3) && nearlyEqual(geo.center.y, 0.3));
        assert(nearlyEqual(geo.rx, 0.2));
        assert(nearlyEqual(geo.ry, 0.15));
    }

    return 0;
}
