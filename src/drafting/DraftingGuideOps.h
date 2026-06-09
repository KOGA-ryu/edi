#pragma once

#include "drafting/DraftingDocument.h"

#include <optional>
#include <string>

namespace edi::drafting {

enum class DraftingGuideDrawablePlacement {
    Origin,
    Center,
    Max
};

struct DraftingGuidePlan {
    bool ok = false;
    DraftingResultCode code = DraftingResultCode::None;
    std::string message;
    GuideGeometry geometry;

    static DraftingGuidePlan accepted(GuideGeometry geometry);
    static DraftingGuidePlan rejected(DraftingResultCode code, std::string message);
};

bool sameGuide(const GuideGeometry &a, const GuideGeometry &b);
bool isGuideObject(const DraftingObject &object);
std::optional<DraftingObjectId> existingGuideId(const DraftingDocument &document, const GuideGeometry &guide);
std::optional<double> nearestVisibleGuidePosition(const DraftingDocument &document, GuideOrientation orientation, double target);
DraftingGuidePlan moveGuideToDrawable(const GuideGeometry &guide, Bounds2D drawable, DraftingGuideDrawablePlacement placement);
DraftingGuidePlan offsetGuide(const GuideGeometry &guide, const std::string &direction, double stepX, double stepY, double scale);
DraftingGuidePlan guideFromBoundsPlacement(Bounds2D bounds, const std::string &placementId);
DraftingGuidePlan offsetGuideFromBoundsPlacement(Bounds2D bounds, const std::string &placementId, double stepX, double stepY);

} // namespace edi::drafting
