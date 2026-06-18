#include "drafting/DraftingIntersectionOps.h"

#include "drafting/DraftingDocument.h"
#include "drafting/DraftingGeometry.h" // computeBounds, makeDraftingObject
#include "drafting/DraftingLayerOps.h" // makeDefaultLayer (to hide layers)

#include <cassert>
#include <cmath>

using namespace edi::drafting;

static bool near(double a, double b, double eps = 1e-9)
{
    return std::abs(a - b) < eps;
}

// Make a visible Line object with given endpoints.
static DraftingObject makeLine(const std::string &id, Point2D a, Point2D b,
                                bool visible = true)
{
    DraftingObject obj = makeDraftingObject(id, DraftingShapeKind::Line,
                                            DraftingGeometry{LineGeometry{a, b}});
    obj.bounds  = computeBounds(obj.geometry);
    obj.visible = visible;
    return obj;
}

// Make a visible ConstructionLine object.
static DraftingObject makeConstructionLine(const std::string &id, Point2D a, Point2D b)
{
    DraftingObject obj = makeDraftingObject(id, DraftingShapeKind::ConstructionLine,
                                            DraftingGeometry{ConstructionLineGeometry{a, b}});
    obj.bounds = computeBounds(obj.geometry);
    return obj;
}

int main()
{
    // --- documentIntersectionPoints -------------------------------------------

    // X of two lines crossing at (0.5, 0.5) → exactly 1 point.
    {
        DraftingDocument doc = makeDraftingDocument("d1");
        // Horizontal: (0.2, 0.5) → (0.8, 0.5)
        // Vertical:   (0.5, 0.2) → (0.5, 0.8)
        doc.objects.push_back(makeLine("l1", {0.2, 0.5}, {0.8, 0.5}));
        doc.objects.push_back(makeLine("l2", {0.5, 0.2}, {0.5, 0.8}));

        const auto pts = documentIntersectionPoints(doc);
        assert(pts.size() == 1);
        assert(near(pts[0].x, 0.5) && near(pts[0].y, 0.5));
    }

    // Three lines all passing through (0.5, 0.5):
    //   horizontal, vertical, and diagonal — three pairs all intersect at (0.5,0.5).
    // After dedup → exactly 1 point.
    {
        DraftingDocument doc = makeDraftingDocument("d2");
        doc.objects.push_back(makeLine("l1", {0.2, 0.5}, {0.8, 0.5})); // horizontal
        doc.objects.push_back(makeLine("l2", {0.5, 0.2}, {0.5, 0.8})); // vertical
        doc.objects.push_back(makeLine("l3", {0.2, 0.2}, {0.8, 0.8})); // diagonal through (0.5,0.5)

        const auto pts = documentIntersectionPoints(doc);
        assert(pts.size() == 1);
        assert(near(pts[0].x, 0.5) && near(pts[0].y, 0.5));
    }

    // Parallel lines: same slope → segmentIntersection returns nullopt → 0 points.
    {
        DraftingDocument doc = makeDraftingDocument("d3");
        doc.objects.push_back(makeLine("l1", {0.1, 0.3}, {0.7, 0.3})); // y=0.3
        doc.objects.push_back(makeLine("l2", {0.1, 0.6}, {0.7, 0.6})); // y=0.6

        const auto pts = documentIntersectionPoints(doc);
        assert(pts.empty());
    }

    // Hidden line contributes nothing: the crossing with the visible line is absent.
    {
        DraftingDocument doc = makeDraftingDocument("d4");
        doc.objects.push_back(makeLine("l1", {0.2, 0.5}, {0.8, 0.5}));          // visible
        doc.objects.push_back(makeLine("l2", {0.5, 0.2}, {0.5, 0.8}, false));   // HIDDEN

        const auto pts = documentIntersectionPoints(doc);
        assert(pts.empty()); // hidden line ignored, nothing crosses the one visible line
    }

    // ConstructionLine participates like a Line segment.
    {
        DraftingDocument doc = makeDraftingDocument("d5");
        doc.objects.push_back(makeLine("l1", {0.2, 0.5}, {0.8, 0.5}));
        doc.objects.push_back(makeConstructionLine("cl1", {0.5, 0.2}, {0.5, 0.8}));

        const auto pts = documentIntersectionPoints(doc);
        assert(pts.size() == 1);
        assert(near(pts[0].x, 0.5) && near(pts[0].y, 0.5));
    }

    // --- subsetIntersectionPoints ---------------------------------------------

    // Three lines: only the two selected ones cross; the third's intersection
    // with the first is not in the subset.
    {
        DraftingDocument doc = makeDraftingDocument("d6");
        // l1 horizontal y=0.5, l2 vertical x=0.5 → cross at (0.5,0.5)
        // l3 diagonal crosses l1 at (0.3,0.5) — but l3 is NOT in the subset
        doc.objects.push_back(makeLine("l1", {0.1, 0.5}, {0.9, 0.5}));
        doc.objects.push_back(makeLine("l2", {0.5, 0.1}, {0.5, 0.9}));
        doc.objects.push_back(makeLine("l3", {0.1, 0.1}, {0.9, 0.9})); // not selected

        const auto pts = subsetIntersectionPoints(doc, {"l1", "l2"});
        assert(pts.size() == 1);
        assert(near(pts[0].x, 0.5) && near(pts[0].y, 0.5));
    }

    // --- filterExistingIntersections ------------------------------------------

    // Document already has a Point at (0.5, 0.5); the candidate should be filtered.
    {
        DraftingDocument doc = makeDraftingDocument("d7");
        DraftingObject existingPt = makeDraftingObject("pt1", DraftingShapeKind::Point,
                                        DraftingGeometry{PointGeometry{{0.5, 0.5}}});
        existingPt.bounds = computeBounds(existingPt.geometry);
        doc.objects.push_back(existingPt);

        const std::vector<Point2D> candidates = {{0.5, 0.5}, {0.3, 0.7}};
        const auto filtered = filterExistingIntersections(doc, candidates);
        assert(filtered.size() == 1);
        assert(near(filtered[0].x, 0.3) && near(filtered[0].y, 0.7));
    }

    // No existing Point → all candidates pass through.
    {
        DraftingDocument doc = makeDraftingDocument("d8");
        const std::vector<Point2D> candidates = {{0.1, 0.2}, {0.3, 0.4}};
        const auto filtered = filterExistingIntersections(doc, candidates);
        assert(filtered.size() == 2);
    }

    return 0;
}
