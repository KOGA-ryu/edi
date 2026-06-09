#include "drafting/DraftingNudgeOps.h"

#include <cassert>
#include <cmath>
#include <limits>

using namespace edi::drafting;

namespace {

bool near(double a, double b)
{
    return std::abs(a - b) < 0.000001;
}

DraftingSnapSettings settings()
{
    DraftingSnapSettings result;
    result.gridStep = 0.1;
    result.gridStepX = 0.2;
    result.gridStepY = 0.05;
    return result;
}

} // namespace

int main()
{
    assert(near(draftingNudgeScaleForMode("fine"), 0.25));
    assert(near(draftingNudgeScaleForMode("coarse"), 4.0));
    assert(near(draftingNudgeScaleForMode("grid"), 1.0));

    DraftingSnapSettings nudgeSettings = settings();
    assert(near(effectiveNudgeStepX(nudgeSettings), 0.2));
    assert(near(effectiveNudgeStepY(nudgeSettings), 0.05));
    nudgeSettings.gridStepX = 0.0;
    nudgeSettings.gridStepY = -1.0;
    assert(near(effectiveNudgeStepX(nudgeSettings), 0.1));
    assert(near(effectiveNudgeStepY(nudgeSettings), 0.1));

    auto right = planNudgeDelta("right", settings(), "grid");
    assert(right.ok);
    assert(near(right.dx, 0.2));
    assert(near(right.dy, 0.0));
    auto leftFine = planNudgeDelta("left", settings(), "fine");
    assert(leftFine.ok);
    assert(near(leftFine.dx, -0.05));
    auto downCoarse = planNudgeDelta("down", settings(), "coarse");
    assert(downCoarse.ok);
    assert(near(downCoarse.dy, 0.2));
    assert(!planNudgeDelta("diagonal", settings(), "grid").ok);

    Bounds2D selected{0.2, 0.2, 0.2, 0.2};
    Bounds2D drawable{0.1, 0.1, 0.7, 0.7};
    auto inside = planNudgeInsideDrawable("right", settings(), "grid", selected, drawable);
    assert(inside.ok);
    assert(near(inside.dx, 0.2));

    Bounds2D nearLeft{0.1, 0.2, 0.2, 0.2};
    auto outside = planNudgeInsideDrawable("left", settings(), "grid", nearLeft, drawable);
    assert(!outside.ok);
    assert(outside.code == DraftingResultCode::InvalidGeometry);

    Bounds2D tooLarge{0.1, 0.1, 0.8, 0.2};
    auto tooLargePlan = planNudgeInsideDrawable("right", settings(), "grid", tooLarge, drawable);
    assert(!tooLargePlan.ok);
    assert(tooLargePlan.code == DraftingResultCode::InvalidGeometry);

    Bounds2D outsideLeftTop{0.0, 0.05, 0.2, 0.2};
    auto fitInside = planSelectionDrawableMove(outsideLeftTop, drawable, DraftingSelectionDrawablePlacement::FitInside);
    assert(fitInside.ok);
    assert(near(fitInside.dx, 0.1));
    assert(near(fitInside.dy, 0.05));

    Bounds2D outsideRightBottom{0.7, 0.75, 0.2, 0.1};
    auto fitFromMax = planSelectionDrawableMove(outsideRightBottom, drawable, DraftingSelectionDrawablePlacement::FitInside);
    assert(fitFromMax.ok);
    assert(near(fitFromMax.dx, -0.1));
    assert(near(fitFromMax.dy, -0.05));

    auto alreadyInside = planSelectionDrawableMove(selected, drawable, DraftingSelectionDrawablePlacement::FitInside);
    assert(alreadyInside.ok);
    assert(near(alreadyInside.dx, 0.0));
    assert(near(alreadyInside.dy, 0.0));

    auto tooLargeFit = planSelectionDrawableMove(tooLarge, drawable, DraftingSelectionDrawablePlacement::FitInside);
    assert(!tooLargeFit.ok);
    assert(tooLargeFit.code == DraftingResultCode::InvalidGeometry);

    auto center = planSelectionDrawableMove(selected, drawable, DraftingSelectionDrawablePlacement::Center);
    assert(center.ok);
    assert(near(center.dx, 0.15));
    assert(near(center.dy, 0.15));

    auto origin = planSelectionDrawableMove(selected, drawable, DraftingSelectionDrawablePlacement::Origin);
    assert(origin.ok);
    assert(near(origin.dx, -0.1));
    assert(near(origin.dy, -0.1));

    Bounds2D invalid{0.0, 0.0, std::numeric_limits<double>::infinity(), 0.2};
    auto invalidPlacement = planSelectionDrawableMove(invalid, drawable, DraftingSelectionDrawablePlacement::Center);
    assert(!invalidPlacement.ok);
    assert(invalidPlacement.code == DraftingResultCode::InvalidGeometry);

    return 0;
}
