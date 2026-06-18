#include "drafting/DraftingCommands.h"

#include "drafting/DraftingBlockOps.h"
#include "drafting/DraftingGeometry.h"
#include "drafting/DraftingGraphOps.h"
#include "drafting/DraftingGuideOps.h"
#include "drafting/DraftingNumericEdit.h"
#include "drafting/DraftingObjectEdit.h"
#include "drafting/DraftingSelection.h"
#include "drafting/DraftingStore.h"

#include <algorithm>
#include <cmath>
#include <iterator>
#include <unordered_set>
#include <utility>
#include <vector>

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

// The copy -> loop(moveObject) -> commit skeleton that MoveSelection,
// AlignSelection and DistributeSelection used to inline THREE times. Each command
// still owns its OWN translation DECISION (a uniform delta, an align plan, or a
// distribute plan); only the apply step is shared. The variation is pure DATA —
// the list of per-object translations — so no class hierarchy or callable is
// needed. Behavior-bearing details preserved verbatim from the inlined copies:
// the moves run on a COPY and a mid-loop moveObject failure returns its store
// result with the document UNTOUCHED (all-or-nothing), and the revision bumps
// only when there was at least one translation to apply.
DraftingCommandResult applyTranslationPlan(
    DraftingDocument &document,
    const std::vector<DraftingTranslation> &translations)
{
    DraftingDocument candidate = document;
    for (const DraftingTranslation &translation : translations) {
        const DraftingStoreResult move = moveObject(candidate, translation.objectId, translation.dx, translation.dy);
        if (!move.ok) {
            return fromStoreResult(move);
        }
    }
    if (!translations.empty()) {
        candidate.revision = document.revision + 1;
        document = std::move(candidate);
    }
    return DraftingCommandResult::accepted();
}

bool sameGuidePosition(const DraftingObject &a, const DraftingObject &b)
{
    const auto *guideA = std::get_if<GuideGeometry>(&a.geometry);
    const auto *guideB = std::get_if<GuideGeometry>(&b.geometry);
    if (guideA == nullptr || guideB == nullptr) {
        return false;
    }
    return sameGuide(*guideA, *guideB);
}

bool hasEarlierEquivalentGuide(const std::vector<DraftingObject> &objects, std::size_t index)
{
    for (std::size_t previous = 0; previous < index; ++previous) {
        if (isGuideObject(objects[previous]) && sameGuidePosition(objects[previous], objects[index])) {
            return true;
        }
    }
    return false;
}

} // namespace

