#include "drafting/DraftingOverlap.h"

namespace edi::drafting {

// AABB overlap: two rectangles overlap iff they intersect in BOTH axes AND the
// intersection is strictly interior (> eps strip). The formula
//     A.min < B.max - eps  &&  B.min < A.max - eps
// is equivalent to "separation in this axis < -eps", which is true only when the
// rectangles interpenetrate (not just touch). Applying it to both x and y and
// ANDing gives the 2D interior intersection test.
//
// This is extracted verbatim from the former local `roomsOverlap` in
// RoomSpecStore.cpp (behavior-preserving refactor, same eps, same formula).
bool footprintsOverlap(Point2D originA, double widthA, double heightA,
                       Point2D originB, double widthB, double heightB,
                       double eps)
{
    const bool xOverlap = originA.x < originB.x + widthB  - eps
                       && originB.x < originA.x + widthA  - eps;
    const bool yOverlap = originA.y < originB.y + heightB - eps
                       && originB.y < originA.y + heightA - eps;
    return xOverlap && yOverlap;
}

} // namespace edi::drafting
