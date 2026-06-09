#include "drafting/DraftingStore.h"

#include "drafting/DraftingGeometry.h"
#include "drafting/DraftingMetadata.h"
#include "drafting/DraftingSelection.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <utility>
#include <vector>

namespace edi::drafting {

namespace {

bool assignSequentialLayerOrders(DraftingDocument &document)
{
    bool changed = false;
    for (std::size_t index = 0; index < document.layers.size(); ++index) {
        changed = changed || document.layers[index].order != static_cast<int>(index);
        document.layers[index].order = static_cast<int>(index);
    }
    return changed;
}

bool sortAndNormalizeLayerOrder(DraftingDocument &document)
{
    std::vector<LayerId> before;
    before.reserve(document.layers.size());
    for (const DraftingLayer &layer : document.layers) {
        before.push_back(layer.id);
    }

    std::stable_sort(document.layers.begin(), document.layers.end(), [](const DraftingLayer &a, const DraftingLayer &b) {
        return a.order < b.order;
    });
    bool changed = assignSequentialLayerOrders(document);
    for (std::size_t index = 0; index < document.layers.size(); ++index) {
        changed = changed || before[index] != document.layers[index].id;
    }
    return changed;
}

} // namespace

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

DraftingStoreResult moveObjectToLayer(DraftingDocument &document, const DraftingObjectId &objectId, const LayerId &layerId)
{
    const auto index = objectIndexById(document, objectId);
    if (!index) {
        return DraftingStoreResult::rejected(DraftingResultCode::ObjectNotFound, "object does not exist");
    }

    DraftingObject &object = document.objects[*index];
    if (object.locked) {
        return DraftingStoreResult::rejected(DraftingResultCode::InvalidSelectionTarget, "object is locked");
    }

    const DraftingLayer *sourceLayer = findLayer(document, object.layerId);
    if (sourceLayer == nullptr) {
        return DraftingStoreResult::rejected(DraftingResultCode::LayerNotFound, "object layer does not exist");
    }
    if (sourceLayer->locked) {
        return DraftingStoreResult::rejected(DraftingResultCode::InvalidSelectionTarget, "object layer is locked");
    }

    const DraftingLayer *targetLayer = findLayer(document, layerId);
    if (targetLayer == nullptr) {
        return DraftingStoreResult::rejected(DraftingResultCode::LayerNotFound, "target layer does not exist");
    }
    if (targetLayer->locked) {
        return DraftingStoreResult::rejected(DraftingResultCode::InvalidSelectionTarget, "target layer is locked");
    }

    if (object.layerId == layerId) {
        return DraftingStoreResult::accepted();
    }

    object.layerId = layerId;
    ++document.revision;
    return DraftingStoreResult::accepted();
}

DraftingStoreResult addLayer(DraftingDocument &document, DraftingLayer layer, bool makeActive)
{
    if (!isValidLayerId(layer.id)) {
        return DraftingStoreResult::rejected(DraftingResultCode::LayerNotFound, "layer id is required");
    }
    if (!isValidLayerName(layer.name)) {
        return DraftingStoreResult::rejected(DraftingResultCode::InvalidSelectionTarget, "layer name is required");
    }
    if (containsLayer(document, layer.id)) {
        return DraftingStoreResult::rejected(DraftingResultCode::DuplicateLayerId, "layer id already exists");
    }

    const LayerId newLayerId = layer.id;
    document.layers.push_back(std::move(layer));
    sortAndNormalizeLayerOrder(document);
    if (makeActive) {
        document.activeLayerId = newLayerId;
    }
    ++document.revision;
    return DraftingStoreResult::accepted();
}

DraftingStoreResult renameLayer(DraftingDocument &document, const LayerId &id, std::string name)
{
    if (!isValidLayerName(name)) {
        return DraftingStoreResult::rejected(DraftingResultCode::InvalidSelectionTarget, "layer name is required");
    }

    DraftingLayer *layer = findLayer(document, id);
    if (layer == nullptr) {
        return DraftingStoreResult::rejected(DraftingResultCode::LayerNotFound, "layer does not exist");
    }

    if (layer->name == name) {
        return DraftingStoreResult::accepted();
    }
    layer->name = std::move(name);
    ++document.revision;
    return DraftingStoreResult::accepted();
}

DraftingStoreResult setActiveLayer(DraftingDocument &document, const LayerId &id)
{
    if (!containsLayer(document, id)) {
        return DraftingStoreResult::rejected(DraftingResultCode::LayerNotFound, "layer does not exist");
    }
    if (document.activeLayerId == id) {
        return DraftingStoreResult::accepted();
    }
    document.activeLayerId = id;
    ++document.revision;
    return DraftingStoreResult::accepted();
}

DraftingStoreResult moveLayer(DraftingDocument &document, const LayerId &id, int delta)
{
    if (delta == 0) {
        return DraftingStoreResult::accepted();
    }

    if (!containsLayer(document, id)) {
        return DraftingStoreResult::rejected(DraftingResultCode::LayerNotFound, "layer does not exist");
    }

    const bool normalized = sortAndNormalizeLayerOrder(document);
    const auto index = layerIndexById(document, id);
    const int direction = delta < 0 ? -1 : 1;
    const auto target = static_cast<std::ptrdiff_t>(*index) + direction;
    if (target < 0 || target >= static_cast<std::ptrdiff_t>(document.layers.size())) {
        if (normalized) {
            ++document.revision;
        }
        return DraftingStoreResult::accepted();
    }

    std::swap(document.layers[*index], document.layers[static_cast<std::size_t>(target)]);
    assignSequentialLayerOrders(document);
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
