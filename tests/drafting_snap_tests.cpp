#include "drafting/DraftingSnap.h"
#include "drafting/DraftingGrid.h"
#include "drafting/DraftingGeometry.h" // computeBounds (for Node snap test's isectPt)
#include "drafting/DraftingStore.h"

#include "EdiAssert.h"
#include <cmath>
#include <string>

using namespace edi::drafting;

namespace {

DraftingObject object(std::string id, DraftingShapeKind kind, DraftingGeometry geometry)
{
    auto built = buildDraftingObject(std::move(id), kind, std::move(geometry));
    EDI_CHECK(built.ok);
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
    EDI_CHECK(normalized.x == 0.0);
    EDI_CHECK(normalized.y == 1.0);
    EDI_CHECK(std::string(draftingSnapKindName(DraftingSnapKind::Object)) == "object");
    EDI_CHECK(std::string(draftingSnapSourceKindName(DraftingSnapSourceKind::Midpoint)) == "midpoint");
    EDI_CHECK(std::string(draftingSnapSourceKindName(DraftingSnapSourceKind::Guide)) == "guide");
    EDI_CHECK(std::string(draftingSnapTolerancePresetId(0.015)) == "tight");
    EDI_CHECK(std::string(draftingSnapTolerancePresetId(0.03)) == "normal");
    EDI_CHECK(std::string(draftingSnapTolerancePresetId(0.06)) == "loose");
    EDI_CHECK(nearlyEqual(draftingSnapToleranceForPreset("tight"), 0.015));
    EDI_CHECK(nearlyEqual(draftingSnapToleranceForPreset("normal"), 0.03));
    EDI_CHECK(nearlyEqual(draftingSnapToleranceForPreset("loose"), 0.06));
    EDI_CHECK(nearlyEqual(draftingSnapToleranceForPreset("unknown"), 0.03));

    DraftingGridSettings physicalGrid;
    physicalGrid.width = 10.0;
    physicalGrid.height = 5.0;
    physicalGrid.minorStep = 2.0;
    DraftingSnapSettings snapFromGrid;
    applyDraftingGridToSnapSettings(snapFromGrid, physicalGrid);
    EDI_CHECK(nearlyEqual(snapFromGrid.gridStepX, 0.2));
    EDI_CHECK(nearlyEqual(snapFromGrid.gridStepY, 0.4));
    EDI_CHECK(nearlyEqual(snapFromGrid.gridStep, 0.2));

    DraftingSnapSettings gridSettings;
    gridSettings.gridEnabled = true;
    gridSettings.gridStep = 0.25;
    DraftingSnapResult grid = gridSnap({0.37, 0.62}, gridSettings);
    EDI_CHECK(grid.kind == DraftingSnapKind::Grid);
    EDI_CHECK(nearlyEqual(grid.point.x, 0.25));
    EDI_CHECK(nearlyEqual(grid.point.y, 0.5));

    gridSettings.gridStepX = 0.2;
    gridSettings.gridStepY = 0.1;
    DraftingSnapResult rectangularGrid = gridSnap({0.37, 0.62}, gridSettings);
    EDI_CHECK(nearlyEqual(rectangularGrid.point.x, 0.4));
    EDI_CHECK(nearlyEqual(rectangularGrid.point.y, 0.6));

    DraftingDocument document = makeDraftingDocument("snap_doc");
    EDI_CHECK(addObject(document, object("point_1", DraftingShapeKind::Point, PointGeometry{{0.1, 0.1}})).ok);
    EDI_CHECK(addObject(document, object("line_1", DraftingShapeKind::Line, LineGeometry{{0.2, 0.4}, {0.8, 0.4}})).ok);
    EDI_CHECK(addObject(document, object("rect_1", DraftingShapeKind::Rectangle, RectangleGeometry{{0.25, 0.25}, 0.25, 0.25})).ok);
    EDI_CHECK(addObject(document, object("guide_1", DraftingShapeKind::Guide, GuideGeometry{GuideOrientation::Horizontal, 0.75})).ok);
    EDI_CHECK(addObject(document, object("guide_2", DraftingShapeKind::Guide, GuideGeometry{GuideOrientation::Vertical, 0.33})).ok);
    EDI_CHECK(addObject(document, object("construction_1", DraftingShapeKind::ConstructionLine, ConstructionLineGeometry{{0.1, 0.9}, {0.9, 0.9}})).ok);
    EDI_CHECK(addObject(document, object("dimension_1", DraftingShapeKind::Dimension, DimensionGeometry{DimensionKind::Distance, {0.1, 0.7}, {0.9, 0.7}, 0.04})).ok);

    DraftingSnapSettings objectSettings;
    objectSettings.objectSnapEnabled = true;
    objectSettings.objectTolerance = 0.05;
    std::vector<DraftingSnapCandidate> lineCandidates = snapCandidatesForObject(document.objects[1], objectSettings);
    EDI_CHECK(lineCandidates.size() == 3);

    DraftingSnapSettings noEndpointSettings = objectSettings;
    noEndpointSettings.endpointEnabled = false;
    std::vector<DraftingSnapCandidate> noEndpointLineCandidates = snapCandidatesForObject(document.objects[1], noEndpointSettings);
    EDI_CHECK(noEndpointLineCandidates.size() == 1);
    EDI_CHECK(noEndpointLineCandidates.front().sourceKind == DraftingSnapSourceKind::Midpoint);

    DraftingSnapSettings noMidpointSettings = objectSettings;
    noMidpointSettings.midpointEnabled = false;
    std::vector<DraftingSnapCandidate> noMidpointLineCandidates = snapCandidatesForObject(document.objects[1], noMidpointSettings);
    EDI_CHECK(noMidpointLineCandidates.size() == 2);

    DraftingSnapSettings noVertexSettings = objectSettings;
    noVertexSettings.vertexEnabled = false;
    std::vector<DraftingSnapCandidate> noVertexRectCandidates = snapCandidatesForObject(document.objects[2], noVertexSettings);
    for (const DraftingSnapCandidate &candidate : noVertexRectCandidates) {
        EDI_CHECK(candidate.sourceKind != DraftingSnapSourceKind::Vertex);
    }

    DraftingSnapSettings noCenterSettings = objectSettings;
    noCenterSettings.centerEnabled = false;
    std::vector<DraftingSnapCandidate> noCenterRectCandidates = snapCandidatesForObject(document.objects[2], noCenterSettings);
    for (const DraftingSnapCandidate &candidate : noCenterRectCandidates) {
        EDI_CHECK(candidate.sourceKind != DraftingSnapSourceKind::Center);
    }

    DraftingSnapResult endpoint = resolveSnap({0.11, 0.09}, document, objectSettings);
    EDI_CHECK(endpoint.kind == DraftingSnapKind::Object);
    EDI_CHECK(endpoint.sourceKind == DraftingSnapSourceKind::Endpoint);
    EDI_CHECK(endpoint.sourceObjectId == "point_1");
    EDI_CHECK(nearlyEqual(endpoint.point.x, 0.1));
    EDI_CHECK(nearlyEqual(endpoint.point.y, 0.1));

    DraftingSnapResult midpoint = resolveSnap({0.5, 0.42}, document, objectSettings);
    EDI_CHECK(midpoint.kind == DraftingSnapKind::Object);
    EDI_CHECK(midpoint.sourceKind == DraftingSnapSourceKind::Midpoint);
    EDI_CHECK(midpoint.sourceObjectId == "line_1");
    EDI_CHECK(nearlyEqual(midpoint.point.x, 0.5));
    EDI_CHECK(nearlyEqual(midpoint.point.y, 0.4));

    std::vector<DraftingSnapCandidate> guideCandidates = snapCandidatesForObject(document.objects[3], objectSettings);
    EDI_CHECK(guideCandidates.empty());

    DraftingSnapResult horizontalGuide = resolveSnap({0.51, 0.74}, document, objectSettings);
    EDI_CHECK(horizontalGuide.kind == DraftingSnapKind::Object);
    EDI_CHECK(horizontalGuide.sourceKind == DraftingSnapSourceKind::Guide);
    EDI_CHECK(horizontalGuide.sourceObjectId == "guide_1");
    EDI_CHECK(horizontalGuide.label == "guide");
    EDI_CHECK(nearlyEqual(horizontalGuide.point.x, 0.51));
    EDI_CHECK(nearlyEqual(horizontalGuide.point.y, 0.75));

    DraftingSnapResult verticalGuide = resolveSnap({0.34, 0.2}, document, objectSettings);
    EDI_CHECK(verticalGuide.kind == DraftingSnapKind::Object);
    EDI_CHECK(verticalGuide.sourceKind == DraftingSnapSourceKind::Guide);
    EDI_CHECK(verticalGuide.sourceObjectId == "guide_2");
    EDI_CHECK(verticalGuide.label == "guide");
    EDI_CHECK(nearlyEqual(verticalGuide.point.x, 0.33));
    EDI_CHECK(nearlyEqual(verticalGuide.point.y, 0.2));

    DraftingSnapResult guideIntersection = resolveSnap({0.34, 0.74}, document, objectSettings);
    EDI_CHECK(guideIntersection.kind == DraftingSnapKind::Object);
    EDI_CHECK(guideIntersection.sourceKind == DraftingSnapSourceKind::Guide);
    EDI_CHECK(guideIntersection.label == "guide");
    EDI_CHECK(nearlyEqual(guideIntersection.point.x, 0.33));
    EDI_CHECK(nearlyEqual(guideIntersection.point.y, 0.75));

    DraftingSnapSettings objectSnapDisabled = objectSettings;
    objectSnapDisabled.objectSnapEnabled = false;
    DraftingSnapResult disabledObjectAndGuide = resolveSnap({0.34, 0.74}, document, objectSnapDisabled);
    EDI_CHECK(disabledObjectAndGuide.kind == DraftingSnapKind::None);

    DraftingSnapSettings guideSnapDisabled = objectSettings;
    guideSnapDisabled.guideEnabled = false;
    DraftingSnapResult disabledGuide = resolveSnap({0.34, 0.74}, document, guideSnapDisabled);
    EDI_CHECK(disabledGuide.kind == DraftingSnapKind::None);

    DraftingSnapSettings guideSnapWithoutCenter = objectSettings;
    guideSnapWithoutCenter.centerEnabled = false;
    DraftingSnapResult guideIgnoresCenterToggle = resolveSnap({0.34, 0.2}, document, guideSnapWithoutCenter);
    EDI_CHECK(guideIgnoresCenterToggle.kind == DraftingSnapKind::Object);
    EDI_CHECK(guideIgnoresCenterToggle.sourceKind == DraftingSnapSourceKind::Guide);

    DraftingSnapResult constructionEndpoint = resolveSnap({0.09, 0.91}, document, objectSettings);
    EDI_CHECK(constructionEndpoint.kind == DraftingSnapKind::Object);
    EDI_CHECK(constructionEndpoint.sourceKind == DraftingSnapSourceKind::Endpoint);
    EDI_CHECK(constructionEndpoint.sourceObjectId == "construction_1");
    EDI_CHECK(nearlyEqual(constructionEndpoint.point.x, 0.1));
    EDI_CHECK(nearlyEqual(constructionEndpoint.point.y, 0.9));

    DraftingSnapResult constructionMidpoint = resolveSnap({0.51, 0.91}, document, objectSettings);
    EDI_CHECK(constructionMidpoint.kind == DraftingSnapKind::Object);
    EDI_CHECK(constructionMidpoint.sourceKind == DraftingSnapSourceKind::Midpoint);
    EDI_CHECK(constructionMidpoint.sourceObjectId == "construction_1");
    EDI_CHECK(nearlyEqual(constructionMidpoint.point.x, 0.5));
    EDI_CHECK(nearlyEqual(constructionMidpoint.point.y, 0.9));

    DraftingSnapResult dimensionEndpoint = resolveSnap({0.09, 0.71}, document, objectSettings);
    EDI_CHECK(dimensionEndpoint.kind == DraftingSnapKind::Object);
    EDI_CHECK(dimensionEndpoint.sourceKind == DraftingSnapSourceKind::Endpoint);
    EDI_CHECK(dimensionEndpoint.sourceObjectId == "dimension_1");
    EDI_CHECK(nearlyEqual(dimensionEndpoint.point.x, 0.1));
    EDI_CHECK(nearlyEqual(dimensionEndpoint.point.y, 0.7));

    DraftingSnapResult dimensionMidpoint = resolveSnap({0.51, 0.71}, document, objectSettings);
    EDI_CHECK(dimensionMidpoint.kind == DraftingSnapKind::Object);
    EDI_CHECK(dimensionMidpoint.sourceKind == DraftingSnapSourceKind::Midpoint);
    EDI_CHECK(dimensionMidpoint.sourceObjectId == "dimension_1");
    EDI_CHECK(nearlyEqual(dimensionMidpoint.point.x, 0.5));
    EDI_CHECK(nearlyEqual(dimensionMidpoint.point.y, 0.7));

    DraftingSnapSettings prioritySettings = objectSettings;
    prioritySettings.gridEnabled = true;
    prioritySettings.gridStep = 0.25;
    DraftingSnapResult objectFirst = resolveSnap({0.11, 0.09}, document, prioritySettings);
    EDI_CHECK(objectFirst.kind == DraftingSnapKind::Object);
    prioritySettings.objectPriorityBeforeGrid = false;
    DraftingSnapResult gridFirst = resolveSnap({0.11, 0.09}, document, prioritySettings);
    EDI_CHECK(gridFirst.kind == DraftingSnapKind::Grid);

    DraftingSnapSettings guidePrioritySettings = objectSettings;
    guidePrioritySettings.gridEnabled = true;
    guidePrioritySettings.gridStep = 0.25;
    DraftingSnapResult guideFirst = resolveSnap({0.34, 0.74}, document, guidePrioritySettings);
    EDI_CHECK(guideFirst.kind == DraftingSnapKind::Object);
    EDI_CHECK(guideFirst.sourceKind == DraftingSnapSourceKind::Guide);
    guidePrioritySettings.objectPriorityBeforeGrid = false;
    DraftingSnapResult guideAfterGrid = resolveSnap({0.34, 0.74}, document, guidePrioritySettings);
    EDI_CHECK(guideAfterGrid.kind == DraftingSnapKind::Grid);

    DraftingSnapSettings tightTolerance = objectSettings;
    tightTolerance.objectTolerance = 0.005;
    DraftingSnapResult tightMiss = resolveSnap({0.11, 0.09}, document, tightTolerance);
    EDI_CHECK(tightMiss.kind == DraftingSnapKind::None);

    DraftingSnapSettings looseTolerance = objectSettings;
    looseTolerance.objectTolerance = 0.06;
    DraftingSnapResult looseHit = resolveSnap({0.14, 0.14}, document, looseTolerance);
    EDI_CHECK(looseHit.kind == DraftingSnapKind::Object);

    DraftingSnapSettings noSnap;
    DraftingSnapResult none = resolveSnap({2.0, -1.0}, document, noSnap);
    EDI_CHECK(none.kind == DraftingSnapKind::None);
    EDI_CHECK(none.point.x == 1.0);
    EDI_CHECK(none.point.y == 0.0);

    document.objects.front().visible = false;
    DraftingSnapResult hiddenMiss = resolveSnap({0.11, 0.09}, document, objectSettings);
    EDI_CHECK(hiddenMiss.kind == DraftingSnapKind::None);

    DraftingDocument hiddenGuideDocument = makeDraftingDocument("hidden_guide_doc");
    DraftingObject hiddenGuide = object("hidden_guide", DraftingShapeKind::Guide, GuideGeometry{GuideOrientation::Vertical, 0.4});
    hiddenGuide.visible = false;
    EDI_CHECK(addObject(hiddenGuideDocument, hiddenGuide).ok);
    DraftingSnapResult hiddenGuideMiss = resolveSnap({0.41, 0.2}, hiddenGuideDocument, objectSettings);
    EDI_CHECK(hiddenGuideMiss.kind == DraftingSnapKind::None);

    DraftingDocument hiddenLayerGuideDocument = makeDraftingDocument("hidden_layer_guide_doc");
    DraftingLayer hiddenLayer = makeDraftingLayer("hidden", "Hidden", 1);
    hiddenLayer.visible = false;
    hiddenLayerGuideDocument.layers.push_back(hiddenLayer);
    DraftingObject hiddenLayerGuide = object("hidden_layer_guide", DraftingShapeKind::Guide, GuideGeometry{GuideOrientation::Horizontal, 0.4});
    hiddenLayerGuide.layerId = "hidden";
    EDI_CHECK(addObject(hiddenLayerGuideDocument, hiddenLayerGuide).ok);
    DraftingSnapResult hiddenLayerGuideMiss = resolveSnap({0.2, 0.41}, hiddenLayerGuideDocument, objectSettings);
    EDI_CHECK(hiddenLayerGuideMiss.kind == DraftingSnapKind::None);

    DraftingDocument lockedGuideDocument = makeDraftingDocument("locked_guide_doc");
    DraftingObject lockedGuide = object("locked_guide", DraftingShapeKind::Guide, GuideGeometry{GuideOrientation::Vertical, 0.4});
    lockedGuide.locked = true;
    EDI_CHECK(addObject(lockedGuideDocument, lockedGuide).ok);
    DraftingSnapResult lockedGuideHit = resolveSnap({0.41, 0.2}, lockedGuideDocument, objectSettings);
    EDI_CHECK(lockedGuideHit.kind == DraftingSnapKind::Object);
    EDI_CHECK(lockedGuideHit.sourceKind == DraftingSnapSourceKind::Guide);
    EDI_CHECK(lockedGuideHit.sourceObjectId == "locked_guide");
    EDI_CHECK(nearlyEqual(lockedGuideHit.point.x, 0.4));
    EDI_CHECK(nearlyEqual(lockedGuideHit.point.y, 0.2));

    // Intersection snap: where two line SEGMENTS actually cross is a pairwise
    // candidate (it belongs to no single object), reusing the same
    // segmentIntersection the trim/fillet verbs are built on.
    EDI_CHECK(std::string(draftingSnapSourceKindName(DraftingSnapSourceKind::Intersection)) == "intersection");
    {
        DraftingDocument crossDoc = makeDraftingDocument("cross_doc");
        // Cross at (0.6, 0.5) — deliberately NOT a midpoint of either line, so
        // the snap below is the intersection specifically, not a coincidence.
        EDI_CHECK(addObject(crossDoc, object("h", DraftingShapeKind::Line, LineGeometry{{0.2, 0.5}, {0.8, 0.5}})).ok);
        EDI_CHECK(addObject(crossDoc, object("v", DraftingShapeKind::Line, LineGeometry{{0.6, 0.2}, {0.6, 0.9}})).ok);

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
        EDI_CHECK(intersectionCount == 1);
        EDI_CHECK(nearlyEqual(crossing.point.x, 0.6) && nearlyEqual(crossing.point.y, 0.5));

        // A cursor near the crossing snaps onto it specifically.
        DraftingSnapResult snapped = resolveSnap({0.61, 0.49}, crossDoc, snapSettings);
        EDI_CHECK(snapped.kind == DraftingSnapKind::Object);
        EDI_CHECK(snapped.sourceKind == DraftingSnapSourceKind::Intersection);
        EDI_CHECK(nearlyEqual(snapped.point.x, 0.6) && nearlyEqual(snapped.point.y, 0.5));

        // The per-kind flag gates it: disabling intersection snap drops the candidate.
        DraftingSnapSettings noIntersection = snapSettings;
        noIntersection.intersectionEnabled = false;
        for (const DraftingSnapCandidate &candidate : snapCandidatesForDocument(crossDoc, noIntersection)) {
            EDI_CHECK(candidate.sourceKind != DraftingSnapSourceKind::Intersection);
        }

        // Lines whose segments do NOT cross (they'd only meet when extended)
        // produce no intersection candidate — it is segment, not line, crossing.
        DraftingDocument apartDoc = makeDraftingDocument("apart_doc");
        EDI_CHECK(addObject(apartDoc, object("a", DraftingShapeKind::Line, LineGeometry{{0.1, 0.3}, {0.4, 0.3}})).ok);
        EDI_CHECK(addObject(apartDoc, object("b", DraftingShapeKind::Line, LineGeometry{{0.7, 0.1}, {0.7, 0.25}})).ok);
        for (const DraftingSnapCandidate &candidate : snapCandidatesForDocument(apartDoc, snapSettings)) {
            EDI_CHECK(candidate.sourceKind != DraftingSnapSourceKind::Intersection);
        }
    }

    // DR-02: quadrant + nearest-on-curve snap sources.
    EDI_CHECK(std::string(draftingSnapSourceKindName(DraftingSnapSourceKind::Quadrant)) == "quadrant");
    EDI_CHECK(std::string(draftingSnapSourceKindName(DraftingSnapSourceKind::OnCurve)) == "on_curve");
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
        EDI_CHECK(circleQuadrants == 4);
        EDI_CHECK(sawEast && sawNorth && sawWest && sawSouth);

        // Disabling the flag drops the quadrant candidates (center still present).
        DraftingSnapSettings noQuadrant = snapSettings;
        noQuadrant.quadrantEnabled = false;
        for (const DraftingSnapCandidate &c : snapCandidatesForObject(circle, noQuadrant)) {
            EDI_CHECK(c.sourceKind != DraftingSnapSourceKind::Quadrant);
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
            EDI_CHECK((nearlyEqual(c.point.x, 0.7) && nearlyEqual(c.point.y, 0.5))
                || (nearlyEqual(c.point.x, 0.5) && nearlyEqual(c.point.y, 0.7)));
        }
        EDI_CHECK(arcQuadrants == 2);

        // A cursor near a circle's quadrant snaps onto it (Quadrant participates in
        // resolveSnap; the perimeter point beats the far-away center).
        DraftingDocument circleDoc = makeDraftingDocument("circle_doc");
        EDI_CHECK(addObject(circleDoc, circle).ok);
        DraftingSnapResult quadrantSnap = resolveSnap({0.71, 0.5}, circleDoc, snapSettings);
        EDI_CHECK(quadrantSnap.kind == DraftingSnapKind::Object);
        EDI_CHECK(quadrantSnap.sourceKind == DraftingSnapSourceKind::Quadrant);
        EDI_CHECK(nearlyEqual(quadrantSnap.point.x, 0.7) && nearlyEqual(quadrantSnap.point.y, 0.5));
    }
    {
        // OnCurve: a cursor just off a line (away from any keypoint) snaps onto its
        // orthogonal projection.
        DraftingDocument lineDoc = makeDraftingDocument("oncurve_doc");
        EDI_CHECK(addObject(lineDoc, object("seg", DraftingShapeKind::Line, LineGeometry{{0.2, 0.4}, {0.8, 0.4}})).ok);
        DraftingSnapSettings snapSettings;
        snapSettings.objectSnapEnabled = true;
        snapSettings.objectTolerance = 0.05;

        DraftingSnapResult onCurve = resolveSnap({0.35, 0.42}, lineDoc, snapSettings);
        EDI_CHECK(onCurve.kind == DraftingSnapKind::Object);
        EDI_CHECK(onCurve.sourceKind == DraftingSnapSourceKind::OnCurve);
        EDI_CHECK(onCurve.label == "on_curve");
        EDI_CHECK(nearlyEqual(onCurve.point.x, 0.35) && nearlyEqual(onCurve.point.y, 0.4));

        // Disabling the flag suppresses OnCurve — nothing else is in range here.
        DraftingSnapSettings noOnCurve = snapSettings;
        noOnCurve.onCurveEnabled = false;
        EDI_CHECK(resolveSnap({0.35, 0.42}, lineDoc, noOnCurve).kind == DraftingSnapKind::None);

        // PRIORITY: an endpoint beats a COINCIDENT OnCurve. A cursor just past the
        // 'a' end projects (clamped) back onto the endpoint, so the projection and
        // the endpoint coincide — the endpoint wins (OnCurve is the fallback tier).
        DraftingSnapResult endpointWins = resolveSnap({0.18, 0.41}, lineDoc, snapSettings);
        EDI_CHECK(endpointWins.kind == DraftingSnapKind::Object);
        EDI_CHECK(endpointWins.sourceKind == DraftingSnapSourceKind::Endpoint);
        EDI_CHECK(nearlyEqual(endpointWins.point.x, 0.2) && nearlyEqual(endpointWins.point.y, 0.4));
    }

