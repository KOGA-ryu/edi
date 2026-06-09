#include "drafting/DraftingOffset.h"

#include <cassert>
#include <cmath>
#include <limits>

using namespace edi::drafting;

namespace {

bool nearlyEqual(double a, double b)
{
    return std::abs(a - b) < 0.000001;
}

DraftingObject object(std::string id, DraftingShapeKind kind, DraftingGeometry geometry)
{
    auto built = buildDraftingObject(std::move(id), kind, std::move(geometry));
    assert(built.ok);
    return built.object;
}

} // namespace

int main()
{
    assert(draftingOffsetSideFromId("right") == DraftingOffsetSide::Right);
    assert(draftingOffsetSideFromId("left") == DraftingOffsetSide::Left);
    assert(draftingOffsetSideFromId("missing") == DraftingOffsetSide::Left);

    DraftingObject line = object("line_1", DraftingShapeKind::Line, LineGeometry{{0.0, 0.0}, {1.0, 0.0}});
    line.stroke.color = "#ffaa00";
    line.layerId = "default";
    auto leftLine = offsetDraftingObject(line, "line_offset_left", 0.1, DraftingOffsetSide::Left);
    assert(leftLine.ok);
    assert(leftLine.object.id == "line_offset_left");
    assert(leftLine.object.kind == DraftingShapeKind::Line);
    assert(leftLine.object.metadata.toolProvenance == "offset");
    assert(leftLine.object.metadata.source == "line_1");
    assert(leftLine.object.stroke.color == "#ffaa00");
    const auto *leftLineGeometry = std::get_if<LineGeometry>(&leftLine.object.geometry);
    assert(leftLineGeometry != nullptr);
    assert(nearlyEqual(leftLineGeometry->a.x, 0.0));
    assert(nearlyEqual(leftLineGeometry->a.y, 0.1));
    assert(nearlyEqual(leftLineGeometry->b.x, 1.0));
    assert(nearlyEqual(leftLineGeometry->b.y, 0.1));

    auto rightLine = offsetDraftingObject(line, "line_offset_right", 0.1, DraftingOffsetSide::Right);
    assert(rightLine.ok);
    const auto *rightLineGeometry = std::get_if<LineGeometry>(&rightLine.object.geometry);
    assert(rightLineGeometry != nullptr);
    assert(nearlyEqual(rightLineGeometry->a.y, -0.1));
    assert(nearlyEqual(rightLineGeometry->b.y, -0.1));

    DraftingObject verticalConstruction = object(
        "construction_1",
        DraftingShapeKind::ConstructionLine,
        ConstructionLineGeometry{{0.5, 0.0}, {0.5, 1.0}});
    auto leftConstruction = offsetDraftingObject(verticalConstruction, "construction_offset", 0.2, DraftingOffsetSide::Left);
    assert(leftConstruction.ok);
    assert(leftConstruction.object.kind == DraftingShapeKind::ConstructionLine);
    const auto *leftConstructionGeometry = std::get_if<ConstructionLineGeometry>(&leftConstruction.object.geometry);
    assert(leftConstructionGeometry != nullptr);
    assert(nearlyEqual(leftConstructionGeometry->a.x, 0.3));
    assert(nearlyEqual(leftConstructionGeometry->a.y, 0.0));
    assert(nearlyEqual(leftConstructionGeometry->b.x, 0.3));
    assert(nearlyEqual(leftConstructionGeometry->b.y, 1.0));

    DraftingObject point = object("point_1", DraftingShapeKind::Point, PointGeometry{{0.0, 0.0}});
    auto unsupported = offsetDraftingObject(point, "point_offset", 0.1, DraftingOffsetSide::Left);
    assert(!unsupported.ok);
    assert(unsupported.code == DraftingResultCode::InvalidSelectionTarget);

    auto badDistance = offsetDraftingObject(line, "bad_offset", std::numeric_limits<double>::infinity(), DraftingOffsetSide::Left);
    assert(!badDistance.ok);
    assert(badDistance.code == DraftingResultCode::InvalidGeometry);

    DraftingObject mismatched = line;
    mismatched.kind = DraftingShapeKind::Point;
    auto mismatch = offsetDraftingObject(mismatched, "mismatch_offset", 0.1, DraftingOffsetSide::Left);
    assert(!mismatch.ok);
    assert(mismatch.code == DraftingResultCode::KindGeometryMismatch);

    return 0;
}
