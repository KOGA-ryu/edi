#include "drafting/DraftingOffset.h"

#include "EdiAssert.h"
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
    EDI_CHECK(built.ok);
    return built.object;
}

} // namespace

int main()
{
    EDI_CHECK(draftingOffsetSideFromId("right") == DraftingOffsetSide::Right);
    EDI_CHECK(draftingOffsetSideFromId("left") == DraftingOffsetSide::Left);
    EDI_CHECK(draftingOffsetSideFromId("missing") == DraftingOffsetSide::Left);
    EDI_CHECK(nearlyEqual(defaultDraftingOffsetDistance(), 0.05));

    DraftingObject line = object("line_1", DraftingShapeKind::Line, LineGeometry{{0.0, 0.0}, {1.0, 0.0}});
    line.stroke.color = "#ffaa00";
    line.layerId = "default";
    auto leftLine = offsetDraftingObject(line, "line_offset_left", 0.1, DraftingOffsetSide::Left);
    EDI_CHECK(leftLine.ok);
    EDI_CHECK(leftLine.object.id == "line_offset_left");
    EDI_CHECK(leftLine.object.kind == DraftingShapeKind::Line);
    EDI_CHECK(leftLine.object.metadata.toolProvenance == "offset");
    EDI_CHECK(leftLine.object.metadata.source == "line_1");
    EDI_CHECK(leftLine.object.stroke.color == "#ffaa00");
    const auto *leftLineGeometry = std::get_if<LineGeometry>(&leftLine.object.geometry);
    EDI_CHECK(leftLineGeometry != nullptr);
    EDI_CHECK(nearlyEqual(leftLineGeometry->a.x, 0.0));
    EDI_CHECK(nearlyEqual(leftLineGeometry->a.y, 0.1));
    EDI_CHECK(nearlyEqual(leftLineGeometry->b.x, 1.0));
    EDI_CHECK(nearlyEqual(leftLineGeometry->b.y, 0.1));

    auto rightLine = offsetDraftingObject(line, "line_offset_right", 0.1, DraftingOffsetSide::Right);
    EDI_CHECK(rightLine.ok);
    const auto *rightLineGeometry = std::get_if<LineGeometry>(&rightLine.object.geometry);
    EDI_CHECK(rightLineGeometry != nullptr);
    EDI_CHECK(nearlyEqual(rightLineGeometry->a.y, -0.1));
    EDI_CHECK(nearlyEqual(rightLineGeometry->b.y, -0.1));

    DraftingObject verticalConstruction = object(
        "construction_1",
        DraftingShapeKind::ConstructionLine,
        ConstructionLineGeometry{{0.5, 0.0}, {0.5, 1.0}});
    auto leftConstruction = offsetDraftingObject(verticalConstruction, "construction_offset", 0.2, DraftingOffsetSide::Left);
    EDI_CHECK(leftConstruction.ok);
    EDI_CHECK(leftConstruction.object.kind == DraftingShapeKind::ConstructionLine);
    const auto *leftConstructionGeometry = std::get_if<ConstructionLineGeometry>(&leftConstruction.object.geometry);
    EDI_CHECK(leftConstructionGeometry != nullptr);
    EDI_CHECK(nearlyEqual(leftConstructionGeometry->a.x, 0.3));
    EDI_CHECK(nearlyEqual(leftConstructionGeometry->a.y, 0.0));
    EDI_CHECK(nearlyEqual(leftConstructionGeometry->b.x, 0.3));
    EDI_CHECK(nearlyEqual(leftConstructionGeometry->b.y, 1.0));

    DraftingObject point = object("point_1", DraftingShapeKind::Point, PointGeometry{{0.0, 0.0}});
    auto unsupported = offsetDraftingObject(point, "point_offset", 0.1, DraftingOffsetSide::Left);
    EDI_CHECK(!unsupported.ok);
    EDI_CHECK(unsupported.code == DraftingResultCode::InvalidSelectionTarget);

    auto badDistance = offsetDraftingObject(line, "bad_offset", std::numeric_limits<double>::infinity(), DraftingOffsetSide::Left);
    EDI_CHECK(!badDistance.ok);
    EDI_CHECK(badDistance.code == DraftingResultCode::InvalidGeometry);

    DraftingObject mismatched = line;
    mismatched.kind = DraftingShapeKind::Point;
    auto mismatch = offsetDraftingObject(mismatched, "mismatch_offset", 0.1, DraftingOffsetSide::Left);
    EDI_CHECK(!mismatch.ok);
    EDI_CHECK(mismatch.code == DraftingResultCode::KindGeometryMismatch);

    return 0;
}
