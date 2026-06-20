#include "drafting/DraftingClipboard.h"

#include "drafting/DraftingGeometry.h"
#include "drafting/DraftingStore.h"

#include "EdiAssert.h"
#include <cmath>
#include <string>
#include <utility>

using namespace edi::drafting;

namespace {

bool near(double a, double b) { return std::abs(a - b) < 1e-9; }

DraftingObject object(DraftingObjectId id, DraftingShapeKind kind, DraftingGeometry geometry)
{
    auto built = buildDraftingObject(std::move(id), kind, std::move(geometry));
    EDI_CHECK(built.ok);
    return built.object;
}

} // namespace

int main()
{
    // Two sources: a point and a line, each on a non-default layer / locked, to
    // prove the carry-over is verbatim except id/geometry/bounds.
    DraftingObject point = object("point_0001", DraftingShapeKind::Point, PointGeometry{{0.2, 0.3}});
    point.layerId = "annotations";
    point.locked = true;
    DraftingObject line = object("line_0007", DraftingShapeKind::Line, LineGeometry{{0.1, 0.1}, {0.4, 0.5}});

    const DraftingPasteResult pasted = planDraftingPaste({point, line}, "paste", 10, 0.05, -0.05);

    // Source order preserved, serials minted in sequence from startSerial.
    EDI_CHECK(pasted.objects.size() == 2);
    EDI_CHECK(pasted.objects[0].id == "paste_0010");
    EDI_CHECK(pasted.objects[1].id == "paste_0011");
    EDI_CHECK(pasted.nextSerial == 12); // past the last minted

    // Fresh ids never collide with the originals.
    EDI_CHECK(pasted.objects[0].id != point.id);
    EDI_CHECK(pasted.objects[1].id != line.id);

    // Geometry translated by the offset; bounds recomputed to match.
    const auto *pastedPoint = std::get_if<PointGeometry>(&pasted.objects[0].geometry);
    EDI_CHECK(pastedPoint != nullptr);
    EDI_CHECK(near(pastedPoint->point.x, 0.25) && near(pastedPoint->point.y, 0.25));
    const auto *pastedLine = std::get_if<LineGeometry>(&pasted.objects[1].geometry);
    EDI_CHECK(pastedLine != nullptr);
    EDI_CHECK(near(pastedLine->a.x, 0.15) && near(pastedLine->b.y, 0.45));
    const Bounds2D expected = computeBounds(pasted.objects[1].geometry);
    EDI_CHECK(near(pasted.objects[1].bounds.x, expected.x) && near(pasted.objects[1].bounds.width, expected.width));

    // Non-geometry attributes carry over verbatim — a paste is the same object
    // elsewhere, not a new kind of thing.
    EDI_CHECK(pasted.objects[0].layerId == "annotations");
    EDI_CHECK(pasted.objects[0].locked);
    EDI_CHECK(pasted.objects[0].kind == DraftingShapeKind::Point);

    // Empty clipboard: nothing minted, serial unmoved.
    const DraftingPasteResult none = planDraftingPaste({}, "paste", 10, 0.05, 0.05);
    EDI_CHECK(none.objects.empty() && none.nextSerial == 10);

    return 0;
}