    // DR-03: relative (anchor-dependent) tangent + perpendicular candidates.
    EDI_CHECK(std::string(draftingSnapSourceKindName(DraftingSnapSourceKind::Tangent)) == "tangent");
    EDI_CHECK(std::string(draftingSnapSourceKindName(DraftingSnapSourceKind::Perpendicular)) == "perpendicular");
    {
        // Tangent from an external anchor to a circle: two contacts, each with the
        // anchor→contact segment perpendicular to the radius at the contact.
        DraftingDocument tanDoc = makeDraftingDocument("tangent_doc");
        const Point2D center{0.5, 0.5};
        const double radius = 0.2;
        EDI_CHECK(addObject(tanDoc, object("circle_t", DraftingShapeKind::Circle, CircleGeometry{center, radius})).ok);

        const Point2D anchor{0.9, 0.5}; // d = 0.4 = 2r → contacts at ±60° from the bearing
        std::vector<DraftingSnapCandidate> tangents;
        for (const DraftingSnapCandidate &c : relativeSnapCandidatesForDocument(tanDoc, anchor)) {
            if (c.sourceKind == DraftingSnapSourceKind::Tangent) {
                tangents.push_back(c);
            }
        }
        EDI_CHECK(tangents.size() == 2);
        bool sawUpper = false, sawLower = false;
        for (const DraftingSnapCandidate &c : tangents) {
            // anchor→contact ⊥ center→contact (dot product ≈ 0).
            const double rx = c.point.x - center.x;
            const double ry = c.point.y - center.y;
            const double px = c.point.x - anchor.x;
            const double py = c.point.y - anchor.y;
            EDI_CHECK(std::abs(rx * px + ry * py) < 0.000001);
            // The contact lies on the circle (radius preserved).
            EDI_CHECK(nearlyEqual(std::sqrt(rx * rx + ry * ry), radius));
            EDI_CHECK(nearlyEqual(c.point.x, 0.6)); // both contacts share x = 0.6
            sawUpper = sawUpper || c.point.y > 0.5;
            sawLower = sawLower || c.point.y < 0.5;
        }
        EDI_CHECK(sawUpper && sawLower);

        // The flag gates it.
        DraftingSnapSettings noTangent;
        noTangent.tangentEnabled = false;
        for (const DraftingSnapCandidate &c : relativeSnapCandidatesForDocument(tanDoc, anchor, noTangent)) {
            EDI_CHECK(c.sourceKind != DraftingSnapSourceKind::Tangent);
        }

        // An anchor INSIDE the circle yields no tangent candidate.
        for (const DraftingSnapCandidate &c : relativeSnapCandidatesForDocument(tanDoc, {0.55, 0.5})) {
            EDI_CHECK(c.sourceKind != DraftingSnapSourceKind::Tangent);
        }
    }
    {
        // Perpendicular foot from an anchor onto a known line = orthogonal projection.
        DraftingDocument perpDoc = makeDraftingDocument("perp_doc");
        EDI_CHECK(addObject(perpDoc, object("seg_p", DraftingShapeKind::Line, LineGeometry{{0.2, 0.4}, {0.8, 0.4}})).ok);

        const Point2D anchor{0.5, 0.9};
        std::vector<DraftingSnapCandidate> feet;
        for (const DraftingSnapCandidate &c : relativeSnapCandidatesForDocument(perpDoc, anchor)) {
            if (c.sourceKind == DraftingSnapSourceKind::Perpendicular) {
                feet.push_back(c);
            }
        }
        EDI_CHECK(feet.size() == 1);
        EDI_CHECK(nearlyEqual(feet.front().point.x, 0.5) && nearlyEqual(feet.front().point.y, 0.4));
        // anchor→foot ⊥ the line direction.
        const double dirX = 0.6, dirY = 0.0;
        const double fx = feet.front().point.x - anchor.x;
        const double fy = feet.front().point.y - anchor.y;
        EDI_CHECK(std::abs(fx * dirX + fy * dirY) < 0.000001);

        DraftingSnapSettings noPerp;
        noPerp.perpendicularEnabled = false;
        for (const DraftingSnapCandidate &c : relativeSnapCandidatesForDocument(perpDoc, anchor, noPerp)) {
            EDI_CHECK(c.sourceKind != DraftingSnapSourceKind::Perpendicular);
        }

        // On-segment-only: an anchor whose perpendicular foot would fall BEYOND an
        // endpoint emits NO perpendicular candidate (it is not a genuine
        // perpendicular, and the endpoint is already an Endpoint snap). The line
        // spans x∈[0.2,0.8]; an anchor at x=0.9 projects to t>1 → skipped.
        for (const DraftingSnapCandidate &c : relativeSnapCandidatesForDocument(perpDoc, {0.9, 0.9})) {
            EDI_CHECK(c.sourceKind != DraftingSnapSourceKind::Perpendicular);
        }
    }

