#include "drafting/DraftingSnap.h"
#include "drafting/DraftingGrid.h"
#include "drafting/DraftingStore.h"

#include <cassert>
#include <cmath>
#include <string>

using namespace edi::drafting;

namespace {

DraftingObject object(std::string id, DraftingShapeKind kind, DraftingGeometry geometry)
{
    auto built = buildDraftingObject(std::move(id), kind, std::move(geometry));
    assert(built.ok);
    return built.object;
}

bool nearlyEqual(double a, double b)
{
    return std::abs(a - b) < 0.000001;
}

} // namespace

int main()
{
    Point2D normalized = normalizeDraftingPoint({-0.25, 1.25});
    assert(normalized.x == 0.0);
    assert(normalized.y == 1.0);
    assert(std::string(draftingSnapKindName(DraftingSnapKind::Object)) == "object");
    assert(std::string(draftingSnapSourceKindName(DraftingSnapSourceKind::Midpoint)) == "midpoint");
    assert(std::string(draftingSnapSourceKindName(DraftingSnapSourceKind::Guide)) == "guide");
    assert(std::string(draftingSnapTolerancePresetId(0.015)) == "tight");
    assert(std::string(draftingSnapTolerancePresetId(0.03)) == "normal");
    assert(std::string(draftingSnapTolerancePresetId(0.06)) == "loose");
    assert(nearlyEqual(draftingSnapToleranceForPreset("tight"), 0.015));
    assert(nearlyEqual(draftingSnapToleranceForPreset("normal"), 0.03));
    assert(nearlyEqual(draftingSnapToleranceForPreset("loose"), 0.06));
    assert(nearlyEqual(draftingSnapToleranceForPreset("unknown"), 0.03));

    DraftingGridSettings physicalGrid;
    physicalGrid.width = 10.0;
    physicalGrid.height = 5.0;
    physicalGrid.minorStep = 2.0;
    DraftingSnapSettings snapFromGrid;
    applyDraftingGridToSnapSettings(snapFromGrid, physicalGrid);
    assert(nearlyEqual(snapFromGrid.gridStepX, 0.2));
    assert(nearlyEqual(snapFromGrid.gridStepY, 0.4));
    assert(nearlyEqual(snapFromGrid.gridStep, 0.2));

    DraftingSnapSettings gridSettings;
    gridSettings.gridEnabled = true;
    gridSettings.gridStep = 0.25;
    DraftingSnapResult grid = gridSnap({0.37, 0.62}, gridSettings);
    assert(grid.kind == DraftingSnapKind::Grid);
    assert(nearlyEqual(grid.point.x, 0.25));
    assert(nearlyEqual(grid.point.y, 0.5));

    gridSettings.gridStepX = 0.2;
    gridSettings.gridStepY = 0.1;
    DraftingSnapResult rectangularGrid = gridSnap({0.37, 0.62}, gridSettings);
    assert(nearlyEqual(rectangularGrid.point.x, 0.4));
    assert(nearlyEqual(rectangularGrid.point.y, 0.6));

    DraftingDocument document = makeDraftingDocument("snap_doc");
    assert(addObject(document, object("point_1", DraftingShapeKind::Point, PointGeometry{{0.1, 0.1}})).ok);
    assert(addObject(document, object("line_1", DraftingShapeKind::Line, LineGeometry{{0.2, 0.4}, {0.8, 0.4}})).ok);
    assert(addObject(document, object("rect_1", DraftingShapeKind::Rectangle, RectangleGeometry{{0.25, 0.25}, 0.25, 0.25})).ok);
    assert(addObject(document, object("guide_1", DraftingShapeKind::Guide, GuideGeometry{GuideOrientation::Horizontal, 0.75})).ok);
    assert(addObject(document, object("guide_2", DraftingShapeKind::Guide, GuideGeometry{GuideOrientation::Vertical, 0.33})).ok);
    assert(addObject(document, object("construction_1", DraftingShapeKind::ConstructionLine, ConstructionLineGeometry{{0.1, 0.9}, {0.9, 0.9}})).ok);
    assert(addObject(document, object("dimension_1", DraftingShapeKind::Dimension, DimensionGeometry{DimensionKind::Distance, {0.1, 0.7}, {0.9, 0.7}, 0.04})).ok);

    DraftingSnapSettings objectSettings;
    objectSettings.objectSnapEnabled = true;
    objectSettings.objectTolerance = 0.05;
    std::vector<DraftingSnapCandidate> lineCandidates = snapCandidatesForObject(document.objects[1], objectSettings);
    assert(lineCandidates.size() == 3);

    DraftingSnapSettings noEndpointSettings = objectSettings;
    noEndpointSettings.endpointEnabled = false;
    std::vector<DraftingSnapCandidate> noEndpointLineCandidates = snapCandidatesForObject(document.objects[1], noEndpointSettings);
    assert(noEndpointLineCandidates.size() == 1);
    assert(noEndpointLineCandidates.front().sourceKind == DraftingSnapSourceKind::Midpoint);

    DraftingSnapSettings noMidpointSettings = objectSettings;
    noMidpointSettings.midpointEnabled = false;
    std::vector<DraftingSnapCandidate> noMidpointLineCandidates = snapCandidatesForObject(document.objects[1], noMidpointSettings);
    assert(noMidpointLineCandidates.size() == 2);

    DraftingSnapSettings noVertexSettings = objectSettings;
    noVertexSettings.vertexEnabled = false;
    std::vector<DraftingSnapCandidate> noVertexRectCandidates = snapCandidatesForObject(document.objects[2], noVertexSettings);
    for (const DraftingSnapCandidate &candidate : noVertexRectCandidates) {
        assert(candidate.sourceKind != DraftingSnapSourceKind::Vertex);
    }

    DraftingSnapSettings noCenterSettings = objectSettings;
    noCenterSettings.centerEnabled = false;
    std::vector<DraftingSnapCandidate> noCenterRectCandidates = snapCandidatesForObject(document.objects[2], noCenterSettings);
    for (const DraftingSnapCandidate &candidate : noCenterRectCandidates) {
        assert(candidate.sourceKind != DraftingSnapSourceKind::Center);
    }

    DraftingSnapResult endpoint = resolveSnap({0.11, 0.09}, document, objectSettings);
    assert(endpoint.kind == DraftingSnapKind::Object);
    assert(endpoint.sourceKind == DraftingSnapSourceKind::Endpoint);
    assert(endpoint.sourceObjectId == "point_1");
    assert(nearlyEqual(endpoint.point.x, 0.1));
    assert(nearlyEqual(endpoint.point.y, 0.1));

    DraftingSnapResult midpoint = resolveSnap({0.5, 0.42}, document, objectSettings);
    assert(midpoint.kind == DraftingSnapKind::Object);
    assert(midpoint.sourceKind == DraftingSnapSourceKind::Midpoint);
    assert(midpoint.sourceObjectId == "line_1");
    assert(nearlyEqual(midpoint.point.x, 0.5));
    assert(nearlyEqual(midpoint.point.y, 0.4));

    std::vector<DraftingSnapCandidate> guideCandidates = snapCandidatesForObject(document.objects[3], objectSettings);
    assert(guideCandidates.empty());

    DraftingSnapResult horizontalGuide = resolveSnap({0.51, 0.74}, document, objectSettings);
    assert(horizontalGuide.kind == DraftingSnapKind::Object);
    assert(horizontalGuide.sourceKind == DraftingSnapSourceKind::Guide);
    assert(horizontalGuide.sourceObjectId == "guide_1");
    assert(horizontalGuide.label == "guide");
    assert(nearlyEqual(horizontalGuide.point.x, 0.51));
    assert(nearlyEqual(horizontalGuide.point.y, 0.75));

    DraftingSnapResult verticalGuide = resolveSnap({0.34, 0.2}, document, objectSettings);
    assert(verticalGuide.kind == DraftingSnapKind::Object);
    assert(verticalGuide.sourceKind == DraftingSnapSourceKind::Guide);
    assert(verticalGuide.sourceObjectId == "guide_2");
    assert(verticalGuide.label == "guide");
    assert(nearlyEqual(verticalGuide.point.x, 0.33));
    assert(nearlyEqual(verticalGuide.point.y, 0.2));

    DraftingSnapResult guideIntersection = resolveSnap({0.34, 0.74}, document, objectSettings);
    assert(guideIntersection.kind == DraftingSnapKind::Object);
    assert(guideIntersection.sourceKind == DraftingSnapSourceKind::Guide);
    assert(guideIntersection.label == "guide");
    assert(nearlyEqual(guideIntersection.point.x, 0.33));
    assert(nearlyEqual(guideIntersection.point.y, 0.75));

    DraftingSnapSettings objectSnapDisabled = objectSettings;
    objectSnapDisabled.objectSnapEnabled = false;
    DraftingSnapResult disabledObjectAndGuide = resolveSnap({0.34, 0.74}, document, objectSnapDisabled);
    assert(disabledObjectAndGuide.kind == DraftingSnapKind::None);

    DraftingSnapSettings guideSnapDisabled = objectSettings;
    guideSnapDisabled.guideEnabled = false;
    DraftingSnapResult disabledGuide = resolveSnap({0.34, 0.74}, document, guideSnapDisabled);
    assert(disabledGuide.kind == DraftingSnapKind::None);

    DraftingSnapSettings guideSnapWithoutCenter = objectSettings;
    guideSnapWithoutCenter.centerEnabled = false;
    DraftingSnapResult guideIgnoresCenterToggle = resolveSnap({0.34, 0.2}, document, guideSnapWithoutCenter);
    assert(guideIgnoresCenterToggle.kind == DraftingSnapKind::Object);
    assert(guideIgnoresCenterToggle.sourceKind == DraftingSnapSourceKind::Guide);

    DraftingSnapResult constructionEndpoint = resolveSnap({0.09, 0.91}, document, objectSettings);
    assert(constructionEndpoint.kind == DraftingSnapKind::Object);
    assert(constructionEndpoint.sourceKind == DraftingSnapSourceKind::Endpoint);
    assert(constructionEndpoint.sourceObjectId == "construction_1");
    assert(nearlyEqual(constructionEndpoint.point.x, 0.1));
    assert(nearlyEqual(constructionEndpoint.point.y, 0.9));

    DraftingSnapResult constructionMidpoint = resolveSnap({0.51, 0.91}, document, objectSettings);
    assert(constructionMidpoint.kind == DraftingSnapKind::Object);
    assert(constructionMidpoint.sourceKind == DraftingSnapSourceKind::Midpoint);
    assert(constructionMidpoint.sourceObjectId == "construction_1");
    assert(nearlyEqual(constructionMidpoint.point.x, 0.5));
    assert(nearlyEqual(constructionMidpoint.point.y, 0.9));

    DraftingSnapResult dimensionEndpoint = resolveSnap({0.09, 0.71}, document, objectSettings);
    assert(dimensionEndpoint.kind == DraftingSnapKind::Object);
    assert(dimensionEndpoint.sourceKind == DraftingSnapSourceKind::Endpoint);
    assert(dimensionEndpoint.sourceObjectId == "dimension_1");
    assert(nearlyEqual(dimensionEndpoint.point.x, 0.1));
    assert(nearlyEqual(dimensionEndpoint.point.y, 0.7));

    DraftingSnapResult dimensionMidpoint = resolveSnap({0.51, 0.71}, document, objectSettings);
    assert(dimensionMidpoint.kind == DraftingSnapKind::Object);
    assert(dimensionMidpoint.sourceKind == DraftingSnapSourceKind::Midpoint);
    assert(dimensionMidpoint.sourceObjectId == "dimension_1");
    assert(nearlyEqual(dimensionMidpoint.point.x, 0.5));
    assert(nearlyEqual(dimensionMidpoint.point.y, 0.7));

    DraftingSnapSettings prioritySettings = objectSettings;
    prioritySettings.gridEnabled = true;
    prioritySettings.gridStep = 0.25;
    DraftingSnapResult objectFirst = resolveSnap({0.11, 0.09}, document, prioritySettings);
    assert(objectFirst.kind == DraftingSnapKind::Object);
    prioritySettings.objectPriorityBeforeGrid = false;
    DraftingSnapResult gridFirst = resolveSnap({0.11, 0.09}, document, prioritySettings);
    assert(gridFirst.kind == DraftingSnapKind::Grid);

    DraftingSnapSettings guidePrioritySettings = objectSettings;
    guidePrioritySettings.gridEnabled = true;
    guidePrioritySettings.gridStep = 0.25;
    DraftingSnapResult guideFirst = resolveSnap({0.34, 0.74}, document, guidePrioritySettings);
    assert(guideFirst.kind == DraftingSnapKind::Object);
    assert(guideFirst.sourceKind == DraftingSnapSourceKind::Guide);
    guidePrioritySettings.objectPriorityBeforeGrid = false;
    DraftingSnapResult guideAfterGrid = resolveSnap({0.34, 0.74}, document, guidePrioritySettings);
    assert(guideAfterGrid.kind == DraftingSnapKind::Grid);

    DraftingSnapSettings tightTolerance = objectSettings;
    tightTolerance.objectTolerance = 0.005;
    DraftingSnapResult tightMiss = resolveSnap({0.11, 0.09}, document, tightTolerance);
    assert(tightMiss.kind == DraftingSnapKind::None);

    DraftingSnapSettings looseTolerance = objectSettings;
    looseTolerance.objectTolerance = 0.06;
    DraftingSnapResult looseHit = resolveSnap({0.14, 0.14}, document, looseTolerance);
    assert(looseHit.kind == DraftingSnapKind::Object);

    DraftingSnapSettings noSnap;
    DraftingSnapResult none = resolveSnap({2.0, -1.0}, document, noSnap);
    assert(none.kind == DraftingSnapKind::None);
    assert(none.point.x == 1.0);
    assert(none.point.y == 0.0);

    document.objects.front().visible = false;
    DraftingSnapResult hiddenMiss = resolveSnap({0.11, 0.09}, document, objectSettings);
    assert(hiddenMiss.kind == DraftingSnapKind::None);

    DraftingDocument hiddenGuideDocument = makeDraftingDocument("hidden_guide_doc");
    DraftingObject hiddenGuide = object("hidden_guide", DraftingShapeKind::Guide, GuideGeometry{GuideOrientation::Vertical, 0.4});
    hiddenGuide.visible = false;
    assert(addObject(hiddenGuideDocument, hiddenGuide).ok);
    DraftingSnapResult hiddenGuideMiss = resolveSnap({0.41, 0.2}, hiddenGuideDocument, objectSettings);
    assert(hiddenGuideMiss.kind == DraftingSnapKind::None);

    DraftingDocument hiddenLayerGuideDocument = makeDraftingDocument("hidden_layer_guide_doc");
    DraftingLayer hiddenLayer = makeDraftingLayer("hidden", "Hidden", 1);
    hiddenLayer.visible = false;
    hiddenLayerGuideDocument.layers.push_back(hiddenLayer);
    DraftingObject hiddenLayerGuide = object("hidden_layer_guide", DraftingShapeKind::Guide, GuideGeometry{GuideOrientation::Horizontal, 0.4});
    hiddenLayerGuide.layerId = "hidden";
    assert(addObject(hiddenLayerGuideDocument, hiddenLayerGuide).ok);
    DraftingSnapResult hiddenLayerGuideMiss = resolveSnap({0.2, 0.41}, hiddenLayerGuideDocument, objectSettings);
    assert(hiddenLayerGuideMiss.kind == DraftingSnapKind::None);

    DraftingDocument lockedGuideDocument = makeDraftingDocument("locked_guide_doc");
    DraftingObject lockedGuide = object("locked_guide", DraftingShapeKind::Guide, GuideGeometry{GuideOrientation::Vertical, 0.4});
    lockedGuide.locked = true;
    assert(addObject(lockedGuideDocument, lockedGuide).ok);
    DraftingSnapResult lockedGuideHit = resolveSnap({0.41, 0.2}, lockedGuideDocument, objectSettings);
    assert(lockedGuideHit.kind == DraftingSnapKind::Object);
    assert(lockedGuideHit.sourceKind == DraftingSnapSourceKind::Guide);
    assert(lockedGuideHit.sourceObjectId == "locked_guide");
    assert(nearlyEqual(lockedGuideHit.point.x, 0.4));
    assert(nearlyEqual(lockedGuideHit.point.y, 0.2));

    // Intersection snap: where two line SEGMENTS actually cross is a pairwise
    // candidate (it belongs to no single object), reusing the same
    // segmentIntersection the trim/fillet verbs are built on.
    assert(std::string(draftingSnapSourceKindName(DraftingSnapSourceKind::Intersection)) == "intersection");
    {
        DraftingDocument crossDoc = makeDraftingDocument("cross_doc");
        // Cross at (0.6, 0.5) — deliberately NOT a midpoint of either line, so
        // the snap below is the intersection specifically, not a coincidence.
        assert(addObject(crossDoc, object("h", DraftingShapeKind::Line, LineGeometry{{0.2, 0.5}, {0.8, 0.5}})).ok);
        assert(addObject(crossDoc, object("v", DraftingShapeKind::Line, LineGeometry{{0.6, 0.2}, {0.6, 0.9}})).ok);

        DraftingSnapSettings snapSettings;
        snapSettings.objectSnapEnabled = true;
        snapSettings.objectTolerance = 0.05;

        int intersectionCount = 0;
        DraftingSnapCandidate crossing;
        for (const DraftingSnapCandidate &candidate : snapCandidatesForDocument(crossDoc, snapSettings)) {
            if (candidate.sourceKind == DraftingSnapSourceKind::Intersection) {
                ++intersectionCount;
                crossing = candidate;
            }
        }
        assert(intersectionCount == 1);
        assert(nearlyEqual(crossing.point.x, 0.6) && nearlyEqual(crossing.point.y, 0.5));

        // A cursor near the crossing snaps onto it specifically.
        DraftingSnapResult snapped = resolveSnap({0.61, 0.49}, crossDoc, snapSettings);
        assert(snapped.kind == DraftingSnapKind::Object);
        assert(snapped.sourceKind == DraftingSnapSourceKind::Intersection);
        assert(nearlyEqual(snapped.point.x, 0.6) && nearlyEqual(snapped.point.y, 0.5));

        // The per-kind flag gates it: disabling intersection snap drops the candidate.
        DraftingSnapSettings noIntersection = snapSettings;
        noIntersection.intersectionEnabled = false;
        for (const DraftingSnapCandidate &candidate : snapCandidatesForDocument(crossDoc, noIntersection)) {
            assert(candidate.sourceKind != DraftingSnapSourceKind::Intersection);
        }

        // Lines whose segments do NOT cross (they'd only meet when extended)
        // produce no intersection candidate — it is segment, not line, crossing.
        DraftingDocument apartDoc = makeDraftingDocument("apart_doc");
        assert(addObject(apartDoc, object("a", DraftingShapeKind::Line, LineGeometry{{0.1, 0.3}, {0.4, 0.3}})).ok);
        assert(addObject(apartDoc, object("b", DraftingShapeKind::Line, LineGeometry{{0.7, 0.1}, {0.7, 0.25}})).ok);
        for (const DraftingSnapCandidate &candidate : snapCandidatesForDocument(apartDoc, snapSettings)) {
            assert(candidate.sourceKind != DraftingSnapSourceKind::Intersection);
        }
    }

    // DR-02: quadrant + nearest-on-curve snap sources.
    assert(std::string(draftingSnapSourceKindName(DraftingSnapSourceKind::Quadrant)) == "quadrant");
    assert(std::string(draftingSnapSourceKindName(DraftingSnapSourceKind::OnCurve)) == "on_curve");
    {
        DraftingSnapSettings snapSettings;
        snapSettings.objectSnapEnabled = true;
        snapSettings.objectTolerance = 0.05;

        // A circle emits exactly 4 quadrant candidates at the cardinal offsets.
        DraftingObject circle = object("circle_1", DraftingShapeKind::Circle, CircleGeometry{{0.5, 0.5}, 0.2});
        std::vector<DraftingSnapCandidate> circleCandidates = snapCandidatesForObject(circle, snapSettings);
        int circleQuadrants = 0;
        bool sawEast = false, sawNorth = false, sawWest = false, sawSouth = false;
        for (const DraftingSnapCandidate &c : circleCandidates) {
            if (c.sourceKind != DraftingSnapSourceKind::Quadrant) {
                continue;
            }
            ++circleQuadrants;
            sawEast = sawEast || (nearlyEqual(c.point.x, 0.7) && nearlyEqual(c.point.y, 0.5));
            sawNorth = sawNorth || (nearlyEqual(c.point.x, 0.5) && nearlyEqual(c.point.y, 0.7));
            sawWest = sawWest || (nearlyEqual(c.point.x, 0.3) && nearlyEqual(c.point.y, 0.5));
            sawSouth = sawSouth || (nearlyEqual(c.point.x, 0.5) && nearlyEqual(c.point.y, 0.3));
        }
        assert(circleQuadrants == 4);
        assert(sawEast && sawNorth && sawWest && sawSouth);

        // Disabling the flag drops the quadrant candidates (center still present).
        DraftingSnapSettings noQuadrant = snapSettings;
        noQuadrant.quadrantEnabled = false;
        for (const DraftingSnapCandidate &c : snapCandidatesForObject(circle, noQuadrant)) {
            assert(c.sourceKind != DraftingSnapSourceKind::Quadrant);
        }

        // An arc emits ONLY the in-sweep quadrants: a 0→90° arc has the 0° and 90°
        // cardinal points, not 180°/270°.
        DraftingObject arc = object("arc_1", DraftingShapeKind::Arc, ArcGeometry{{0.5, 0.5}, 0.2, 0.0, 90.0});
        int arcQuadrants = 0;
        for (const DraftingSnapCandidate &c : snapCandidatesForObject(arc, snapSettings)) {
            if (c.sourceKind != DraftingSnapSourceKind::Quadrant) {
                continue;
            }
            ++arcQuadrants;
            // Only east (0°) and north (90°) are in sweep.
            assert((nearlyEqual(c.point.x, 0.7) && nearlyEqual(c.point.y, 0.5))
                || (nearlyEqual(c.point.x, 0.5) && nearlyEqual(c.point.y, 0.7)));
        }
        assert(arcQuadrants == 2);

        // A cursor near a circle's quadrant snaps onto it (Quadrant participates in
        // resolveSnap; the perimeter point beats the far-away center).
        DraftingDocument circleDoc = makeDraftingDocument("circle_doc");
        assert(addObject(circleDoc, circle).ok);
        DraftingSnapResult quadrantSnap = resolveSnap({0.71, 0.5}, circleDoc, snapSettings);
        assert(quadrantSnap.kind == DraftingSnapKind::Object);
        assert(quadrantSnap.sourceKind == DraftingSnapSourceKind::Quadrant);
        assert(nearlyEqual(quadrantSnap.point.x, 0.7) && nearlyEqual(quadrantSnap.point.y, 0.5));
    }
    {
        // OnCurve: a cursor just off a line (away from any keypoint) snaps onto its
        // orthogonal projection.
        DraftingDocument lineDoc = makeDraftingDocument("oncurve_doc");
        assert(addObject(lineDoc, object("seg", DraftingShapeKind::Line, LineGeometry{{0.2, 0.4}, {0.8, 0.4}})).ok);
        DraftingSnapSettings snapSettings;
        snapSettings.objectSnapEnabled = true;
        snapSettings.objectTolerance = 0.05;

        DraftingSnapResult onCurve = resolveSnap({0.35, 0.42}, lineDoc, snapSettings);
        assert(onCurve.kind == DraftingSnapKind::Object);
        assert(onCurve.sourceKind == DraftingSnapSourceKind::OnCurve);
        assert(onCurve.label == "on_curve");
        assert(nearlyEqual(onCurve.point.x, 0.35) && nearlyEqual(onCurve.point.y, 0.4));

        // Disabling the flag suppresses OnCurve — nothing else is in range here.
        DraftingSnapSettings noOnCurve = snapSettings;
        noOnCurve.onCurveEnabled = false;
        assert(resolveSnap({0.35, 0.42}, lineDoc, noOnCurve).kind == DraftingSnapKind::None);

        // PRIORITY: an endpoint beats a COINCIDENT OnCurve. A cursor just past the
        // 'a' end projects (clamped) back onto the endpoint, so the projection and
        // the endpoint coincide — the endpoint wins (OnCurve is the fallback tier).
        DraftingSnapResult endpointWins = resolveSnap({0.18, 0.41}, lineDoc, snapSettings);
        assert(endpointWins.kind == DraftingSnapKind::Object);
        assert(endpointWins.sourceKind == DraftingSnapSourceKind::Endpoint);
        assert(nearlyEqual(endpointWins.point.x, 0.2) && nearlyEqual(endpointWins.point.y, 0.4));
    }

    // DR-03: relative (anchor-dependent) tangent + perpendicular candidates.
    assert(std::string(draftingSnapSourceKindName(DraftingSnapSourceKind::Tangent)) == "tangent");
    assert(std::string(draftingSnapSourceKindName(DraftingSnapSourceKind::Perpendicular)) == "perpendicular");
    {
        // Tangent from an external anchor to a circle: two contacts, each with the
        // anchor→contact segment perpendicular to the radius at the contact.
        DraftingDocument tanDoc = makeDraftingDocument("tangent_doc");
        const Point2D center{0.5, 0.5};
        const double radius = 0.2;
        assert(addObject(tanDoc, object("circle_t", DraftingShapeKind::Circle, CircleGeometry{center, radius})).ok);

        const Point2D anchor{0.9, 0.5}; // d = 0.4 = 2r → contacts at ±60° from the bearing
        std::vector<DraftingSnapCandidate> tangents;
        for (const DraftingSnapCandidate &c : relativeSnapCandidatesForDocument(tanDoc, anchor)) {
            if (c.sourceKind == DraftingSnapSourceKind::Tangent) {
                tangents.push_back(c);
            }
        }
        assert(tangents.size() == 2);
        bool sawUpper = false, sawLower = false;
        for (const DraftingSnapCandidate &c : tangents) {
            // anchor→contact ⊥ center→contact (dot product ≈ 0).
            const double rx = c.point.x - center.x;
            const double ry = c.point.y - center.y;
            const double px = c.point.x - anchor.x;
            const double py = c.point.y - anchor.y;
            assert(std::abs(rx * px + ry * py) < 0.000001);
            // The contact lies on the circle (radius preserved).
            assert(nearlyEqual(std::sqrt(rx * rx + ry * ry), radius));
            assert(nearlyEqual(c.point.x, 0.6)); // both contacts share x = 0.6
            sawUpper = sawUpper || c.point.y > 0.5;
            sawLower = sawLower || c.point.y < 0.5;
        }
        assert(sawUpper && sawLower);

        // The flag gates it.
        DraftingSnapSettings noTangent;
        noTangent.tangentEnabled = false;
        for (const DraftingSnapCandidate &c : relativeSnapCandidatesForDocument(tanDoc, anchor, noTangent)) {
            assert(c.sourceKind != DraftingSnapSourceKind::Tangent);
        }

        // An anchor INSIDE the circle yields no tangent candidate.
        for (const DraftingSnapCandidate &c : relativeSnapCandidatesForDocument(tanDoc, {0.55, 0.5})) {
            assert(c.sourceKind != DraftingSnapSourceKind::Tangent);
        }
    }
    {
        // Perpendicular foot from an anchor onto a known line = orthogonal projection.
        DraftingDocument perpDoc = makeDraftingDocument("perp_doc");
        assert(addObject(perpDoc, object("seg_p", DraftingShapeKind::Line, LineGeometry{{0.2, 0.4}, {0.8, 0.4}})).ok);

        const Point2D anchor{0.5, 0.9};
        std::vector<DraftingSnapCandidate> feet;
        for (const DraftingSnapCandidate &c : relativeSnapCandidatesForDocument(perpDoc, anchor)) {
            if (c.sourceKind == DraftingSnapSourceKind::Perpendicular) {
                feet.push_back(c);
            }
        }
        assert(feet.size() == 1);
        assert(nearlyEqual(feet.front().point.x, 0.5) && nearlyEqual(feet.front().point.y, 0.4));
        // anchor→foot ⊥ the line direction.
        const double dirX = 0.6, dirY = 0.0;
        const double fx = feet.front().point.x - anchor.x;
        const double fy = feet.front().point.y - anchor.y;
        assert(std::abs(fx * dirX + fy * dirY) < 0.000001);

        DraftingSnapSettings noPerp;
        noPerp.perpendicularEnabled = false;
        for (const DraftingSnapCandidate &c : relativeSnapCandidatesForDocument(perpDoc, anchor, noPerp)) {
            assert(c.sourceKind != DraftingSnapSourceKind::Perpendicular);
        }
    }

    return 0;
}
