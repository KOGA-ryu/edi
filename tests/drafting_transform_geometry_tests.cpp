#include "drafting/DraftingGeometry.h"

#include "EdiAssert.h"
#include <cmath>
#include <string>
#include <variant>
#include <vector>

using namespace edi::drafting;

namespace {

bool nearlyEqual(double a, double b)
{
    return std::abs(a - b) < 0.000001;
}

bool pointsNear(Point2D a, Point2D b)
{
    return nearlyEqual(a.x, b.x) && nearlyEqual(a.y, b.y);
}

bool pointVecNear(const std::vector<Point2D> &a, const std::vector<Point2D> &b)
{
    if (a.size() != b.size()) {
        return false;
    }
    for (std::size_t i = 0; i < a.size(); ++i) {
        if (!pointsNear(a[i], b[i])) {
            return false;
        }
    }
    return true;
}

// Exhaustive near-equality over all 14 kinds — used for the identity and
// inverse-round-trip checks. Exhaustive on purpose (terminal static_assert), so a
// future 15th kind forces this comparator to be taught too.
bool geometryNear(const DraftingGeometry &ga, const DraftingGeometry &gb)
{
    if (ga.index() != gb.index()) {
        return false;
    }
    return std::visit([&](const auto &a) -> bool {
        using G = std::decay_t<decltype(a)>;
        const auto &b = std::get<G>(gb);
        if constexpr (std::is_same_v<G, PointGeometry>) {
            return pointsNear(a.point, b.point);
        } else if constexpr (std::is_same_v<G, LineGeometry>) {
            return pointsNear(a.a, b.a) && pointsNear(a.b, b.b);
        } else if constexpr (std::is_same_v<G, RectangleGeometry>) {
            return pointsNear(a.origin, b.origin) && nearlyEqual(a.width, b.width)
                && nearlyEqual(a.height, b.height) && nearlyEqual(a.rotationDeg, b.rotationDeg)
                && nearlyEqual(a.cornerRadius, b.cornerRadius) && nearlyEqual(a.inset, b.inset);
        } else if constexpr (std::is_same_v<G, CircleGeometry>) {
            return pointsNear(a.center, b.center) && nearlyEqual(a.radius, b.radius);
        } else if constexpr (std::is_same_v<G, ArcGeometry>) {
            return pointsNear(a.center, b.center) && nearlyEqual(a.radius, b.radius)
                && nearlyEqual(a.startAngleDeg, b.startAngleDeg) && nearlyEqual(a.endAngleDeg, b.endAngleDeg);
        } else if constexpr (std::is_same_v<G, EllipseGeometry>) {
            return pointsNear(a.center, b.center) && nearlyEqual(a.rx, b.rx) && nearlyEqual(a.ry, b.ry);
        } else if constexpr (std::is_same_v<G, PolygonGeometry> || std::is_same_v<G, PolylineGeometry>) {
            return pointVecNear(a.vertices, b.vertices);
        } else if constexpr (std::is_same_v<G, GuideGeometry>) {
            return a.orientation == b.orientation && nearlyEqual(a.position, b.position);
        } else if constexpr (std::is_same_v<G, ConstructionLineGeometry>) {
            return pointsNear(a.a, b.a) && pointsNear(a.b, b.b);
        } else if constexpr (std::is_same_v<G, DimensionGeometry>) {
            return a.kind == b.kind && pointsNear(a.a, b.a) && pointsNear(a.b, b.b) && nearlyEqual(a.offset, b.offset);
        } else if constexpr (std::is_same_v<G, TextAnnotationGeometry>) {
            return pointsNear(a.position, b.position) && a.content == b.content && nearlyEqual(a.height, b.height);
        } else if constexpr (std::is_same_v<G, SplineGeometry>) {
            return pointVecNear(a.controlPoints, b.controlPoints);
        } else if constexpr (std::is_same_v<G, WallGeometry>) {
            return pointsNear(a.a, b.a) && pointsNear(a.b, b.b) && nearlyEqual(a.thickness, b.thickness);
        } else {
            static_assert(always_false_v<G>, "geometryNear: unhandled DraftingGeometry arm");
        }
    }, ga);
}

// One representative of every kind, with non-degenerate fields.
std::vector<DraftingGeometry> allKinds()
{
    return {
        PointGeometry{{0.2, 0.5}},
        LineGeometry{{0.0, 0.0}, {1.0, 2.0}},
        RectangleGeometry{{0.1, 0.2}, 1.5, 0.8, 12.0, 0.05, 0.02},
        CircleGeometry{{0.4, 0.4}, 0.6},
        ArcGeometry{{0.2, 0.3}, 0.5, 10.0, 80.0},
        EllipseGeometry{{0.5, 0.5}, 0.7, 0.3},
        PolygonGeometry{{{0.0, 0.0}, {1.0, 0.0}, {1.0, 1.0}}},
        PolylineGeometry{{{0.0, 0.0}, {0.5, 0.5}, {1.0, 0.0}}},
        GuideGeometry{GuideOrientation::Horizontal, 0.5},
        ConstructionLineGeometry{{0.0, 0.0}, {2.0, 1.0}},
        DimensionGeometry{DimensionKind::Distance, {0.0, 0.0}, {1.0, 0.0}, 0.04},
        TextAnnotationGeometry{{0.2, 0.2}, "hi", 0.04},
        SplineGeometry{{{0.0, 0.0}, {0.3, 0.5}, {0.6, 0.1}, {1.0, 0.4}}},
        WallGeometry{{0.0, 0.0}, {1.0, 1.0}, 0.1},
    };
}

} // namespace

