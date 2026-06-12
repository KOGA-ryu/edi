#include "recipe/RecipeMeasure.h"

#include "drafting/DraftingGeometry.h"
#include "drafting/DraftingPhysicalGeometry.h"

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

} // namespace edi::recipe