    // ── M2-S3: Node snap source ──────────────────────────────────────────────
    // A standalone Point (incl. materialized intersection nodes) should snap as
    // DraftingSnapSourceKind::Node so tools can distinguish a geometry node from
    // a line end. The implementation emits Node BEFORE Endpoint for PointGeometry
    // so that when both flags are on, Endpoint wins via the `<=` last-wins rule
    // in nearestObjectSnap (existing behaviour preserved). Node is exclusively
    // visible when endpointEnabled=false.
    {
        DraftingDocument nodeDoc = makeDraftingDocument("node_doc");

        // --- 1. A cursor near a standalone Point resolves Node when
        //        endpointEnabled=false, nodeEnabled=true. ----------------------
        EDI_CHECK(addObject(nodeDoc, object("node_pt", DraftingShapeKind::Point,
                                          PointGeometry{{0.5, 0.5}})).ok);

        DraftingSnapSettings nodeSettings;
        nodeSettings.objectSnapEnabled = true;
        nodeSettings.objectTolerance   = 0.05;
        nodeSettings.endpointEnabled   = false; // suppress Endpoint so Node surfaces
        nodeSettings.nodeEnabled       = true;

        DraftingSnapResult nodeSnap = resolveSnap({0.51, 0.49}, nodeDoc, nodeSettings);
        EDI_CHECK(nodeSnap.kind       == DraftingSnapKind::Object);
        EDI_CHECK(nodeSnap.sourceKind == DraftingSnapSourceKind::Node);
        EDI_CHECK(nodeSnap.sourceObjectId == "node_pt");
        EDI_CHECK(nearlyEqual(nodeSnap.point.x, 0.5));
        EDI_CHECK(nearlyEqual(nodeSnap.point.y, 0.5));

        // --- 2. nodeEnabled=false → no snap from the Point at all
        //        (endpointEnabled also off, so no Endpoint shadow either). -----
        DraftingSnapSettings suppressNode = nodeSettings;
        suppressNode.nodeEnabled = false;
        DraftingSnapResult noSnap = resolveSnap({0.51, 0.49}, nodeDoc, suppressNode);
        EDI_CHECK(noSnap.kind != DraftingSnapKind::Object);

        // --- 3. When BOTH endpointEnabled and nodeEnabled are on, Endpoint wins
        //        (the `<=` last-wins rule: Node is emitted first, Endpoint second,
        //        so Endpoint replaces Node at equal distance). -------------------
        DraftingSnapSettings bothEnabled = nodeSettings;
        bothEnabled.endpointEnabled = true;
        bothEnabled.nodeEnabled     = true;
        DraftingSnapResult endpointWins = resolveSnap({0.51, 0.49}, nodeDoc, bothEnabled);
        EDI_CHECK(endpointWins.kind       == DraftingSnapKind::Object);
        EDI_CHECK(endpointWins.sourceKind == DraftingSnapSourceKind::Endpoint);

        // --- 4. A Line endpoint at the SAME position beats a Point's Node
        //        (priority preserved). Point added first so its candidates come
        //        before the Line's in the flat list; Line.Endpoint is last → wins.
        //
        //        Document order: node_pt (0.5,0.5) is already in nodeDoc;
        //        add a line whose START endpoint lands at (0.5,0.5). -----------
        EDI_CHECK(addObject(nodeDoc, object("priority_line", DraftingShapeKind::Line,
                                          LineGeometry{{0.5, 0.5}, {0.9, 0.5}})).ok);

        DraftingSnapSettings prioritySettings;
        prioritySettings.objectSnapEnabled = true;
        prioritySettings.objectTolerance   = 0.05;
        prioritySettings.endpointEnabled   = true;  // Line emits Endpoint
        prioritySettings.nodeEnabled       = true;  // Point emits Node (and Endpoint)
        // Candidates in document order: [Point.Node, Point.Endpoint, Line.Endpoint]
        // With `<=` (last equal-distance wins): Line.Endpoint (from priority_line).
        DraftingSnapResult linePriority = resolveSnap({0.5, 0.5}, nodeDoc, prioritySettings);
        EDI_CHECK(linePriority.kind           == DraftingSnapKind::Object);
        EDI_CHECK(linePriority.sourceKind     == DraftingSnapSourceKind::Endpoint);
        EDI_CHECK(linePriority.sourceObjectId == "priority_line");

        // --- 5. A materialized intersection Point (toolProvenance="intersection")
        //        uses the same PointGeometry → same Node snap path. --------------
        DraftingDocument isectDoc = makeDraftingDocument("isect_doc");
        DraftingObject isectPt = makeDraftingObject("isect_1", DraftingShapeKind::Point,
                                     DraftingGeometry{PointGeometry{{0.3, 0.7}}});
        isectPt.metadata.toolProvenance = "intersection";
        isectPt.bounds = computeBounds(isectPt.geometry);
        EDI_CHECK(addObject(isectDoc, isectPt).ok);

        DraftingSnapResult isectSnap = resolveSnap({0.31, 0.71}, isectDoc, nodeSettings);
        EDI_CHECK(isectSnap.kind       == DraftingSnapKind::Object);
        EDI_CHECK(isectSnap.sourceKind == DraftingSnapSourceKind::Node);
        EDI_CHECK(isectSnap.sourceObjectId == "isect_1");
    }

    return 0;
}
