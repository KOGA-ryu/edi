#include "drafting/DraftingPlotBounds.h"
#include "drafting/DraftingStore.h"

#include <cassert>
#include <cmath>
#include <string>

using namespace edi::drafting;

namespace {

DraftingObject makeObject(std::string id, DraftingShapeKind kind, DraftingGeometry geometry)
{
    auto built = buildDraftingObject(std::move(id), kind, std::move(geometry));
    assert(built.ok);
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
    assert(nearlyEqual(bounds.x, x));
    assert(nearlyEqual(bounds.y, y));
    assert(nearlyEqual(bounds.width, width));
    assert(nearlyEqual(bounds.height, height));
}

} // namespace

int main()
{
    const DraftingGridProjection grid = plotGrid();

    DraftingDocument pointDocument = makeDraftingDocument("point_bounds_doc");
    assert(addObject(pointDocument, makeObject("point_1", DraftingShapeKind::Point, PointGeometry{{0.5, 0.5}})).ok);
    const DraftingPlotBoundsResult pointBounds = selectedRawPlotOutputBounds(pointDocument, {"point_1"}, grid);
    assert(pointBounds.ok);
    assertBounds(pointBounds.bounds, 0.495, 0.495, 0.01, 0.01);
    assert(pointBounds.status == DraftingPlotBoundsStatus::InsideDrawable);
    assert(std::string(draftingPlotBoundsStatusName(pointBounds.status)) == "inside");

    DraftingDocument lineDocument = makeDraftingDocument("line_bounds_doc");
    assert(addObject(lineDocument, makeObject("line_1", DraftingShapeKind::Line, LineGeometry{{0.2, 0.3}, {0.4, 0.5}})).ok);
    const DraftingPlotBoundsResult lineBounds = selectedRawPlotOutputBounds(lineDocument, {"line_1"}, grid);
    assert(lineBounds.ok);
    assertBounds(lineBounds.bounds, 0.2, 0.3, 0.2, 0.2);

    DraftingDocument combinedDocument = makeDraftingDocument("combined_bounds_doc");
    assert(addObject(combinedDocument, makeObject("point_1", DraftingShapeKind::Point, PointGeometry{{0.5, 0.5}})).ok);
    assert(addObject(combinedDocument, makeObject("line_1", DraftingShapeKind::Line, LineGeometry{{0.2, 0.3}, {0.4, 0.5}})).ok);
    const DraftingPlotBoundsResult combinedBounds = selectedRawPlotOutputBounds(combinedDocument, {"point_1", "line_1"}, grid);
    assert(combinedBounds.ok);
    assertBounds(combinedBounds.bounds, 0.2, 0.3, 0.305, 0.205);

    const DraftingPlotBoundsResult allBounds = rawPlotOutputBounds(combinedDocument, grid);
    assert(allBounds.ok);
    assertBounds(allBounds.bounds, 0.2, 0.3, 0.305, 0.205);

    DraftingDocument rejectedDocument = makeDraftingDocument("rejected_bounds_doc");
    assert(addObject(rejectedDocument, makeObject("guide_1", DraftingShapeKind::Guide, GuideGeometry{GuideOrientation::Horizontal, 0.25})).ok);
    assert(!selectedRawPlotOutputBounds(rejectedDocument, {"guide_1"}, grid).ok);
    assert(!selectedRawPlotOutputBounds(rejectedDocument, {"missing_1"}, grid).ok);

    DraftingObject lockedLine = makeObject("locked_line", DraftingShapeKind::Line, LineGeometry{{0.3, 0.3}, {0.4, 0.4}});
    lockedLine.locked = true;
    assert(addObject(rejectedDocument, lockedLine).ok);
    assert(!selectedRawPlotOutputBounds(rejectedDocument, {"locked_line"}, grid).ok);

    DraftingDocument outsideDocument = makeDraftingDocument("outside_bounds_doc");
    assert(addObject(outsideDocument, makeObject("outside_point", DraftingShapeKind::Point, PointGeometry{{0.0, 0.0}})).ok);
    const DraftingPlotBoundsResult outsideBounds = selectedRawPlotOutputBounds(outsideDocument, {"outside_point"}, grid);
    assert(outsideBounds.ok);
    assert(outsideBounds.status == DraftingPlotBoundsStatus::OutsideDrawable);
    assert(std::string(draftingPlotBoundsStatusName(outsideBounds.status)) == "outside");
    assert(!boundsInsideDrawable(outsideBounds.bounds, grid.drawableBounds));

    assert(!selectedRawPlotOutputBounds(outsideDocument, {}, grid).ok);
    assert(std::string(draftingPlotBoundsStatusName(DraftingPlotBoundsStatus::Unavailable)) == "unavailable");
    assertBounds(translateBounds({0.1, 0.2, 0.3, 0.4}, 0.05, -0.1), 0.15, 0.1, 0.3, 0.4);

    return 0;
}
