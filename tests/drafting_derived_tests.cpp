#include "drafting/DraftingDerived.h"

#include "EdiAssert.h"
#include <cmath>
#include <string>
#include <vector>

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
    // Three points on the unit circle centered at the origin recover (0,0), r=1.
    {
        const DraftingDerivedCircleResult result = circleThroughThreePoints({1.0, 0.0}, {0.0, 1.0}, {-1.0, 0.0});
        EDI_CHECK(result.ok);
        EDI_CHECK(nearlyEqual(result.geometry.center.x, 0.0));
        EDI_CHECK(nearlyEqual(result.geometry.center.y, 0.0));
        EDI_CHECK(nearlyEqual(result.geometry.radius, 1.0));
    }

    // An off-origin circle: three points on a circle centered (2,3) radius 5.
    {
        const DraftingDerivedCircleResult result = circleThroughThreePoints({7.0, 3.0}, {2.0, 8.0}, {-3.0, 3.0});
        EDI_CHECK(result.ok);
        EDI_CHECK(nearlyEqual(result.geometry.center.x, 2.0));
        EDI_CHECK(nearlyEqual(result.geometry.center.y, 3.0));
        EDI_CHECK(nearlyEqual(result.geometry.radius, 5.0));
    }

    // Collinear triple rejects with a code + message.
    {
        const DraftingDerivedCircleResult result = circleThroughThreePoints({0.0, 0.0}, {1.0, 0.0}, {2.0, 0.0});
        EDI_CHECK(!result.ok);
        EDI_CHECK(result.code == DraftingResultCode::InvalidGeometry);
        EDI_CHECK(!result.message.empty());
    }

    // Non-finite input rejects.
    {
        const DraftingDerivedCircleResult result = circleThroughThreePoints({std::nan(""), 0.0}, {0.0, 1.0}, {-1.0, 0.0});
        EDI_CHECK(!result.ok);
        EDI_CHECK(result.code == DraftingResultCode::InvalidGeometry);
    }

    // Two-point diameter: (0,0)-(2,0) → center (1,0), radius 1.
    {
        const DraftingDerivedCircleResult result = circleThroughTwoPoints({0.0, 0.0}, {2.0, 0.0});
        EDI_CHECK(result.ok);
        EDI_CHECK(nearlyEqual(result.geometry.center.x, 1.0));
        EDI_CHECK(nearlyEqual(result.geometry.center.y, 0.0));
        EDI_CHECK(nearlyEqual(result.geometry.radius, 1.0));
    }

    // A diagonal diameter: (1,1)-(4,5) → center (2.5,3), radius = 5/2.
    {
        const DraftingDerivedCircleResult result = circleThroughTwoPoints({1.0, 1.0}, {4.0, 5.0});
        EDI_CHECK(result.ok);
        EDI_CHECK(nearlyEqual(result.geometry.center.x, 2.5));
        EDI_CHECK(nearlyEqual(result.geometry.center.y, 3.0));
        EDI_CHECK(nearlyEqual(result.geometry.radius, 2.5));
    }

    // Coincident points reject (zero-diameter circle).
    {
        const DraftingDerivedCircleResult result = circleThroughTwoPoints({0.5, 0.5}, {0.5, 0.5});
        EDI_CHECK(!result.ok);
        EDI_CHECK(result.code == DraftingResultCode::InvalidGeometry);
        EDI_CHECK(!result.message.empty());
    }

    // --- DR-06: divide circle + inscribe regular / star polygons. ---
    const CircleGeometry unit{{0.0, 0.0}, 1.0};
    const double pi = 3.14159265358979323846;

    // divideCirclePoints: 6 hexagon vertices at 0/60/120/180/240/300°.
    {
        const std::vector<Point2D> points = divideCirclePoints(unit, 6);
        EDI_CHECK(points.size() == 6);
        EDI_CHECK(pointNear(points[0], 1.0, 0.0));
        EDI_CHECK(pointNear(points[1], std::cos(pi / 3.0), std::sin(pi / 3.0))); // 60°
        EDI_CHECK(pointNear(points[3], -1.0, 0.0));                               // 180°
        // n < 2 yields an empty list.
        EDI_CHECK(divideCirclePoints(unit, 1).empty());
    }

    // inscribeRegularPolygon: vertices match divideCirclePoints in order; n<3 rejects.
    {
        const DraftingInscribedResult hexagon = inscribeRegularPolygon(unit, 6);
        EDI_CHECK(hexagon.ok);
        const std::vector<Point2D> expected = divideCirclePoints(unit, 6);
        EDI_CHECK(hexagon.geometry.vertices.size() == expected.size());
        for (std::size_t i = 0; i < expected.size(); ++i) {
            EDI_CHECK(pointNear(hexagon.geometry.vertices[i], expected[i].x, expected[i].y));
        }

        const DraftingInscribedResult tooFew = inscribeRegularPolygon(unit, 2);
        EDI_CHECK(!tooFew.ok && tooFew.code == DraftingResultCode::InvalidGeometry && !tooFew.message.empty());
    }

    // inscribeStarPolygon {5/2} (pentagram): visits indices 0,2,4,1,3.
    {
        const DraftingInscribedResult star = inscribeStarPolygon(unit, 5, 2);
        EDI_CHECK(star.ok);
        const std::vector<Point2D> points = divideCirclePoints(unit, 5);
        const int order[5] = {0, 2, 4, 1, 3};
        EDI_CHECK(star.geometry.vertices.size() == 5);
        for (std::size_t i = 0; i < 5; ++i) {
            EDI_CHECK(pointNear(star.geometry.vertices[i], points[order[i]].x, points[order[i]].y));
        }
    }

    // Invalid {n/k} cases each reject with InvalidGeometry.
    {
        EDI_CHECK(!inscribeStarPolygon(unit, 5, 1).ok); // step 1 = regular polygon
        EDI_CHECK(inscribeStarPolygon(unit, 5, 1).code == DraftingResultCode::InvalidGeometry);
        EDI_CHECK(!inscribeStarPolygon(unit, 7, 4).ok); // step >= n/2 (retraces complement)
        EDI_CHECK(inscribeStarPolygon(unit, 7, 4).code == DraftingResultCode::InvalidGeometry);
        EDI_CHECK(!inscribeStarPolygon(unit, 6, 2).ok); // gcd(6,2)=2 → compound ({6/2}=2 triangles)
        EDI_CHECK(inscribeStarPolygon(unit, 6, 2).code == DraftingResultCode::InvalidGeometry);
        EDI_CHECK(!inscribeStarPolygon(unit, 4, 2).ok); // n < 5 → no proper star
    }

    return 0;
}
