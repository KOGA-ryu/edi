#include "drafting/DraftingQuickMeasure.h"

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
    EDI_CHECK(!emptyMeasure.ok);
    EDI_CHECK(emptyMeasure.kind == DraftingQuickMeasureKind::None);
    EDI_CHECK(emptyMeasure.code == DraftingResultCode::ObjectNotFound);
    EDI_CHECK(emptyMeasure.message == "no measurable target");
    EDI_CHECK(emptyMeasure.unit == DraftingGridUnit::Inch);
    EDI_CHECK(emptyMeasure.unitName == "inch");
    EDI_CHECK(emptyMeasure.unitLabel == "in");

    DraftingDocument document = makeDraftingDocument("measure_doc");
    EDI_CHECK(addObject(document, object("line_1", DraftingShapeKind::Line, LineGeometry{{0.25, 0.25}, {0.75, 0.75}})).ok);
    EDI_CHECK(addObject(document, object("circle_1", DraftingShapeKind::Circle, CircleGeometry{{0.25, 0.75}, 0.1})).ok);
    EDI_CHECK(addObject(document, object("rect_1", DraftingShapeKind::Rectangle, RectangleGeometry{{0.65, 0.1}, 0.2, 0.3})).ok);
    EDI_CHECK(addObject(document, object("point_1", DraftingShapeKind::Point, PointGeometry{{0.1, 0.1}})).ok);
    EDI_CHECK(addObject(document, object("dimension_1", DraftingShapeKind::Dimension, DimensionGeometry{DimensionKind::Distance, {0.1, 0.9}, {0.5, 0.9}, 0.05})).ok);
    EDI_CHECK(addObject(document, object("guide_1", DraftingShapeKind::Guide, GuideGeometry{GuideOrientation::Vertical, 0.92})).ok);

    const std::uint64_t revisionBefore = document.revision;
    const std::size_t objectCountBefore = document.objects.size();

    DraftingQuickMeasureResult lineMeasure = quickMeasureAt(document, {0.5, 0.5}, grid);
    EDI_CHECK(lineMeasure.ok);
    EDI_CHECK(lineMeasure.kind == DraftingQuickMeasureKind::Line);
    EDI_CHECK(lineMeasure.objectId == "line_1");
    EDI_CHECK(nearlyEqual(lineMeasure.length, std::sqrt(0.5)));
    EDI_CHECK(nearlyEqual(lineMeasure.physicalLength, std::sqrt(4.0 * 4.0 + 6.0 * 6.0)));
    EDI_CHECK(nearlyEqual(lineMeasure.angleDeg, 45.0));
    EDI_CHECK(nearlyEqual(lineMeasure.physicalAngleDeg, 33.69006752598));
    EDI_CHECK(lineMeasure.label == "line 7.2111 in @ 33.6901 deg");

    DraftingQuickMeasureResult circleMeasure = quickMeasureAt(document, {0.35, 0.75}, grid);
    EDI_CHECK(circleMeasure.ok);
    EDI_CHECK(circleMeasure.kind == DraftingQuickMeasureKind::Circle);
    EDI_CHECK(circleMeasure.objectId == "circle_1");
    EDI_CHECK(nearlyEqual(circleMeasure.radius, 0.1));
    EDI_CHECK(nearlyEqual(circleMeasure.diameter, 0.2));
    EDI_CHECK(nearlyEqual(circleMeasure.physicalRadius, 1.2));
    EDI_CHECK(nearlyEqual(circleMeasure.physicalDiameter, 2.4));
    EDI_CHECK(nearlyEqual(circleMeasure.physicalRadiusY, 0.8));
    EDI_CHECK(nearlyEqual(circleMeasure.physicalDiameterY, 1.6));

    DraftingQuickMeasureResult rectMeasure = quickMeasureAt(document, {0.7, 0.2}, grid);
    EDI_CHECK(rectMeasure.ok);
    EDI_CHECK(rectMeasure.kind == DraftingQuickMeasureKind::Rectangle);
    EDI_CHECK(rectMeasure.objectId == "rect_1");
    EDI_CHECK(nearlyEqual(rectMeasure.width, 0.2));
    EDI_CHECK(nearlyEqual(rectMeasure.height, 0.3));
    EDI_CHECK(nearlyEqual(rectMeasure.area, 0.06));
    EDI_CHECK(nearlyEqual(rectMeasure.physicalWidth, 2.4));
    EDI_CHECK(nearlyEqual(rectMeasure.physicalHeight, 2.4));
    EDI_CHECK(nearlyEqual(rectMeasure.physicalArea, 5.76));

    DraftingQuickMeasureResult pointMeasure = quickMeasureAt(document, {0.1, 0.1}, grid);
    EDI_CHECK(pointMeasure.ok);
    EDI_CHECK(pointMeasure.kind == DraftingQuickMeasureKind::Point);
    EDI_CHECK(pointMeasure.objectId == "point_1");
    EDI_CHECK(nearlyEqual(pointMeasure.x, 0.1));
    EDI_CHECK(nearlyEqual(pointMeasure.y, 0.1));
    EDI_CHECK(nearlyEqual(pointMeasure.physicalX, 1.2));
    EDI_CHECK(nearlyEqual(pointMeasure.physicalY, 0.8));

    DraftingQuickMeasureResult dimensionMeasure = quickMeasureAt(document, {0.3, 0.95}, grid);
    EDI_CHECK(dimensionMeasure.ok);
    EDI_CHECK(dimensionMeasure.kind == DraftingQuickMeasureKind::Dimension);
    EDI_CHECK(dimensionMeasure.objectId == "dimension_1");
    EDI_CHECK(dimensionMeasure.dimensionKind == DimensionKind::Distance);
    EDI_CHECK(nearlyEqual(dimensionMeasure.length, 0.4));
    EDI_CHECK(nearlyEqual(dimensionMeasure.displayedLength, 0.4));
    EDI_CHECK(nearlyEqual(dimensionMeasure.physicalLength, 4.8));
    EDI_CHECK(nearlyEqual(dimensionMeasure.physicalDisplayedLength, 4.8));
    EDI_CHECK(nearlyEqual(dimensionMeasure.angleDeg, 0.0));
    EDI_CHECK(nearlyEqual(dimensionMeasure.physicalAngleDeg, 0.0));
    EDI_CHECK(nearlyEqual(dimensionMeasure.offset, 0.05));
    EDI_CHECK(nearlyEqual(dimensionMeasure.physicalOffset, 0.4));
    EDI_CHECK(dimensionMeasure.label == "dimension distance 4.8 in @ 0 deg");

    DraftingDocument diameterDocument = makeDraftingDocument("diameter_measure_doc");
    EDI_CHECK(addObject(diameterDocument, object("diameter_dimension_1", DraftingShapeKind::Dimension, DimensionGeometry{DimensionKind::Diameter, {0.1, 0.5}, {0.3, 0.5}, 0.05})).ok);
    DraftingQuickMeasureResult diameterMeasure = quickMeasureAt(diameterDocument, {0.2, 0.55}, grid);
    EDI_CHECK(diameterMeasure.ok);
    EDI_CHECK(diameterMeasure.kind == DraftingQuickMeasureKind::Dimension);
    EDI_CHECK(diameterMeasure.dimensionKind == DimensionKind::Diameter);
    EDI_CHECK(nearlyEqual(diameterMeasure.length, 0.2));
    EDI_CHECK(nearlyEqual(diameterMeasure.displayedLength, 0.4));
    EDI_CHECK(nearlyEqual(diameterMeasure.physicalLength, 2.4));
    EDI_CHECK(nearlyEqual(diameterMeasure.physicalDisplayedLength, 4.8));

    DraftingQuickMeasureResult guideMeasure = quickMeasureAt(document, {0.92, 0.5}, grid);
    EDI_CHECK(!guideMeasure.ok);
    EDI_CHECK(guideMeasure.kind == DraftingQuickMeasureKind::Unsupported);
    EDI_CHECK(guideMeasure.objectId == "guide_1");
    EDI_CHECK(guideMeasure.message == "target has no quick measurement yet");

    EDI_CHECK(document.revision == revisionBefore);
    EDI_CHECK(document.objects.size() == objectCountBefore);

    EDI_CHECK(draftingQuickMeasureKindName(DraftingQuickMeasureKind::None) == std::string("none"));
    EDI_CHECK(draftingQuickMeasureKindName(DraftingQuickMeasureKind::Point) == std::string("point"));
    EDI_CHECK(draftingQuickMeasureKindName(DraftingQuickMeasureKind::Line) == std::string("line"));
    EDI_CHECK(draftingQuickMeasureKindName(DraftingQuickMeasureKind::Rectangle) == std::string("rectangle"));
    EDI_CHECK(draftingQuickMeasureKindName(DraftingQuickMeasureKind::Circle) == std::string("circle"));
    EDI_CHECK(draftingQuickMeasureKindName(DraftingQuickMeasureKind::Dimension) == std::string("dimension"));
    EDI_CHECK(draftingQuickMeasureKindName(DraftingQuickMeasureKind::Unsupported) == std::string("unsupported"));

    return 0;
}
