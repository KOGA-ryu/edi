#include "drafting/DraftingStore.h"

#include "drafting/DraftingGeometry.h"
#include "drafting/DraftingSelection.h"

#include <algorithm>
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
    if (object.id.empty()) {
        return DraftingStoreResult::rejected(DraftingResultCode::EmptyObjectId, "object id is required");
    }
    if (containsObject(document, object.id)) {
        return DraftingStoreResult::rejected(DraftingResultCode::DuplicateObjectId, "object id already exists");
    }
    if (!kindMatchesGeometry(object.kind, object.geometry)) {
        return DraftingStoreResult::rejected(DraftingResultCode::KindGeometryMismatch, "shape kind does not match geometry");
    }
    const auto geometryValidation = validateGeometry(object.geometry);
    if (!geometryValidation.ok) {
        return DraftingStoreResult::rejected(geometryValidation.code, geometryValidation.message);
    }
    if (findLayer(document, object.layerId) == nullptr) {
        return DraftingStoreResult::rejected(DraftingResultCode::LayerNotFound, "layer does not exist");
    }

    object.bounds = computeBounds(object.geometry);
    document.objects.push_back(std::move(object));
    ++document.revision;
    return DraftingStoreResult::accepted();
}

DraftingStoreResult removeObject(DraftingDocument &document, const DraftingObjectId &id)
{
    const auto before = document.objects.size();
    document.objects.erase(
        std::remove_if(document.objects.begin(), document.objects.end(), [&](const DraftingObject &object) {
            return object.id == id;
        }),
        document.objects.end());

    if (document.objects.size() == before) {
        return DraftingStoreResult::rejected(DraftingResultCode::ObjectNotFound, "object does not exist");
    }

    normalizeSelection(document);
    ++document.revision;
    return DraftingStoreResult::accepted();
}

DraftingStoreResult updateObjectGeometry(DraftingDocument &document, const DraftingObjectId &id, DraftingGeometry geometry)
{
    DraftingObject *object = findObject(document, id);
    if (object == nullptr) {
        return DraftingStoreResult::rejected(DraftingResultCode::ObjectNotFound, "object does not exist");
    }
    if (!kindMatchesGeometry(object->kind, geometry)) {
        return DraftingStoreResult::rejected(DraftingResultCode::KindGeometryMismatch, "shape kind does not match geometry");
    }
    const auto geometryValidation = validateGeometry(geometry);
    if (!geometryValidation.ok) {
        return DraftingStoreResult::rejected(geometryValidation.code, geometryValidation.message);
    }

    object->geometry = std::move(geometry);
    object->bounds = computeBounds(object->geometry);
    ++document.revision;
    return DraftingStoreResult::accepted();
}

DraftingStoreResult updateObjectMetadata(DraftingDocument &document, const DraftingObjectId &id, ObjectMetadata metadata)
{
    DraftingObject *object = findObject(document, id);
    if (object == nullptr) {
        return DraftingStoreResult::rejected(DraftingResultCode::ObjectNotFound, "object does not exist");
    }

    object->metadata = std::move(metadata);
    ++document.revision;
    return DraftingStoreResult::accepted();
}

DraftingStoreResult moveObject(DraftingDocument &document, const DraftingObjectId &id, double dx, double dy)
{
    DraftingObject *object = findObject(document, id);
    if (object == nullptr) {
        return DraftingStoreResult::rejected(DraftingResultCode::ObjectNotFound, "object does not exist");
    }

    if (!std::isfinite(dx) || !std::isfinite(dy)) {
        return DraftingStoreResult::rejected(DraftingResultCode::InvalidGeometry, "move delta must be finite");
    }

    DraftingGeometry movedGeometry = translateGeometry(object->geometry, dx, dy);
    const auto geometryValidation = validateGeometry(movedGeometry);
    if (!geometryValidation.ok) {
        return DraftingStoreResult::rejected(geometryValidation.code, geometryValidation.message);
    }

    object->geometry = std::move(movedGeometry);
    object->bounds = computeBounds(object->geometry);
    ++document.revision;
    return DraftingStoreResult::accepted();
}

} // namespace edi::drafting
