#include "drafting/DraftingNudgeOps.h"

#include "drafting/DraftingGeometry.h"

#include <algorithm>
#include <cmath>
#include <utility>

namespace edi::drafting {

DraftingNudgePlan DraftingNudgePlan::accepted(double dx, double dy)
{
    DraftingNudgePlan result;
    result.ok = true;
    result.code = DraftingResultCode::None;
    result.dx = dx;
    result.dy = dy;
    return result;
}

DraftingNudgePlan DraftingNudgePlan::rejected(DraftingResultCode code, std::string message)
{
    DraftingNudgePlan result;
    result.ok = false;
    result.code = code;
    result.message = std::move(message);
    return result;
}

double draftingNudgeScaleForMode(const std::string &stepMode)
{
    if (stepMode == "fine") {
        return 0.25;
    }
    if (stepMode == "coarse") {
        return 4.0;
    }
    return 1.0;
}

double effectiveNudgeStepX(const DraftingSnapSettings &settings)
{
    return std::max(0.000001, settings.gridStepX > 0.0 ? settings.gridStepX : settings.gridStep);
}

double effectiveNudgeStepY(const DraftingSnapSettings &settings)
{
    return std::max(0.000001, settings.gridStepY > 0.0 ? settings.gridStepY : settings.gridStep);
}

DraftingNudgePlan planNudgeDelta(const std::string &direction, const DraftingSnapSettings &settings, const std::string &stepMode)
{
    const double scale = draftingNudgeScaleForMode(stepMode);
    const double stepX = effectiveNudgeStepX(settings);
    const double stepY = effectiveNudgeStepY(settings);

    if (direction == "left") {
        return DraftingNudgePlan::accepted(-stepX * scale, 0.0);
    }
    if (direction == "right") {
        return DraftingNudgePlan::accepted(stepX * scale, 0.0);
    }
    if (direction == "up") {
        return DraftingNudgePlan::accepted(0.0, -stepY * scale);
    }
    if (direction == "down") {
        return DraftingNudgePlan::accepted(0.0, stepY * scale);
    }
    return DraftingNudgePlan::rejected(DraftingResultCode::InvalidGeometry, "nudge direction is invalid");
}

DraftingNudgePlan planNudgeInsideDrawable(
    const std::string &direction,
    const DraftingSnapSettings &settings,
    const std::string &stepMode,
    Bounds2D selectedBounds,
    Bounds2D drawableBounds)
{
    const DraftingNudgePlan delta = planNudgeDelta(direction, settings, stepMode);
    if (!delta.ok) {
        return delta;
    }
    if (!isFinite(selectedBounds) || !isFinite(drawableBounds)) {
        return DraftingNudgePlan::rejected(DraftingResultCode::InvalidGeometry, "nudge bounds must be finite");
    }
    if (selectedBounds.width > drawableBounds.width || selectedBounds.height > drawableBounds.height) {
        return DraftingNudgePlan::rejected(DraftingResultCode::InvalidGeometry, "selection is larger than drawable bounds");
    }
    if (!boundsInsideDrawable(translateBounds(selectedBounds, delta.dx, delta.dy), drawableBounds)) {
        return DraftingNudgePlan::rejected(DraftingResultCode::InvalidGeometry, "nudge would move selection outside drawable bounds");
    }
    return delta;
}

DraftingNudgePlan planSelectionDrawableMove(Bounds2D selectedBounds, Bounds2D drawableBounds, DraftingSelectionDrawablePlacement placement)
{
    if (!isFinite(selectedBounds) || !isFinite(drawableBounds)) {
        return DraftingNudgePlan::rejected(DraftingResultCode::InvalidGeometry, "selection and drawable bounds must be finite");
    }

    if (placement == DraftingSelectionDrawablePlacement::FitInside) {
        if (selectedBounds.width > drawableBounds.width || selectedBounds.height > drawableBounds.height) {
            return DraftingNudgePlan::rejected(DraftingResultCode::InvalidGeometry, "selection is larger than drawable bounds");
        }

        double dx = 0.0;
        double dy = 0.0;
        if (selectedBounds.x < drawableBounds.x) {
            dx = drawableBounds.x - selectedBounds.x;
        } else if (selectedBounds.x + selectedBounds.width > drawableBounds.x + drawableBounds.width) {
            dx = drawableBounds.x + drawableBounds.width - (selectedBounds.x + selectedBounds.width);
        }
        if (selectedBounds.y < drawableBounds.y) {
            dy = drawableBounds.y - selectedBounds.y;
        } else if (selectedBounds.y + selectedBounds.height > drawableBounds.y + drawableBounds.height) {
            dy = drawableBounds.y + drawableBounds.height - (selectedBounds.y + selectedBounds.height);
        }
        return DraftingNudgePlan::accepted(dx, dy);
    }

    if (placement == DraftingSelectionDrawablePlacement::Center) {
        const double targetX = drawableBounds.x + (drawableBounds.width - selectedBounds.width) / 2.0;
        const double targetY = drawableBounds.y + (drawableBounds.height - selectedBounds.height) / 2.0;
        return DraftingNudgePlan::accepted(targetX - selectedBounds.x, targetY - selectedBounds.y);
    }

    return DraftingNudgePlan::accepted(drawableBounds.x - selectedBounds.x, drawableBounds.y - selectedBounds.y);
}

} // namespace edi::drafting
