#include "recipe/RecipeMeasure.h"

#include "drafting/DraftingGeometry.h"
#include "drafting/DraftingPhysicalGeometry.h"

#include <cmath>

namespace edi::recipe {

// Moved verbatim from RecipeDocument.cpp's anonymous-namespace
// resolveMeasurement (R1-B03): the dispatch, the bounds computation, and the
// X-axis radius convention are byte-for-byte the same logic — only the
// out-shape changed (a bare MeasureFieldResult instead of A's ResolvedParam
// wrapper). The B01 contract pins in recipe_document_tests.cpp prove the move
// preserved behavior.
MeasureFieldResult resolveMeasurementField(
    const edi::drafting::DraftingDocument &drafting,
    const edi::drafting::DraftingGridProjection &grid,
    const std::string &objectId,
    const std::string &field)
{
    using namespace edi::drafting;

    MeasureFieldResult result;

    const DraftingObject *object = findObject(drafting, objectId);
    if (object == nullptr) {
        result.message = "object not found: " + objectId;
        return result;
    }
    const Bounds2D bounds = computeBounds(object->geometry);

    if (field == "width") {
        result.value = physicalWidth(bounds.width, grid);
        result.ok = true;
    } else if (field == "height") {
        result.value = physicalHeight(bounds.height, grid);
        result.ok = true;
    } else if (field == "length") {
        if (const auto *line = std::get_if<LineGeometry>(&object->geometry)) {
            result.value = physicalDistance(line->a, line->b, grid);
            result.ok = true;
        } else {
            result.message = "length needs a line";
        }
    } else if (field == "radius") {
        // Radius scales along the grid's X axis — the same convention the
        // canvas projection uses for physical circle readouts.
        if (const auto *circle = std::get_if<CircleGeometry>(&object->geometry)) {
            result.value = physicalWidth(circle->radius, grid);
            result.ok = true;
        } else if (const auto *arc = std::get_if<ArcGeometry>(&object->geometry)) {
            result.value = physicalWidth(arc->radius, grid);
            result.ok = true;
        } else {
            result.message = "radius needs a circle or arc";
        }
    } else {
        result.message = "unknown measurement field: " + field;
    }
    return result;
}

namespace {

// Document-space profile points for the supported source kinds — moved
// verbatim from RecipeDocument.cpp (R1-B04). An arc is sampled
// deterministically — 64 segments per full circle, endpoints exact — so
// the same drafted arc always yields the same mesh, byte for byte.
struct ProfileSource {
    bool ok = false;
    std::string message;
    std::vector<edi::drafting::Point2D> points;
};

ProfileSource profileSourcePoints(const edi::drafting::DraftingObject &object)
{
    using namespace edi::drafting;
    ProfileSource source;
    if (const auto *line = std::get_if<LineGeometry>(&object.geometry)) {
        source.points = {line->a, line->b};
        source.ok = true;
    } else if (const auto *polyline = std::get_if<PolylineGeometry>(&object.geometry)) {
        source.points = polyline->vertices;
        // The drafting ops validate vertex counts, but the .edidraw LOAD
        // path does not re-validate geometry — a corrupted file can deliver
        // a short polyline here, so this branch is a real contract.
        source.ok = source.points.size() >= 2;
        if (!source.ok) {
            source.message = "profile polyline needs at least two vertices";
        }
    } else if (const auto *arc = std::get_if<ArcGeometry>(&object.geometry)) {
        const double sweepDeg = arc->endAngleDeg - arc->startAngleDeg;
        const int steps = std::max(1, static_cast<int>(std::lround(std::abs(sweepDeg) / 360.0 * 64.0)));
        source.points.reserve(static_cast<std::size_t>(steps) + 1);
        for (int i = 0; i <= steps; ++i) {
            const double angleDeg = arc->startAngleDeg + sweepDeg * (static_cast<double>(i) / steps);
            const double angleRad = angleDeg * M_PI / 180.0;
            source.points.push_back({
                arc->center.x + arc->radius * std::cos(angleRad),
                arc->center.y + arc->radius * std::sin(angleRad),
            });
        }
        source.ok = true;
    } else {
        source.message = "profile needs a line, polyline, or arc";
    }
    return source;
}

} // namespace

// The page-to-part mapping, the convention the whole lathe rests on: the
// page's LEFT EDGE is the spin axis (drafted x = radius, scaled along the
// grid's X like every radius measurement), and the page BOTTOM is z = 0
// (drafted y points down, parts stand up — hence 1 - y). Explicit, never
// inferred: an auto-detected axis would be guesswork wearing a convenience
// costume.
ProfilePointsResult resolveProfilePoints(
    const edi::drafting::DraftingDocument &drafting,
    const edi::drafting::DraftingGridProjection &grid,
    const std::string &objectId)
{
    using namespace edi::drafting;

    ProfilePointsResult result;
    if (objectId.empty()) {
        result.message = "no profile bound";
        return result;
    }
    const DraftingObject *object = findObject(drafting, objectId);
    if (object == nullptr) {
        result.message = "profile object not found: " + objectId;
        return result;
    }
    ProfileSource source = profileSourcePoints(*object);
    if (!source.ok) {
        result.message = source.message;
        return result;
    }
    result.points.reserve(source.points.size());
    for (const Point2D &point : source.points) {
        result.points.push_back({
            physicalWidth(point.x, grid),
            physicalHeight(1.0 - point.y, grid),
        });
    }
    result.ok = true;
    return result;
}

} // namespace edi::recipe
