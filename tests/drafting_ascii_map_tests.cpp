#include "drafting/DraftingAsciiMap.h"
#include "drafting/DraftingGeometry.h"

#include "EdiAssert.h"
#include <cmath>
#include <functional>
#include <memory>
#include <string>

using namespace edi::drafting;

namespace {

bool nearlyEqual(double a, double b, double eps = 0.000001)
{
    return std::abs(a - b) <= eps;
}

std::function<DraftingObjectId()> counter()
{
    auto n = std::make_shared<int>(0);
    return [n]() { return "a" + std::to_string((*n)++); };
}

// A hollow room: a door (+) hosted in the right wall, a monster (M) inside.
const std::string kMap =
    "######\n"
    "#....#\n"
    "#.M..+\n"
    "#....#\n"
    "######";

} // namespace

int main()
{
    const AsciiMapParseResult parsed = parseAsciiMap(kMap);
    EDI_CHECK(parsed.ok);
    const AsciiMap &map = parsed.map;
    EDI_CHECK(map.rows == 5 && map.cols == 6);

    // The glyphs resolved to kinds + neutral tags.
    EDI_CHECK(map.at(0, 0).kind == AsciiCellKind::Wall);
    EDI_CHECK(map.at(1, 1).kind == AsciiCellKind::Floor);
    EDI_CHECK(map.at(2, 2).kind == AsciiCellKind::Feature && map.at(2, 2).tag == "monster");
    EDI_CHECK(map.at(2, 5).kind == AsciiCellKind::Door && map.at(2, 5).tag == "door");

    // THE CONTROL GATE: the parsed map re-renders to exactly what was authored —
    // "what you see is what gets built". A wrong parse would diverge here.
    EDI_CHECK(renderAsciiMap(map) == kMap);

    // Geometry: walls from runs (the door splits the right wall into two), plus a
    // tagged marker for the door and the monster.
    {
        const AsciiMapBuildResult plan = planAsciiMapGeometry(map, {0.0, 0.0}, 0.1, counter());
        EDI_CHECK(plan.ok);

        int walls = 0, doors = 0, features = 0;
        DraftingObject doorObj, featureObj;
        for (const DraftingObject &obj : plan.objects) {
            if (obj.kind == DraftingShapeKind::Wall) {
                ++walls;
            } else if (obj.kind == DraftingShapeKind::Point) {
                if (obj.metadata.toolProvenance == "ascii-door") {
                    ++doors;
                    doorObj = obj;
                } else if (obj.metadata.toolProvenance == "ascii-feature") {
                    ++features;
                    featureObj = obj;
                }
            }
        }
        // top, bottom, left column, right-upper, right-lower (split by the door) = 5.
        EDI_CHECK(walls == 5);
        EDI_CHECK(doors == 1 && features == 1);

        // The door marker sits at the centre of its cell (2,5): (5.5, 2.5) * 0.1.
        EDI_CHECK(doorObj.metadata.tags.size() == 1 && doorObj.metadata.tags[0] == "door");
        const auto doorPt = std::get<PointGeometry>(doorObj.geometry).point;
        EDI_CHECK(nearlyEqual(doorPt.x, 0.55) && nearlyEqual(doorPt.y, 0.25));
        // The monster marker at cell (2,2): (2.5, 2.5) * 0.1.
        EDI_CHECK(featureObj.metadata.tags.size() == 1 && featureObj.metadata.tags[0] == "monster");
        const auto featPt = std::get<PointGeometry>(featureObj.geometry).point;
        EDI_CHECK(nearlyEqual(featPt.x, 0.25) && nearlyEqual(featPt.y, 0.25));

        // A wall band is one cell THICK (so corners fill by overlap).
        for (const DraftingObject &obj : plan.objects) {
            if (obj.kind == DraftingShapeKind::Wall) {
                EDI_CHECK(nearlyEqual(std::get<WallGeometry>(obj.geometry).thickness, 0.1));
            }
        }
    }

    // An isolated wall glyph (no run) still builds as a single-cell square.
    {
        const AsciiMapParseResult lone = parseAsciiMap(".#.\n...");
        EDI_CHECK(lone.ok);
        const AsciiMapBuildResult plan = planAsciiMapGeometry(lone.map, {0.0, 0.0}, 0.1, counter());
        EDI_CHECK(plan.ok && plan.objects.size() == 1);
        EDI_CHECK(plan.objects[0].kind == DraftingShapeKind::Wall);
    }

    // Empty input is refused; a map with only floor builds nothing.
    {
        EDI_CHECK(!parseAsciiMap("").ok);
        const AsciiMapParseResult floors = parseAsciiMap("...\n...");
        EDI_CHECK(floors.ok);
        EDI_CHECK(!planAsciiMapGeometry(floors.map, {0.0, 0.0}, 0.1, counter()).ok);
    }

    return 0;
}
