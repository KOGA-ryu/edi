#include "drafting/DraftingDocument.h"
#include "drafting/DraftingGeometry.h"
#include "drafting/DraftingHitTest.h"
#include "drafting/DraftingObjectEdit.h"
#include "drafting/DraftingSerialize.h"
#include "drafting/DraftingToolCreation.h"

#include <cassert>
#include <cmath>
#include <string>

using namespace edi::drafting;

namespace {

bool nearlyEqual(double a, double b, double eps = 0.000001)
{
    return std::abs(a - b) <= eps;
}

bool sampleContains(const std::vector<Point2D> &points, Point2D target, double eps = 0.0001)
{
    for (const Point2D &point : points) {
        if (nearlyEqual(point.x, target.x, eps) && nearlyEqual(point.y, target.y, eps)) {
            return true;
        }
    }
    return false;
}

DraftingObject splineObject(const SplineGeometry &s)
{
    DraftingObject object = makeDraftingObject("spline-1", DraftingShapeKind::Spline, s);
    object.bounds = computeBounds(object.geometry);
    return object;
}

} // namespace

int main()
{
    const SplineGeometry spline{{{0.1, 0.5}, {0.3, 0.2}, {0.6, 0.7}, {0.9, 0.4}}};

    // validate: two or more finite control points pass; one point or a
    // non-finite coordinate is rejected (mirrors the polyline gate).
    assert(validateGeometry(spline).ok);
    {
        assert(!validateGeometry(SplineGeometry{{{0.1, 0.1}}}).ok); // one point
        SplineGeometry bad = spline;
        bad.controlPoints[1].x = std::nan("");
        assert(!validateGeometry(bad).ok);
        // two points is the minimum — a straight segment, still a valid spline.
        assert(validateGeometry(SplineGeometry{{{0.1, 0.1}, {0.4, 0.4}}}).ok);
    }

    // kind round-trips through the variant and the name maps.
    assert(geometryKind(spline) == DraftingShapeKind::Spline);
    assert(std::string(shapeKindName(DraftingShapeKind::Spline)) == "spline");
    assert(shapeKindFromName("spline") == DraftingShapeKind::Spline);

    // sampleSpline: the curve PASSES THROUGH every control point (Catmull-Rom),
    // starts at the first knot and ends at the last. 4 points = 3 segments.
    {
        const std::vector<Point2D> sampled = sampleSpline(spline, 16);
        assert(sampled.size() == 1 + 3 * 16);
        assert(nearlyEqual(sampled.front().x, 0.1) && nearlyEqual(sampled.front().y, 0.5));
        assert(nearlyEqual(sampled.back().x, 0.9) && nearlyEqual(sampled.back().y, 0.4));
        for (const Point2D &knot : spline.controlPoints) {
            assert(sampleContains(sampled, knot));
        }
        // Fewer than three points: nothing to curve — returned verbatim.
        const SplineGeometry two{{{0.1, 0.1}, {0.4, 0.4}}};
        assert(sampleSpline(two).size() == 2);
        assert(sampleSpline(SplineGeometry{}).empty());
    }

    // bounds: measured on the sampled curve, so they must contain every knot
    // (the curve runs through all of them).
    {
        const Bounds2D b = computeBounds(DraftingGeometry{spline});
        for (const Point2D &knot : spline.controlPoints) {
            assert(knot.x >= b.x - 0.000001 && knot.x <= b.x + b.width + 0.000001);
            assert(knot.y >= b.y - 0.000001 && knot.y <= b.y + b.height + 0.000001);
        }
    }

    // translate moves every control point; the curve follows because it is
    // recomputed from them.
    {
        const auto moved = std::get<SplineGeometry>(translateGeometry(DraftingGeometry{spline}, 0.05, -0.1));
        assert(moved.controlPoints.size() == spline.controlPoints.size());
        for (std::size_t i = 0; i < moved.controlPoints.size(); ++i) {
            assert(nearlyEqual(moved.controlPoints[i].x, spline.controlPoints[i].x + 0.05));
            assert(nearlyEqual(moved.controlPoints[i].y, spline.controlPoints[i].y - 0.1));
        }
    }

    // hit: a control point lies on the curve, so its distance is ~0; a far
    // point is large.
    {
        const DraftingObject object = splineObject(spline);
        assert(hitDistance(object.geometry, {0.6, 0.7}) < 0.0001); // a knot, on the curve
        assert(hitDistance(object.geometry, {2.0, 2.0}) > 1.0);
    }

    // handles: one read-only knot marker per control point, at the control point.
    {
        const DraftingObject object = splineObject(spline);
        const auto anchors = handleAnchors(object.geometry);
        assert(anchors.size() == spline.controlPoints.size());
        assert(anchors[0].id == "control_0");
        assert(nearlyEqual(anchors[0].point.x, 0.1) && nearlyEqual(anchors[0].point.y, 0.5));

        const auto descriptors = draftingHandlesForObject(object);
        assert(descriptors.size() == spline.controlPoints.size());
        assert(descriptors[2].id == "control_2" && descriptors[2].readOnly);
    }

    // tool creation: a multi-click trail (request.vertices) becomes the spline's
    // control points — the SAME gesture as polyline, a different geometry kind.
    {
        DraftingToolCreationRequest request;
        request.tool = DraftingToolKind::Spline;
        request.objectId = "s2";
        request.vertices = {{0.2, 0.2}, {0.5, 0.4}, {0.8, 0.1}};
        assert(draftingToolKindFromId("spline_tool") == DraftingToolKind::Spline);
        assert(std::string(draftingToolKindName(DraftingToolKind::Spline)) == "spline");
        const DraftingObjectBuildResult built = buildDraftingObjectForTool(request);
        assert(built.ok && built.object.kind == DraftingShapeKind::Spline);
        const auto &geo = std::get<SplineGeometry>(built.object.geometry);
        assert(geo.controlPoints.size() == 3);
        assert(nearlyEqual(geo.controlPoints[2].x, 0.8) && nearlyEqual(geo.controlPoints[2].y, 0.1));
        // One control point cannot be a spline — the geometry gate holds.
        DraftingToolCreationRequest single = request;
        single.vertices = {{0.3, 0.3}};
        assert(!buildDraftingObjectForTool(single).ok);
    }

    // serialize round-trip: the control points survive (MsgPack array).
    {
        DraftingDocument doc = makeDraftingDocument("d", "t");
        doc.objects = {makeDraftingObject("spline-rt", DraftingShapeKind::Spline,
                                          SplineGeometry{{{0.15, 0.25}, {0.45, 0.65}, {0.85, 0.35}}})};
        const auto value = draftingDocumentToValue(doc);
        const auto restored = draftingDocumentFromValue(value);
        assert(restored.ok && restored.value);
        const auto &g = std::get<SplineGeometry>(restored.value->objects[0].geometry);
        assert(g.controlPoints.size() == 3);
        assert(nearlyEqual(g.controlPoints[1].x, 0.45) && nearlyEqual(g.controlPoints[1].y, 0.65));
    }

    return 0;
}
