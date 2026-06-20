#include "drafting/DraftingConstructionOps.h"

#include "EdiAssert.h"
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
    EDI_CHECK(isHorizontalConstructionLine(horizontal));
    EDI_CHECK(!isVerticalConstructionLine(horizontal));
    auto fittedHorizontal = fitConstructionLineToDrawable(horizontal, drawable);
    EDI_CHECK(fittedHorizontal.ok);
    EDI_CHECK(near(fittedHorizontal.geometry.a.x, 0.1));
    EDI_CHECK(near(fittedHorizontal.geometry.a.y, 0.5));
    EDI_CHECK(near(fittedHorizontal.geometry.b.x, 0.7));
    EDI_CHECK(near(fittedHorizontal.geometry.b.y, 0.5));

    ConstructionLineGeometry vertical{{0.4, 0.1}, {0.4, 0.9}};
    EDI_CHECK(!isHorizontalConstructionLine(vertical));
    EDI_CHECK(isVerticalConstructionLine(vertical));
    auto fittedVertical = fitConstructionLineToDrawable(vertical, drawable);
    EDI_CHECK(fittedVertical.ok);
    EDI_CHECK(near(fittedVertical.geometry.a.x, 0.4));
    EDI_CHECK(near(fittedVertical.geometry.a.y, 0.2));
    EDI_CHECK(near(fittedVertical.geometry.b.x, 0.4));
    EDI_CHECK(near(fittedVertical.geometry.b.y, 0.6));

    auto angled = fitConstructionLineToDrawable({{0.1, 0.2}, {0.5, 0.7}}, drawable);
    EDI_CHECK(!angled.ok);
    EDI_CHECK(angled.code == DraftingResultCode::InvalidGeometry);

    auto badDrawable = fitConstructionLineToDrawable(horizontal, Bounds2D{0.0, 0.0, 0.0, 1.0});
    EDI_CHECK(!badDrawable.ok);
    EDI_CHECK(badDrawable.code == DraftingResultCode::InvalidGeometry);

    auto nonFinite = fitConstructionLineToDrawable(
        {{std::numeric_limits<double>::infinity(), 0.2}, {0.4, 0.2}},
        drawable);
    EDI_CHECK(!nonFinite.ok);
    EDI_CHECK(nonFinite.code == DraftingResultCode::InvalidGeometry);

    return 0;
}
