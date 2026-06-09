#include "drafting/DraftingSnap.h"
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
    assert(addObject(document, object("construction_1", DraftingShapeKind::ConstructionLine, ConstructionLineGeometry{{0.1, 0.9}, {0.9, 0.9}})).ok);
    assert(addObject(document, object("dimension_1", DraftingShapeKind::Dimension, DimensionGeometry{{0.1, 0.7}, {0.9, 0.7}, 0.04})).ok);

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

    DraftingSnapResult guideCenter = resolveSnap({0.51, 0.74}, document, objectSettings);
    assert(guideCenter.kind == DraftingSnapKind::Object);
    assert(guideCenter.sourceKind == DraftingSnapSourceKind::Center);
    assert(guideCenter.sourceObjectId == "guide_1");
    assert(nearlyEqual(guideCenter.point.x, 0.5));
    assert(nearlyEqual(guideCenter.point.y, 0.75));

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

    return 0;
}
