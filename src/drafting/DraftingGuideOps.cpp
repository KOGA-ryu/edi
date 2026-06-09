#include "drafting/DraftingGuideOps.h"

#include "drafting/DraftingGeometry.h"

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

} // namespace edi::drafting
