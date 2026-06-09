#include "drafting/DraftingStore.h"

#include "drafting/DraftingGeometry.h"
#include "drafting/DraftingMetadata.h"
#include "drafting/DraftingSelection.h"

#include <cmath>
#include <utility>

namespace edi::drafting {

DraftingStoreResult DraftingStoreResult::accepted()
{
    return {true, DraftingResultCode::None, {}};
}

DraftingStoreResult DraftingStoreResult::rejected(DraftingResultCode code, std::string message)
{
    return {false, code, std::move(message)};
}

DraftingStoreResult addObject(DraftingDocument &document, DraftingObject object)
{
    const auto shapeValidation = validateDraftingObjectShape(object);
    if (!shapeValidation.ok) {
        return DraftingStoreResult::rejected(shapeValidation.code, shapeValidation.message);
    }
    if (objectIndexById(document, object.id)) {
        return DraftingStoreResult::rejected(DraftingResultCode::DuplicateObjectId, "object id already exists");
    }
    DraftingLayer *layer = findLayer(document, object.layerId);
    if (layer == nullptr) {
        return DraftingStoreResult::rejected(DraftingResultCode::LayerNotFound, "layer does not exist");
    }
    if (layer->locked) {
        return DraftingStoreResult::rejected(DraftingResultCode::InvalidSelectionTarget, "object layer is locked");
    }

    object.bounds = computeBounds(object.geometry);
    document.objects.push_back(std::move(object));
    ++document.revision;
    return DraftingStoreResult::accepted();
}

DraftingStoreResult removeObject(DraftingDocument &document, const DraftingObjectId &id)
{
    const auto index = objectIndexById(document, id);
    if (!index) {
        return DraftingStoreResult::rejected(DraftingResultCode::ObjectNotFound, "object does not exist");
    }
    if (document.objects[*index].locked) {
        return DraftingStoreResult::rejected(DraftingResultCode::InvalidSelectionTarget, "object is locked");
    }
    const DraftingLayer *layer = findLayer(document, document.objects[*index].layerId);
    if (layer != nullptr && layer->locked) {
        return DraftingStoreResult::rejected(DraftingResultCode::InvalidSelectionTarget, "object layer is locked");
    }

    document.objects.erase(document.objects.begin() + static_cast<std::ptrdiff_t>(*index));
    normalizeSelection(document);
    ++document.revision;
    return DraftingStoreResult::accepted();
}

DraftingStoreResult updateObjectGeometry(DraftingDocument &document, const DraftingObjectId &id, DraftingGeometry geometry)
{
    const auto index = objectIndexById(document, id);
    if (!index) {
        return DraftingStoreResult::rejected(DraftingResultCode::ObjectNotFound, "object does not exist");
    }
    DraftingObject &object = document.objects[*index];
    if (object.locked) {
        return DraftingStoreResult::rejected(DraftingResultCode::InvalidSelectionTarget, "object is locked");
    }
    const DraftingLayer *layer = findLayer(document, object.layerId);
    if (layer != nullptr && layer->locked) {
        return DraftingStoreResult::rejected(DraftingResultCode::InvalidSelectionTarget, "object layer is locked");
    }

    DraftingObject candidate = object;
    candidate.geometry = std::move(geometry);
    const auto shapeValidation = validateDraftingObjectShape(candidate);
    if (!shapeValidation.ok) {
        return DraftingStoreResult::rejected(shapeValidation.code, shapeValidation.message);
    }

    object.geometry = std::move(candidate.geometry);
    object.bounds = computeBounds(object.geometry);
    ++document.revision;
    return DraftingStoreResult::accepted();
}

DraftingStoreResult updateObjectMetadata(DraftingDocument &document, const DraftingObjectId &id, ObjectMetadata metadata)
{
    const auto index = objectIndexById(document, id);
    if (!index) {
        return DraftingStoreResult::rejected(DraftingResultCode::ObjectNotFound, "object does not exist");
    }

    DraftingObject candidate = document.objects[*index];
    candidate.metadata = std::move(metadata);
    const auto metadataValidation = validateObjectMetadata(candidate.metadata);
    if (!metadataValidation.ok) {
        return DraftingStoreResult::rejected(metadataValidation.code, metadataValidation.message);
    }

    document.objects[*index].metadata = std::move(candidate.metadata);
    ++document.revision;
    return DraftingStoreResult::accepted();
}

DraftingStoreResult updateObjectFlags(DraftingDocument &document, const DraftingObjectId &id, bool locked, bool visible)
{
    const auto index = objectIndexById(document, id);
    if (!index) {
        return DraftingStoreResult::rejected(DraftingResultCode::ObjectNotFound, "object does not exist");
    }

    DraftingObject &object = document.objects[*index];
    const DraftingLayer *layer = findLayer(document, object.layerId);
    if (layer != nullptr && layer->locked) {
        return DraftingStoreResult::rejected(DraftingResultCode::InvalidSelectionTarget, "object layer is locked");
    }
    if (object.locked == locked && object.visible == visible) {
        return DraftingStoreResult::accepted();
    }
    object.locked = locked;
    object.visible = visible;
    ++document.revision;
    return DraftingStoreResult::accepted();
}

DraftingStoreResult updateLayerFlags(DraftingDocument &document, const LayerId &id, bool locked, bool visible)
{
    DraftingLayer *layer = findLayer(document, id);
    if (layer == nullptr) {
        return DraftingStoreResult::rejected(DraftingResultCode::LayerNotFound, "layer does not exist");
    }

    if (layer->locked == locked && layer->visible == visible) {
        return DraftingStoreResult::accepted();
    }
    layer->locked = locked;
    layer->visible = visible;
    ++document.revision;
    return DraftingStoreResult::accepted();
}

DraftingStoreResult moveObject(DraftingDocument &document, const DraftingObjectId &id, double dx, double dy)
{
    const auto index = objectIndexById(document, id);
    if (!index) {
        return DraftingStoreResult::rejected(DraftingResultCode::ObjectNotFound, "object does not exist");
    }
    DraftingObject &object = document.objects[*index];
    if (object.locked) {
        return DraftingStoreResult::rejected(DraftingResultCode::InvalidSelectionTarget, "object is locked");
    }
    const DraftingLayer *layer = findLayer(document, object.layerId);
    if (layer != nullptr && layer->locked) {
        return DraftingStoreResult::rejected(DraftingResultCode::InvalidSelectionTarget, "object layer is locked");
    }

    if (!std::isfinite(dx) || !std::isfinite(dy)) {
        return DraftingStoreResult::rejected(DraftingResultCode::InvalidGeometry, "move delta must be finite");
    }

    DraftingObject candidate = object;
    candidate.geometry = translateGeometry(candidate.geometry, dx, dy);
    const auto shapeValidation = validateDraftingObjectShape(candidate);
    if (!shapeValidation.ok) {
        return DraftingStoreResult::rejected(shapeValidation.code, shapeValidation.message);
    }

    object.geometry = std::move(candidate.geometry);
    object.bounds = computeBounds(object.geometry);
    ++document.revision;
    return DraftingStoreResult::accepted();
}

} // namespace edi::drafting
