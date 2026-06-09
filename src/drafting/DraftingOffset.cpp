#include "drafting/DraftingOffset.h"

#include "drafting/DraftingGeometry.h"

#include <cmath>
#include <utility>

namespace edi::drafting {
namespace {

Point2D offsetVector(Point2D a, Point2D b, double distance, DraftingOffsetSide side)
{
    const double length = edi::drafting::distance(a, b);
    if (length <= 0.000001) {
        return {};
    }
    const double direction = side == DraftingOffsetSide::Left ? 1.0 : -1.0;
    return {
        -(b.y - a.y) / length * distance * direction,
        (b.x - a.x) / length * distance * direction,
    };
}

LineGeometry offsetLine(LineGeometry line, double distance, DraftingOffsetSide side)
{
    const Point2D offset = offsetVector(line.a, line.b, distance, side);
    line.a = translatePoint(line.a, offset.x, offset.y);
    line.b = translatePoint(line.b, offset.x, offset.y);
    return line;
}

ConstructionLineGeometry offsetConstructionLine(ConstructionLineGeometry line, double distance, DraftingOffsetSide side)
{
    const Point2D offset = offsetVector(line.a, line.b, distance, side);
    line.a = translatePoint(line.a, offset.x, offset.y);
    line.b = translatePoint(line.b, offset.x, offset.y);
    return line;
}

} // namespace

DraftingOffsetResult DraftingOffsetResult::accepted(DraftingObject object)
{
    DraftingOffsetResult result;
    result.ok = true;
    result.code = DraftingResultCode::None;
    result.object = std::move(object);
    return result;
}

DraftingOffsetResult DraftingOffsetResult::rejected(DraftingResultCode code, std::string message)
{
    DraftingOffsetResult result;
    result.ok = false;
    result.code = code;
    result.message = std::move(message);
    return result;
}

DraftingOffsetResult offsetDraftingObject(
    const DraftingObject &source,
    DraftingObjectId newObjectId,
    double distanceValue,
    DraftingOffsetSide side)
{
    if (!std::isfinite(distanceValue) || distanceValue <= 0.0) {
        return DraftingOffsetResult::rejected(DraftingResultCode::InvalidGeometry, "offset distance must be positive and finite");
    }
    if (!kindMatchesGeometry(source.kind, source.geometry)) {
        return DraftingOffsetResult::rejected(DraftingResultCode::KindGeometryMismatch, "shape kind does not match geometry");
    }

    DraftingObject offsetObject = source;
    offsetObject.id = std::move(newObjectId);
    offsetObject.metadata.toolProvenance = "offset";
    offsetObject.metadata.source = source.id;

    if (const auto *line = std::get_if<LineGeometry>(&source.geometry)) {
        offsetObject.kind = DraftingShapeKind::Line;
        offsetObject.geometry = offsetLine(*line, distanceValue, side);
    } else if (const auto *constructionLine = std::get_if<ConstructionLineGeometry>(&source.geometry)) {
        offsetObject.kind = DraftingShapeKind::ConstructionLine;
        offsetObject.geometry = offsetConstructionLine(*constructionLine, distanceValue, side);
    } else {
        return DraftingOffsetResult::rejected(DraftingResultCode::InvalidSelectionTarget, "offset supports line and construction line objects");
    }

    const auto validation = validateDraftingObjectShape(offsetObject);
    if (!validation.ok) {
        return DraftingOffsetResult::rejected(validation.code, validation.message);
    }
    offsetObject.bounds = computeBounds(offsetObject.geometry);
    return DraftingOffsetResult::accepted(std::move(offsetObject));
}

} // namespace edi::drafting