int main()
{
    const Point2D pivot{0.3, -0.7};

    // --- Per-kind IDENTITY: zero rotation + unit scale is a no-op for all 14. ---
    for (const DraftingGeometry &g : allKinds()) {
        const DraftingGeometry out = transformGeometry(g, pivot, 0.0, 1.0);
        EDI_CHECK(geometryNear(out, g));
    }

    // --- Inverse ROUND-TRIP: (pivot, θ, s) then (pivot, −θ, 1/s) ≈ original. ---
    // Holds for every kind, including the lossy Ellipse/Text — rotation never
    // touches their axis/baseline, so the loss is symmetric and cancels.
    {
        const double theta = 37.0;
        const double scale = 1.7;
        for (const DraftingGeometry &g : allKinds()) {
            const DraftingGeometry forward = transformGeometry(g, pivot, theta, scale);
            const DraftingGeometry back = transformGeometry(forward, pivot, -theta, 1.0 / scale);
            EDI_CHECK(geometryNear(back, g));
        }
    }

    // --- Value check: Point rotated 90° about origin (model matrix). ---
    {
        const DraftingGeometry out = transformGeometry(PointGeometry{{2.0, 0.0}}, {0.0, 0.0}, 90.0, 1.0);
        const auto &p = std::get<PointGeometry>(out);
        EDI_CHECK(pointsNear(p.point, {0.0, 2.0}));
    }

    // --- Rectangle ORBITS the pivot (the center-anchor fix), accumulates angle. ---
    // The square [0,2]×[0,2] rotated 90° about (0,0) must land at [-2,0]×[0,2] —
    // asserted on computeBounds (the rendered footprint), not just stored origin.
    {
        const DraftingGeometry out = transformGeometry(
            RectangleGeometry{{0.0, 0.0}, 2.0, 2.0, 0.0, 0.0, 0.0}, {0.0, 0.0}, 90.0, 1.0);
        const auto &r = std::get<RectangleGeometry>(out);
        EDI_CHECK(nearlyEqual(r.rotationDeg, 90.0));
        EDI_CHECK(nearlyEqual(r.width, 2.0) && nearlyEqual(r.height, 2.0));
        const Bounds2D bounds = computeBounds(out);
        EDI_CHECK(nearlyEqual(bounds.x, -2.0));
        EDI_CHECK(nearlyEqual(bounds.y, 0.0));
        EDI_CHECK(nearlyEqual(bounds.width, 2.0));
        EDI_CHECK(nearlyEqual(bounds.height, 2.0));
    }

    // --- Rectangle rotationDeg accumulates (30° + 45° = 75°) and w/h scale. ---
    {
        const DraftingGeometry out = transformGeometry(
            RectangleGeometry{{0.1, 0.1}, 1.0, 2.0, 30.0, 0.2, 0.1}, pivot, 45.0, 2.0);
        const auto &r = std::get<RectangleGeometry>(out);
        EDI_CHECK(nearlyEqual(r.rotationDeg, 75.0));
        EDI_CHECK(nearlyEqual(r.width, 2.0));
        EDI_CHECK(nearlyEqual(r.height, 4.0));
        EDI_CHECK(nearlyEqual(r.cornerRadius, 0.4));
        EDI_CHECK(nearlyEqual(r.inset, 0.2));
    }

    // --- Arc: start/end shift by the angle, radius scales, center maps. ---
    {
        const DraftingGeometry out = transformGeometry(
            ArcGeometry{{0.0, 0.0}, 2.0, 0.0, 90.0}, {0.0, 0.0}, 30.0, 2.0);
        const auto &arc = std::get<ArcGeometry>(out);
        EDI_CHECK(nearlyEqual(arc.startAngleDeg, 30.0));
        EDI_CHECK(nearlyEqual(arc.endAngleDeg, 120.0));
        EDI_CHECK(nearlyEqual(arc.radius, 4.0));
        EDI_CHECK(pointsNear(arc.center, {0.0, 0.0}));
    }

    // --- Circle: radius scales, stays a Circle (no demotion). ---
    {
        const DraftingGeometry out = transformGeometry(CircleGeometry{{0.4, 0.4}, 2.0}, pivot, 33.0, 1.5);
        const auto &c = std::get<CircleGeometry>(out);
        EDI_CHECK(nearlyEqual(c.radius, 3.0));
    }

    // --- Wall thickness scales; Dimension offset scales. ---
    {
        // Bind BY VALUE, not by const ref: transformGeometry returns a prvalue
        // DraftingGeometry temporary, and std::get<> on that prvalue yields a
        // reference INTO the temporary — destroyed at the end of this full
        // expression, so a `const auto &` dangles on the next line (D05;
        // -Wdangling-reference). Copying the small geometry struct out is the fix.
        const auto wall = std::get<WallGeometry>(
            transformGeometry(WallGeometry{{0.0, 0.0}, {1.0, 1.0}, 0.1}, pivot, 20.0, 3.0));
        EDI_CHECK(nearlyEqual(wall.thickness, 0.3));

        const auto dim = std::get<DimensionGeometry>(
            transformGeometry(DimensionGeometry{DimensionKind::Distance, {0.0, 0.0}, {1.0, 0.0}, 0.04}, pivot, 20.0, 2.0));
        EDI_CHECK(nearlyEqual(dim.offset, 0.08));
    }

    // --- PINNED v1 LIMITATION: Ellipse drops axis tilt — rotation only moves the
    //     center and scales rx/ry; the axes stay axis-aligned (no tilt field). ---
    {
        const DraftingGeometry out = transformGeometry(
            EllipseGeometry{{1.0, 0.0}, 2.0, 1.0}, {0.0, 0.0}, 90.0, 1.0);
        const auto &e = std::get<EllipseGeometry>(out);
        EDI_CHECK(pointsNear(e.center, {0.0, 1.0})); // center orbits the pivot
        EDI_CHECK(nearlyEqual(e.rx, 2.0));           // rx unchanged (NOT tilted/swapped)
        EDI_CHECK(nearlyEqual(e.ry, 1.0));           // ry unchanged
    }

    // --- PINNED v1 LIMITATION: TextAnnotation does NOT rotate — only the anchor
    //     moves and height scales; content is preserved. ---
    {
        const DraftingGeometry out = transformGeometry(
            TextAnnotationGeometry{{1.0, 0.0}, "label", 0.04}, {0.0, 0.0}, 90.0, 2.0);
        const auto &t = std::get<TextAnnotationGeometry>(out);
        EDI_CHECK(pointsNear(t.position, {0.0, 2.0}));
        EDI_CHECK(nearlyEqual(t.height, 0.08));
        EDI_CHECK(t.content == "label");
    }

    // --- PINNED: Guide is an IDENTITY no-op under any transform. ---
    {
        const DraftingGeometry out = transformGeometry(
            GuideGeometry{GuideOrientation::Horizontal, 0.5}, {3.0, 3.0}, 37.0, 2.5);
        const auto &gd = std::get<GuideGeometry>(out);
        EDI_CHECK(gd.orientation == GuideOrientation::Horizontal);
        EDI_CHECK(nearlyEqual(gd.position, 0.5));
    }

    return 0;
}
