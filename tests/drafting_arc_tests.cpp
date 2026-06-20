#include "drafting/DraftingDocument.h"
#include "drafting/DraftingGeometry.h"
#include "drafting/DraftingHitTest.h"
#include "drafting/DraftingObjectEdit.h"

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

ArcGeometry quarterArc()
{
    // Centre at origin, radius 0.25, sweeping 0deg -> 90deg (right to top).
    return ArcGeometry{{0.5, 0.5}, 0.25, 0.0, 90.0};
}

DraftingObject arcObject(const ArcGeometry &arc)
{
    DraftingObject object = makeDraftingObject("arc-1", DraftingShapeKind::Arc, arc);
    object.bounds = computeBounds(object.geometry);
    return object;
}

} // namespace

int main()
{
    const ArcGeometry arc = quarterArc();

    // validate: a well-formed arc is accepted; non-finite / negative radius rejected.
    {
        EDI_CHECK(validateGeometry(arc).ok);
        ArcGeometry bad = arc;
        bad.radius = -1.0;
        EDI_CHECK(!validateGeometry(bad).ok);
        bad = arc;
        bad.startAngleDeg = std::numeric_limits<double>::quiet_NaN();
        EDI_CHECK(!validateGeometry(bad).ok);
    }

    // shape kind round-trips through the variant.
    EDI_CHECK(geometryKind(arc) == DraftingShapeKind::Arc);
    EDI_CHECK(std::string(shapeKindName(DraftingShapeKind::Arc)) == "arc");

    // bounds: a 0..90 arc of radius r spans [cx, cx+r] x [cy, cy+r]; the start
    // (cx+r, cy) and end (cx, cy+r) plus the centre define the box.
    {
        const Bounds2D b = computeBounds(DraftingGeometry{arc});
        EDI_CHECK(nearlyEqual(b.x, 0.5));
        EDI_CHECK(nearlyEqual(b.y, 0.5));
        EDI_CHECK(nearlyEqual(b.width, 0.25));
        EDI_CHECK(nearlyEqual(b.height, 0.25));

        // A 0..180 arc reaches x = cx-r on the left (180deg crosses cardinal 180).
        ArcGeometry half{{0.5, 0.5}, 0.25, 0.0, 180.0};
        const Bounds2D hb = computeBounds(DraftingGeometry{half});
        EDI_CHECK(nearlyEqual(hb.x, 0.25));        // leftmost point at 180deg
        EDI_CHECK(nearlyEqual(hb.width, 0.5));     // -r .. +r
        EDI_CHECK(nearlyEqual(hb.y, 0.5));         // top of the upper semicircle
        EDI_CHECK(nearlyEqual(hb.height, 0.25));   // up to 90deg
    }

    // translate moves the centre, angles and radius unchanged.
    {
        const auto moved = std::get<ArcGeometry>(translateGeometry(DraftingGeometry{arc}, 0.1, -0.2));
        EDI_CHECK(nearlyEqual(moved.center.x, 0.6));
        EDI_CHECK(nearlyEqual(moved.center.y, 0.3));
        EDI_CHECK(nearlyEqual(moved.radius, 0.25));
        EDI_CHECK(nearlyEqual(moved.startAngleDeg, 0.0));
        EDI_CHECK(nearlyEqual(moved.endAngleDeg, 90.0));
    }

    // hit test: a point on the arc (45deg, on the radius) is near; a point off
    // the span (at 270deg) measures to the nearer endpoint, not the radius.
    {
        const DraftingObject object = arcObject(arc);
        const Point2D onArc = arcPointAtAngle(arc.center, arc.radius, 45.0);
        EDI_CHECK(hitDistance(object.geometry, onArc) < 0.001);
        // A point just outside the radius at 45deg is ~its radial offset away.
        const Point2D outside = arcPointAtAngle(arc.center, arc.radius + 0.05, 45.0);
        EDI_CHECK(nearlyEqual(hitDistance(object.geometry, outside), 0.05, 0.0001));
        // A point at 270deg is outside the 0..90 span: distance to nearest endpoint.
        const Point2D below{0.5, 0.2};
        const double endpointDistance = std::min(
            distance(arcPointAtAngle(arc.center, arc.radius, 0.0), below),
            distance(arcPointAtAngle(arc.center, arc.radius, 90.0), below));
        EDI_CHECK(nearlyEqual(hitDistance(object.geometry, below), endpointDistance, 0.0001));
    }

    // handles: centre, radius (at mid angle), start, and end.
    {
        const DraftingObject object = arcObject(arc);
        const auto handles = draftingHandlesForObject(object);
        EDI_CHECK(handles.size() == 4);
        EDI_CHECK(handles[0].id == "arc_center");
        EDI_CHECK(handles[1].id == "arc_radius");
        EDI_CHECK(handles[2].id == "arc_start");
        EDI_CHECK(handles[3].id == "arc_end");
        // Start handle sits at angle 0 -> (cx+r, cy).
        EDI_CHECK(nearlyEqual(handles[2].point.x, 0.75));
        EDI_CHECK(nearlyEqual(handles[2].point.y, 0.5));
    }

    // handle edits: dragging the radius handle changes radius; the start handle
    // changes the start angle to the dragged direction.
    {
        const DraftingObject object = arcObject(arc);
        const Point2D widerRadius = arcPointAtAngle(arc.center, 0.4, 45.0);
        const DraftingHandleEditPlan radiusPlan = handleEditPlan(object, "arc_radius", widerRadius);
        EDI_CHECK(radiusPlan.ok);
        const DraftingObjectEditResult radiusEdit = applyObjectEdit(object, radiusPlan.edit);
        EDI_CHECK(radiusEdit.ok);
        EDI_CHECK(nearlyEqual(std::get<ArcGeometry>(radiusEdit.geometry).radius, 0.4));

        const Point2D startDir = arcPointAtAngle(arc.center, arc.radius, 30.0);
        const DraftingHandleEditPlan startPlan = handleEditPlan(object, "arc_start", startDir);
        EDI_CHECK(startPlan.ok);
        const DraftingObjectEditResult startEdit = applyObjectEdit(object, startPlan.edit);
        EDI_CHECK(startEdit.ok);
        EDI_CHECK(nearlyEqual(std::get<ArcGeometry>(startEdit.geometry).startAngleDeg, 30.0, 0.0001));
    }

    // flatten: sampleArc yields a connected chain along the span, endpoints exact.
    {
        const auto points = sampleArc(arc); // default 2deg step over a 90deg span
        EDI_CHECK(points.size() >= 2);
        EDI_CHECK(nearlyEqual(points.front().x, 0.75)); // start at 0deg
        EDI_CHECK(nearlyEqual(points.front().y, 0.5));
        EDI_CHECK(nearlyEqual(points.back().x, 0.5));    // end at 90deg
        EDI_CHECK(nearlyEqual(points.back().y, 0.75));
        // Every sample lies on the radius circle.
        for (const Point2D &p : points) {
            EDI_CHECK(nearlyEqual(distance(arc.center, p), arc.radius, 0.0001));
        }
        // A 90deg span at 2deg steps yields 45 segments => 46 points.
        EDI_CHECK(points.size() == 46);
    }

    return 0;
}
