#include "drafting/DraftingSpan.h"

#include <algorithm>   // std::min / std::max
#include <cmath>       // std::hypot, std::abs

namespace edi::drafting {

// The span is the A->B segment widened perpendicular by `bandWidth` (centered) and extended along the
// axis by `endPad` past EACH end — a rotated rectangle. The footprint is the AABB of that rectangle's
// 4 corners.
//
// ONE unified formula: this single rotated-band -> AABB computation yields a TIGHT AABB when the pair
// is axis-aligned and a correct-but-LOOSE bound when diagonal. We deliberately do NOT branch the
// GEOMETRY on axis-alignment — branching would duplicate the corner math and risk the two paths
// drifting apart. The `axisAligned` flag is pure CLASSIFICATION (a shared x or shared y within eps),
// computed independently of the geometry; it tells the caller whether the AABB is exact.
//
// Sanity for the axis-aligned case: horizontal pair A=(0,5),B=(10,5), bandWidth W, endPad P ->
// ux=1,uy=0,px=0,py=1 -> corners x in [-P,10+P], y in [5-W/2,5+W/2] -> origin=(-P,5-W/2),
// width=10+2P, height=W, axisAligned=true. Tight + centered on the segment.
SpanFootprint deriveSpanFootprint(Point2D anchorA, Point2D anchorB,
                                  double bandWidth, double endPad)
{
    const double dx = anchorB.x - anchorA.x;
    const double dy = anchorB.y - anchorA.y;
    const double L  = std::hypot(dx, dy);
    const double half = bandWidth / 2.0;

    // Degenerate: coincident anchors. No axis is defined, so return a bandWidth x bandWidth box
    // centered on A — avoids dividing by zero (which would yield NaN unit vectors).
    if (L < kSpanAxisEps) {
        return { {anchorA.x - half, anchorA.y - half}, bandWidth, bandWidth, true };
    }

    const double ux = dx / L;   // unit along-axis
    const double uy = dy / L;
    const double px = -uy;      // unit perpendicular (axis rotated 90 deg)
    const double py = ux;

    // Segment ends pushed out by endPad along the axis.
    const double a0x = anchorA.x - ux * endPad;
    const double a0y = anchorA.y - uy * endPad;
    const double b0x = anchorB.x + ux * endPad;
    const double b0y = anchorB.y + uy * endPad;

    // 4 corners = each pushed-out end +/- perp*half.
    const double cornerX[4] = { a0x + px * half, a0x - px * half,
                                b0x + px * half, b0x - px * half };
    const double cornerY[4] = { a0y + py * half, a0y - py * half,
                                b0y + py * half, b0y - py * half };

    double minx = cornerX[0], maxx = cornerX[0];
    double miny = cornerY[0], maxy = cornerY[0];
    for (int i = 1; i < 4; ++i) {
        minx = std::min(minx, cornerX[i]);
        maxx = std::max(maxx, cornerX[i]);
        miny = std::min(miny, cornerY[i]);
        maxy = std::max(maxy, cornerY[i]);
    }

    const bool axisAligned = std::abs(dx) < kSpanAxisEps || std::abs(dy) < kSpanAxisEps;
    return { {minx, miny}, maxx - minx, maxy - miny, axisAligned };
}

} // namespace edi::drafting
