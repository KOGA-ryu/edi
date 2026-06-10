#include "drafting/DraftingClipboard.h"

#include "drafting/DraftingGeometry.h"

namespace edi::drafting {

DraftingPasteResult planDraftingPaste(
    const std::vector<DraftingObject> &sources,
    const std::string &prefix, int startSerial, double dx, double dy)
{
    DraftingPasteResult result;
    result.objects.reserve(sources.size());
    int serial = startSerial;
    for (const DraftingObject &source : sources) {
        DraftingObject clone = source;
        // Fresh id first: two pastes of the same clipboard must not collide,
        // and a paste must not collide with the original it came from.
        clone.id = draftingObjectIdForSerial(prefix, serial++);
        clone.geometry = translateGeometry(source.geometry, dx, dy);
        // Bounds are derived data — recompute rather than translate the old
        // box, so a geometry whose bounds were stale can't poison the clone.
        clone.bounds = computeBounds(clone.geometry);
        result.objects.push_back(std::move(clone));
    }
    result.nextSerial = serial;
    return result;
}

} // namespace edi::drafting
