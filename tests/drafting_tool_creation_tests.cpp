#include "drafting/DraftingToolCreation.h"

#include <cassert>
#include <cmath>
#include <string>

using namespace edi::drafting;

namespace {

bool nearlyEqual(double a, double b)
{
    return std::abs(a - b) < 0.000001;
}

DraftingObjectBuildResult build(std::string id, DraftingToolKind tool, Point2D start, Point2D end)
{
    return buildDraftingObjectForTool({tool, std::move(id), start, end, draftingToolKindName(tool)});
}

} // namespace

int main()
{
    assert(draftingToolKindFromId("point_tool") == DraftingToolKind::Point);
    assert(draftingToolKindFromId("line_tool") == DraftingToolKind::Line);
    assert(std::string(draftingToolKindName(DraftingToolKind::Circle)) == "circle");

    auto point = build("point_1", DraftingToolKind::Point, {0.1, 0.2}, {0.3, 0.4});
    assert(point.ok);
    assert(point.object.kind == DraftingShapeKind::Point);
    assert(point.object.metadata.toolProvenance == "point");
    const auto *pointGeometry = std::get_if<PointGeometry>(&point.object.geometry);
    assert(pointGeometry != nullptr);
    assert(pointGeometry->point.x == 0.3);
    assert(pointGeometry->point.y == 0.4);

    auto line = build("line_1", DraftingToolKind::Line, {0.1, 0.2}, {0.8, 0.9});
    assert(line.ok);
    const auto *lineGeometry = std::get_if<LineGeometry>(&line.object.geometry);
    assert(lineGeometry != nullptr);
    assert(lineGeometry->a.x == 0.1);
    assert(lineGeometry->b.y == 0.9);

    auto rect = build("rect_1", DraftingToolKind::Rectangle, {0.8, 0.9}, {0.1, 0.2});
    assert(rect.ok);
    const auto *rectGeometry = std::get_if<RectangleGeometry>(&rect.object.geometry);
    assert(rectGeometry != nullptr);
    assert(rectGeometry->origin.x == 0.1);
    assert(rectGeometry->origin.y == 0.2);
    assert(nearlyEqual(rectGeometry->width, 0.7));
    assert(nearlyEqual(rectGeometry->height, 0.7));

    auto circle = build("circle_1", DraftingToolKind::Circle, {0.5, 0.5}, {0.5, 0.75});
    assert(circle.ok);
    const auto *circleGeometry = std::get_if<CircleGeometry>(&circle.object.geometry);
    assert(circleGeometry != nullptr);
    assert(nearlyEqual(circleGeometry->radius, 0.25));

    auto unknown = build("bad_1", DraftingToolKind::Unknown, {0.0, 0.0}, {1.0, 1.0});
    assert(!unknown.ok);
    assert(unknown.code == DraftingResultCode::InvalidGeometry);

    auto emptyId = build("", DraftingToolKind::Point, {0.0, 0.0}, {1.0, 1.0});
    assert(!emptyId.ok);
    assert(emptyId.code == DraftingResultCode::EmptyObjectId);

    return 0;
}
