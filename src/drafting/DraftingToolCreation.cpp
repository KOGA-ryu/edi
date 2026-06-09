#include "drafting/DraftingToolCreation.h"

#include "drafting/DraftingGeometry.h"

#include <algorithm>
#include <utility>

namespace edi::drafting {

DraftingToolKind draftingToolKindFromId(const std::string &toolId)
{
    if (toolId == "select_move") {
        return DraftingToolKind::SelectMove;
    }
    if (toolId == "point_tool") {
        return DraftingToolKind::Point;
    }
    if (toolId == "line_tool") {
        return DraftingToolKind::Line;
    }
    if (toolId == "rectangle_tool") {
        return DraftingToolKind::Rectangle;
    }
    if (toolId == "circle_tool") {
        return DraftingToolKind::Circle;
    }
    if (toolId == "horizontal_guide_tool") {
        return DraftingToolKind::HorizontalGuide;
    }
    if (toolId == "vertical_guide_tool") {
        return DraftingToolKind::VerticalGuide;
    }
    return DraftingToolKind::Unknown;
}

const char *draftingToolKindName(DraftingToolKind kind)
{
    switch (kind) {
    case DraftingToolKind::SelectMove:
        return "select_move";
    case DraftingToolKind::Point:
        return "point";
    case DraftingToolKind::Line:
        return "line";
    case DraftingToolKind::Rectangle:
        return "rectangle";
    case DraftingToolKind::Circle:
        return "circle";
    case DraftingToolKind::HorizontalGuide:
        return "horizontal_guide";
    case DraftingToolKind::VerticalGuide:
        return "vertical_guide";
    case DraftingToolKind::Unknown:
        return "unknown";
    }
    return "unknown";
}

DraftingObjectBuildResult buildDraftingObjectForTool(const DraftingToolCreationRequest &request)
{
    DraftingShapeKind kind = DraftingShapeKind::Point;
    DraftingGeometry geometry = PointGeometry{request.start};
    if (request.tool == DraftingToolKind::Point) {
        kind = DraftingShapeKind::Point;
        geometry = PointGeometry{request.end};
    } else if (request.tool == DraftingToolKind::Line) {
        kind = DraftingShapeKind::Line;
        geometry = LineGeometry{request.start, request.end};
    } else if (request.tool == DraftingToolKind::Rectangle) {
        const double left = std::min(request.start.x, request.end.x);
        const double top = std::min(request.start.y, request.end.y);
        const double right = std::max(request.start.x, request.end.x);
        const double bottom = std::max(request.start.y, request.end.y);
        kind = DraftingShapeKind::Rectangle;
        geometry = RectangleGeometry{{left, top}, right - left, bottom - top};
    } else if (request.tool == DraftingToolKind::Circle) {
        kind = DraftingShapeKind::Circle;
        geometry = CircleGeometry{request.start, std::min(1.0, distance(request.start, request.end))};
    } else if (request.tool == DraftingToolKind::HorizontalGuide) {
        kind = DraftingShapeKind::Guide;
        geometry = GuideGeometry{GuideOrientation::Horizontal, request.end.y};
    } else if (request.tool == DraftingToolKind::VerticalGuide) {
        kind = DraftingShapeKind::Guide;
        geometry = GuideGeometry{GuideOrientation::Vertical, request.end.x};
    } else {
        return DraftingObjectBuildResult::rejected(DraftingResultCode::InvalidGeometry, "tool cannot create a drafting object");
    }

    auto built = buildDraftingObject(request.objectId, kind, std::move(geometry));
    if (!built.ok) {
        return built;
    }
    built.object.metadata.toolProvenance = request.toolProvenance.empty()
        ? draftingToolKindName(request.tool)
        : request.toolProvenance;
    return built;
}

} // namespace edi::drafting
