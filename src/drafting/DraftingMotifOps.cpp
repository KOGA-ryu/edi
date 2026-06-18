#include "drafting/DraftingMotifOps.h"

#include "drafting/DraftingGeometry.h" // computeBounds, includeBounds, translateGeometry

#include <utility>

namespace edi::drafting {

std::optional<std::size_t> motifIndexByName(const DraftingDocument &document,
                                             const std::string &name)
{
    for (std::size_t index = 0; index < document.motifs.size(); ++index) {
        if (document.motifs[index].name == name) {
            return index;
        }
    }
    return std::nullopt;
}

DraftingMotif buildMotifFromObjects(std::string name,
                                    const std::vector<DraftingObject> &sources)
{
    DraftingMotif motif;
    motif.name = std::move(name);

    // STEP 1 — filter: exclude Guide objects (no re-droppable local position).
    // ConstructionLines and all other kinds are kept — they translate correctly.
    // bounds stays {0,0,0,0} when nothing survives the filter; addMotif rejects that.
    std::vector<const DraftingObject *> kept;
    kept.reserve(sources.size());
    for (const DraftingObject &object : sources) {
        if (object.kind != DraftingShapeKind::Guide) {
            kept.push_back(&object);
        }
    }
    if (kept.empty()) {
        return motif; // empty; addMotif rejects it
    }

    // STEP 2 — compute the union bounds from GEOMETRY (not the cached box, which
    // may be stale).  Mirrors the block logic exactly.
    Bounds2D unionBounds = computeBounds(kept.front()->geometry);
    for (std::size_t i = 1; i < kept.size(); ++i) {
        unionBounds = includeBounds(unionBounds, computeBounds(kept[i]->geometry));
    }
    const double dx = -unionBounds.x;
    const double dy = -unionBounds.y;

    // STEP 3 — copy, translate, recompute, reset lock/visible.
    motif.objects.reserve(kept.size());
    for (const DraftingObject *source : kept) {
        DraftingObject copy = *source;           // COPY: motif owns its geometry
        copy.geometry = translateGeometry(copy.geometry, dx, dy);
        copy.bounds   = computeBounds(copy.geometry); // recompute, not just shift
        // Reset template-specific state so a future placement starts clean —
        // a locked or hidden source object must not produce a frozen placement.
        copy.locked  = false;
        copy.visible = true;
        motif.objects.push_back(std::move(copy));
    }
    // STEP 4 — cache the origin-based union extent (not serialised; recomputed on load).
    motif.bounds = {0.0, 0.0, unionBounds.width, unionBounds.height};
    return motif;
}

DraftingStoreResult addMotif(DraftingDocument &document, DraftingMotif motif)
{
    // A motif is name-keyed (like DraftingMapRoom), so the name must be present
    // and unique within the document — addMapRoom's shape.
    if (motif.name.empty()) {
        return DraftingStoreResult::rejected(DraftingResultCode::EmptyObjectId,
                                             "motif name is required");
    }
    if (motifIndexByName(document, motif.name)) {
        return DraftingStoreResult::rejected(DraftingResultCode::DuplicateObjectId,
                                             "motif name already exists");
    }
    // A template must define SOMETHING — an empty motif would place nothing, just
    // as an empty block would.
    if (motif.objects.empty()) {
        return DraftingStoreResult::rejected(DraftingResultCode::InvalidSelectionTarget,
                                             "motif must contain at least one object");
    }

    document.motifs.push_back(std::move(motif));
    ++document.revision;
    return DraftingStoreResult::accepted();
}

DraftingStoreResult removeMotif(DraftingDocument &document, const std::string &name)
{
    const auto index = motifIndexByName(document, name);
    if (!index) {
        return DraftingStoreResult::rejected(DraftingResultCode::ObjectNotFound,
                                             "motif does not exist");
    }
    // No cascade: motif stores independent copies; nothing in the document
    // references a motif by name at the data level yet (placements are S2).
    document.motifs.erase(document.motifs.begin() + static_cast<std::ptrdiff_t>(*index));
    ++document.revision;
    return DraftingStoreResult::accepted();
}

DraftingArrayResult placeMotif(const DraftingMotif &motif,
                               Point2D point,
                               const std::vector<DraftingObjectId> &newObjectIds)
{
    if (motif.objects.empty()) {
        return DraftingArrayResult::rejected(DraftingResultCode::InvalidGeometry,
                                             "cannot place an empty motif");
    }
    if (newObjectIds.size() != motif.objects.size()) {
        return DraftingArrayResult::rejected(DraftingResultCode::InvalidGeometry,
                                             "newObjectIds count must match motif object count");
    }

    // Motif objects are normalized to origin (0,0); translating by `point` places
    // the motif's lower-left corner at `point`.  Each copy is a fully independent
    // DraftingObject — FLATTEN means NO back-reference to the motif.
    std::vector<DraftingObject> placed;
    placed.reserve(motif.objects.size());
    for (std::size_t i = 0; i < motif.objects.size(); ++i) {
        DraftingObject copy = motif.objects[i];       // deep copy (owns its geometry)
        copy.geometry = translateGeometry(copy.geometry, point.x, point.y);
        copy.bounds   = computeBounds(copy.geometry); // recompute; do not shift the box
        copy.id       = newObjectIds[i];              // fresh id — no motif id carried
        placed.push_back(std::move(copy));
    }
    return DraftingArrayResult::accepted(std::move(placed));
}

} // namespace edi::drafting
