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

bool sameGuide(const GuideGeometry &a, const GuideGeometry &b)
{
    constexpr double epsilon = 0.000001;
    return a.orientation == b.orientation && std::abs(a.position - b.position) <= epsilon;
}

std::optional<DraftingObjectId> existingGuideId(const DraftingDocument &document, const GuideGeometry &guide)
{
    for (const DraftingObject &object : document.objects) {
        if (object.kind != DraftingShapeKind::Guide || !kindMatchesGeometry(object.kind, object.geometry)) {
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
        if (object.kind != DraftingShapeKind::Guide || !kindMatchesGeometry(object.kind, object.geometry)) {
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

} // namespace edi::drafting
