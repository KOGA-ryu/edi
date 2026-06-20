#include "drafting/DraftingPlotBounds.h"
#include "drafting/DraftingStore.h"

#include "EdiAssert.h"
#include <cmath>
#include <string>

using namespace edi::drafting;

namespace {

DraftingObject makeObject(std::string id, DraftingShapeKind kind, DraftingGeometry geometry)
{
    auto built = buildDraftingObject(std::move(id), kind, std::move(geometry));
    EDI_CHECK(built.ok);
    return built.object;
}

DraftingGridProjection plotGrid()
{
    DraftingGridSettings settings = defaultDraftingGridSettings();
    settings.marginLeft = 0.1 * settings.width;
    settings.marginTop = 0.1 * settings.height;
    settings.marginRight = 0.1 * settings.width;
    settings.marginBottom = 0.1 * settings.height;
    return projectDraftingGrid(settings);
}

bool nearlyEqual(double a, double b)
{
    return std::abs(a - b) < 0.000001;
}

void assertBounds(Bounds2D bounds, double x, double y, double width, double height)
{
    EDI_CHECK(nearlyEqual(bounds.x, x));
    EDI_CHECK(nearlyEqual(bounds.y, y));
    EDI_CHECK(nearlyEqual(bounds.width, width));
    EDI_CHECK(nearlyEqual(bounds.height, height));
}

} // namespace

int main()
{
    const DraftingGridProjection grid = plotGrid();

    DraftingDocument pointDocument = makeDraftingDocument("point_bounds_doc");
    EDI_CHECK(addObject(pointDocument, makeObject("point_1", DraftingShapeKind::Point, PointGeometry{{0.5, 0.5}})).ok);
    const DraftingPlotBoundsResult pointBounds = selectedRawPlotOutputBounds(pointDocument, {"point_1"}, grid);
    EDI_CHECK(pointBounds.ok);
    assertBounds(pointBounds.bounds, 0.495, 0.495, 0.01, 0.01);
    EDI_CHECK(pointBounds.status == DraftingPlotBoundsStatus::InsideDrawable);
    EDI_CHECK(pointBounds.relation == DraftingDrawableBoundsRelation::Inside);
    EDI_CHECK(std::string(draftingPlotBoundsStatusName(pointBounds.status)) == "inside");
    EDI_CHECK(std::string(draftingDrawableBoundsRelationName(pointBounds.relation)) == "inside");

    DraftingDocument lineDocument = makeDraftingDocument("line_bounds_doc");
    EDI_CHECK(addObject(lineDocument, makeObject("line_1", DraftingShapeKind::Line, LineGeometry{{0.2, 0.3}, {0.4, 0.5}})).ok);
    const DraftingPlotBoundsResult lineBounds = selectedRawPlotOutputBounds(lineDocument, {"line_1"}, grid);
    EDI_CHECK(lineBounds.ok);
    assertBounds(lineBounds.bounds, 0.2, 0.3, 0.2, 0.2);

    DraftingDocument combinedDocument = makeDraftingDocument("combined_bounds_doc");
    EDI_CHECK(addObject(combinedDocument, makeObject("point_1", DraftingShapeKind::Point, PointGeometry{{0.5, 0.5}})).ok);
    EDI_CHECK(addObject(combinedDocument, makeObject("line_1", DraftingShapeKind::Line, LineGeometry{{0.2, 0.3}, {0.4, 0.5}})).ok);
    const DraftingPlotBoundsResult combinedBounds = selectedRawPlotOutputBounds(combinedDocument, {"point_1", "line_1"}, grid);
    EDI_CHECK(combinedBounds.ok);
    assertBounds(combinedBounds.bounds, 0.2, 0.3, 0.305, 0.205);

    const DraftingPlotBoundsResult allBounds = rawPlotOutputBounds(combinedDocument, grid);
    EDI_CHECK(allBounds.ok);
    assertBounds(allBounds.bounds, 0.2, 0.3, 0.305, 0.205);

    DraftingDocument rejectedDocument = makeDraftingDocument("rejected_bounds_doc");
    EDI_CHECK(addObject(rejectedDocument, makeObject("guide_1", DraftingShapeKind::Guide, GuideGeometry{GuideOrientation::Horizontal, 0.25})).ok);
    EDI_CHECK(!selectedRawPlotOutputBounds(rejectedDocument, {"guide_1"}, grid).ok);
    EDI_CHECK(!selectedRawPlotOutputBounds(rejectedDocument, {"missing_1"}, grid).ok);

    DraftingObject lockedLine = makeObject("locked_line", DraftingShapeKind::Line, LineGeometry{{0.3, 0.3}, {0.4, 0.4}});
    lockedLine.locked = true;
    EDI_CHECK(addObject(rejectedDocument, lockedLine).ok);
    EDI_CHECK(!selectedRawPlotOutputBounds(rejectedDocument, {"locked_line"}, grid).ok);

    DraftingDocument outsideDocument = makeDraftingDocument("outside_bounds_doc");
    EDI_CHECK(addObject(outsideDocument, makeObject("outside_point", DraftingShapeKind::Point, PointGeometry{{0.0, 0.0}})).ok);
    const DraftingPlotBoundsResult outsideBounds = selectedRawPlotOutputBounds(outsideDocument, {"outside_point"}, grid);
    EDI_CHECK(outsideBounds.ok);
    EDI_CHECK(outsideBounds.status == DraftingPlotBoundsStatus::OutsideDrawable);
    EDI_CHECK(outsideBounds.relation == DraftingDrawableBoundsRelation::FullyOutside);
    EDI_CHECK(std::string(draftingPlotBoundsStatusName(outsideBounds.status)) == "outside");
    EDI_CHECK(!boundsInsideDrawable(outsideBounds.bounds, grid.drawableBounds));

    EDI_CHECK(classifyBoundsAgainstDrawable({0.2, 0.2, 0.1, 0.1}, grid.drawableBounds) == DraftingDrawableBoundsRelation::Inside);
    EDI_CHECK(classifyBoundsAgainstDrawable({0.0, 0.2, 0.2, 0.1}, grid.drawableBounds) == DraftingDrawableBoundsRelation::PartiallyOutside);
    EDI_CHECK(classifyBoundsAgainstDrawable({1.2, 1.2, 0.1, 0.1}, grid.drawableBounds) == DraftingDrawableBoundsRelation::FullyOutside);
    EDI_CHECK(classifyBoundsAgainstDrawable({0.0, 0.2, 1.0, 0.1}, grid.drawableBounds) == DraftingDrawableBoundsRelation::TooLarge);
    EDI_CHECK(std::string(draftingDrawableBoundsRelationName(DraftingDrawableBoundsRelation::FullyOutside)) == "fully_outside");
    EDI_CHECK(std::string(draftingDrawableBoundsRelationName(DraftingDrawableBoundsRelation::TooLarge)) == "too_large");
    EDI_CHECK(!selectedRawPlotOutputBounds(outsideDocument, {}, grid).ok);
    EDI_CHECK(std::string(draftingPlotBoundsStatusName(DraftingPlotBoundsStatus::Unavailable)) == "unavailable");
    assertBounds(translateBounds({0.1, 0.2, 0.3, 0.4}, 0.05, -0.1), 0.15, 0.1, 0.3, 0.4);

    return 0;
}
