#include "drafting/DraftingNudgeOps.h"

#include "EdiAssert.h"
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
    EDI_CHECK(near(draftingNudgeScaleForMode("fine"), 0.25));
    EDI_CHECK(near(draftingNudgeScaleForMode("coarse"), 4.0));
    EDI_CHECK(near(draftingNudgeScaleForMode("grid"), 1.0));

    DraftingSnapSettings nudgeSettings = settings();
    EDI_CHECK(near(effectiveNudgeStepX(nudgeSettings), 0.2));
    EDI_CHECK(near(effectiveNudgeStepY(nudgeSettings), 0.05));
    nudgeSettings.gridStepX = 0.0;
    nudgeSettings.gridStepY = -1.0;
    EDI_CHECK(near(effectiveNudgeStepX(nudgeSettings), 0.1));
    EDI_CHECK(near(effectiveNudgeStepY(nudgeSettings), 0.1));

    auto right = planNudgeDelta("right", settings(), "grid");
    EDI_CHECK(right.ok);
    EDI_CHECK(near(right.dx, 0.2));
    EDI_CHECK(near(right.dy, 0.0));
    auto leftFine = planNudgeDelta("left", settings(), "fine");
    EDI_CHECK(leftFine.ok);
    EDI_CHECK(near(leftFine.dx, -0.05));
    auto downCoarse = planNudgeDelta("down", settings(), "coarse");
    EDI_CHECK(downCoarse.ok);
    EDI_CHECK(near(downCoarse.dy, 0.2));
    EDI_CHECK(!planNudgeDelta("diagonal", settings(), "grid").ok);

    Bounds2D selected{0.2, 0.2, 0.2, 0.2};
    Bounds2D drawable{0.1, 0.1, 0.7, 0.7};
    auto inside = planNudgeInsideDrawable("right", settings(), "grid", selected, drawable);
    EDI_CHECK(inside.ok);
    EDI_CHECK(near(inside.dx, 0.2));

    Bounds2D nearLeft{0.1, 0.2, 0.2, 0.2};
    auto outside = planNudgeInsideDrawable("left", settings(), "grid", nearLeft, drawable);
    EDI_CHECK(!outside.ok);
    EDI_CHECK(outside.code == DraftingResultCode::InvalidGeometry);

    Bounds2D tooLarge{0.1, 0.1, 0.8, 0.2};
    auto tooLargePlan = planNudgeInsideDrawable("right", settings(), "grid", tooLarge, drawable);
    EDI_CHECK(!tooLargePlan.ok);
    EDI_CHECK(tooLargePlan.code == DraftingResultCode::InvalidGeometry);

    Bounds2D outsideLeftTop{0.0, 0.05, 0.2, 0.2};
    auto fitInside = planSelectionDrawableMove(outsideLeftTop, drawable, DraftingSelectionDrawablePlacement::FitInside);
    EDI_CHECK(fitInside.ok);
    EDI_CHECK(near(fitInside.dx, 0.1));
    EDI_CHECK(near(fitInside.dy, 0.05));

    Bounds2D outsideRightBottom{0.7, 0.75, 0.2, 0.1};
    auto fitFromMax = planSelectionDrawableMove(outsideRightBottom, drawable, DraftingSelectionDrawablePlacement::FitInside);
    EDI_CHECK(fitFromMax.ok);
    EDI_CHECK(near(fitFromMax.dx, -0.1));
    EDI_CHECK(near(fitFromMax.dy, -0.05));

    auto alreadyInside = planSelectionDrawableMove(selected, drawable, DraftingSelectionDrawablePlacement::FitInside);
    EDI_CHECK(alreadyInside.ok);
    EDI_CHECK(near(alreadyInside.dx, 0.0));
    EDI_CHECK(near(alreadyInside.dy, 0.0));

    auto tooLargeFit = planSelectionDrawableMove(tooLarge, drawable, DraftingSelectionDrawablePlacement::FitInside);
    EDI_CHECK(!tooLargeFit.ok);
    EDI_CHECK(tooLargeFit.code == DraftingResultCode::InvalidGeometry);

    auto center = planSelectionDrawableMove(selected, drawable, DraftingSelectionDrawablePlacement::Center);
    EDI_CHECK(center.ok);
    EDI_CHECK(near(center.dx, 0.15));
    EDI_CHECK(near(center.dy, 0.15));

    auto origin = planSelectionDrawableMove(selected, drawable, DraftingSelectionDrawablePlacement::Origin);
    EDI_CHECK(origin.ok);
    EDI_CHECK(near(origin.dx, -0.1));
    EDI_CHECK(near(origin.dy, -0.1));

    Bounds2D invalid{0.0, 0.0, std::numeric_limits<double>::infinity(), 0.2};
    auto invalidPlacement = planSelectionDrawableMove(invalid, drawable, DraftingSelectionDrawablePlacement::Center);
    EDI_CHECK(!invalidPlacement.ok);
    EDI_CHECK(invalidPlacement.code == DraftingResultCode::InvalidGeometry);

    return 0;
}
