#include "drafting/DraftingCommands.h"
#include "drafting/DraftingSelection.h"

#include <cassert>
#include <limits>

using namespace edi::drafting;

int main()
{
    DraftingDocument document = makeDraftingDocument("doc");

    auto acceptedResult = DraftingCommandResult::accepted();
    assert(acceptedResult.ok);
    assert(acceptedResult.code == DraftingResultCode::None);
    auto rejectedResult = DraftingCommandResult::rejected(DraftingResultCode::ObjectNotFound, "missing");
    assert(!rejectedResult.ok);
    assert(rejectedResult.code == DraftingResultCode::ObjectNotFound);

    auto builtPoint = buildDraftingObject("point_1", DraftingShapeKind::Point, PointGeometry{{4.0, 5.0}});
    assert(builtPoint.ok);
    DraftingObject point = builtPoint.object;

    auto create = applyDraftingCommand(document, CreateObjectCommand{point});
    assert(create.ok);
    assert(create.code == DraftingResultCode::None);
    assert(containsObject(document, "point_1"));

    auto select = applyDraftingCommand(document, SelectObjectCommand{"point_1"});
    assert(select.ok);
    assert(document.activeObjectId == "point_1");

    auto move = applyDraftingCommand(document, MoveObjectCommand{"point_1", 1.0, 1.0});
    assert(move.ok);
    const auto *moved = findObject(document, "point_1");
    assert(moved != nullptr);
    assert(moved->bounds.x == 5.0);
    assert(moved->bounds.y == 6.0);

    auto del = applyDraftingCommand(document, DeleteObjectCommand{"point_1"});
    assert(del.ok);
    assert(!containsObject(document, "point_1"));

    const auto revisionAfterDelete = document.revision;
    auto missingSelect = applyDraftingCommand(document, SelectObjectCommand{"point_1"});
    assert(!missingSelect.ok);
    assert(missingSelect.code == DraftingResultCode::InvalidSelectionTarget);
    assert(document.revision == revisionAfterDelete);
    assert(!document.activeObjectId);

    DraftingObject invalidPolyline = makeDraftingObject("polyline_1", DraftingShapeKind::Polyline, PolylineGeometry{{{0.0, 0.0}}});
    auto invalidCreate = applyDraftingCommand(document, CreateObjectCommand{invalidPolyline});
    assert(!invalidCreate.ok);
    assert(invalidCreate.code == DraftingResultCode::InvalidGeometry);
    assert(document.revision == revisionAfterDelete);

    // CreateObjectsCommand (#30): an atomic batch. Either every object lands
    // with ONE revision bump, or a rejection anywhere leaves the document
    // exactly as it was — no half-committed batches.
    {
        DraftingDocument batchDocument = makeDraftingDocument("batch_doc");
        auto seed = applyDraftingCommand(batchDocument,
            CreateObjectCommand{makeDraftingObject("existing_1", DraftingShapeKind::Point, PointGeometry{{0.1, 0.1}})});
        assert(seed.ok);
        const auto revisionBeforeBatch = batchDocument.revision;

        // Empty batch: accepted no-op, no revision bump.
        auto emptyBatch = applyDraftingCommand(batchDocument, CreateObjectsCommand{});
        assert(emptyBatch.ok);
        assert(batchDocument.revision == revisionBeforeBatch);

        std::vector<DraftingObject> batch = {
            makeDraftingObject("batch_1", DraftingShapeKind::Point, PointGeometry{{0.2, 0.2}}),
            makeDraftingObject("batch_2", DraftingShapeKind::Line, LineGeometry{{0.1, 0.1}, {0.3, 0.3}}),
            makeDraftingObject("batch_3", DraftingShapeKind::Point, PointGeometry{{0.4, 0.4}}),
        };
        auto batchCreate = applyDraftingCommand(batchDocument, CreateObjectsCommand{batch});
        assert(batchCreate.ok);
        assert(batchDocument.objects.size() == 4);
        assert(batchDocument.revision == revisionBeforeBatch + 1); // ONE bump for the batch
        assert(containsObject(batchDocument, "batch_2"));
        // Bounds are computed on the way in, like addObject does.
        assert(findObject(batchDocument, "batch_3")->bounds.x == 0.4);

        // Duplicate against an EXISTING object: rejected, nothing applied.
        const auto sizeBeforeRejects = batchDocument.objects.size();
        const auto revisionBeforeRejects = batchDocument.revision;
        std::vector<DraftingObject> dupExisting = {
            makeDraftingObject("fresh_1", DraftingShapeKind::Point, PointGeometry{{0.5, 0.5}}),
            makeDraftingObject("existing_1", DraftingShapeKind::Point, PointGeometry{{0.6, 0.6}}),
        };
        auto dupExistingResult = applyDraftingCommand(batchDocument, CreateObjectsCommand{dupExisting});
        assert(!dupExistingResult.ok);
        assert(dupExistingResult.code == DraftingResultCode::DuplicateObjectId);
        assert(batchDocument.objects.size() == sizeBeforeRejects);
        assert(!containsObject(batchDocument, "fresh_1")); // atomic: the valid head did NOT land

        // Duplicate WITHIN the batch: same rejection, same atomicity.
        std::vector<DraftingObject> dupInternal = {
            makeDraftingObject("twin_1", DraftingShapeKind::Point, PointGeometry{{0.5, 0.5}}),
            makeDraftingObject("twin_1", DraftingShapeKind::Point, PointGeometry{{0.6, 0.6}}),
        };
        auto dupInternalResult = applyDraftingCommand(batchDocument, CreateObjectsCommand{dupInternal});
        assert(!dupInternalResult.ok);
        assert(dupInternalResult.code == DraftingResultCode::DuplicateObjectId);
        assert(!containsObject(batchDocument, "twin_1"));

        // An invalid shape mid-batch rejects the whole batch.
        std::vector<DraftingObject> invalidTail = {
            makeDraftingObject("good_1", DraftingShapeKind::Point, PointGeometry{{0.5, 0.5}}),
            makeDraftingObject("bad_1", DraftingShapeKind::Polyline, PolylineGeometry{{{0.0, 0.0}}}),
        };
        auto invalidTailResult = applyDraftingCommand(batchDocument, CreateObjectsCommand{invalidTail});
        assert(!invalidTailResult.ok);
        assert(invalidTailResult.code == DraftingResultCode::InvalidGeometry);
        assert(!containsObject(batchDocument, "good_1"));

        // An unknown layer rejects the whole batch.
        DraftingObject orphan = makeDraftingObject("orphan_1", DraftingShapeKind::Point, PointGeometry{{0.5, 0.5}});
        orphan.layerId = "no_such_layer";
        auto orphanResult = applyDraftingCommand(batchDocument, CreateObjectsCommand{{orphan}});
        assert(!orphanResult.ok);
        assert(orphanResult.code == DraftingResultCode::LayerNotFound);

        assert(batchDocument.objects.size() == sizeBeforeRejects);
        assert(batchDocument.revision == revisionBeforeRejects);

        // A locked layer rejects the whole batch. (The batch handler
        // duplicates addObject's checks inline — without this case a
        // regression deleting its locked-layer branch passes the suite.)
        assert(applyDraftingCommand(batchDocument, CreateLayerCommand{makeDraftingLayer("frozen", "Frozen", 1), false}).ok);
        assert(applyDraftingCommand(batchDocument, UpdateLayerFlagsCommand{"frozen", true, true}).ok);
        const auto sizeBeforeLocked = batchDocument.objects.size();
        const auto revisionBeforeLocked = batchDocument.revision;
        DraftingObject onLocked = makeDraftingObject("locked_1", DraftingShapeKind::Point, PointGeometry{{0.5, 0.5}});
        onLocked.layerId = "frozen";
        auto lockedResult = applyDraftingCommand(batchDocument, CreateObjectsCommand{{onLocked}});
        assert(!lockedResult.ok);
        assert(lockedResult.code == DraftingResultCode::InvalidSelectionTarget);
        assert(batchDocument.objects.size() == sizeBeforeLocked);
        assert(batchDocument.revision == revisionBeforeLocked);
    }

    DraftingDocument layerCommandDocument = makeDraftingDocument("layer_command_doc");
    auto createLayer = applyDraftingCommand(layerCommandDocument, CreateLayerCommand{makeDraftingLayer("ink", "Ink", 1), true});
    assert(createLayer.ok);
    assert(layerCommandDocument.activeLayerId == "ink");
    assert(layerCommandDocument.layers.size() == 2);
    auto renameLayerCommand = applyDraftingCommand(layerCommandDocument, RenameLayerCommand{"ink", "Ink Layer"});
    assert(renameLayerCommand.ok);
    const DraftingLayer *inkLayer = findLayer(layerCommandDocument, "ink");
    assert(inkLayer != nullptr);
    assert(inkLayer->name == "Ink Layer");
    auto setDefaultLayerCommand = applyDraftingCommand(layerCommandDocument, SetActiveLayerCommand{"default"});
    assert(setDefaultLayerCommand.ok);
    assert(layerCommandDocument.activeLayerId == "default");
    const auto revisionBeforeMoveLayerCommand = layerCommandDocument.revision;
    auto moveDefaultLayerUpCommand = applyDraftingCommand(layerCommandDocument, MoveLayerCommand{"default", 1});
    assert(moveDefaultLayerUpCommand.ok);
    assert(layerCommandDocument.layers[0].id == "ink");
    assert(layerCommandDocument.layers[1].id == "default");
    assert(layerCommandDocument.revision == revisionBeforeMoveLayerCommand + 1);
    auto moveMissingLayerCommand = applyDraftingCommand(layerCommandDocument, MoveLayerCommand{"missing_layer", 1});
    assert(!moveMissingLayerCommand.ok);
    assert(moveMissingLayerCommand.code == DraftingResultCode::LayerNotFound);
    LayerPlotStyle inkPlot;
    inkPlot.plotEnabled = false;
    inkPlot.penId = "pen_red";
    inkPlot.strokeColor = "#d98b8b";
    inkPlot.strokeWidth = 3.0;
    const auto revisionBeforePlotCommand = layerCommandDocument.revision;
    auto updateInkPlotCommand = applyDraftingCommand(layerCommandDocument, UpdateLayerPlotStyleCommand{"ink", inkPlot});
    assert(updateInkPlotCommand.ok);
    inkLayer = findLayer(layerCommandDocument, "ink");
    assert(inkLayer != nullptr);
    assert(!inkLayer->plot.plotEnabled);
    assert(inkLayer->plot.penId == "pen_red");
    assert(inkLayer->plot.strokeWidth == 3.0);
    assert(layerCommandDocument.revision == revisionBeforePlotCommand + 1);
    inkPlot.strokeWidth = -1.0;
    auto invalidPlotCommand = applyDraftingCommand(layerCommandDocument, UpdateLayerPlotStyleCommand{"ink", inkPlot});
    assert(!invalidPlotCommand.ok);
    assert(invalidPlotCommand.code == DraftingResultCode::InvalidGeometry);
    auto commandLineBuild = buildDraftingObject("command_line", DraftingShapeKind::Line, LineGeometry{{0.0, 0.0}, {1.0, 0.0}});
    assert(commandLineBuild.ok);
    assert(applyDraftingCommand(layerCommandDocument, CreateObjectCommand{commandLineBuild.object}).ok);
    auto moveToInkLayer = applyDraftingCommand(layerCommandDocument, MoveObjectToLayerCommand{"command_line", "ink"});
    assert(moveToInkLayer.ok);
    const DraftingObject *commandLine = findObject(layerCommandDocument, "command_line");
    assert(commandLine != nullptr);
    assert(commandLine->layerId == "ink");
    assert(applyDraftingCommand(layerCommandDocument, UpdateLayerFlagsCommand{"ink", true, true}).ok);
    const auto revisionBeforeLockedLayerMove = layerCommandDocument.revision;
    auto moveFromLockedLayer = applyDraftingCommand(layerCommandDocument, MoveObjectToLayerCommand{"command_line", "default"});
    assert(!moveFromLockedLayer.ok);
    assert(moveFromLockedLayer.code == DraftingResultCode::InvalidSelectionTarget);
    assert(layerCommandDocument.revision == revisionBeforeLockedLayerMove);

    DraftingDocument editDocument = makeDraftingDocument("edit_doc");
    auto builtLine = buildDraftingObject("line_1", DraftingShapeKind::Line, LineGeometry{{0.0, 0.0}, {10.0, 10.0}});
    assert(builtLine.ok);
    assert(applyDraftingCommand(editDocument, CreateObjectCommand{builtLine.object}).ok);
    const auto revisionAfterLineCreate = editDocument.revision;
    auto editLine = applyDraftingCommand(editDocument, EditObjectHandleCommand{"line_1", "line_end", {20.0, 5.0}});
    assert(editLine.ok);
    const auto *editedLineObject = findObject(editDocument, "line_1");
    assert(editedLineObject != nullptr);
    const auto *editedLine = std::get_if<LineGeometry>(&editedLineObject->geometry);
    assert(editedLine != nullptr);
    assert(editedLine->b.x == 20.0);
    assert(editedLine->b.y == 5.0);
    assert(editDocument.revision == revisionAfterLineCreate + 1);

    const auto revisionBeforeNumericEdit = editDocument.revision;
    auto numericLineEdit = applyDraftingCommand(editDocument, NumericGeometryEditCommand{"line_1", "x2", 12.0});
    assert(numericLineEdit.ok);
    editedLineObject = findObject(editDocument, "line_1");
    assert(editedLineObject != nullptr);
    editedLine = std::get_if<LineGeometry>(&editedLineObject->geometry);
    assert(editedLine != nullptr);
    assert(editedLine->b.x == 12.0);
    assert(editedLine->b.y == 5.0);
    assert(editDocument.revision == revisionBeforeNumericEdit + 1);

    const auto revisionAfterNumericEdit = editDocument.revision;
    auto invalidNumericField = applyDraftingCommand(editDocument, NumericGeometryEditCommand{"line_1", "missing_field", 0.0});
    assert(!invalidNumericField.ok);
    assert(invalidNumericField.code == DraftingResultCode::InvalidGeometry);
    editedLineObject = findObject(editDocument, "line_1");
    assert(editedLineObject != nullptr);
    editedLine = std::get_if<LineGeometry>(&editedLineObject->geometry);
    assert(editedLine != nullptr);
    assert(editedLine->b.x == 12.0);
    assert(editedLine->b.y == 5.0);
    assert(editDocument.revision == revisionAfterNumericEdit);

    auto invalidNumericValue = applyDraftingCommand(editDocument, NumericGeometryEditCommand{"line_1", "x2", std::numeric_limits<double>::infinity()});
    assert(!invalidNumericValue.ok);
    assert(invalidNumericValue.code == DraftingResultCode::InvalidGeometry);
    assert(editDocument.revision == revisionAfterNumericEdit);

    auto missingNumericObject = applyDraftingCommand(editDocument, NumericGeometryEditCommand{"missing_line", "x2", 1.0});
    assert(!missingNumericObject.ok);
    assert(missingNumericObject.code == DraftingResultCode::ObjectNotFound);
    assert(editDocument.revision == revisionAfterNumericEdit);

    const auto revisionAfterLineEdit = editDocument.revision;
    auto badHandle = applyDraftingCommand(editDocument, EditObjectHandleCommand{"line_1", "missing_handle", {1.0, 1.0}});
    assert(!badHandle.ok);
    assert(badHandle.code == DraftingResultCode::InvalidSelectionTarget);
    assert(editDocument.revision == revisionAfterLineEdit);

    const auto revisionBeforeLock = editDocument.revision;
    auto lockLine = applyDraftingCommand(editDocument, UpdateObjectFlagsCommand{"line_1", true, true});
    assert(lockLine.ok);
    const auto *lockedLineObject = findObject(editDocument, "line_1");
    assert(lockedLineObject != nullptr);
    assert(lockedLineObject->locked);
    assert(editDocument.revision == revisionBeforeLock + 1);
    const auto revisionAfterLock = editDocument.revision;
    auto lockedEdit = applyDraftingCommand(editDocument, EditObjectHandleCommand{"line_1", "line_end", {30.0, 30.0}});
    assert(!lockedEdit.ok);
    assert(lockedEdit.code == DraftingResultCode::InvalidSelectionTarget);
    auto lockedNumericEdit = applyDraftingCommand(editDocument, NumericGeometryEditCommand{"line_1", "x2", 30.0});
    assert(!lockedNumericEdit.ok);
    assert(lockedNumericEdit.code == DraftingResultCode::InvalidSelectionTarget);
    auto lockedMove = applyDraftingCommand(editDocument, MoveObjectCommand{"line_1", 1.0, 0.0});
    assert(!lockedMove.ok);
    assert(lockedMove.code == DraftingResultCode::InvalidSelectionTarget);
    auto lockedDelete = applyDraftingCommand(editDocument, DeleteObjectCommand{"line_1"});
    assert(!lockedDelete.ok);
    assert(lockedDelete.code == DraftingResultCode::InvalidSelectionTarget);
    assert(editDocument.revision == revisionAfterLock);
    auto unlockLine = applyDraftingCommand(editDocument, UpdateObjectFlagsCommand{"line_1", false, true});
    assert(unlockLine.ok);

    auto builtRect = buildDraftingObject("rect_1", DraftingShapeKind::Rectangle, RectangleGeometry{{1.0, 1.0}, 3.0, 4.0});
    assert(builtRect.ok);
    assert(applyDraftingCommand(editDocument, CreateObjectCommand{builtRect.object}).ok);
    assert(applyDraftingCommand(editDocument, SelectObjectCommand{"line_1"}).ok);
    toggleSelection(editDocument, "rect_1");
    const auto revisionBeforeSelectionMove = editDocument.revision;
    auto moveSelection = applyDraftingCommand(editDocument, MoveSelectionCommand{2.0, 3.0});
    assert(moveSelection.ok);
    assert(editDocument.revision == revisionBeforeSelectionMove + 1);
    const auto *movedLineObject = findObject(editDocument, "line_1");
    const auto *movedRectObject = findObject(editDocument, "rect_1");
    assert(movedLineObject != nullptr);
    assert(movedRectObject != nullptr);
    const auto *movedLine = std::get_if<LineGeometry>(&movedLineObject->geometry);
    const auto *movedRect = std::get_if<RectangleGeometry>(&movedRectObject->geometry);
    assert(movedLine != nullptr);
    assert(movedRect != nullptr);
    assert(movedLine->a.x == 2.0);
    assert(movedLine->a.y == 3.0);
    assert(movedRect->origin.x == 3.0);
    assert(movedRect->origin.y == 4.0);

    const auto revisionBeforeAlign = editDocument.revision;
    auto alignLeft = applyDraftingCommand(editDocument, AlignSelectionCommand{DraftingAlignmentMode::Left});
    assert(alignLeft.ok);
    assert(editDocument.revision == revisionBeforeAlign + 1);
    movedRectObject = findObject(editDocument, "rect_1");
    assert(movedRectObject != nullptr);
    movedRect = std::get_if<RectangleGeometry>(&movedRectObject->geometry);
    assert(movedRect != nullptr);
    assert(movedRect->origin.x == 2.0);
    assert(movedRect->origin.y == 4.0);

    auto builtMiddlePoint = buildDraftingObject("middle_point", DraftingShapeKind::Point, PointGeometry{{10.0, 1.0}});
    assert(builtMiddlePoint.ok);
    assert(applyDraftingCommand(editDocument, CreateObjectCommand{builtMiddlePoint.object}).ok);

    // Multi-select edge cases: an unknown id rejects the whole command and
    // leaves the previous selection intact; duplicate input ids dedupe (no
    // double insert), with the active id being the last UNIQUE id.
    const auto revisionBeforeBadSelect = editDocument.revision;
    auto badSelect = applyDraftingCommand(editDocument, SelectObjectsCommand{{"line_1", "no_such_object"}});
    assert(!badSelect.ok);
    assert(badSelect.code == DraftingResultCode::InvalidSelectionTarget);
    assert(editDocument.revision == revisionBeforeBadSelect);
    auto dupSelect = applyDraftingCommand(editDocument, SelectObjectsCommand{{"line_1", "line_1", "rect_1"}});
    assert(dupSelect.ok);
    assert(editDocument.selectedObjectIds.size() == 2);
    assert(editDocument.activeObjectId == "rect_1");

    assert(applyDraftingCommand(editDocument, SelectObjectsCommand{{"line_1", "middle_point", "rect_1"}}).ok);
    const auto revisionBeforeDistribute = editDocument.revision;
    auto distributeY = applyDraftingCommand(editDocument, DistributeSelectionCommand{DraftingAlignmentMode::DistributeY});
    assert(distributeY.ok);
    assert(editDocument.revision == revisionBeforeDistribute + 1);
    const auto *distributedLineObject = findObject(editDocument, "line_1");
    assert(distributedLineObject != nullptr);
    const auto *distributedLine = std::get_if<LineGeometry>(&distributedLineObject->geometry);
    assert(distributedLine != nullptr);
    assert(distributedLine->a.y == 1.0);
    assert(distributedLine->b.y == 6.0);
    const auto revisionBeforeBadArrangeMode = editDocument.revision;
    auto badAlignMode = applyDraftingCommand(editDocument, AlignSelectionCommand{DraftingAlignmentMode::DistributeX});
    assert(!badAlignMode.ok);
    assert(badAlignMode.code == DraftingResultCode::InvalidGeometry);
    assert(editDocument.revision == revisionBeforeBadArrangeMode);
    auto badDistributeMode = applyDraftingCommand(editDocument, DistributeSelectionCommand{DraftingAlignmentMode::Left});
    assert(!badDistributeMode.ok);
    assert(badDistributeMode.code == DraftingResultCode::InvalidGeometry);
    assert(editDocument.revision == revisionBeforeBadArrangeMode);

    const auto revisionBeforeLayerLock = editDocument.revision;
    auto lockDefaultLayer = applyDraftingCommand(editDocument, UpdateLayerFlagsCommand{"default", true, false});
    assert(lockDefaultLayer.ok);
    const DraftingLayer *defaultLayer = findLayer(editDocument, "default");
    assert(defaultLayer != nullptr);
    assert(defaultLayer->locked);
    assert(!defaultLayer->visible);
    assert(editDocument.revision == revisionBeforeLayerLock + 1);
    const auto revisionAfterLayerLock = editDocument.revision;
    auto layerLockedMove = applyDraftingCommand(editDocument, MoveSelectionCommand{1.0, 0.0});
    assert(!layerLockedMove.ok);
    assert(layerLockedMove.code == DraftingResultCode::InvalidSelectionTarget);
    auto layerLockedFlags = applyDraftingCommand(editDocument, UpdateObjectFlagsCommand{"line_1", true, true});
    assert(!layerLockedFlags.ok);
    assert(layerLockedFlags.code == DraftingResultCode::InvalidSelectionTarget);
    assert(editDocument.revision == revisionAfterLayerLock);
    auto unlockDefaultLayer = applyDraftingCommand(editDocument, UpdateLayerFlagsCommand{"default", false, true});
    assert(unlockDefaultLayer.ok);

    const auto revisionBeforeBadMove = editDocument.revision;
    auto badSelectionMove = applyDraftingCommand(editDocument, MoveSelectionCommand{std::numeric_limits<double>::infinity(), 0.0});
    assert(!badSelectionMove.ok);
    assert(badSelectionMove.code == DraftingResultCode::InvalidGeometry);
    assert(editDocument.revision == revisionBeforeBadMove);

    DraftingDocument guideCommandDocument = makeDraftingDocument("guide_command_doc");
    auto guideA = buildDraftingObject("guide_a", DraftingShapeKind::Guide, GuideGeometry{GuideOrientation::Vertical, 0.25});
    auto guideDuplicate = buildDraftingObject("guide_dup", DraftingShapeKind::Guide, GuideGeometry{GuideOrientation::Vertical, 0.2500005});
    auto guideHorizontal = buildDraftingObject("guide_h", DraftingShapeKind::Guide, GuideGeometry{GuideOrientation::Horizontal, 0.25});
    auto guideLine = buildDraftingObject("guide_line", DraftingShapeKind::Line, LineGeometry{{0.0, 0.0}, {1.0, 0.0}});
    assert(guideA.ok);
    assert(guideDuplicate.ok);
    assert(guideHorizontal.ok);
    assert(guideLine.ok);
    assert(applyDraftingCommand(guideCommandDocument, CreateObjectCommand{guideA.object}).ok);
    assert(applyDraftingCommand(guideCommandDocument, CreateObjectCommand{guideDuplicate.object}).ok);
    assert(applyDraftingCommand(guideCommandDocument, CreateObjectCommand{guideHorizontal.object}).ok);
    assert(applyDraftingCommand(guideCommandDocument, CreateObjectCommand{guideLine.object}).ok);
    assert(applyDraftingCommand(guideCommandDocument, SelectObjectCommand{"guide_dup"}).ok);
    const auto revisionBeforeGuideMerge = guideCommandDocument.revision;
    auto mergeGuides = applyDraftingCommand(guideCommandDocument, MergeDuplicateGuidesCommand{});
    assert(mergeGuides.ok);
    assert(guideCommandDocument.revision == revisionBeforeGuideMerge + 1);
    assert(containsObject(guideCommandDocument, "guide_a"));
    assert(!containsObject(guideCommandDocument, "guide_dup"));
    assert(containsObject(guideCommandDocument, "guide_h"));
    assert(containsObject(guideCommandDocument, "guide_line"));
    assert(!guideCommandDocument.activeObjectId);

    const auto revisionBeforeNoopGuideMerge = guideCommandDocument.revision;
    auto noopMergeGuides = applyDraftingCommand(guideCommandDocument, MergeDuplicateGuidesCommand{});
    assert(noopMergeGuides.ok);
    assert(guideCommandDocument.revision == revisionBeforeNoopGuideMerge);

    auto hideGuides = applyDraftingCommand(guideCommandDocument, SetAllGuidesVisibleCommand{false});
    assert(hideGuides.ok);
    const DraftingObject *hiddenGuideA = findObject(guideCommandDocument, "guide_a");
    const DraftingObject *hiddenGuideH = findObject(guideCommandDocument, "guide_h");
    const DraftingObject *visibleLine = findObject(guideCommandDocument, "guide_line");
    assert(hiddenGuideA != nullptr);
    assert(hiddenGuideH != nullptr);
    assert(visibleLine != nullptr);
    assert(!hiddenGuideA->visible);
    assert(!hiddenGuideH->visible);
    assert(visibleLine->visible);

    auto lockGuides = applyDraftingCommand(guideCommandDocument, SetAllGuidesLockedCommand{true});
    assert(lockGuides.ok);
    hiddenGuideA = findObject(guideCommandDocument, "guide_a");
    hiddenGuideH = findObject(guideCommandDocument, "guide_h");
    visibleLine = findObject(guideCommandDocument, "guide_line");
    assert(hiddenGuideA != nullptr);
    assert(hiddenGuideH != nullptr);
    assert(visibleLine != nullptr);
    assert(hiddenGuideA->locked);
    assert(hiddenGuideH->locked);
    assert(!visibleLine->locked);

    auto deleteGuides = applyDraftingCommand(guideCommandDocument, DeleteAllGuidesCommand{});
    assert(deleteGuides.ok);
    assert(!containsObject(guideCommandDocument, "guide_a"));
    assert(!containsObject(guideCommandDocument, "guide_h"));
    assert(containsObject(guideCommandDocument, "guide_line"));

    return 0;
}
