#include "drafting/DraftingCommands.h"
#include "drafting/DraftingSelection.h"

#include "EdiAssert.h"
#include <limits>

using namespace edi::drafting;

int main()
{
    DraftingDocument document = makeDraftingDocument("doc");

    auto acceptedResult = DraftingCommandResult::accepted();
    EDI_CHECK(acceptedResult.ok);
    EDI_CHECK(acceptedResult.code == DraftingResultCode::None);
    auto rejectedResult = DraftingCommandResult::rejected(DraftingResultCode::ObjectNotFound, "missing");
    EDI_CHECK(!rejectedResult.ok);
    EDI_CHECK(rejectedResult.code == DraftingResultCode::ObjectNotFound);

    auto builtPoint = buildDraftingObject("point_1", DraftingShapeKind::Point, PointGeometry{{4.0, 5.0}});
    EDI_CHECK(builtPoint.ok);
    DraftingObject point = builtPoint.object;

    auto create = applyDraftingCommand(document, CreateObjectCommand{point});
    EDI_CHECK(create.ok);
    EDI_CHECK(create.code == DraftingResultCode::None);
    EDI_CHECK(containsObject(document, "point_1"));

    auto select = applyDraftingCommand(document, SelectObjectCommand{"point_1"});
    EDI_CHECK(select.ok);
    EDI_CHECK(document.activeObjectId == "point_1");

    auto move = applyDraftingCommand(document, MoveObjectCommand{"point_1", 1.0, 1.0});
    EDI_CHECK(move.ok);
    const auto *moved = findObject(document, "point_1");
    EDI_CHECK(moved != nullptr);
    EDI_CHECK(moved->bounds.x == 5.0);
    EDI_CHECK(moved->bounds.y == 6.0);

    auto del = applyDraftingCommand(document, DeleteObjectCommand{"point_1"});
    EDI_CHECK(del.ok);
    EDI_CHECK(!containsObject(document, "point_1"));

    const auto revisionAfterDelete = document.revision;
    auto missingSelect = applyDraftingCommand(document, SelectObjectCommand{"point_1"});
    EDI_CHECK(!missingSelect.ok);
    EDI_CHECK(missingSelect.code == DraftingResultCode::InvalidSelectionTarget);
    EDI_CHECK(document.revision == revisionAfterDelete);
    EDI_CHECK(!document.activeObjectId);

    DraftingObject invalidPolyline = makeDraftingObject("polyline_1", DraftingShapeKind::Polyline, PolylineGeometry{{{0.0, 0.0}}});
    auto invalidCreate = applyDraftingCommand(document, CreateObjectCommand{invalidPolyline});
    EDI_CHECK(!invalidCreate.ok);
    EDI_CHECK(invalidCreate.code == DraftingResultCode::InvalidGeometry);
    EDI_CHECK(document.revision == revisionAfterDelete);

    // CreateObjectsCommand (#30): an atomic batch. Either every object lands
    // with ONE revision bump, or a rejection anywhere leaves the document
    // exactly as it was — no half-committed batches.
    {
        DraftingDocument batchDocument = makeDraftingDocument("batch_doc");
        auto seed = applyDraftingCommand(batchDocument,
            CreateObjectCommand{makeDraftingObject("existing_1", DraftingShapeKind::Point, PointGeometry{{0.1, 0.1}})});
        EDI_CHECK(seed.ok);
        const auto revisionBeforeBatch = batchDocument.revision;

        // Empty batch: accepted no-op, no revision bump.
        auto emptyBatch = applyDraftingCommand(batchDocument, CreateObjectsCommand{});
        EDI_CHECK(emptyBatch.ok);
        EDI_CHECK(batchDocument.revision == revisionBeforeBatch);

        std::vector<DraftingObject> batch = {
            makeDraftingObject("batch_1", DraftingShapeKind::Point, PointGeometry{{0.2, 0.2}}),
            makeDraftingObject("batch_2", DraftingShapeKind::Line, LineGeometry{{0.1, 0.1}, {0.3, 0.3}}),
            makeDraftingObject("batch_3", DraftingShapeKind::Point, PointGeometry{{0.4, 0.4}}),
        };
        auto batchCreate = applyDraftingCommand(batchDocument, CreateObjectsCommand{batch});
        EDI_CHECK(batchCreate.ok);
        EDI_CHECK(batchDocument.objects.size() == 4);
        EDI_CHECK(batchDocument.revision == revisionBeforeBatch + 1); // ONE bump for the batch
        EDI_CHECK(containsObject(batchDocument, "batch_2"));
        // Bounds are computed on the way in, like addObject does.
        EDI_CHECK(findObject(batchDocument, "batch_3")->bounds.x == 0.4);

        // Duplicate against an EXISTING object: rejected, nothing applied.
        const auto sizeBeforeRejects = batchDocument.objects.size();
        const auto revisionBeforeRejects = batchDocument.revision;
        std::vector<DraftingObject> dupExisting = {
            makeDraftingObject("fresh_1", DraftingShapeKind::Point, PointGeometry{{0.5, 0.5}}),
            makeDraftingObject("existing_1", DraftingShapeKind::Point, PointGeometry{{0.6, 0.6}}),
        };
        auto dupExistingResult = applyDraftingCommand(batchDocument, CreateObjectsCommand{dupExisting});
        EDI_CHECK(!dupExistingResult.ok);
        EDI_CHECK(dupExistingResult.code == DraftingResultCode::DuplicateObjectId);
        EDI_CHECK(batchDocument.objects.size() == sizeBeforeRejects);
        EDI_CHECK(!containsObject(batchDocument, "fresh_1")); // atomic: the valid head did NOT land

        // Duplicate WITHIN the batch: same rejection, same atomicity.
        std::vector<DraftingObject> dupInternal = {
            makeDraftingObject("twin_1", DraftingShapeKind::Point, PointGeometry{{0.5, 0.5}}),
            makeDraftingObject("twin_1", DraftingShapeKind::Point, PointGeometry{{0.6, 0.6}}),
        };
        auto dupInternalResult = applyDraftingCommand(batchDocument, CreateObjectsCommand{dupInternal});
        EDI_CHECK(!dupInternalResult.ok);
        EDI_CHECK(dupInternalResult.code == DraftingResultCode::DuplicateObjectId);
        EDI_CHECK(!containsObject(batchDocument, "twin_1"));

        // An invalid shape mid-batch rejects the whole batch.
        std::vector<DraftingObject> invalidTail = {
            makeDraftingObject("good_1", DraftingShapeKind::Point, PointGeometry{{0.5, 0.5}}),
            makeDraftingObject("bad_1", DraftingShapeKind::Polyline, PolylineGeometry{{{0.0, 0.0}}}),
        };
        auto invalidTailResult = applyDraftingCommand(batchDocument, CreateObjectsCommand{invalidTail});
        EDI_CHECK(!invalidTailResult.ok);
        EDI_CHECK(invalidTailResult.code == DraftingResultCode::InvalidGeometry);
        EDI_CHECK(!containsObject(batchDocument, "good_1"));

        // An unknown layer rejects the whole batch.
        DraftingObject orphan = makeDraftingObject("orphan_1", DraftingShapeKind::Point, PointGeometry{{0.5, 0.5}});
        orphan.layerId = "no_such_layer";
        auto orphanResult = applyDraftingCommand(batchDocument, CreateObjectsCommand{{orphan}});
        EDI_CHECK(!orphanResult.ok);
        EDI_CHECK(orphanResult.code == DraftingResultCode::LayerNotFound);

        EDI_CHECK(batchDocument.objects.size() == sizeBeforeRejects);
        EDI_CHECK(batchDocument.revision == revisionBeforeRejects);

        // A locked layer rejects the whole batch. (The batch handler
        // duplicates addObject's checks inline — without this case a
        // regression deleting its locked-layer branch passes the suite.)
        EDI_CHECK(applyDraftingCommand(batchDocument, CreateLayerCommand{makeDraftingLayer("frozen", "Frozen", 1), false}).ok);
        EDI_CHECK(applyDraftingCommand(batchDocument, UpdateLayerFlagsCommand{"frozen", true, true}).ok);
        const auto sizeBeforeLocked = batchDocument.objects.size();
        const auto revisionBeforeLocked = batchDocument.revision;
        DraftingObject onLocked = makeDraftingObject("locked_1", DraftingShapeKind::Point, PointGeometry{{0.5, 0.5}});
        onLocked.layerId = "frozen";
        auto lockedResult = applyDraftingCommand(batchDocument, CreateObjectsCommand{{onLocked}});
        EDI_CHECK(!lockedResult.ok);
        EDI_CHECK(lockedResult.code == DraftingResultCode::InvalidSelectionTarget);
        EDI_CHECK(batchDocument.objects.size() == sizeBeforeLocked);
        EDI_CHECK(batchDocument.revision == revisionBeforeLocked);
    }

    DraftingDocument layerCommandDocument = makeDraftingDocument("layer_command_doc");
    auto createLayer = applyDraftingCommand(layerCommandDocument, CreateLayerCommand{makeDraftingLayer("ink", "Ink", 1), true});
    EDI_CHECK(createLayer.ok);
    EDI_CHECK(layerCommandDocument.activeLayerId == "ink");
    EDI_CHECK(layerCommandDocument.layers.size() == 2);
    auto renameLayerCommand = applyDraftingCommand(layerCommandDocument, RenameLayerCommand{"ink", "Ink Layer"});
    EDI_CHECK(renameLayerCommand.ok);
    const DraftingLayer *inkLayer = findLayer(layerCommandDocument, "ink");
    EDI_CHECK(inkLayer != nullptr);
    EDI_CHECK(inkLayer->name == "Ink Layer");
    auto setDefaultLayerCommand = applyDraftingCommand(layerCommandDocument, SetActiveLayerCommand{"default"});
    EDI_CHECK(setDefaultLayerCommand.ok);
    EDI_CHECK(layerCommandDocument.activeLayerId == "default");
    const auto revisionBeforeMoveLayerCommand = layerCommandDocument.revision;
    auto moveDefaultLayerUpCommand = applyDraftingCommand(layerCommandDocument, MoveLayerCommand{"default", 1});
    EDI_CHECK(moveDefaultLayerUpCommand.ok);
    EDI_CHECK(layerCommandDocument.layers[0].id == "ink");
    EDI_CHECK(layerCommandDocument.layers[1].id == "default");
    EDI_CHECK(layerCommandDocument.revision == revisionBeforeMoveLayerCommand + 1);
    auto moveMissingLayerCommand = applyDraftingCommand(layerCommandDocument, MoveLayerCommand{"missing_layer", 1});
    EDI_CHECK(!moveMissingLayerCommand.ok);
    EDI_CHECK(moveMissingLayerCommand.code == DraftingResultCode::LayerNotFound);
    LayerPlotStyle inkPlot;
    inkPlot.plotEnabled = false;
    inkPlot.penId = "pen_red";
    inkPlot.strokeColor = "#d98b8b";
    inkPlot.strokeWidth = 3.0;
    const auto revisionBeforePlotCommand = layerCommandDocument.revision;
    auto updateInkPlotCommand = applyDraftingCommand(layerCommandDocument, UpdateLayerPlotStyleCommand{"ink", inkPlot});
    EDI_CHECK(updateInkPlotCommand.ok);
    inkLayer = findLayer(layerCommandDocument, "ink");
    EDI_CHECK(inkLayer != nullptr);
    EDI_CHECK(!inkLayer->plot.plotEnabled);
    EDI_CHECK(inkLayer->plot.penId == "pen_red");
    EDI_CHECK(inkLayer->plot.strokeWidth == 3.0);
    EDI_CHECK(layerCommandDocument.revision == revisionBeforePlotCommand + 1);
    inkPlot.strokeWidth = -1.0;
    auto invalidPlotCommand = applyDraftingCommand(layerCommandDocument, UpdateLayerPlotStyleCommand{"ink", inkPlot});
    EDI_CHECK(!invalidPlotCommand.ok);
    EDI_CHECK(invalidPlotCommand.code == DraftingResultCode::InvalidGeometry);
    auto commandLineBuild = buildDraftingObject("command_line", DraftingShapeKind::Line, LineGeometry{{0.0, 0.0}, {1.0, 0.0}});
    EDI_CHECK(commandLineBuild.ok);
    EDI_CHECK(applyDraftingCommand(layerCommandDocument, CreateObjectCommand{commandLineBuild.object}).ok);
    auto moveToInkLayer = applyDraftingCommand(layerCommandDocument, MoveObjectToLayerCommand{"command_line", "ink"});
    EDI_CHECK(moveToInkLayer.ok);
    const DraftingObject *commandLine = findObject(layerCommandDocument, "command_line");
    EDI_CHECK(commandLine != nullptr);
    EDI_CHECK(commandLine->layerId == "ink");
    EDI_CHECK(applyDraftingCommand(layerCommandDocument, UpdateLayerFlagsCommand{"ink", true, true}).ok);
    const auto revisionBeforeLockedLayerMove = layerCommandDocument.revision;
    auto moveFromLockedLayer = applyDraftingCommand(layerCommandDocument, MoveObjectToLayerCommand{"command_line", "default"});
    EDI_CHECK(!moveFromLockedLayer.ok);
    EDI_CHECK(moveFromLockedLayer.code == DraftingResultCode::InvalidSelectionTarget);
    EDI_CHECK(layerCommandDocument.revision == revisionBeforeLockedLayerMove);

    DraftingDocument editDocument = makeDraftingDocument("edit_doc");
    auto builtLine = buildDraftingObject("line_1", DraftingShapeKind::Line, LineGeometry{{0.0, 0.0}, {10.0, 10.0}});
    EDI_CHECK(builtLine.ok);
    EDI_CHECK(applyDraftingCommand(editDocument, CreateObjectCommand{builtLine.object}).ok);
    const auto revisionAfterLineCreate = editDocument.revision;
    auto editLine = applyDraftingCommand(editDocument, EditObjectHandleCommand{"line_1", "line_end", {20.0, 5.0}});
    EDI_CHECK(editLine.ok);
    const auto *editedLineObject = findObject(editDocument, "line_1");
    EDI_CHECK(editedLineObject != nullptr);
    const auto *editedLine = std::get_if<LineGeometry>(&editedLineObject->geometry);
    EDI_CHECK(editedLine != nullptr);
    EDI_CHECK(editedLine->b.x == 20.0);
    EDI_CHECK(editedLine->b.y == 5.0);
    EDI_CHECK(editDocument.revision == revisionAfterLineCreate + 1);

    const auto revisionBeforeNumericEdit = editDocument.revision;
    auto numericLineEdit = applyDraftingCommand(editDocument, NumericGeometryEditCommand{"line_1", "x2", 12.0});
    EDI_CHECK(numericLineEdit.ok);
    editedLineObject = findObject(editDocument, "line_1");
    EDI_CHECK(editedLineObject != nullptr);
    editedLine = std::get_if<LineGeometry>(&editedLineObject->geometry);
    EDI_CHECK(editedLine != nullptr);
    EDI_CHECK(editedLine->b.x == 12.0);
    EDI_CHECK(editedLine->b.y == 5.0);
    EDI_CHECK(editDocument.revision == revisionBeforeNumericEdit + 1);

    const auto revisionAfterNumericEdit = editDocument.revision;
    auto invalidNumericField = applyDraftingCommand(editDocument, NumericGeometryEditCommand{"line_1", "missing_field", 0.0});
    EDI_CHECK(!invalidNumericField.ok);
    EDI_CHECK(invalidNumericField.code == DraftingResultCode::InvalidGeometry);
    editedLineObject = findObject(editDocument, "line_1");
    EDI_CHECK(editedLineObject != nullptr);
    editedLine = std::get_if<LineGeometry>(&editedLineObject->geometry);
    EDI_CHECK(editedLine != nullptr);
    EDI_CHECK(editedLine->b.x == 12.0);
    EDI_CHECK(editedLine->b.y == 5.0);
    EDI_CHECK(editDocument.revision == revisionAfterNumericEdit);

    auto invalidNumericValue = applyDraftingCommand(editDocument, NumericGeometryEditCommand{"line_1", "x2", std::numeric_limits<double>::infinity()});
    EDI_CHECK(!invalidNumericValue.ok);
    EDI_CHECK(invalidNumericValue.code == DraftingResultCode::InvalidGeometry);
    EDI_CHECK(editDocument.revision == revisionAfterNumericEdit);

    auto missingNumericObject = applyDraftingCommand(editDocument, NumericGeometryEditCommand{"missing_line", "x2", 1.0});
    EDI_CHECK(!missingNumericObject.ok);
    EDI_CHECK(missingNumericObject.code == DraftingResultCode::ObjectNotFound);
    EDI_CHECK(editDocument.revision == revisionAfterNumericEdit);

    const auto revisionAfterLineEdit = editDocument.revision;
    auto badHandle = applyDraftingCommand(editDocument, EditObjectHandleCommand{"line_1", "missing_handle", {1.0, 1.0}});
    EDI_CHECK(!badHandle.ok);
    EDI_CHECK(badHandle.code == DraftingResultCode::InvalidSelectionTarget);
    EDI_CHECK(editDocument.revision == revisionAfterLineEdit);

    const auto revisionBeforeLock = editDocument.revision;
    auto lockLine = applyDraftingCommand(editDocument, UpdateObjectFlagsCommand{"line_1", true, true});
    EDI_CHECK(lockLine.ok);
    const auto *lockedLineObject = findObject(editDocument, "line_1");
    EDI_CHECK(lockedLineObject != nullptr);
    EDI_CHECK(lockedLineObject->locked);
    EDI_CHECK(editDocument.revision == revisionBeforeLock + 1);
    const auto revisionAfterLock = editDocument.revision;
    auto lockedEdit = applyDraftingCommand(editDocument, EditObjectHandleCommand{"line_1", "line_end", {30.0, 30.0}});
    EDI_CHECK(!lockedEdit.ok);
    EDI_CHECK(lockedEdit.code == DraftingResultCode::InvalidSelectionTarget);
    auto lockedNumericEdit = applyDraftingCommand(editDocument, NumericGeometryEditCommand{"line_1", "x2", 30.0});
    EDI_CHECK(!lockedNumericEdit.ok);
    EDI_CHECK(lockedNumericEdit.code == DraftingResultCode::InvalidSelectionTarget);
    auto lockedMove = applyDraftingCommand(editDocument, MoveObjectCommand{"line_1", 1.0, 0.0});
    EDI_CHECK(!lockedMove.ok);
    EDI_CHECK(lockedMove.code == DraftingResultCode::InvalidSelectionTarget);
    auto lockedDelete = applyDraftingCommand(editDocument, DeleteObjectCommand{"line_1"});
    EDI_CHECK(!lockedDelete.ok);
    EDI_CHECK(lockedDelete.code == DraftingResultCode::InvalidSelectionTarget);
    EDI_CHECK(editDocument.revision == revisionAfterLock);
    auto unlockLine = applyDraftingCommand(editDocument, UpdateObjectFlagsCommand{"line_1", false, true});
    EDI_CHECK(unlockLine.ok);

    auto builtRect = buildDraftingObject("rect_1", DraftingShapeKind::Rectangle, RectangleGeometry{{1.0, 1.0}, 3.0, 4.0});
    EDI_CHECK(builtRect.ok);
    EDI_CHECK(applyDraftingCommand(editDocument, CreateObjectCommand{builtRect.object}).ok);
    EDI_CHECK(applyDraftingCommand(editDocument, SelectObjectCommand{"line_1"}).ok);
    toggleSelection(editDocument, "rect_1");
    const auto revisionBeforeSelectionMove = editDocument.revision;
    auto moveSelection = applyDraftingCommand(editDocument, MoveSelectionCommand{2.0, 3.0});
    EDI_CHECK(moveSelection.ok);
    EDI_CHECK(editDocument.revision == revisionBeforeSelectionMove + 1);
    const auto *movedLineObject = findObject(editDocument, "line_1");
    const auto *movedRectObject = findObject(editDocument, "rect_1");
    EDI_CHECK(movedLineObject != nullptr);
    EDI_CHECK(movedRectObject != nullptr);
    const auto *movedLine = std::get_if<LineGeometry>(&movedLineObject->geometry);
    const auto *movedRect = std::get_if<RectangleGeometry>(&movedRectObject->geometry);
    EDI_CHECK(movedLine != nullptr);
    EDI_CHECK(movedRect != nullptr);
    EDI_CHECK(movedLine->a.x == 2.0);
    EDI_CHECK(movedLine->a.y == 3.0);
    EDI_CHECK(movedRect->origin.x == 3.0);
    EDI_CHECK(movedRect->origin.y == 4.0);

    const auto revisionBeforeAlign = editDocument.revision;
    auto alignLeft = applyDraftingCommand(editDocument, AlignSelectionCommand{DraftingAlignmentMode::Left});
    EDI_CHECK(alignLeft.ok);
    EDI_CHECK(editDocument.revision == revisionBeforeAlign + 1);
    movedRectObject = findObject(editDocument, "rect_1");
    EDI_CHECK(movedRectObject != nullptr);
    movedRect = std::get_if<RectangleGeometry>(&movedRectObject->geometry);
    EDI_CHECK(movedRect != nullptr);
    EDI_CHECK(movedRect->origin.x == 2.0);
    EDI_CHECK(movedRect->origin.y == 4.0);

    auto builtMiddlePoint = buildDraftingObject("middle_point", DraftingShapeKind::Point, PointGeometry{{10.0, 1.0}});
    EDI_CHECK(builtMiddlePoint.ok);
    EDI_CHECK(applyDraftingCommand(editDocument, CreateObjectCommand{builtMiddlePoint.object}).ok);

    // Multi-select edge cases: an unknown id rejects the whole command and
    // leaves the previous selection intact; duplicate input ids dedupe (no
    // double insert), with the active id being the last UNIQUE id.
    const auto revisionBeforeBadSelect = editDocument.revision;
    auto badSelect = applyDraftingCommand(editDocument, SelectObjectsCommand{{"line_1", "no_such_object"}});
    EDI_CHECK(!badSelect.ok);
    EDI_CHECK(badSelect.code == DraftingResultCode::InvalidSelectionTarget);
    EDI_CHECK(editDocument.revision == revisionBeforeBadSelect);
    auto dupSelect = applyDraftingCommand(editDocument, SelectObjectsCommand{{"line_1", "line_1", "rect_1"}});
    EDI_CHECK(dupSelect.ok);
    EDI_CHECK(editDocument.selectedObjectIds.size() == 2);
    EDI_CHECK(editDocument.activeObjectId == "rect_1");

    EDI_CHECK(applyDraftingCommand(editDocument, SelectObjectsCommand{{"line_1", "middle_point", "rect_1"}}).ok);
    const auto revisionBeforeDistribute = editDocument.revision;
    auto distributeY = applyDraftingCommand(editDocument, DistributeSelectionCommand{DraftingAlignmentMode::DistributeY});
    EDI_CHECK(distributeY.ok);
    EDI_CHECK(editDocument.revision == revisionBeforeDistribute + 1);
    const auto *distributedLineObject = findObject(editDocument, "line_1");
    EDI_CHECK(distributedLineObject != nullptr);
    const auto *distributedLine = std::get_if<LineGeometry>(&distributedLineObject->geometry);
    EDI_CHECK(distributedLine != nullptr);
    EDI_CHECK(distributedLine->a.y == 1.0);
    EDI_CHECK(distributedLine->b.y == 6.0);
    const auto revisionBeforeBadArrangeMode = editDocument.revision;
    auto badAlignMode = applyDraftingCommand(editDocument, AlignSelectionCommand{DraftingAlignmentMode::DistributeX});
    EDI_CHECK(!badAlignMode.ok);
    EDI_CHECK(badAlignMode.code == DraftingResultCode::InvalidGeometry);
    EDI_CHECK(editDocument.revision == revisionBeforeBadArrangeMode);
    auto badDistributeMode = applyDraftingCommand(editDocument, DistributeSelectionCommand{DraftingAlignmentMode::Left});
    EDI_CHECK(!badDistributeMode.ok);
    EDI_CHECK(badDistributeMode.code == DraftingResultCode::InvalidGeometry);
    EDI_CHECK(editDocument.revision == revisionBeforeBadArrangeMode);

    const auto revisionBeforeLayerLock = editDocument.revision;
    auto lockDefaultLayer = applyDraftingCommand(editDocument, UpdateLayerFlagsCommand{"default", true, false});
    EDI_CHECK(lockDefaultLayer.ok);
    const DraftingLayer *defaultLayer = findLayer(editDocument, "default");
    EDI_CHECK(defaultLayer != nullptr);
    EDI_CHECK(defaultLayer->locked);
    EDI_CHECK(!defaultLayer->visible);
    EDI_CHECK(editDocument.revision == revisionBeforeLayerLock + 1);
    const auto revisionAfterLayerLock = editDocument.revision;
    auto layerLockedMove = applyDraftingCommand(editDocument, MoveSelectionCommand{1.0, 0.0});
    EDI_CHECK(!layerLockedMove.ok);
    EDI_CHECK(layerLockedMove.code == DraftingResultCode::InvalidSelectionTarget);
    auto layerLockedFlags = applyDraftingCommand(editDocument, UpdateObjectFlagsCommand{"line_1", true, true});
    EDI_CHECK(!layerLockedFlags.ok);
    EDI_CHECK(layerLockedFlags.code == DraftingResultCode::InvalidSelectionTarget);
    EDI_CHECK(editDocument.revision == revisionAfterLayerLock);
    auto unlockDefaultLayer = applyDraftingCommand(editDocument, UpdateLayerFlagsCommand{"default", false, true});
    EDI_CHECK(unlockDefaultLayer.ok);

    const auto revisionBeforeBadMove = editDocument.revision;
    auto badSelectionMove = applyDraftingCommand(editDocument, MoveSelectionCommand{std::numeric_limits<double>::infinity(), 0.0});
    EDI_CHECK(!badSelectionMove.ok);
    EDI_CHECK(badSelectionMove.code == DraftingResultCode::InvalidGeometry);
    EDI_CHECK(editDocument.revision == revisionBeforeBadMove);

    // MoveSelection rejection ORDER (protects the interleaved containsObject
    // guard against a future "pre-scan all ids first" dedup): for a
    // [present+locked, stale] selection the FIRST id decides the message. The
    // locked object is examined first and moveObject reports "object is locked";
    // a pre-scan would instead report the LATER missing id's "selection target
    // does not exist" — same CODE, different message, and message is observable
    // upstream (finishEdit). So the guard must stay interleaved with the move.
    {
        DraftingDocument lockOrderDocument = makeDraftingDocument("lock_order_doc");
        auto lockOrderBuilt = buildDraftingObject("locked_obj", DraftingShapeKind::Point, PointGeometry{{1.0, 1.0}});
        EDI_CHECK(lockOrderBuilt.ok);
        EDI_CHECK(applyDraftingCommand(lockOrderDocument, CreateObjectCommand{lockOrderBuilt.object}).ok);
        EDI_CHECK(applyDraftingCommand(lockOrderDocument, UpdateObjectFlagsCommand{"locked_obj", true, true}).ok);
        const auto revisionBeforeLockOrder = lockOrderDocument.revision;

        // Locked FIRST, stale SECOND. Inject the selection directly —
        // SelectObjectsCommand would reject the stale id at entry.
        lockOrderDocument.selectedObjectIds = {"locked_obj", "ghost_obj"};
        auto lockedThenMissing = applyDraftingCommand(lockOrderDocument, MoveSelectionCommand{1.0, 0.0});
        EDI_CHECK(!lockedThenMissing.ok);
        EDI_CHECK(lockedThenMissing.code == DraftingResultCode::InvalidSelectionTarget);
        EDI_CHECK(lockedThenMissing.message == "object is locked");
        EDI_CHECK(lockOrderDocument.revision == revisionBeforeLockOrder);

        // Stale FIRST: the guard fires before any move is attempted.
        lockOrderDocument.selectedObjectIds = {"ghost_obj", "locked_obj"};
        auto missingThenLocked = applyDraftingCommand(lockOrderDocument, MoveSelectionCommand{1.0, 0.0});
        EDI_CHECK(!missingThenLocked.ok);
        EDI_CHECK(missingThenLocked.code == DraftingResultCode::InvalidSelectionTarget);
        EDI_CHECK(missingThenLocked.message == "selection target does not exist");
        EDI_CHECK(lockOrderDocument.revision == revisionBeforeLockOrder);
    }

    DraftingDocument guideCommandDocument = makeDraftingDocument("guide_command_doc");
    auto guideA = buildDraftingObject("guide_a", DraftingShapeKind::Guide, GuideGeometry{GuideOrientation::Vertical, 0.25});
    auto guideDuplicate = buildDraftingObject("guide_dup", DraftingShapeKind::Guide, GuideGeometry{GuideOrientation::Vertical, 0.2500005});
    auto guideHorizontal = buildDraftingObject("guide_h", DraftingShapeKind::Guide, GuideGeometry{GuideOrientation::Horizontal, 0.25});
    auto guideLine = buildDraftingObject("guide_line", DraftingShapeKind::Line, LineGeometry{{0.0, 0.0}, {1.0, 0.0}});
    EDI_CHECK(guideA.ok);
    EDI_CHECK(guideDuplicate.ok);
    EDI_CHECK(guideHorizontal.ok);
    EDI_CHECK(guideLine.ok);
    EDI_CHECK(applyDraftingCommand(guideCommandDocument, CreateObjectCommand{guideA.object}).ok);
    EDI_CHECK(applyDraftingCommand(guideCommandDocument, CreateObjectCommand{guideDuplicate.object}).ok);
    EDI_CHECK(applyDraftingCommand(guideCommandDocument, CreateObjectCommand{guideHorizontal.object}).ok);
    EDI_CHECK(applyDraftingCommand(guideCommandDocument, CreateObjectCommand{guideLine.object}).ok);
    EDI_CHECK(applyDraftingCommand(guideCommandDocument, SelectObjectCommand{"guide_dup"}).ok);
    const auto revisionBeforeGuideMerge = guideCommandDocument.revision;
    auto mergeGuides = applyDraftingCommand(guideCommandDocument, MergeDuplicateGuidesCommand{});
    EDI_CHECK(mergeGuides.ok);
    EDI_CHECK(guideCommandDocument.revision == revisionBeforeGuideMerge + 1);
    EDI_CHECK(containsObject(guideCommandDocument, "guide_a"));
    EDI_CHECK(!containsObject(guideCommandDocument, "guide_dup"));
    EDI_CHECK(containsObject(guideCommandDocument, "guide_h"));
    EDI_CHECK(containsObject(guideCommandDocument, "guide_line"));
    EDI_CHECK(!guideCommandDocument.activeObjectId);

    const auto revisionBeforeNoopGuideMerge = guideCommandDocument.revision;
    auto noopMergeGuides = applyDraftingCommand(guideCommandDocument, MergeDuplicateGuidesCommand{});
    EDI_CHECK(noopMergeGuides.ok);
    EDI_CHECK(guideCommandDocument.revision == revisionBeforeNoopGuideMerge);

    auto hideGuides = applyDraftingCommand(guideCommandDocument, SetAllGuidesVisibleCommand{false});
    EDI_CHECK(hideGuides.ok);
    const DraftingObject *hiddenGuideA = findObject(guideCommandDocument, "guide_a");
    const DraftingObject *hiddenGuideH = findObject(guideCommandDocument, "guide_h");
    const DraftingObject *visibleLine = findObject(guideCommandDocument, "guide_line");
    EDI_CHECK(hiddenGuideA != nullptr);
    EDI_CHECK(hiddenGuideH != nullptr);
    EDI_CHECK(visibleLine != nullptr);
    EDI_CHECK(!hiddenGuideA->visible);
    EDI_CHECK(!hiddenGuideH->visible);
    EDI_CHECK(visibleLine->visible);

    auto lockGuides = applyDraftingCommand(guideCommandDocument, SetAllGuidesLockedCommand{true});
    EDI_CHECK(lockGuides.ok);
    hiddenGuideA = findObject(guideCommandDocument, "guide_a");
    hiddenGuideH = findObject(guideCommandDocument, "guide_h");
    visibleLine = findObject(guideCommandDocument, "guide_line");
    EDI_CHECK(hiddenGuideA != nullptr);
    EDI_CHECK(hiddenGuideH != nullptr);
    EDI_CHECK(visibleLine != nullptr);
    EDI_CHECK(hiddenGuideA->locked);
    EDI_CHECK(hiddenGuideH->locked);
    EDI_CHECK(!visibleLine->locked);

    auto deleteGuides = applyDraftingCommand(guideCommandDocument, DeleteAllGuidesCommand{});
    EDI_CHECK(deleteGuides.ok);
    EDI_CHECK(!containsObject(guideCommandDocument, "guide_a"));
    EDI_CHECK(!containsObject(guideCommandDocument, "guide_h"));
    EDI_CHECK(containsObject(guideCommandDocument, "guide_line"));

    // S2: map-graph commands reach DraftingGraphOps through the dispatcher and
    // carry the store result back out, so they undo via DocumentSnapshot like any
    // other command. Validation lives in the ops (S1); here we prove the wiring.
    {
        DraftingDocument graphDoc = makeDraftingDocument("graph-cmd");
        graphDoc.objects.push_back(makeDraftingObject("m.0", DraftingShapeKind::Point, PointGeometry{}));
        graphDoc.objects.push_back(makeDraftingObject("m.1", DraftingShapeKind::Point, PointGeometry{}));

        DraftingPlug a; a.id = "plug_a"; a.anchorObjectId = "m.0"; a.name = "north"; a.type = "door";
        DraftingPlug b; b.id = "plug_b"; b.anchorObjectId = "m.1"; b.name = "south"; b.type = "door";
        EDI_CHECK(applyDraftingCommand(graphDoc, CreatePlugCommand{a}).ok);
        EDI_CHECK(applyDraftingCommand(graphDoc, CreatePlugCommand{b}).ok);
        EDI_CHECK(graphDoc.plugs.size() == 2);

        // A plug whose anchor is missing is rejected by the op, surfaced here.
        DraftingPlug orphan; orphan.id = "plug_x"; orphan.anchorObjectId = "ghost";
        auto badPlug = applyDraftingCommand(graphDoc, CreatePlugCommand{orphan});
        EDI_CHECK(!badPlug.ok);
        EDI_CHECK(badPlug.code == DraftingResultCode::ObjectNotFound);
        EDI_CHECK(graphDoc.plugs.size() == 2);

        DraftingDeclaredConnection ab; ab.id = "conn_ab"; ab.plugA = "plug_a"; ab.plugB = "plug_b"; ab.type = "corridor";
        EDI_CHECK(applyDraftingCommand(graphDoc, DeclareConnectionCommand{ab}).ok);
        EDI_CHECK(graphDoc.connections.size() == 1);

        // An edge to a missing plug is rejected.
        DraftingDeclaredConnection bad; bad.id = "conn_bad"; bad.plugA = "plug_a"; bad.plugB = "plug_ghost";
        EDI_CHECK(!applyDraftingCommand(graphDoc, DeclareConnectionCommand{bad}).ok);
        EDI_CHECK(graphDoc.connections.size() == 1);

        // Deleting a plug cascades its edge away through the command path too.
        EDI_CHECK(applyDraftingCommand(graphDoc, DeletePlugCommand{"plug_a"}).ok);
        EDI_CHECK(graphDoc.plugs.size() == 1);
        EDI_CHECK(graphDoc.connections.empty());

        // Deleting a now-missing connection is rejected; the explicit delete of a
        // live connection works (declare a fresh one to prove it).
        DraftingPlug c; c.id = "plug_c"; c.anchorObjectId = "m.0";
        EDI_CHECK(applyDraftingCommand(graphDoc, CreatePlugCommand{c}).ok);
        DraftingDeclaredConnection bc; bc.id = "conn_bc"; bc.plugA = "plug_b"; bc.plugB = "plug_c";
        EDI_CHECK(applyDraftingCommand(graphDoc, DeclareConnectionCommand{bc}).ok);
        EDI_CHECK(applyDraftingCommand(graphDoc, DeleteConnectionCommand{"conn_missing"}).ok == false);
        EDI_CHECK(applyDraftingCommand(graphDoc, DeleteConnectionCommand{"conn_bc"}).ok);
        EDI_CHECK(graphDoc.connections.empty());
    }

    // B2-3: UpdatePlugCommand arm — delegate to updatePlug, same accepted/rejected shape.
    {
        DraftingDocument doc = makeDraftingDocument("cmd-updateplug");
        doc.objects.push_back(makeDraftingObject("m.0", DraftingShapeKind::Point, PointGeometry{}));
        DraftingPlug p; p.id = "plug_a"; p.anchorObjectId = "m.0"; p.type = "door";
        EDI_CHECK(applyDraftingCommand(doc, CreatePlugCommand{p}).ok);

        // UpdatePlugCommand sets type correctly.
        EDI_CHECK(applyDraftingCommand(doc, UpdatePlugCommand{"plug_a", "secret"}).ok);
        EDI_CHECK(doc.plugs[0].type == "secret");

        // UpdatePlugCommand for unknown plug is rejected.
        EDI_CHECK(!applyDraftingCommand(doc, UpdatePlugCommand{"no_such_plug", "window"}).ok);
    }

    // S4: deleting the object a plug anchors to, through the command path, prunes
    // the plug with it (so undo via DocumentSnapshot restores both atomically).
    {
        DraftingDocument doc = makeDraftingDocument("graph-objdelete");
        doc.objects.push_back(makeDraftingObject("m.0", DraftingShapeKind::Point, PointGeometry{}));
        DraftingPlug a; a.id = "plug_a"; a.anchorObjectId = "m.0";
        EDI_CHECK(applyDraftingCommand(doc, CreatePlugCommand{a}).ok);
        EDI_CHECK(doc.plugs.size() == 1);

        EDI_CHECK(applyDraftingCommand(doc, DeleteObjectCommand{"m.0"}).ok);
        EDI_CHECK(!containsObject(doc, "m.0"));
        EDI_CHECK(doc.plugs.empty()); // the plug went with its anchor
    }

    return 0;
}
