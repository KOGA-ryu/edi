// DM-01: documentObjectsBounds folds every object's cached bounds into one box, or
// returns nullopt for an empty document (so edi-ui's fit-view can no-op rather than
// fit a degenerate rect). Pure free function — no Qt.
#include "drafting/DraftingDocument.h"
#include "drafting/DraftingGeometry.h"

#include "EdiAssert.h"
#include <optional>

using namespace edi::drafting;

namespace {

bool nearlyEqual(double a, double b)
{
    return std::abs(a - b) < 0.000001;
}

DraftingObject pointAt(const std::string &id, Point2D p)
{
    DraftingObject o = makeDraftingObject(id, DraftingShapeKind::Point, PointGeometry{p});
    o.bounds = computeBounds(o.geometry);
    return o;
}

} // namespace

int main()
{
    // Empty document -> nullopt (no extent to fit).
    {
        DraftingDocument doc = makeDraftingDocument("empty");
        EDI_CHECK(!documentObjectsBounds(doc).has_value());
    }

    // A single object -> its own bounds.
    {
        DraftingDocument doc = makeDraftingDocument("one");
        DraftingObject rect = makeDraftingObject("r", DraftingShapeKind::Rectangle,
                                                 RectangleGeometry{{0.2, 0.3}, 0.4, 0.5, 0.0, 0.0, 0.0});
        rect.bounds = computeBounds(rect.geometry);
        doc.objects = {rect};
        const auto bounds = documentObjectsBounds(doc);
        EDI_CHECK(bounds.has_value());
        EDI_CHECK(nearlyEqual(bounds->x, rect.bounds.x) && nearlyEqual(bounds->y, rect.bounds.y));
        EDI_CHECK(nearlyEqual(bounds->width, rect.bounds.width) && nearlyEqual(bounds->height, rect.bounds.height));
    }

    // N objects -> the union min/max across all of them.
    {
        DraftingDocument doc = makeDraftingDocument("many");
        doc.objects = {
            pointAt("a", {0.1, 0.2}),
            pointAt("b", {0.7, 0.9}),
            pointAt("c", {0.4, 0.05}),
        };
        const auto bounds = documentObjectsBounds(doc);
        EDI_CHECK(bounds.has_value());
        // min x = 0.1, min y = 0.05; max x = 0.7, max y = 0.9.
        EDI_CHECK(nearlyEqual(bounds->x, 0.1) && nearlyEqual(bounds->y, 0.05));
        EDI_CHECK(nearlyEqual(bounds->x + bounds->width, 0.7));
        EDI_CHECK(nearlyEqual(bounds->y + bounds->height, 0.9));
    }

    return 0;
}
