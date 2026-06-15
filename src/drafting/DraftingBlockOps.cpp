#include "drafting/DraftingBlockOps.h"

#include "drafting/DraftingGeometry.h" // computeBounds, includeBounds, translateGeometry

#include <utility>

namespace edi::drafting {

std::optional<std::size_t> blockIndexById(const DraftingDocument &document, const DraftingBlockId &id)
{
    for (std::size_t index = 0; index < document.blocks.size(); ++index) {
        if (document.blocks[index].id == id) {
            return index;
        }
    }
    return std::nullopt;
}

DraftingBlock buildBlockFromObjects(DraftingBlockId id, std::string name,
                                    const std::vector<DraftingObject> &sources,
                                    std::string assetRef)
{
    DraftingBlock block;
    block.id = std::move(id);
    block.name = std::move(name);
    block.assetRef = std::move(assetRef);
    if (sources.empty()) {
        return block; // empty block; addBlock rejects it. bounds stays {0,0,0,0}.
    }

    // Union the sources' bounds from GEOMETRY (recomputed, never the possibly-stale
    // cached box), then normalize so the union's lower-left corner sits at (0,0).
    Bounds2D unionBounds = computeBounds(sources.front().geometry);
    for (std::size_t i = 1; i < sources.size(); ++i) {
        unionBounds = includeBounds(unionBounds, computeBounds(sources[i].geometry));
    }
    const double dx = -unionBounds.x;
    const double dy = -unionBounds.y;

    block.objects.reserve(sources.size());
    for (const DraftingObject &source : sources) {
        DraftingObject copy = source; // copy: the block OWNS its geometry (FLATTEN)
        copy.geometry = translateGeometry(copy.geometry, dx, dy);
        copy.bounds = computeBounds(copy.geometry); // recompute, do not translate the box
        block.objects.push_back(std::move(copy));
    }
    // The union, shifted to the origin: a placement offsets against this frame.
    block.bounds = {0.0, 0.0, unionBounds.width, unionBounds.height};
    return block;
}

DraftingStoreResult addBlock(DraftingDocument &document, DraftingBlock block)
{
    // A block id is an object-style id, so reuse the same non-empty validity check,
    // pairing a coarse result code with a specific message (as addObject/addPlug do).
    if (!isValidDraftingObjectId(block.id)) {
        return DraftingStoreResult::rejected(DraftingResultCode::EmptyObjectId, "block id is required");
    }
    if (blockIndexById(document, block.id)) {
        return DraftingStoreResult::rejected(DraftingResultCode::DuplicateObjectId, "block id already exists");
    }
    // A definition must define SOMETHING — an empty block would place nothing.
    // Rejecting it here (the source) keeps the empty-placement footgun from ever
    // reaching the document, the way createObjectsAndSelect refuses an empty batch.
    if (block.objects.empty()) {
        return DraftingStoreResult::rejected(DraftingResultCode::InvalidSelectionTarget,
                                             "block must contain at least one object");
    }

    document.blocks.push_back(std::move(block));
    ++document.revision;
    return DraftingStoreResult::accepted();
}

DraftingStoreResult removeBlock(DraftingDocument &document, const DraftingBlockId &id)
{
    const auto index = blockIndexById(document, id);
    if (!index) {
        return DraftingStoreResult::rejected(DraftingResultCode::ObjectNotFound, "block does not exist");
    }
    // No cascade: a block stores independent object COPIES, so nothing in the
    // document references it and nothing it references can dangle (FLATTEN). This
    // is exactly the coupling the plug/connection deletes must handle and blocks
    // do not.
    document.blocks.erase(document.blocks.begin() + static_cast<std::ptrdiff_t>(*index));
    ++document.revision;
    return DraftingStoreResult::accepted();
}

} // namespace edi::drafting
