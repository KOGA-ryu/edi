#include "drafting/DraftingCommands.h"

#include "drafting/DraftingObjectEdit.h"
#include "drafting/DraftingSelection.h"
#include "drafting/DraftingStore.h"

#include <cmath>
#include <utility>

namespace edi::drafting {

DraftingCommandResult DraftingCommandResult::accepted()
{
    return {true, DraftingResultCode::None, {}};
}

DraftingCommandResult DraftingCommandResult::rejected(DraftingResultCode code, std::string message)
{
    return {false, code, std::move(message)};
}

namespace {

DraftingCommandResult fromStoreResult(const DraftingStoreResult &result)
{
    if (result.ok) {
        return DraftingCommandResult::accepted();
    }
    return DraftingCommandResult::rejected(result.code, result.message);
}

bool commandModeIsDistribute(DraftingAlignmentMode mode)
{
    return mode == DraftingAlignmentMode::DistributeX || mode == DraftingAlignmentMode::DistributeY;
}

} // namespace

DraftingCommandResult applyDraftingCommand(DraftingDocument &document, const DraftingCommand &command)
{
    return std::visit([&](const auto &typedCommand) -> DraftingCommandResult {
        using Command = std::decay_t<decltype(typedCommand)>;
        if constexpr (std::is_same_v<Command, CreateObjectCommand>) {
            return fromStoreResult(addObject(document, typedCommand.object));
        } else if constexpr (std::is_same_v<Command, DeleteObjectCommand>) {
            return fromStoreResult(removeObject(document, typedCommand.objectId));
        } else if constexpr (std::is_same_v<Command, MoveObjectCommand>) {
            return fromStoreResult(moveObject(document, typedCommand.objectId, typedCommand.dx, typedCommand.dy));
        } else if constexpr (std::is_same_v<Command, MoveSelectionCommand>) {
            if (!std::isfinite(typedCommand.dx) || !std::isfinite(typedCommand.dy)) {
                return DraftingCommandResult::rejected(DraftingResultCode::InvalidGeometry, "move delta must be finite");
            }
            DraftingDocument candidate = document;
            for (const DraftingObjectId &objectId : document.selectedObjectIds) {
                if (!containsObject(candidate, objectId)) {
                    return DraftingCommandResult::rejected(DraftingResultCode::InvalidSelectionTarget, "selection target does not exist");
                }
                const DraftingStoreResult move = moveObject(candidate, objectId, typedCommand.dx, typedCommand.dy);
                if (!move.ok) {
                    return fromStoreResult(move);
                }
            }
            if (!document.selectedObjectIds.empty()) {
                candidate.revision = document.revision + 1;
                document = std::move(candidate);
            }
            return DraftingCommandResult::accepted();
        } else if constexpr (std::is_same_v<Command, EditObjectHandleCommand>) {
            const DraftingObject *object = findObject(document, typedCommand.objectId);
            if (object == nullptr) {
                return DraftingCommandResult::rejected(DraftingResultCode::ObjectNotFound, "object does not exist");
            }
            const DraftingHandleEditPlan plan = handleEditPlan(*object, typedCommand.handleId, typedCommand.point);
            if (!plan.ok) {
                return DraftingCommandResult::rejected(plan.code, plan.message);
            }
            const DraftingObjectEditResult edit = applyObjectEdit(*object, plan.edit);
            if (!edit.ok) {
                return DraftingCommandResult::rejected(edit.code, edit.message);
            }
            return fromStoreResult(updateObjectGeometry(document, typedCommand.objectId, edit.geometry));
        } else if constexpr (std::is_same_v<Command, UpdateGeometryCommand>) {
            return fromStoreResult(updateObjectGeometry(document, typedCommand.objectId, typedCommand.geometry));
        } else if constexpr (std::is_same_v<Command, UpdateMetadataCommand>) {
            return fromStoreResult(updateObjectMetadata(document, typedCommand.objectId, typedCommand.metadata));
        } else if constexpr (std::is_same_v<Command, UpdateObjectFlagsCommand>) {
            return fromStoreResult(updateObjectFlags(document, typedCommand.objectId, typedCommand.locked, typedCommand.visible));
        } else if constexpr (std::is_same_v<Command, MoveObjectToLayerCommand>) {
            return fromStoreResult(moveObjectToLayer(document, typedCommand.objectId, typedCommand.layerId));
        } else if constexpr (std::is_same_v<Command, CreateLayerCommand>) {
            return fromStoreResult(addLayer(document, typedCommand.layer, typedCommand.makeActive));
        } else if constexpr (std::is_same_v<Command, RenameLayerCommand>) {
            return fromStoreResult(renameLayer(document, typedCommand.layerId, typedCommand.name));
        } else if constexpr (std::is_same_v<Command, SetActiveLayerCommand>) {
            return fromStoreResult(setActiveLayer(document, typedCommand.layerId));
        } else if constexpr (std::is_same_v<Command, MoveLayerCommand>) {
            return fromStoreResult(moveLayer(document, typedCommand.layerId, typedCommand.delta));
        } else if constexpr (std::is_same_v<Command, UpdateLayerFlagsCommand>) {
            return fromStoreResult(updateLayerFlags(document, typedCommand.layerId, typedCommand.locked, typedCommand.visible));
        } else if constexpr (std::is_same_v<Command, AlignSelectionCommand>) {
            if (commandModeIsDistribute(typedCommand.mode)) {
                return DraftingCommandResult::rejected(DraftingResultCode::InvalidGeometry, "align command requires an align mode");
            }
            const DraftingAlignmentResult plan = planDraftingAlignment(document, document.selectedObjectIds, typedCommand.mode);
            if (!plan.ok) {
                return DraftingCommandResult::rejected(plan.code, plan.message);
            }

            DraftingDocument candidate = document;
            for (const DraftingTranslation &translation : plan.translations) {
                const DraftingStoreResult move = moveObject(candidate, translation.objectId, translation.dx, translation.dy);
                if (!move.ok) {
                    return fromStoreResult(move);
                }
            }
            if (!plan.translations.empty()) {
                candidate.revision = document.revision + 1;
                document = std::move(candidate);
            }
            return DraftingCommandResult::accepted();
        } else if constexpr (std::is_same_v<Command, DistributeSelectionCommand>) {
            if (!commandModeIsDistribute(typedCommand.mode)) {
                return DraftingCommandResult::rejected(DraftingResultCode::InvalidGeometry, "distribute command requires a distribute mode");
            }
            const DraftingAlignmentResult plan = planDraftingAlignment(document, document.selectedObjectIds, typedCommand.mode);
            if (!plan.ok) {
                return DraftingCommandResult::rejected(plan.code, plan.message);
            }

            DraftingDocument candidate = document;
            for (const DraftingTranslation &translation : plan.translations) {
                const DraftingStoreResult move = moveObject(candidate, translation.objectId, translation.dx, translation.dy);
                if (!move.ok) {
                    return fromStoreResult(move);
                }
            }
            if (!plan.translations.empty()) {
                candidate.revision = document.revision + 1;
                document = std::move(candidate);
            }
            return DraftingCommandResult::accepted();
        } else if constexpr (std::is_same_v<Command, SelectObjectCommand>) {
            if (!containsObject(document, typedCommand.objectId)) {
                return DraftingCommandResult::rejected(DraftingResultCode::InvalidSelectionTarget, "selection target does not exist");
            }
            selectOnly(document, typedCommand.objectId);
            ++document.revision;
            return DraftingCommandResult::accepted();
        } else if constexpr (std::is_same_v<Command, SelectObjectsCommand>) {
            for (const DraftingObjectId &objectId : typedCommand.objectIds) {
                if (!containsObject(document, objectId)) {
                    return DraftingCommandResult::rejected(DraftingResultCode::InvalidSelectionTarget, "selection target does not exist");
                }
            }
            selectMany(document, typedCommand.objectIds);
            ++document.revision;
            return DraftingCommandResult::accepted();
        } else {
            return DraftingCommandResult::rejected(DraftingResultCode::InvalidGeometry, "unsupported command");
        }
    }, command);
}

} // namespace edi::drafting
