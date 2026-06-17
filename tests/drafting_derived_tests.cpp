#include "drafting/DraftingDerived.h"

#include <cassert>
#include <cmath>
#include <string>

using namespace edi::drafting;

namespace {

bool nearlyEqual(double a, double b)
{
    return std::abs(a - b) < 0.000001;
}

} // namespace

int main()
{
    // Three points on the unit circle centered at the origin recover (0,0), r=1.
    {
        const DraftingDerivedCircleResult result = circleThroughThreePoints({1.0, 0.0}, {0.0, 1.0}, {-1.0, 0.0});
        assert(result.ok);
        assert(nearlyEqual(result.geometry.center.x, 0.0));
        assert(nearlyEqual(result.geometry.center.y, 0.0));
        assert(nearlyEqual(result.geometry.radius, 1.0));
    }

    // An off-origin circle: three points on a circle centered (2,3) radius 5.
    {
        const DraftingDerivedCircleResult result = circleThroughThreePoints({7.0, 3.0}, {2.0, 8.0}, {-3.0, 3.0});
        assert(result.ok);
        assert(nearlyEqual(result.geometry.center.x, 2.0));
        assert(nearlyEqual(result.geometry.center.y, 3.0));
        assert(nearlyEqual(result.geometry.radius, 5.0));
    }

    // Collinear triple rejects with a code + message.
    {
        const DraftingDerivedCircleResult result = circleThroughThreePoints({0.0, 0.0}, {1.0, 0.0}, {2.0, 0.0});
        assert(!result.ok);
        assert(result.code == DraftingResultCode::InvalidGeometry);
        assert(!result.message.empty());
    }

    // Non-finite input rejects.
    {
        const DraftingDerivedCircleResult result = circleThroughThreePoints({std::nan(""), 0.0}, {0.0, 1.0}, {-1.0, 0.0});
        assert(!result.ok);
        assert(result.code == DraftingResultCode::InvalidGeometry);
    }

    // Two-point diameter: (0,0)-(2,0) → center (1,0), radius 1.
    {
        const DraftingDerivedCircleResult result = circleThroughTwoPoints({0.0, 0.0}, {2.0, 0.0});
        assert(result.ok);
        assert(nearlyEqual(result.geometry.center.x, 1.0));
        assert(nearlyEqual(result.geometry.center.y, 0.0));
        assert(nearlyEqual(result.geometry.radius, 1.0));
    }

    // A diagonal diameter: (1,1)-(4,5) → center (2.5,3), radius = 5/2.
    {
        const DraftingDerivedCircleResult result = circleThroughTwoPoints({1.0, 1.0}, {4.0, 5.0});
        assert(result.ok);
        assert(nearlyEqual(result.geometry.center.x, 2.5));
        assert(nearlyEqual(result.geometry.center.y, 3.0));
        assert(nearlyEqual(result.geometry.radius, 2.5));
    }

    // Coincident points reject (zero-diameter circle).
    {
        const DraftingDerivedCircleResult result = circleThroughTwoPoints({0.5, 0.5}, {0.5, 0.5});
        assert(!result.ok);
        assert(result.code == DraftingResultCode::InvalidGeometry);
        assert(!result.message.empty());
    }

    return 0;
}
