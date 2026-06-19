// deriveSpanFootprint unit tests (Phase-2 P2-B / brief 073).
//
// WHY these tests live here (pure drafting, not in a map test)?
//   deriveSpanFootprint is in edi_drafting_core: a pure geometric primitive with no IO or Qt
//   dependency. dungeon-map's createMapFromSpec consumes it, but the AABB math is owned and tested
//   here, consistent with every other DraftingXxx module (mirrors drafting_overlap_tests).
#include "drafting/DraftingSpan.h"

#include <cassert>
#include <cmath>

using namespace edi::drafting;

static bool nearlyEqual(double a, double b, double tol = 1e-9)
{
    return std::abs(a - b) <= tol;
}

int main()
{
    // 1. HORIZONTAL pair (shared y) -> wide horizontal AABB, axis-aligned + tight.
    {
        const double W = 0.4, P = 0.1;
        const SpanFootprint f = deriveSpanFootprint({0.0, 5.0}, {10.0, 5.0}, W, P);
        assert(nearlyEqual(f.origin.x, -P));          // pushed back endPad past A
        assert(nearlyEqual(f.origin.y, 5.0 - W / 2)); // centered on shared y
        assert(nearlyEqual(f.width, 10.0 + 2 * P));   // |dx| + 2*endPad
        assert(nearlyEqual(f.height, W));             // bandWidth
        assert(f.axisAligned);
    }

    // 2. VERTICAL pair (shared x) -> tall AABB, axis-aligned + tight.
    {
        const double W = 0.4, P = 0.1;
        const SpanFootprint f = deriveSpanFootprint({3.0, 0.0}, {3.0, 8.0}, W, P);
        assert(nearlyEqual(f.width, W));              // bandWidth
        assert(nearlyEqual(f.height, 8.0 + 2 * P));   // |dy| + 2*endPad
        assert(nearlyEqual(f.origin.x, 3.0 - W / 2)); // centered on shared x
        assert(nearlyEqual(f.origin.y, -P));
        assert(f.axisAligned);
    }

    // 3. A/B ORDER INDEPENDENCE: swapping anchors yields the identical footprint.
    {
        const double W = 0.3, P = 0.07;
        const SpanFootprint ab = deriveSpanFootprint({1.0, 2.0}, {9.0, 2.0}, W, P);
        const SpanFootprint ba = deriveSpanFootprint({9.0, 2.0}, {1.0, 2.0}, W, P);
        assert(nearlyEqual(ab.origin.x, ba.origin.x));
        assert(nearlyEqual(ab.origin.y, ba.origin.y));
        assert(nearlyEqual(ab.width, ba.width));
        assert(nearlyEqual(ab.height, ba.height));
        assert(ab.axisAligned == ba.axisAligned);
    }

    // 4. bandWidth scales the PERPENDICULAR dim; endPad scales the ALONG-AXIS dim.
    {
        // Horizontal pair: bandWidth -> height, endPad -> width.
        const SpanFootprint base = deriveSpanFootprint({0.0, 0.0}, {10.0, 0.0}, 0.2, 0.05);
        const SpanFootprint wide = deriveSpanFootprint({0.0, 0.0}, {10.0, 0.0}, 0.6, 0.05);
        // bandWidth tripled -> height tripled; width unchanged.
        assert(nearlyEqual(wide.height, base.height * 3.0));
        assert(nearlyEqual(wide.width, base.width));

        const SpanFootprint padded = deriveSpanFootprint({0.0, 0.0}, {10.0, 0.0}, 0.2, 0.20);
        // endPad grew by 0.15 each end -> width grows by 0.30; height unchanged.
        assert(nearlyEqual(padded.width, base.width + 0.30));
        assert(nearlyEqual(padded.height, base.height));
    }

    // 5. CENTERED: footprint center == anchor-segment midpoint (axis-aligned cases).
    {
        const SpanFootprint f = deriveSpanFootprint({2.0, 7.0}, {12.0, 7.0}, 0.4, 0.1);
        const double cx = f.origin.x + f.width / 2;
        const double cy = f.origin.y + f.height / 2;
        assert(nearlyEqual(cx, (2.0 + 12.0) / 2));
        assert(nearlyEqual(cy, 7.0));
    }

    // 6. DIAGONAL pair -> axisAligned == false; AABB CONTAINS both anchors and all four corners.
    {
        const double W = 0.4, P = 0.1;
        const Point2D A{0.0, 0.0}, B{10.0, 10.0};
        const SpanFootprint f = deriveSpanFootprint(A, B, W, P);
        assert(!f.axisAligned);
        // Both anchors are inside the AABB.
        assert(A.x >= f.origin.x - 1e-9 && A.x <= f.origin.x + f.width + 1e-9);
        assert(A.y >= f.origin.y - 1e-9 && A.y <= f.origin.y + f.height + 1e-9);
        assert(B.x >= f.origin.x - 1e-9 && B.x <= f.origin.x + f.width + 1e-9);
        assert(B.y >= f.origin.y - 1e-9 && B.y <= f.origin.y + f.height + 1e-9);

        // All four widened/pushed-out corners are inside the AABB (valid bound).
        const double dx = B.x - A.x, dy = B.y - A.y;
        const double L = std::hypot(dx, dy);
        const double ux = dx / L, uy = dy / L, px = -uy, py = ux, half = W / 2;
        const double a0x = A.x - ux * P, a0y = A.y - uy * P;
        const double b0x = B.x + ux * P, b0y = B.y + uy * P;
        const double cx[4] = { a0x + px * half, a0x - px * half, b0x + px * half, b0x - px * half };
        const double cy[4] = { a0y + py * half, a0y - py * half, b0y + py * half, b0y - py * half };
        for (int i = 0; i < 4; ++i) {
            assert(cx[i] >= f.origin.x - 1e-9 && cx[i] <= f.origin.x + f.width + 1e-9);
            assert(cy[i] >= f.origin.y - 1e-9 && cy[i] <= f.origin.y + f.height + 1e-9);
        }
    }

    // 7. DEGENERATE A==B -> bandWidth x bandWidth box centered on A, axis-aligned, no NaN.
    {
        const double W = 0.4;
        const Point2D A{4.0, 6.0};
        const SpanFootprint f = deriveSpanFootprint(A, A, W, 0.1);
        assert(nearlyEqual(f.width, W));
        assert(nearlyEqual(f.height, W));
        assert(nearlyEqual(f.origin.x, A.x - W / 2));
        assert(nearlyEqual(f.origin.y, A.y - W / 2));
        assert(f.axisAligned);
        assert(!std::isnan(f.origin.x) && !std::isnan(f.origin.y));
        assert(!std::isnan(f.width) && !std::isnan(f.height));
    }

    // 8. DEFAULT PARAMS: anchors-only call uses kDefaultSpanBandWidth/kDefaultSpanEndPad.
    {
        const SpanFootprint def = deriveSpanFootprint({0.0, 0.0}, {10.0, 0.0});
        const SpanFootprint exp = deriveSpanFootprint({0.0, 0.0}, {10.0, 0.0},
                                                      kDefaultSpanBandWidth, kDefaultSpanEndPad);
        assert(nearlyEqual(def.origin.x, exp.origin.x));
        assert(nearlyEqual(def.origin.y, exp.origin.y));
        assert(nearlyEqual(def.width, exp.width));
        assert(nearlyEqual(def.height, exp.height));
        assert(def.axisAligned == exp.axisAligned);
    }

    return 0;
}
