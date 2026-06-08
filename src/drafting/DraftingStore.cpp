#include "drafting/DraftingStore.h"

#include "drafting/DraftingGeometry.h"
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
    if (!containsLayer(document, object.layerId)) {
        return DraftingStoreResult::rejected(DraftingResultCode::LayerNotFound, "layer does not exist");
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

    document.objects[*index].metadata = std::move(metadata);
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

    if (!std::isfinite(dx) || !std::isfinite(dy)) {
        return DraftingStoreResult::rejected(DraftingResultCode::InvalidGeometry, "move delta must be finite");
    }

    DraftingGeometry movedGeometry = translateGeometry(object.geometry, dx, dy);
    const auto geometryValidation = validateGeometry(movedGeometry);
    if (!geometryValidation.ok) {
        return DraftingStoreResult::rejected(geometryValidation.code, geometryValidation.message);
    }

    object.geometry = std::move(movedGeometry);
    object.bounds = computeBounds(object.geometry);
    ++document.revision;
    return DraftingStoreResult::accepted();
}

} // namespace edi::drafting