DraftingCommandResult applyDraftingCommand(DraftingDocument &document, const DraftingCommand &command)
{
    return std::visit([&](const auto &typedCommand) -> DraftingCommandResult {
        using Command = std::decay_t<decltype(typedCommand)>;
        if constexpr (std::is_same_v<Command, CreateObjectCommand>) {
            return fromStoreResult(addObject(document, typedCommand.object));
        } else if constexpr (std::is_same_v<Command, CreateObjectsCommand>) {
            if (typedCommand.objects.empty()) {
                return DraftingCommandResult::accepted(); // no-op: nothing changed, no revision bump
            }
            // One id set covers the existing document AND the batch itself —
            // the per-object path re-scans the whole document per insert,
            // which makes an N-object batch O(N^2) (measured: 0.5s at 9800).
            std::unordered_set<DraftingObjectId> ids = objectIdSet(document);
            ids.reserve(document.objects.size() + typedCommand.objects.size());
            // Validate the whole batch into a staging vector before touching
            // the document: any rejection leaves it exactly as it was. The
            // checks mirror addObject one for one.
            std::vector<DraftingObject> staged;
            staged.reserve(typedCommand.objects.size());
            for (const DraftingObject &object : typedCommand.objects) {
                const auto shapeValidation = validateDraftingObjectShape(object);
                if (!shapeValidation.ok) {
                    return DraftingCommandResult::rejected(shapeValidation.code, shapeValidation.message);
                }
                if (!ids.insert(object.id).second) {
                    return DraftingCommandResult::rejected(DraftingResultCode::DuplicateObjectId, "object id already exists");
                }
                const DraftingLayer *layer = findLayer(document, object.layerId);
                if (layer == nullptr) {
                    return DraftingCommandResult::rejected(DraftingResultCode::LayerNotFound, "layer does not exist");
                }
                if (layer->locked) {
                    return DraftingCommandResult::rejected(DraftingResultCode::InvalidSelectionTarget, "object layer is locked");
                }
                staged.push_back(object);
                staged.back().bounds = computeBounds(object.geometry);
            }
            document.objects.insert(document.objects.end(),
                                    std::make_move_iterator(staged.begin()),
                                    std::make_move_iterator(staged.end()));
            ++document.revision;
            return DraftingCommandResult::accepted();
        } else if constexpr (std::is_same_v<Command, DeleteObjectCommand>) {
            return fromStoreResult(removeObject(document, typedCommand.objectId));
        } else if constexpr (std::is_same_v<Command, DeleteAllGuidesCommand>) {
            const auto before = document.objects.size();
            document.objects.erase(std::remove_if(document.objects.begin(), document.objects.end(), isGuideObject), document.objects.end());
            if (document.objects.size() != before) {
                normalizeSelection(document);
                ++document.revision;
            }
            return DraftingCommandResult::accepted();
        } else if constexpr (std::is_same_v<Command, MergeDuplicateGuidesCommand>) {
            std::vector<DraftingObject> merged;
            merged.reserve(document.objects.size());
            bool changed = false;
            for (std::size_t index = 0; index < document.objects.size(); ++index) {
                const DraftingObject &object = document.objects[index];
                if (isGuideObject(object) && hasEarlierEquivalentGuide(document.objects, index)) {
                    changed = true;
                    continue;
                }
                merged.push_back(object);
            }
            if (changed) {
                document.objects = std::move(merged);
                normalizeSelection(document);
                ++document.revision;
            }
            return DraftingCommandResult::accepted();
        } else if constexpr (std::is_same_v<Command, MoveObjectCommand>) {
            return fromStoreResult(moveObject(document, typedCommand.objectId, typedCommand.dx, typedCommand.dy));
        } else if constexpr (std::is_same_v<Command, MoveSelectionCommand>) {
            if (!std::isfinite(typedCommand.dx) || !std::isfinite(typedCommand.dy)) {
                return DraftingCommandResult::rejected(DraftingResultCode::InvalidGeometry, "move delta must be finite");
            }
            // MoveSelection keeps its OWN interleaved loop rather than routing
            // through applyTranslationPlan: the containsObject guard and the
            // moveObject call must stay INTERLEAVED, because the FIRST failing id
            // in selection order decides the rejection MESSAGE. Pre-scanning all
            // existence checks first (the earlier dedup) was the same error CODE
            // but a different message for a [present+locked, missing] selection —
            // the locked move would surface "object is locked", while a pre-scan
            // reports the later missing id's "selection target does not exist".
            // message is observable upstream, so this stays interleaved. (Align/
            // Distribute have no such per-id guard, so they share the helper.)
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
            const DraftingHandleEditPlan plan = handleEditPlan(*object, typedCommand.handleId, typedCommand.point, typedCommand.preserveAspect);
            if (!plan.ok) {
                return DraftingCommandResult::rejected(plan.code, plan.message);
            }
            const DraftingObjectEditResult edit = applyObjectEdit(*object, plan.edit);
            if (!edit.ok) {
                return DraftingCommandResult::rejected(edit.code, edit.message);
            }
            return fromStoreResult(updateObjectGeometry(document, typedCommand.objectId, edit.geometry));
        } else if constexpr (std::is_same_v<Command, NumericGeometryEditCommand>) {
            const DraftingObject *object = findObject(document, typedCommand.objectId);
            if (object == nullptr) {
                return DraftingCommandResult::rejected(DraftingResultCode::ObjectNotFound, "object does not exist");
            }
            const DraftingNumericEditResult edit = applyNumericGeometryEdit(*object, typedCommand.fieldId, typedCommand.value);
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
        } else if constexpr (std::is_same_v<Command, UpdateStrokeStyleCommand>) {
            DraftingObject *object = findObject(document, typedCommand.objectId);
            if (object == nullptr) {
                return DraftingCommandResult::rejected(DraftingResultCode::ObjectNotFound, "object does not exist");
            }
            const DraftingLayer *layer = findLayer(document, object->layerId);
            if (object->locked || (layer != nullptr && layer->locked)) {
                return DraftingCommandResult::rejected(DraftingResultCode::InvalidSelectionTarget, "object is locked");
            }
            if (object->stroke == typedCommand.stroke) {
                return DraftingCommandResult::accepted(); // no-op: focus-out resubmits must not dirty/undo-push
            }
            object->stroke = typedCommand.stroke;
            ++document.revision;
            return DraftingCommandResult::accepted();
        } else if constexpr (std::is_same_v<Command, UpdateFillStyleCommand>) {
            DraftingObject *object = findObject(document, typedCommand.objectId);
            if (object == nullptr) {
                return DraftingCommandResult::rejected(DraftingResultCode::ObjectNotFound, "object does not exist");
            }
            const DraftingLayer *layer = findLayer(document, object->layerId);
            if (object->locked || (layer != nullptr && layer->locked)) {
                return DraftingCommandResult::rejected(DraftingResultCode::InvalidSelectionTarget, "object is locked");
            }
            if (object->fill == typedCommand.fill) {
                return DraftingCommandResult::accepted(); // no-op: focus-out resubmits must not dirty/undo-push
            }
            object->fill = typedCommand.fill;
            ++document.revision;
            return DraftingCommandResult::accepted();
        } else if constexpr (std::is_same_v<Command, SetAllGuidesVisibleCommand>) {
            bool changed = false;
            for (DraftingObject &object : document.objects) {
                if (isGuideObject(object) && object.visible != typedCommand.visible) {
                    object.visible = typedCommand.visible;
                    changed = true;
                }
            }
            if (changed) {
                normalizeSelection(document);
                ++document.revision;
            }
            return DraftingCommandResult::accepted();
        } else if constexpr (std::is_same_v<Command, SetAllGuidesLockedCommand>) {
            bool changed = false;
            for (DraftingObject &object : document.objects) {
                if (isGuideObject(object) && object.locked != typedCommand.locked) {
                    object.locked = typedCommand.locked;
                    changed = true;
                }
            }
            if (changed) {
                ++document.revision;
            }
            return DraftingCommandResult::accepted();
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
        } else if constexpr (std::is_same_v<Command, UpdateLayerPlotStyleCommand>) {
            return fromStoreResult(updateLayerPlotStyle(document, typedCommand.layerId, typedCommand.plot));
        } else if constexpr (std::is_same_v<Command, AlignSelectionCommand>) {
            if (commandModeIsDistribute(typedCommand.mode)) {
                return DraftingCommandResult::rejected(DraftingResultCode::InvalidGeometry, "align command requires an align mode");
            }
            const DraftingAlignmentResult plan = planDraftingAlignment(document, document.selectedObjectIds, typedCommand.mode);
            if (!plan.ok) {
                return DraftingCommandResult::rejected(plan.code, plan.message);
            }
            // Align's translation decision is the alignment plan; the apply is the
            // shared skeleton.
            return applyTranslationPlan(document, plan.translations);
        } else if constexpr (std::is_same_v<Command, DistributeSelectionCommand>) {
            if (!commandModeIsDistribute(typedCommand.mode)) {
                return DraftingCommandResult::rejected(DraftingResultCode::InvalidGeometry, "distribute command requires a distribute mode");
            }
            const DraftingAlignmentResult plan = planDraftingAlignment(document, document.selectedObjectIds, typedCommand.mode);
            if (!plan.ok) {
                return DraftingCommandResult::rejected(plan.code, plan.message);
            }
            // Distribute's translation decision is the distribute plan; same apply.
            return applyTranslationPlan(document, plan.translations);
        } else if constexpr (std::is_same_v<Command, SelectObjectCommand>) {
            if (!containsObject(document, typedCommand.objectId)) {
                return DraftingCommandResult::rejected(DraftingResultCode::InvalidSelectionTarget, "selection target does not exist");
            }
            selectOnly(document, typedCommand.objectId);
            ++document.revision;
            return DraftingCommandResult::accepted();
        } else if constexpr (std::is_same_v<Command, SelectObjectsCommand>) {
            // Validate against one id set, not containsObject per id — an
            // N-id selection over an M-object document was O(N*M), and the
            // batch-create path selects everything it just made (measured:
            // 1.5s at 9800 ids before this).
            const std::unordered_set<DraftingObjectId> existing = objectIdSet(document);
            for (const DraftingObjectId &objectId : typedCommand.objectIds) {
                if (existing.find(objectId) == existing.end()) {
                    return DraftingCommandResult::rejected(DraftingResultCode::InvalidSelectionTarget, "selection target does not exist");
                }
            }
            selectMany(document, typedCommand.objectIds, existing);
            ++document.revision;
            return DraftingCommandResult::accepted();
        } else if constexpr (std::is_same_v<Command, CreatePlugCommand>) {
            return fromStoreResult(addPlug(document, typedCommand.plug));
        } else if constexpr (std::is_same_v<Command, DeletePlugCommand>) {
            return fromStoreResult(removePlug(document, typedCommand.plugId));
        } else if constexpr (std::is_same_v<Command, DeclareConnectionCommand>) {
            return fromStoreResult(declareConnection(document, typedCommand.connection));
        } else if constexpr (std::is_same_v<Command, DeleteConnectionCommand>) {
            return fromStoreResult(undeclareConnection(document, typedCommand.connectionId));
        } else if constexpr (std::is_same_v<Command, UpdatePlugCommand>) {
            return fromStoreResult(updatePlug(document, typedCommand.plugId, typedCommand.type));
        } else if constexpr (std::is_same_v<Command, CreateBlockCommand>) {
            return fromStoreResult(addBlock(document, typedCommand.block));
        } else if constexpr (std::is_same_v<Command, DeleteBlockCommand>) {
            return fromStoreResult(removeBlock(document, typedCommand.blockId));
        } else if constexpr (std::is_same_v<Command, CreateMapRoomCommand>) {
            return fromStoreResult(addMapRoom(document, typedCommand.room));
        } else {
            // Exhaustiveness guard (same idiom as the guarded DraftingGeometry
            // visits): every command alternative is handled above, so this arm is
            // never instantiated today. A NEW DraftingCommand variant must now fail
            // to COMPILE here — naming this function — instead of silently
            // returning a runtime "unsupported command" rejection.
            static_assert(always_false_v<Command>, "applyDraftingCommand: unhandled DraftingCommand arm — add an arm");
        }
    }, command);
}

} // namespace edi::drafting
