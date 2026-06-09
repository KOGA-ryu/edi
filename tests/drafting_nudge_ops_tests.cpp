#include "drafting/DraftingNudgeOps.h"

#include <cassert>
#include <cmath>

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

    return 0;
}
