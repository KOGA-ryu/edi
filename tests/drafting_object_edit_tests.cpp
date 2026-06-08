#include "drafting/DraftingObjectEdit.h"

#include <cassert>
#include <cmath>
#include <limits>

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
    DraftingObject point = object("point_1", DraftingShapeKind::Point, PointGeometry{{1.0, 2.0}});
    auto pointHandles = draftingHandlesForObject(point);
    assert(pointHandles.size() == 1);
    assert(pointHandles[0].id == "point_position");
    auto pointPlan = handleEditPlan(point, "point_position", {3.0, 4.0});
    assert(pointPlan.ok);
    auto pointEdit = applyObjectEdit(point, pointPlan.edit);
    assert(pointEdit.ok);
    const auto *editedPoint = std::get_if<PointGeometry>(&pointEdit.geometry);
    assert(editedPoint != nullptr);
    assert(editedPoint->point.x == 3.0);
    assert(editedPoint->point.y == 4.0);
    assert(pointEdit.bounds.x == 3.0);

    DraftingObject line = object("line_1", DraftingShapeKind::Line, LineGeometry{{0.0, 0.0}, {10.0, 10.0}});
    auto lineHandles = draftingHandlesForObject(line);
    assert(lineHandles.size() == 2);
    assert(lineHandles[0].id == "line_start");
    assert(lineHandles[1].id == "line_end");
    auto linePlan = handleEditPlan(line, "line_end", {20.0, 5.0});
    assert(linePlan.ok);
    auto lineEdit = applyObjectEdit(line, linePlan.edit);
    assert(lineEdit.ok);
    const auto *editedLine = std::get_if<LineGeometry>(&lineEdit.geometry);
    assert(editedLine != nullptr);
    assert(editedLine->b.x == 20.0);
    assert(editedLine->b.y == 5.0);
    assert(lineEdit.bounds.width == 20.0);

    DraftingObject circle = object("circle_1", DraftingShapeKind::Circle, CircleGeometry{{5.0, 5.0}, 2.0});
    auto circleHandles = draftingHandlesForObject(circle);
    assert(circleHandles.size() == 2);
    auto radiusPlan = handleEditPlan(circle, "circle_radius", {8.0, 9.0});
    assert(radiusPlan.ok);
    auto radiusEdit = applyObjectEdit(circle, radiusPlan.edit);
    assert(radiusEdit.ok);
    const auto *editedCircle = std::get_if<CircleGeometry>(&radiusEdit.geometry);
    assert(editedCircle != nullptr);
    assert(nearlyEqual(editedCircle->radius, 5.0));

    DraftingObject rect = object("rect_1", DraftingShapeKind::Rectangle, RectangleGeometry{{1.0, 2.0}, 4.0, 3.0});
    auto rectHandles = draftingHandlesForObject(rect);
    assert(rectHandles.size() == 5);
    assert(draftingHandleById(rect, "rect_rotate").role == "rotate");
    auto cornerPlan = handleEditPlan(rect, "rect_se", {8.0, 9.0});
    assert(cornerPlan.ok);
    auto cornerEdit = applyObjectEdit(rect, cornerPlan.edit);
    assert(cornerEdit.ok);
    const auto *editedRect = std::get_if<RectangleGeometry>(&cornerEdit.geometry);
    assert(editedRect != nullptr);
    assert(editedRect->origin.x == 1.0);
    assert(editedRect->origin.y == 2.0);
    assert(editedRect->width == 7.0);
    assert(editedRect->height == 7.0);

    auto rotatePlan = handleEditPlan(rect, "rect_rotate", {3.0, 8.0});
    assert(rotatePlan.ok);
    auto rotateEdit = applyObjectEdit(rect, rotatePlan.edit);
    assert(rotateEdit.ok);
    const auto *rotatedRect = std::get_if<RectangleGeometry>(&rotateEdit.geometry);
    assert(rotatedRect != nullptr);
    assert(nearlyEqual(rotatedRect->rotationDeg, 180.0));

    DraftingObject polygon = object("polygon_1", DraftingShapeKind::Polygon, PolygonGeometry{{{0.0, 0.0}, {1.0, 0.0}, {1.0, 1.0}}});
    assert(draftingHandlesForObject(polygon).empty());
    assert(!handleEditPlan(polygon, "vertex_0", {0.5, 0.5}).ok);

    auto badPointPlan = handleEditPlan(point, "point_position", {std::numeric_limits<double>::infinity(), 0.0});
    assert(!badPointPlan.ok);

    return 0;
}
