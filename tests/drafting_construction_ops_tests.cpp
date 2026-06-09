#include "drafting/DraftingConstructionOps.h"

#include <cassert>
#include <cmath>
#include <limits>

using namespace edi::drafting;

namespace {

bool near(double a, double b)
{
    return std::abs(a - b) < 0.000001;
}

} // namespace

int main()
{
    Bounds2D drawable{0.1, 0.2, 0.6, 0.4};

    ConstructionLineGeometry horizontal{{0.3, 0.5}, {0.9, 0.5}};
    assert(isHorizontalConstructionLine(horizontal));
    assert(!isVerticalConstructionLine(horizontal));
    auto fittedHorizontal = fitConstructionLineToDrawable(horizontal, drawable);
    assert(fittedHorizontal.ok);
    assert(near(fittedHorizontal.geometry.a.x, 0.1));
    assert(near(fittedHorizontal.geometry.a.y, 0.5));
    assert(near(fittedHorizontal.geometry.b.x, 0.7));
    assert(near(fittedHorizontal.geometry.b.y, 0.5));

    ConstructionLineGeometry vertical{{0.4, 0.1}, {0.4, 0.9}};
    assert(!isHorizontalConstructionLine(vertical));
    assert(isVerticalConstructionLine(vertical));
    auto fittedVertical = fitConstructionLineToDrawable(vertical, drawable);
    assert(fittedVertical.ok);
    assert(near(fittedVertical.geometry.a.x, 0.4));
    assert(near(fittedVertical.geometry.a.y, 0.2));
    assert(near(fittedVertical.geometry.b.x, 0.4));
    assert(near(fittedVertical.geometry.b.y, 0.6));

    auto angled = fitConstructionLineToDrawable({{0.1, 0.2}, {0.5, 0.7}}, drawable);
    assert(!angled.ok);
    assert(angled.code == DraftingResultCode::InvalidGeometry);

    auto badDrawable = fitConstructionLineToDrawable(horizontal, Bounds2D{0.0, 0.0, 0.0, 1.0});
    assert(!badDrawable.ok);
    assert(badDrawable.code == DraftingResultCode::InvalidGeometry);

    auto nonFinite = fitConstructionLineToDrawable(
        {{std::numeric_limits<double>::infinity(), 0.2}, {0.4, 0.2}},
        drawable);
    assert(!nonFinite.ok);
    assert(nonFinite.code == DraftingResultCode::InvalidGeometry);

    return 0;
}
