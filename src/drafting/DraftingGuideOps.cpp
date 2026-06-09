#include "drafting/DraftingGuideOps.h"

#include "drafting/DraftingGeometry.h"
#include "drafting/DraftingMetadata.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <utility>

namespace edi::drafting {

DraftingGuidePlan DraftingGuidePlan::accepted(GuideGeometry geometry)
{
    DraftingGuidePlan result;
    result.ok = true;
    result.code = DraftingResultCode::None;
    result.geometry = geometry;
    return result;
}

DraftingGuidePlan DraftingGuidePlan::rejected(DraftingResultCode code, std::string message)
{
    DraftingGuidePlan result;
    result.ok = false;
    result.code = code;
    result.message = std::move(message);
    return result;
}

DraftingGuidePresetPlan DraftingGuidePresetPlan::accepted(std::vector<DraftingGuidePresetGuide> guides)
{
    DraftingGuidePresetPlan result;
    result.ok = true;
    result.code = DraftingResultCode::None;
    result.guides = std::move(guides);
    return result;
}

DraftingGuidePresetPlan DraftingGuidePresetPlan::rejected(DraftingResultCode code, std::string message)
{
    DraftingGuidePresetPlan result;
    result.ok = false;
    result.code = code;
    result.message = std::move(message);
    return result;
}

DraftingGuideAlignmentPlan DraftingGuideAlignmentPlan::accepted(GuideOrientation orientation, double target, double guidePosition, double dx, double dy)
{
    DraftingGuideAlignmentPlan result;
    result.ok = true;
    result.code = DraftingResultCode::None;
    result.orientation = orientation;
    result.target = target;
    result.guidePosition = guidePosition;
    result.dx = dx;
    result.dy = dy;
    return result;
}

DraftingGuideAlignmentPlan DraftingGuideAlignmentPlan::rejected(DraftingResultCode code, std::string message)
{
    DraftingGuideAlignmentPlan result;
    result.ok = false;
    result.code = code;
    result.message = std::move(message);
    return result;
}

DraftingGuideMoveSnapPlan DraftingGuideMoveSnapPlan::accepted(bool intersection,
    double distance,
    int anchorRank,
    double dx,
    double dy,
    Point2D intendedAnchor,
    Point2D snappedAnchor,
    DraftingObjectId sourceObjectId,
    std::string anchorLabel)
{
    DraftingGuideMoveSnapPlan result;
    result.ok = true;
    result.intersection = intersection;
    result.distance = distance;
    result.anchorRank = anchorRank;
    result.dx = dx;
    result.dy = dy;
    result.intendedAnchor = intendedAnchor;
    result.snappedAnchor = snappedAnchor;
    result.sourceObjectId = std::move(sourceObjectId);
    result.anchorLabel = std::move(anchorLabel);
    return result;
}

namespace {

bool containsObjectId(const std::vector<DraftingObjectId> &ids, const DraftingObjectId &id)
{
    return std::find(ids.begin(), ids.end(), id) != ids.end();
}

bool guideSnapChoiceBetter(const DraftingGuideMoveSnapPlan &candidate, const DraftingGuideMoveSnapPlan &current)
{
    if (!current.ok) {
        return true;
    }
    if (candidate.intersection != current.intersection) {
        return candidate.intersection;
    }
    constexpr double epsilon = 0.000001;
    if (std::abs(candidate.distance - current.distance) > epsilon) {
        return candidate.distance < current.distance;
    }
    if (candidate.anchorRank != current.anchorRank) {
        return candidate.anchorRank < current.anchorRank;
    }
    return false;
}

std::string moveAnchorLabelForHandle(const std::string &handleId)
{
    if (handleId == "point") {
        return "point";
    }
    if (handleId == "line_start" || handleId == "line_end") {
        return "endpoint";
    }
    if (handleId == "circle_center") {
        return "center";
    }
    if (handleId == "circle_radius") {
        return "radius";
    }
    if (handleId.rfind("rect_", 0) == 0) {
        return "corner";
    }
    return "handle";
}

void addUniqueAnchor(std::vector<DraftingGuideMoveSnapAnchor> &anchors, Point2D point, int rank, std::string label)
{
    if (!isFinite(point)) {
        return;
    }
    constexpr double epsilon = 0.000001;
    const auto duplicate = std::find_if(anchors.begin(), anchors.end(), [point](const DraftingGuideMoveSnapAnchor &existing) {
        return std::abs(existing.point.x - point.x) < epsilon && std::abs(existing.point.y - point.y) < epsilon;
    });
    if (duplicate == anchors.end()) {
        anchors.push_back({point, rank, std::move(label)});
    }
}

} // namespace

bool sameGuide(const GuideGeometry &a, const GuideGeometry &b)
{
    constexpr double epsilon = 0.000001;
    return a.orientation == b.orientation && std::abs(a.position - b.position) <= epsilon;
}

bool isGuideObject(const DraftingObject &object)
{
    return object.kind == DraftingShapeKind::Guide && kindMatchesGeometry(object.kind, object.geometry);
}

std::optional<DraftingObjectId> existingGuideId(const DraftingDocument &document, const GuideGeometry &guide)
{
    for (const DraftingObject &object : document.objects) {
        if (!isGuideObject(object)) {
            continue;
        }
        const auto *existing = std::get_if<GuideGeometry>(&object.geometry);
        if (existing != nullptr && sameGuide(*existing, guide)) {
            return object.id;
        }
    }
    return std::nullopt;
}

std::optional<double> nearestVisibleGuidePosition(const DraftingDocument &document, GuideOrientation orientation, double target)
{
    if (!std::isfinite(target)) {
        return std::nullopt;
    }

    bool found = false;
    double bestPosition = 0.0;
    double bestDistance = std::numeric_limits<double>::max();
    for (const DraftingObject &object : document.objects) {
        if (!isGuideObject(object)) {
            continue;
        }
        const DraftingLayer *layer = findLayer(document, object.layerId);
        if (!object.visible || layer == nullptr || !layer->visible) {
            continue;
        }
        const auto *guide = std::get_if<GuideGeometry>(&object.geometry);
        if (guide == nullptr || guide->orientation != orientation || !std::isfinite(guide->position)) {
            continue;
        }
        const double distance = std::abs(guide->position - target);
        if (!found || distance < bestDistance) {
            found = true;
            bestDistance = distance;
            bestPosition = guide->position;
        }
    }
    return found ? std::optional<double>{bestPosition} : std::nullopt;
}

std::vector<DraftingGuideMoveSnapAnchor> guideMoveSnapAnchorsForObject(const DraftingObject &object)
{
    std::vector<DraftingGuideMoveSnapAnchor> anchors;
    for (const HandleAnchor &handle : handleAnchors(object.geometry)) {
        addUniqueAnchor(anchors, handle.point, 0, moveAnchorLabelForHandle(handle.id));
    }

    const Bounds2D bounds = object.bounds;
    if (isFinite(bounds)) {
        const double left = bounds.x;
        const double top = bounds.y;
        const double right = bounds.x + bounds.width;
        const double bottom = bounds.y + bounds.height;
        const double centerX = bounds.x + bounds.width / 2.0;
        const double centerY = bounds.y + bounds.height / 2.0;
        addUniqueAnchor(anchors, {centerX, centerY}, 1, "center");
        addUniqueAnchor(anchors, {left, centerY}, 2, "edge");
        addUniqueAnchor(anchors, {right, centerY}, 2, "edge");
        addUniqueAnchor(anchors, {centerX, top}, 2, "edge");
        addUniqueAnchor(anchors, {centerX, bottom}, 2, "edge");
        addUniqueAnchor(anchors, {left, top}, 3, "corner");
        addUniqueAnchor(anchors, {right, top}, 3, "corner");
        addUniqueAnchor(anchors, {right, bottom}, 3, "corner");
        addUniqueAnchor(anchors, {left, bottom}, 3, "corner");
    }
    return anchors;
}

DraftingObjectBuildResult buildDraftingGuideObject(DraftingObjectId id,
    GuideGeometry geometry,
    LayerId layerId,
    std::string toolProvenance,
    std::string source,
    GuideVisualMetadata visual)
{
    DraftingObjectBuildResult built = buildDraftingObject(std::move(id), DraftingShapeKind::Guide, geometry);
    if (!built.ok) {
        return built;
    }

    built.object.layerId = std::move(layerId);
    built.object.metadata.toolProvenance = std::move(toolProvenance);
    built.object.metadata.source = std::move(source);
    built.object.metadata.guideVisual = std::move(visual);

    const DraftingMetadataValidationResult metadataValidation = validateObjectMetadata(built.object.metadata);
    if (!metadataValidation.ok) {
        return DraftingObjectBuildResult::rejected(metadataValidation.code, metadataValidation.message);
    }

    return built;
}

DraftingGuidePlan moveGuideToDrawable(const GuideGeometry &guide, Bounds2D drawable, DraftingGuideDrawablePlacement placement)
{
    if (!isFinite(drawable)) {
        return DraftingGuidePlan::rejected(DraftingResultCode::InvalidGeometry, "drawable bounds must be finite");
    }

    GuideGeometry next = guide;
    if (placement == DraftingGuideDrawablePlacement::Origin) {
        next.position = guide.orientation == GuideOrientation::Horizontal ? drawable.y : drawable.x;
    } else if (placement == DraftingGuideDrawablePlacement::Center) {
        next.position = guide.orientation == GuideOrientation::Horizontal
            ? drawable.y + drawable.height / 2.0
            : drawable.x + drawable.width / 2.0;
    } else {
        next.position = guide.orientation == GuideOrientation::Horizontal
            ? drawable.y + drawable.height
            : drawable.x + drawable.width;
    }
    return DraftingGuidePlan::accepted(next);
}

DraftingGuidePlan offsetGuide(const GuideGeometry &guide, const std::string &direction, double stepX, double stepY, double scale)
{
    if (!std::isfinite(stepX) || !std::isfinite(stepY) || !std::isfinite(scale) || stepX <= 0.0 || stepY <= 0.0) {
        return DraftingGuidePlan::rejected(DraftingResultCode::InvalidGeometry, "guide offset step must be positive and finite");
    }

    double delta = 0.0;
    if (direction == "negative") {
        delta = -(guide.orientation == GuideOrientation::Horizontal ? stepY : stepX) * scale;
    } else if (direction == "positive") {
        delta = (guide.orientation == GuideOrientation::Horizontal ? stepY : stepX) * scale;
    } else {
        return DraftingGuidePlan::rejected(DraftingResultCode::InvalidGeometry, "guide offset direction is invalid");
    }

    GuideGeometry next = guide;
    next.position += delta;
    return DraftingGuidePlan::accepted(next);
}

DraftingGuidePlan guideFromBoundsPlacement(Bounds2D bounds, const std::string &placementId)
{
    if (!isFinite(bounds)) {
        return DraftingGuidePlan::rejected(DraftingResultCode::InvalidGeometry, "bounds must be finite");
    }

    if (placementId == "left") {
        return DraftingGuidePlan::accepted({GuideOrientation::Vertical, bounds.x});
    }
    if (placementId == "right") {
        return DraftingGuidePlan::accepted({GuideOrientation::Vertical, bounds.x + bounds.width});
    }
    if (placementId == "vertical_center") {
        return DraftingGuidePlan::accepted({GuideOrientation::Vertical, bounds.x + bounds.width / 2.0});
    }
    if (placementId == "top") {
        return DraftingGuidePlan::accepted({GuideOrientation::Horizontal, bounds.y});
    }
    if (placementId == "bottom") {
        return DraftingGuidePlan::accepted({GuideOrientation::Horizontal, bounds.y + bounds.height});
    }
    if (placementId == "horizontal_center") {
        return DraftingGuidePlan::accepted({GuideOrientation::Horizontal, bounds.y + bounds.height / 2.0});
    }
    return DraftingGuidePlan::rejected(DraftingResultCode::InvalidGeometry, "guide bounds placement is invalid");
}

DraftingGuidePlan offsetGuideFromBoundsPlacement(Bounds2D bounds, const std::string &placementId, double stepX, double stepY)
{
    if (!isFinite(bounds)) {
        return DraftingGuidePlan::rejected(DraftingResultCode::InvalidGeometry, "bounds must be finite");
    }
    if (!std::isfinite(stepX) || !std::isfinite(stepY) || stepX <= 0.0 || stepY <= 0.0) {
        return DraftingGuidePlan::rejected(DraftingResultCode::InvalidGeometry, "guide offset step must be positive and finite");
    }

    if (placementId == "left") {
        return DraftingGuidePlan::accepted({GuideOrientation::Vertical, bounds.x - stepX});
    }
    if (placementId == "right") {
        return DraftingGuidePlan::accepted({GuideOrientation::Vertical, bounds.x + bounds.width + stepX});
    }
    if (placementId == "center_x_minus") {
        return DraftingGuidePlan::accepted({GuideOrientation::Vertical, bounds.x + bounds.width / 2.0 - stepX});
    }
    if (placementId == "center_x_plus") {
        return DraftingGuidePlan::accepted({GuideOrientation::Vertical, bounds.x + bounds.width / 2.0 + stepX});
    }
    if (placementId == "top") {
        return DraftingGuidePlan::accepted({GuideOrientation::Horizontal, bounds.y - stepY});
    }
    if (placementId == "bottom") {
        return DraftingGuidePlan::accepted({GuideOrientation::Horizontal, bounds.y + bounds.height + stepY});
    }
    if (placementId == "center_y_minus") {
        return DraftingGuidePlan::accepted({GuideOrientation::Horizontal, bounds.y + bounds.height / 2.0 - stepY});
    }
    if (placementId == "center_y_plus") {
        return DraftingGuidePlan::accepted({GuideOrientation::Horizontal, bounds.y + bounds.height / 2.0 + stepY});
    }
    return DraftingGuidePlan::rejected(DraftingResultCode::InvalidGeometry, "guide offset bounds placement is invalid");
}

DraftingGuidePresetPlan guidePresetForDrawable(const std::string &presetId, Bounds2D drawable)
{
    if (!isFinite(drawable) || drawable.width <= 0.0 || drawable.height <= 0.0) {
        return DraftingGuidePresetPlan::rejected(DraftingResultCode::InvalidGeometry, "drawable bounds must be positive and finite");
    }

    std::vector<DraftingGuidePresetGuide> guides;
    const auto addVertical = [&](double x, std::string label, std::string color) {
        guides.push_back({GuideGeometry{GuideOrientation::Vertical, x}, std::move(label), std::move(color)});
    };
    const auto addHorizontal = [&](double y, std::string label, std::string color) {
        guides.push_back({GuideGeometry{GuideOrientation::Horizontal, y}, std::move(label), std::move(color)});
    };

    const double left = drawable.x;
    const double right = drawable.x + drawable.width;
    const double top = drawable.y;
    const double bottom = drawable.y + drawable.height;
    const double centerX = drawable.x + drawable.width / 2.0;
    const double centerY = drawable.y + drawable.height / 2.0;

    if (presetId == "drawable_bounds") {
        addVertical(left, "drawable left", "#f6c65b");
        addVertical(right, "drawable right", "#f6c65b");
        addHorizontal(top, "drawable top", "#f6c65b");
        addHorizontal(bottom, "drawable bottom", "#f6c65b");
    } else if (presetId == "drawable_centerlines") {
        addVertical(centerX, "center x", "#54d2c6");
        addHorizontal(centerY, "center y", "#54d2c6");
    } else if (presetId == "thirds") {
        addVertical(drawable.x + drawable.width / 3.0, "third x 1", "#91c89b");
        addVertical(drawable.x + drawable.width * 2.0 / 3.0, "third x 2", "#91c89b");
        addHorizontal(drawable.y + drawable.height / 3.0, "third y 1", "#91c89b");
        addHorizontal(drawable.y + drawable.height * 2.0 / 3.0, "third y 2", "#91c89b");
    } else if (presetId == "quarters") {
        addVertical(drawable.x + drawable.width / 4.0, "quarter x 1", "#83aeca");
        addVertical(centerX, "quarter x 2", "#83aeca");
        addVertical(drawable.x + drawable.width * 3.0 / 4.0, "quarter x 3", "#83aeca");
        addHorizontal(drawable.y + drawable.height / 4.0, "quarter y 1", "#83aeca");
        addHorizontal(centerY, "quarter y 2", "#83aeca");
        addHorizontal(drawable.y + drawable.height * 3.0 / 4.0, "quarter y 3", "#83aeca");
    } else if (presetId == "margin_safe") {
        addVertical(left, "safe left", "#d98b8b");
        addVertical(right, "safe right", "#d98b8b");
        addHorizontal(top, "safe top", "#d98b8b");
        addHorizontal(bottom, "safe bottom", "#d98b8b");
        addVertical(centerX, "safe center x", "#d98b8b");
        addHorizontal(centerY, "safe center y", "#d98b8b");
    } else {
        return DraftingGuidePresetPlan::rejected(DraftingResultCode::InvalidGeometry, "guide preset is invalid");
    }

    return DraftingGuidePresetPlan::accepted(std::move(guides));
}

DraftingGuideAlignmentPlan alignBoundsToNearestGuide(const DraftingDocument &document, Bounds2D bounds, const std::string &modeId)
{
    if (!isFinite(bounds)) {
        return DraftingGuideAlignmentPlan::rejected(DraftingResultCode::InvalidGeometry, "bounds must be finite");
    }

    GuideOrientation orientation = GuideOrientation::Vertical;
    double target = 0.0;
    if (modeId == "left") {
        target = bounds.x;
    } else if (modeId == "right") {
        target = bounds.x + bounds.width;
    } else if (modeId == "center_x") {
        target = bounds.x + bounds.width / 2.0;
    } else if (modeId == "top") {
        orientation = GuideOrientation::Horizontal;
        target = bounds.y;
    } else if (modeId == "bottom") {
        orientation = GuideOrientation::Horizontal;
        target = bounds.y + bounds.height;
    } else if (modeId == "center_y") {
        orientation = GuideOrientation::Horizontal;
        target = bounds.y + bounds.height / 2.0;
    } else {
        return DraftingGuideAlignmentPlan::rejected(DraftingResultCode::InvalidGeometry, "guide alignment mode is invalid");
    }

    const std::optional<double> guidePosition = nearestVisibleGuidePosition(document, orientation, target);
    if (!guidePosition) {
        return DraftingGuideAlignmentPlan::rejected(DraftingResultCode::ObjectNotFound, "matching visible guide was not found");
    }

    const double delta = *guidePosition - target;
    return orientation == GuideOrientation::Vertical
        ? DraftingGuideAlignmentPlan::accepted(orientation, target, *guidePosition, delta, 0.0)
        : DraftingGuideAlignmentPlan::accepted(orientation, target, *guidePosition, 0.0, delta);
}

DraftingGuideMoveSnapPlan guideMoveSnapPlan(const DraftingDocument &document,
    const DraftingObject &object,
    const std::vector<DraftingObjectId> &selectedObjectIds,
    const DraftingSnapSettings &settings,
    double dx,
    double dy)
{
    if (!std::isfinite(dx) || !std::isfinite(dy) || object.kind == DraftingShapeKind::Guide || !isFinite(object.bounds)) {
        return {};
    }

    DraftingGuideMoveSnapPlan best;
    for (const DraftingGuideMoveSnapAnchor &anchor : guideMoveSnapAnchorsForObject(object)) {
        const Point2D intendedAnchor {anchor.point.x + dx, anchor.point.y + dy};
        const DraftingSnapResult snap = resolveSnap(intendedAnchor, document, settings);
        if (snap.sourceKind != DraftingSnapSourceKind::Guide || containsObjectId(selectedObjectIds, snap.sourceObjectId)) {
            continue;
        }

        constexpr double epsilon = 0.000001;
        const bool movesX = std::abs(snap.point.x - intendedAnchor.x) > epsilon;
        const bool movesY = std::abs(snap.point.y - intendedAnchor.y) > epsilon;
        const DraftingGuideMoveSnapPlan candidate = DraftingGuideMoveSnapPlan::accepted(
            movesX && movesY,
            distance(intendedAnchor, snap.point),
            anchor.rank,
            dx + snap.point.x - intendedAnchor.x,
            dy + snap.point.y - intendedAnchor.y,
            intendedAnchor,
            snap.point,
            snap.sourceObjectId,
            anchor.label);
        if (guideSnapChoiceBetter(candidate, best)) {
            best = candidate;
        }
    }

    return best;
}

} // namespace edi::drafting
