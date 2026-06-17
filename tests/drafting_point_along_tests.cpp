#include "drafting/DraftingSnap.h"

#include <cassert>
#include <cmath>
#include <string>

using namespace edi::drafting;

namespace {

bool nearlyEqual(double a, double b)
{
    return std::abs(a - b) < 0.000001;
}

bool pointNear(Point2D p, double x, double y)
{
    return nearlyEqual(p.x, x) && nearlyEqual(p.y, y);
}

} // namespace

int main()
{
    constexpr double pi = 3.14159265358979323846;

    // --- Line: half the length from the start lands at the midpoint. ---
    {
        const DraftingGeometry line = LineGeometry{{0.2, 0.4}, {0.8, 0.4}}; // length 0.6
        const DraftingPointAlongResult mid = pointAlongEntity(line, 0.3, false);
        assert(mid.ok);
        assert(pointNear(mid.point, 0.5, 0.4));

        // The SAME distance from the end is symmetric (mirror point about the mid).
        const DraftingPointAlongResult q1 = pointAlongEntity(line, 0.15, false);
        const DraftingPointAlongResult q2 = pointAlongEntity(line, 0.15, true);
        assert(q1.ok && q2.ok);
        assert(pointNear(q1.point, 0.35, 0.4));
        assert(pointNear(q2.point, 0.65, 0.4));

        // Overshoot rejects with a code + message.
        const DraftingPointAlongResult over = pointAlongEntity(line, 0.9, false);
        assert(!over.ok);
        assert(over.code == DraftingResultCode::InvalidGeometry);
        assert(!over.message.empty());

        // Exactly the length lands on the far endpoint (no overshoot).
        const DraftingPointAlongResult end = pointAlongEntity(line, 0.6, false);
        assert(end.ok && pointNear(end.point, 0.8, 0.4));
    }

    // --- Arc: a quarter of the arc length lands at a quarter of the sweep. ---
    {
        const DraftingGeometry arc = ArcGeometry{{0.5, 0.5}, 0.2, 0.0, 90.0};
        const double arcLength = 0.2 * (pi / 2.0); // radius * sweepRadians
        const DraftingPointAlongResult quarter = pointAlongEntity(arc, arcLength / 4.0, false);
        assert(quarter.ok);
        // distance/radius maps to angle: a quarter of a 90° sweep is 22.5°.
        assert(pointNear(quarter.point, 0.5 + 0.2 * std::cos(22.5 * pi / 180.0), 0.5 + 0.2 * std::sin(22.5 * pi / 180.0)));

        // From the end, a quarter lands at 67.5° (90° − 22.5°).
        const DraftingPointAlongResult fromEnd = pointAlongEntity(arc, arcLength / 4.0, true);
        assert(fromEnd.ok);
        assert(pointNear(fromEnd.point, 0.5 + 0.2 * std::cos(67.5 * pi / 180.0), 0.5 + 0.2 * std::sin(67.5 * pi / 180.0)));

        // Overshoot past the arc length rejects.
        const DraftingPointAlongResult over = pointAlongEntity(arc, arcLength * 2.0, false);
        assert(!over.ok && over.code == DraftingResultCode::InvalidGeometry);
    }

    // --- Polyline: a distance spanning into the 2nd segment lands on that edge. ---
    {
        // edge1 (0.1,0.1)->(0.4,0.1) length 0.3; edge2 (0.4,0.1)->(0.4,0.5) length 0.4.
        const DraftingGeometry polyline = PolylineGeometry{{{0.1, 0.1}, {0.4, 0.1}, {0.4, 0.5}}};
        const DraftingPointAlongResult into2nd = pointAlongEntity(polyline, 0.5, false); // 0.3 + 0.2
        assert(into2nd.ok);
        assert(pointNear(into2nd.point, 0.4, 0.3));

        // From the end, 0.2 walks down edge2 to the same spot.
        const DraftingPointAlongResult endSide = pointAlongEntity(polyline, 0.2, true);
        assert(endSide.ok);
        assert(pointNear(endSide.point, 0.4, 0.3));

        // Overshoot past the total (0.7) rejects.
        const DraftingPointAlongResult over = pointAlongEntity(polyline, 0.8, false);
        assert(!over.ok && over.code == DraftingResultCode::InvalidGeometry);
    }

    // --- Polygon: closed loop, walked into the 2nd edge. ---
    {
        // square (0,0)->(0.4,0)->(0.4,0.4)->(0,0.4)->(back to 0,0); perimeter 1.6.
        const DraftingGeometry polygon = PolygonGeometry{{{0.0, 0.0}, {0.4, 0.0}, {0.4, 0.4}, {0.0, 0.4}}};
        const DraftingPointAlongResult into2nd = pointAlongEntity(polygon, 0.6, false); // 0.4 + 0.2 up edge2
        assert(into2nd.ok);
        assert(pointNear(into2nd.point, 0.4, 0.2));
    }

    // --- Rejections: negative/non-finite distance, and unsupported kinds. ---
    {
        const DraftingGeometry line = LineGeometry{{0.2, 0.4}, {0.8, 0.4}};
        assert(!pointAlongEntity(line, -0.1, false).ok);
        assert(!pointAlongEntity(line, std::nan(""), false).ok);

        const DraftingGeometry circle = CircleGeometry{{0.5, 0.5}, 0.2};
        const DraftingPointAlongResult unsupported = pointAlongEntity(circle, 0.1, false);
        assert(!unsupported.ok);
        assert(unsupported.code == DraftingResultCode::InvalidGeometry);
        assert(!unsupported.message.empty());
    }

    return 0;
}
