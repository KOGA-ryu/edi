#pragma once

#include "drafting/DraftingDocument.h"

#include <optional>
#include <string>
#include <vector>

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

struct DraftingGuidePresetGuide {
    GuideGeometry geometry;
    std::string label;
    std::string color;
};

struct DraftingGuidePresetPlan {
    bool ok = false;
    DraftingResultCode code = DraftingResultCode::None;
    std::string message;
    std::vector<DraftingGuidePresetGuide> guides;

    static DraftingGuidePresetPlan accepted(std::vector<DraftingGuidePresetGuide> guides);
    static DraftingGuidePresetPlan rejected(DraftingResultCode code, std::string message);
};

struct DraftingGuideAlignmentPlan {
    bool ok = false;
    DraftingResultCode code = DraftingResultCode::None;
    std::string message;
    GuideOrientation orientation = GuideOrientation::Vertical;
    double target = 0.0;
    double guidePosition = 0.0;
    double dx = 0.0;
    double dy = 0.0;

    static DraftingGuideAlignmentPlan accepted(GuideOrientation orientation, double target, double guidePosition, double dx, double dy);
    static DraftingGuideAlignmentPlan rejected(DraftingResultCode code, std::string message);
};

bool sameGuide(const GuideGeometry &a, const GuideGeometry &b);
bool isGuideObject(const DraftingObject &object);
std::optional<DraftingObjectId> existingGuideId(const DraftingDocument &document, const GuideGeometry &guide);
std::optional<double> nearestVisibleGuidePosition(const DraftingDocument &document, GuideOrientation orientation, double target);
DraftingGuidePlan moveGuideToDrawable(const GuideGeometry &guide, Bounds2D drawable, DraftingGuideDrawablePlacement placement);
DraftingGuidePlan offsetGuide(const GuideGeometry &guide, const std::string &direction, double stepX, double stepY, double scale);
DraftingGuidePlan guideFromBoundsPlacement(Bounds2D bounds, const std::string &placementId);
DraftingGuidePlan offsetGuideFromBoundsPlacement(Bounds2D bounds, const std::string &placementId, double stepX, double stepY);
DraftingGuidePresetPlan guidePresetForDrawable(const std::string &presetId, Bounds2D drawable);
DraftingGuideAlignmentPlan alignBoundsToNearestGuide(const DraftingDocument &document, Bounds2D bounds, const std::string &modeId);

} // namespace edi::drafting
