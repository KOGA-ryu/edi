#pragma once

#include "drafting/DraftingPlotBounds.h"
#include "drafting/DraftingSnap.h"

#include <string>

namespace edi::drafting {

struct DraftingNudgePlan {
    bool ok = false;
    DraftingResultCode code = DraftingResultCode::None;
    std::string message;
    double dx = 0.0;
    double dy = 0.0;

    static DraftingNudgePlan accepted(double dx, double dy);
    static DraftingNudgePlan rejected(DraftingResultCode code, std::string message);
};

enum class DraftingSelectionDrawablePlacement {
    FitInside,
    Center,
    Origin
};

double draftingNudgeScaleForMode(const std::string &stepMode);
double effectiveNudgeStepX(const DraftingSnapSettings &settings);
double effectiveNudgeStepY(const DraftingSnapSettings &settings);
DraftingNudgePlan planNudgeDelta(const std::string &direction, const DraftingSnapSettings &settings, const std::string &stepMode);
DraftingNudgePlan planNudgeInsideDrawable(
    const std::string &direction,
    const DraftingSnapSettings &settings,
    const std::string &stepMode,
    Bounds2D selectedBounds,
    Bounds2D drawableBounds);
DraftingNudgePlan planSelectionDrawableMove(Bounds2D selectedBounds, Bounds2D drawableBounds, DraftingSelectionDrawablePlacement placement);

} // namespace edi::drafting
