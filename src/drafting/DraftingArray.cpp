#include "drafting/DraftingArray.h"

#include "drafting/DraftingGeometry.h"

#include <cmath>
#include <unordered_set>
#include <utility>

namespace edi::drafting {

DraftingArrayResult DraftingArrayResult::accepted(std::vector<DraftingObject> objects)
{
    DraftingArrayResult result;
    result.ok = true;
    result.code = DraftingResultCode::None;
    result.objects = std::move(objects);
    return result;
}

DraftingArrayResult DraftingArrayResult::rejected(DraftingResultCode code, std::string message)
{
    DraftingArrayResult result;
    result.ok = false;
    result.code = code;
    result.message = std::move(message);
    return result;
}

std::optional<DraftingArrayRepeatSettings> draftingArrayRepeatSettingsFromAxisId(const std::string &axisId)
{
    if (axisId == "x") {
        return DraftingArrayRepeatSettings{3, 0.1, 0.0};
    }
    if (axisId == "y") {
        return DraftingArrayRepeatSettings{3, 0.0, 0.1};
    }
    return std::nullopt;
}

DraftingArrayResult repeatDraftingObject(
    const DraftingObject &source,
    const std::vector<DraftingObjectId> &newObjectIds,
    double spacingX,
    double spacingY)
{
    if (newObjectIds.empty()) {
        return DraftingArrayResult::rejected(DraftingResultCode::InvalidGeometry, "repeat requires at least one copy");
    }
    if (!std::isfinite(spacingX) || !std::isfinite(spacingY)) {
        return DraftingArrayResult::rejected(DraftingResultCode::InvalidGeometry, "repeat spacing must be finite");
    }
    if (std::abs(spacingX) <= 0.000001 && std::abs(spacingY) <= 0.000001) {
        return DraftingArrayResult::rejected(DraftingResultCode::InvalidGeometry, "repeat spacing must move copied objects");
    }
    if (!kindMatchesGeometry(source.kind, source.geometry)) {
        return DraftingArrayResult::rejected(DraftingResultCode::KindGeometryMismatch, "shape kind does not match geometry");
    }
    std::unordered_set<DraftingObjectId> usedIds;
    usedIds.reserve(newObjectIds.size());
    for (const DraftingObjectId &id : newObjectIds) {
        if (id.empty() || id == source.id || !usedIds.insert(id).second) {
            return DraftingArrayResult::rejected(DraftingResultCode::DuplicateObjectId, "repeat object ids must be unique");
        }
    }

    std::vector<DraftingObject> repeated;
    repeated.reserve(newObjectIds.size());
    for (std::size_t index = 0; index < newObjectIds.size(); ++index) {
        const double step = static_cast<double>(index + 1);
        DraftingObject object = source;
        object.id = newObjectIds[index];
        object.geometry = translateGeometry(source.geometry, spacingX * step, spacingY * step);
        object.metadata.toolProvenance = "repeat";
        object.metadata.source = source.id;

        const auto validation = validateDraftingObjectShape(object);
        if (!validation.ok) {
            return DraftingArrayResult::rejected(validation.code, validation.message);
        }
        object.bounds = computeBounds(object.geometry);
        repeated.push_back(std::move(object));
    }

    return DraftingArrayResult::accepted(std::move(repeated));
}

} // namespace edi::drafting
