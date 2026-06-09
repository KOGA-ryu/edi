#include "drafting/DraftingQuickMeasure.h"

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

DraftingGridProjection physicalGrid()
{
    DraftingGridSettings settings = defaultDraftingGridSettings();
    settings.width = 12.0;
    settings.height = 8.0;
    settings.unit = DraftingGridUnit::Inch;
    return projectDraftingGrid(settings);
}

} // namespace

int main()
{
    DraftingGridProjection grid = physicalGrid();

    DraftingDocument empty = makeDraftingDocument("empty_measure_doc");
    DraftingQuickMeasureResult emptyMeasure = quickMeasureAt(empty, {0.5, 0.5}, grid);
    assert(!emptyMeasure.ok);
    assert(emptyMeasure.kind == DraftingQuickMeasureKind::None);
    assert(emptyMeasure.code == DraftingResultCode::ObjectNotFound);
    assert(emptyMeasure.message == "no measurable target");
    assert(emptyMeasure.unit == DraftingGridUnit::Inch);
    assert(emptyMeasure.unitName == "inch");
    assert(emptyMeasure.unitLabel == "in");

    DraftingDocument document = makeDraftingDocument("measure_doc");
    assert(addObject(document, object("line_1", DraftingShapeKind::Line, LineGeometry{{0.25, 0.25}, {0.75, 0.75}})).ok);
    assert(addObject(document, object("circle_1", DraftingShapeKind::Circle, CircleGeometry{{0.25, 0.75}, 0.1})).ok);
    assert(addObject(document, object("rect_1", DraftingShapeKind::Rectangle, RectangleGeometry{{0.65, 0.1}, 0.2, 0.3})).ok);
    assert(addObject(document, object("point_1", DraftingShapeKind::Point, PointGeometry{{0.1, 0.1}})).ok);
    assert(addObject(document, object("dimension_1", DraftingShapeKind::Dimension, DimensionGeometry{DimensionKind::Distance, {0.1, 0.9}, {0.5, 0.9}, 0.05})).ok);
    assert(addObject(document, object("guide_1", DraftingShapeKind::Guide, GuideGeometry{GuideOrientation::Vertical, 0.92})).ok);

    const std::uint64_t revisionBefore = document.revision;
    const std::size_t objectCountBefore = document.objects.size();

    DraftingQuickMeasureResult lineMeasure = quickMeasureAt(document, {0.5, 0.5}, grid);
    assert(lineMeasure.ok);
    assert(lineMeasure.kind == DraftingQuickMeasureKind::Line);
    assert(lineMeasure.objectId == "line_1");
    assert(nearlyEqual(lineMeasure.length, std::sqrt(0.5)));
    assert(nearlyEqual(lineMeasure.physicalLength, std::sqrt(4.0 * 4.0 + 6.0 * 6.0)));
    assert(nearlyEqual(lineMeasure.angleDeg, 45.0));
    assert(nearlyEqual(lineMeasure.physicalAngleDeg, 33.69006752598));
    assert(lineMeasure.label == "line 7.2111 in @ 33.6901 deg");

    DraftingQuickMeasureResult circleMeasure = quickMeasureAt(document, {0.35, 0.75}, grid);
    assert(circleMeasure.ok);
    assert(circleMeasure.kind == DraftingQuickMeasureKind::Circle);
    assert(circleMeasure.objectId == "circle_1");
    assert(nearlyEqual(circleMeasure.radius, 0.1));
    assert(nearlyEqual(circleMeasure.diameter, 0.2));
    assert(nearlyEqual(circleMeasure.physicalRadius, 1.2));
    assert(nearlyEqual(circleMeasure.physicalDiameter, 2.4));
    assert(nearlyEqual(circleMeasure.physicalRadiusY, 0.8));
    assert(nearlyEqual(circleMeasure.physicalDiameterY, 1.6));

    DraftingQuickMeasureResult rectMeasure = quickMeasureAt(document, {0.7, 0.2}, grid);
    assert(rectMeasure.ok);
    assert(rectMeasure.kind == DraftingQuickMeasureKind::Rectangle);
    assert(rectMeasure.objectId == "rect_1");
    assert(nearlyEqual(rectMeasure.width, 0.2));
    assert(nearlyEqual(rectMeasure.height, 0.3));
    assert(nearlyEqual(rectMeasure.area, 0.06));
    assert(nearlyEqual(rectMeasure.physicalWidth, 2.4));
    assert(nearlyEqual(rectMeasure.physicalHeight, 2.4));
    assert(nearlyEqual(rectMeasure.physicalArea, 5.76));

    DraftingQuickMeasureResult pointMeasure = quickMeasureAt(document, {0.1, 0.1}, grid);
    assert(pointMeasure.ok);
    assert(pointMeasure.kind == DraftingQuickMeasureKind::Point);
    assert(pointMeasure.objectId == "point_1");
    assert(nearlyEqual(pointMeasure.x, 0.1));
    assert(nearlyEqual(pointMeasure.y, 0.1));
    assert(nearlyEqual(pointMeasure.physicalX, 1.2));
    assert(nearlyEqual(pointMeasure.physicalY, 0.8));

    DraftingQuickMeasureResult dimensionMeasure = quickMeasureAt(document, {0.3, 0.95}, grid);
    assert(dimensionMeasure.ok);
    assert(dimensionMeasure.kind == DraftingQuickMeasureKind::Dimension);
    assert(dimensionMeasure.objectId == "dimension_1");
    assert(dimensionMeasure.dimensionKind == DimensionKind::Distance);
    assert(nearlyEqual(dimensionMeasure.length, 0.4));
    assert(nearlyEqual(dimensionMeasure.displayedLength, 0.4));
    assert(nearlyEqual(dimensionMeasure.physicalLength, 4.8));
    assert(nearlyEqual(dimensionMeasure.physicalDisplayedLength, 4.8));
    assert(nearlyEqual(dimensionMeasure.angleDeg, 0.0));
    assert(nearlyEqual(dimensionMeasure.physicalAngleDeg, 0.0));
    assert(nearlyEqual(dimensionMeasure.offset, 0.05));
    assert(nearlyEqual(dimensionMeasure.physicalOffset, 0.4));
    assert(dimensionMeasure.label == "dimension distance 4.8 in @ 0 deg");

    DraftingDocument diameterDocument = makeDraftingDocument("diameter_measure_doc");
    assert(addObject(diameterDocument, object("diameter_dimension_1", DraftingShapeKind::Dimension, DimensionGeometry{DimensionKind::Diameter, {0.1, 0.5}, {0.3, 0.5}, 0.05})).ok);
    DraftingQuickMeasureResult diameterMeasure = quickMeasureAt(diameterDocument, {0.2, 0.55}, grid);
    assert(diameterMeasure.ok);
    assert(diameterMeasure.kind == DraftingQuickMeasureKind::Dimension);
    assert(diameterMeasure.dimensionKind == DimensionKind::Diameter);
    assert(nearlyEqual(diameterMeasure.length, 0.2));
    assert(nearlyEqual(diameterMeasure.displayedLength, 0.4));
    assert(nearlyEqual(diameterMeasure.physicalLength, 2.4));
    assert(nearlyEqual(diameterMeasure.physicalDisplayedLength, 4.8));

    DraftingQuickMeasureResult guideMeasure = quickMeasureAt(document, {0.92, 0.5}, grid);
    assert(!guideMeasure.ok);
    assert(guideMeasure.kind == DraftingQuickMeasureKind::Unsupported);
    assert(guideMeasure.objectId == "guide_1");
    assert(guideMeasure.message == "target has no quick measurement yet");

    assert(document.revision == revisionBefore);
    assert(document.objects.size() == objectCountBefore);

    assert(draftingQuickMeasureKindName(DraftingQuickMeasureKind::None) == std::string("none"));
    assert(draftingQuickMeasureKindName(DraftingQuickMeasureKind::Point) == std::string("point"));
    assert(draftingQuickMeasureKindName(DraftingQuickMeasureKind::Line) == std::string("line"));
    assert(draftingQuickMeasureKindName(DraftingQuickMeasureKind::Rectangle) == std::string("rectangle"));
    assert(draftingQuickMeasureKindName(DraftingQuickMeasureKind::Circle) == std::string("circle"));
    assert(draftingQuickMeasureKindName(DraftingQuickMeasureKind::Dimension) == std::string("dimension"));
    assert(draftingQuickMeasureKindName(DraftingQuickMeasureKind::Unsupported) == std::string("unsupported"));

    return 0;
}
