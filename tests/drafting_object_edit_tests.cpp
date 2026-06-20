#include "drafting/DraftingObjectEdit.h"

#include "EdiAssert.h"
#include <cmath>
#include <limits>

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
    DraftingObject point = object("point_1", DraftingShapeKind::Point, PointGeometry{{1.0, 2.0}});
    auto pointHandles = draftingHandlesForObject(point);
    EDI_CHECK(pointHandles.size() == 1);
    EDI_CHECK(pointHandles[0].id == "point_position");
    auto pointPlan = handleEditPlan(point, "point_position", {3.0, 4.0});
    EDI_CHECK(pointPlan.ok);
    auto pointEdit = applyObjectEdit(point, pointPlan.edit);
    EDI_CHECK(pointEdit.ok);
    const auto *editedPoint = std::get_if<PointGeometry>(&pointEdit.geometry);
    EDI_CHECK(editedPoint != nullptr);
    EDI_CHECK(editedPoint->point.x == 3.0);
    EDI_CHECK(editedPoint->point.y == 4.0);
    EDI_CHECK(pointEdit.bounds.x == 3.0);

    DraftingObject line = object("line_1", DraftingShapeKind::Line, LineGeometry{{0.0, 0.0}, {10.0, 10.0}});
    auto lineHandles = draftingHandlesForObject(line);
    EDI_CHECK(lineHandles.size() == 2);
    EDI_CHECK(lineHandles[0].id == "line_start");
    EDI_CHECK(lineHandles[1].id == "line_end");
    auto linePlan = handleEditPlan(line, "line_end", {20.0, 5.0});
    EDI_CHECK(linePlan.ok);
    auto lineEdit = applyObjectEdit(line, linePlan.edit);
    EDI_CHECK(lineEdit.ok);
    const auto *editedLine = std::get_if<LineGeometry>(&lineEdit.geometry);
    EDI_CHECK(editedLine != nullptr);
    EDI_CHECK(editedLine->b.x == 20.0);
    EDI_CHECK(editedLine->b.y == 5.0);
    EDI_CHECK(lineEdit.bounds.width == 20.0);

    DraftingObject circle = object("circle_1", DraftingShapeKind::Circle, CircleGeometry{{5.0, 5.0}, 2.0});
    auto circleHandles = draftingHandlesForObject(circle);
    EDI_CHECK(circleHandles.size() == 2);
    auto radiusPlan = handleEditPlan(circle, "circle_radius", {8.0, 9.0});
    EDI_CHECK(radiusPlan.ok);
    auto radiusEdit = applyObjectEdit(circle, radiusPlan.edit);
    EDI_CHECK(radiusEdit.ok);
    const auto *editedCircle = std::get_if<CircleGeometry>(&radiusEdit.geometry);
    EDI_CHECK(editedCircle != nullptr);
    EDI_CHECK(nearlyEqual(editedCircle->radius, 5.0));

    DraftingObject rect = object("rect_1", DraftingShapeKind::Rectangle, RectangleGeometry{{1.0, 2.0}, 4.0, 3.0});
    auto rectHandles = draftingHandlesForObject(rect);
    EDI_CHECK(rectHandles.size() == 5);
    EDI_CHECK(draftingHandleById(rect, "rect_rotate").role == "rotate");
    auto cornerPlan = handleEditPlan(rect, "rect_se", {8.0, 9.0});
    EDI_CHECK(cornerPlan.ok);
    auto cornerEdit = applyObjectEdit(rect, cornerPlan.edit);
    EDI_CHECK(cornerEdit.ok);
    const auto *editedRect = std::get_if<RectangleGeometry>(&cornerEdit.geometry);
    EDI_CHECK(editedRect != nullptr);
    EDI_CHECK(editedRect->origin.x == 1.0);
    EDI_CHECK(editedRect->origin.y == 2.0);
    EDI_CHECK(editedRect->width == 7.0);
    EDI_CHECK(editedRect->height == 7.0);

    // N4 aspect-lock: the same corner drag with preserveAspect keeps the
    // original 4:3 ratio. The anchored (nw) corner stays put; the larger
    // demanded axis wins and the other is derived.
    auto lockedPlan = handleEditPlan(rect, "rect_se", {8.0, 9.0}, true);
    EDI_CHECK(lockedPlan.ok && lockedPlan.edit.preserveAspect);
    auto lockedEdit = applyObjectEdit(rect, lockedPlan.edit);
    EDI_CHECK(lockedEdit.ok);
    const auto *lockedRect = std::get_if<RectangleGeometry>(&lockedEdit.geometry);
    EDI_CHECK(lockedRect != nullptr);
    EDI_CHECK(lockedRect->origin.x == 1.0 && lockedRect->origin.y == 2.0); // anchor unmoved
    EDI_CHECK(nearlyEqual(lockedRect->width / lockedRect->height, 4.0 / 3.0));
    EDI_CHECK(nearlyEqual(lockedRect->height, 7.0)); // free height kept; width derived
    EDI_CHECK(nearlyEqual(lockedRect->width, 7.0 * 4.0 / 3.0));
    EDI_CHECK(!cornerPlan.edit.preserveAspect); // the plain drag above was unconstrained

    auto rotatePlan = handleEditPlan(rect, "rect_rotate", {3.0, 8.0});
    EDI_CHECK(rotatePlan.ok);
    auto rotateEdit = applyObjectEdit(rect, rotatePlan.edit);
    EDI_CHECK(rotateEdit.ok);
    const auto *rotatedRect = std::get_if<RectangleGeometry>(&rotateEdit.geometry);
    EDI_CHECK(rotatedRect != nullptr);
    EDI_CHECK(nearlyEqual(rotatedRect->rotationDeg, 180.0));

    DraftingObject dimension = object("dimension_1", DraftingShapeKind::Dimension, DimensionGeometry{DimensionKind::Distance, {0.1, 0.2}, {0.5, 0.2}, 0.05});
    auto dimensionHandles = draftingHandlesForObject(dimension);
    EDI_CHECK(dimensionHandles.size() == 3);
    EDI_CHECK(dimensionHandles[0].id == "dimension_start");
    EDI_CHECK(dimensionHandles[1].id == "dimension_end");
    EDI_CHECK(dimensionHandles[2].id == "dimension_offset");
    EDI_CHECK(nearlyEqual(dimensionHandles[2].point.x, 0.3));
    EDI_CHECK(nearlyEqual(dimensionHandles[2].point.y, 0.25));
    EDI_CHECK(dimensionHandles[2].hasAnchor);
    EDI_CHECK(nearlyEqual(dimensionHandles[2].anchor.x, 0.3));
    EDI_CHECK(nearlyEqual(dimensionHandles[2].anchor.y, 0.2));
    auto dimensionEndPlan = handleEditPlan(dimension, "dimension_end", {0.8, 0.4});
    EDI_CHECK(dimensionEndPlan.ok);
    auto dimensionEndEdit = applyObjectEdit(dimension, dimensionEndPlan.edit);
    EDI_CHECK(dimensionEndEdit.ok);
    const auto *editedDimensionEnd = std::get_if<DimensionGeometry>(&dimensionEndEdit.geometry);
    EDI_CHECK(editedDimensionEnd != nullptr);
    EDI_CHECK(nearlyEqual(editedDimensionEnd->b.x, 0.8));
    EDI_CHECK(nearlyEqual(editedDimensionEnd->b.y, 0.4));
    auto dimensionOffsetPlan = handleEditPlan(dimension, "dimension_offset", {0.3, 0.35});
    EDI_CHECK(dimensionOffsetPlan.ok);
    auto dimensionOffsetEdit = applyObjectEdit(dimension, dimensionOffsetPlan.edit);
    EDI_CHECK(dimensionOffsetEdit.ok);
    const auto *editedDimensionOffset = std::get_if<DimensionGeometry>(&dimensionOffsetEdit.geometry);
    EDI_CHECK(editedDimensionOffset != nullptr);
    EDI_CHECK(nearlyEqual(editedDimensionOffset->offset, 0.15));

    DraftingObject widthDimension = object("dimension_width", DraftingShapeKind::Dimension, DimensionGeometry{DimensionKind::Width, {0.1, 0.2}, {0.5, 0.2}, 0.05});
    auto widthEndPlan = handleEditPlan(widthDimension, "dimension_end", {0.8, 0.9});
    EDI_CHECK(widthEndPlan.ok);
    auto widthEndEdit = applyObjectEdit(widthDimension, widthEndPlan.edit);
    EDI_CHECK(widthEndEdit.ok);
    const auto *editedWidthDimension = std::get_if<DimensionGeometry>(&widthEndEdit.geometry);
    EDI_CHECK(editedWidthDimension != nullptr);
    EDI_CHECK(editedWidthDimension->kind == DimensionKind::Width);
    EDI_CHECK(nearlyEqual(editedWidthDimension->b.x, 0.8));
    EDI_CHECK(nearlyEqual(editedWidthDimension->b.y, 0.2));

    DraftingObject heightDimension = object("dimension_height", DraftingShapeKind::Dimension, DimensionGeometry{DimensionKind::Height, {0.1, 0.2}, {0.1, 0.5}, 0.05});
    auto heightEndPlan = handleEditPlan(heightDimension, "dimension_end", {0.8, 0.9});
    EDI_CHECK(heightEndPlan.ok);
    auto heightEndEdit = applyObjectEdit(heightDimension, heightEndPlan.edit);
    EDI_CHECK(heightEndEdit.ok);
    const auto *editedHeightDimension = std::get_if<DimensionGeometry>(&heightEndEdit.geometry);
    EDI_CHECK(editedHeightDimension != nullptr);
    EDI_CHECK(editedHeightDimension->kind == DimensionKind::Height);
    EDI_CHECK(nearlyEqual(editedHeightDimension->b.x, 0.1));
    EDI_CHECK(nearlyEqual(editedHeightDimension->b.y, 0.9));

    DraftingObject polygon = object("polygon_1", DraftingShapeKind::Polygon, PolygonGeometry{{{0.0, 0.0}, {1.0, 0.0}, {1.0, 1.0}}});
    auto polygonHandles = draftingHandlesForObject(polygon);
    EDI_CHECK(polygonHandles.size() == 3);
    EDI_CHECK(polygonHandles[0].id == "vertex_0");
    EDI_CHECK(polygonHandles[0].role == "vertex");
    EDI_CHECK(polygonHandles[0].readOnly);
    EDI_CHECK(!handleEditPlan(polygon, "vertex_0", {0.5, 0.5}).ok);

    auto badPointPlan = handleEditPlan(point, "point_position", {std::numeric_limits<double>::infinity(), 0.0});
    EDI_CHECK(!badPointPlan.ok);

    return 0;
}
