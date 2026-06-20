#include "drafting/DraftingDocument.h"
#include "drafting/DraftingGeometry.h"
#include "drafting/DraftingHitTest.h"
#include "drafting/DraftingNumericEdit.h"
#include "drafting/DraftingObjectEdit.h"
#include "drafting/DraftingToolCreation.h"

#include "EdiAssert.h"
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
        EDI_CHECK(validateGeometry(ellipse).ok);
        EllipseGeometry bad = ellipse;
        bad.rx = -1.0;
        EDI_CHECK(!validateGeometry(bad).ok);
        bad = ellipse;
        bad.ry = std::numeric_limits<double>::quiet_NaN();
        EDI_CHECK(!validateGeometry(bad).ok);
    }

    // shape kind round-trips through the variant and the name maps.
    EDI_CHECK(geometryKind(ellipse) == DraftingShapeKind::Ellipse);
    EDI_CHECK(std::string(shapeKindName(DraftingShapeKind::Ellipse)) == "ellipse");
    EDI_CHECK(shapeKindFromName("ellipse") == DraftingShapeKind::Ellipse);

    // bounds: axis-aligned box centre +/- (rx, ry). rx != ry, so a swap fails.
    {
        const Bounds2D b = computeBounds(DraftingGeometry{ellipse});
        EDI_CHECK(nearlyEqual(b.x, 0.2));
        EDI_CHECK(nearlyEqual(b.y, 0.3));
        EDI_CHECK(nearlyEqual(b.width, 0.6));
        EDI_CHECK(nearlyEqual(b.height, 0.4));
    }

    // area = pi * rx * ry.
    EDI_CHECK(nearlyEqual(area(DraftingGeometry{ellipse}), 3.14159265358979323846 * 0.3 * 0.2, 0.0001));

    // translate moves the centre; radii unchanged.
    {
        const auto moved = std::get<EllipseGeometry>(translateGeometry(DraftingGeometry{ellipse}, 0.1, -0.2));
        EDI_CHECK(nearlyEqual(moved.center.x, 0.6));
        EDI_CHECK(nearlyEqual(moved.center.y, 0.3));
        EDI_CHECK(nearlyEqual(moved.rx, 0.3));
        EDI_CHECK(nearlyEqual(moved.ry, 0.2));
    }

    // flatten: a closed perimeter loop; first point at angle 0 is (cx+rx, cy);
    // every sample lies on the ellipse.
    {
        const auto points = sampleEllipse(ellipse);
        EDI_CHECK(points.size() == 64);
        EDI_CHECK(nearlyEqual(points.front().x, 0.8)); // cx + rx
        EDI_CHECK(nearlyEqual(points.front().y, 0.5));
        for (const Point2D &p : points) {
            const double nx = (p.x - ellipse.center.x) / ellipse.rx;
            const double ny = (p.y - ellipse.center.y) / ellipse.ry;
            EDI_CHECK(nearlyEqual(nx * nx + ny * ny, 1.0, 0.0001));
        }
    }

    // hit: a point on the perimeter is near; the centre is ~ry (the minor axis)
    // from the nearest perimeter point.
    {
        const DraftingObject object = ellipseObject(ellipse);
        EDI_CHECK(hitDistance(object.geometry, {0.8, 0.5}) < 0.01); // on the perimeter
        EDI_CHECK(nearlyEqual(hitDistance(object.geometry, ellipse.center), 0.2, 0.01));
    }

    // handles: centre, rx (on +x), ry (on +y).
    {
        const DraftingObject object = ellipseObject(ellipse);
        const auto handles = draftingHandlesForObject(object);
        EDI_CHECK(handles.size() == 3);
        EDI_CHECK(handles[0].id == "ellipse_center");
        EDI_CHECK(handles[1].id == "ellipse_rx");
        EDI_CHECK(handles[2].id == "ellipse_ry");
        EDI_CHECK(nearlyEqual(handles[1].point.x, 0.8) && nearlyEqual(handles[1].point.y, 0.5));
        EDI_CHECK(nearlyEqual(handles[2].point.x, 0.5) && nearlyEqual(handles[2].point.y, 0.7));
    }

    // handle edits: dragging the rx / ry handles sets the radii to the offset.
    {
        const DraftingObject object = ellipseObject(ellipse);
        const DraftingHandleEditPlan rxPlan = handleEditPlan(object, "ellipse_rx", {0.9, 0.5}); // dx 0.4
        EDI_CHECK(rxPlan.ok);
        const DraftingObjectEditResult rxEdit = applyObjectEdit(object, rxPlan.edit);
        EDI_CHECK(rxEdit.ok);
        EDI_CHECK(nearlyEqual(std::get<EllipseGeometry>(rxEdit.geometry).rx, 0.4));

        const DraftingHandleEditPlan ryPlan = handleEditPlan(object, "ellipse_ry", {0.5, 0.95}); // dy 0.45
        EDI_CHECK(ryPlan.ok);
        const DraftingObjectEditResult ryEdit = applyObjectEdit(object, ryPlan.edit);
        EDI_CHECK(ryEdit.ok);
        EDI_CHECK(nearlyEqual(std::get<EllipseGeometry>(ryEdit.geometry).ry, 0.45));
    }

    // numeric edit: setting rx by field id.
    {
        const DraftingObject object = ellipseObject(ellipse);
        const DraftingNumericEditResult edit = applyNumericGeometryEdit(object, "rx", 0.42);
        EDI_CHECK(edit.ok);
        EDI_CHECK(nearlyEqual(std::get<EllipseGeometry>(edit.geometry).rx, 0.42));
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
        EDI_CHECK(built.ok);
        EDI_CHECK(built.object.kind == DraftingShapeKind::Ellipse);
        const auto geo = std::get<EllipseGeometry>(built.object.geometry);
        EDI_CHECK(nearlyEqual(geo.center.x, 0.3) && nearlyEqual(geo.center.y, 0.3));
        EDI_CHECK(nearlyEqual(geo.rx, 0.2));
        EDI_CHECK(nearlyEqual(geo.ry, 0.15));
    }

    return 0;
}
