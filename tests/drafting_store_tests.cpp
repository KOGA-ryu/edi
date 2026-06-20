#include "drafting/DraftingStore.h"

#include "drafting/DraftingDocument.h"
#include "drafting/DraftingGeometry.h"
#include "drafting/DraftingSelection.h"

#include "EdiAssert.h"
#include <limits>
#include <optional>
#include <variant>

using namespace edi::drafting;

namespace {

DraftingObject makeLine(const char *id)
{
    return makeDraftingObject(id, DraftingShapeKind::Line, LineGeometry{{0.0, 0.0}, {10.0, 0.0}});
}

} // namespace

int main()
{
    DraftingDocument document = makeDraftingDocument("doc");
    EDI_CHECK(document.title == "doc");
    DraftingDocument explicitTitle = makeDraftingDocument("doc_with_title", "Plan A");
    EDI_CHECK(explicitTitle.title == "Plan A");
    DraftingDocument emptyDraftingDocument = makeDraftingDocument("");
    EDI_CHECK(emptyDraftingDocument.id.empty());
    EDI_CHECK(emptyDraftingDocument.title.empty());
    EDI_CHECK(isValidDraftingDocumentId("doc"));
    EDI_CHECK(!isValidDraftingDocumentId(""));
    EDI_CHECK(isValidDraftingDocumentTitle("Plan"));
    EDI_CHECK(!isValidDraftingDocumentTitle(""));
    EDI_CHECK(isValidDraftingObjectId("line_1"));
    EDI_CHECK(!isValidDraftingObjectId(""));
    EDI_CHECK(draftingObjectIdForSerial("line", 7) == "line_0007");
    EDI_CHECK(draftingObjectIdForSerial("guide", 42) == "guide_0042");
    EDI_CHECK(isValidLayerId("default"));
    EDI_CHECK(!isValidLayerId(""));
    EDI_CHECK(isValidLayerName("Default"));
    EDI_CHECK(!isValidLayerName(""));
    DraftingLayer defaultLayer = makeDefaultLayer();
    EDI_CHECK(defaultLayer.id == "default");
    EDI_CHECK(defaultLayer.name == "Default");
    EDI_CHECK(defaultLayer.order == 0);
    DraftingLayer explicitLayer = makeDraftingLayer("overlay", "Overlay", 2);
    EDI_CHECK(explicitLayer.id == "overlay");
    EDI_CHECK(explicitLayer.name == "Overlay");
    EDI_CHECK(explicitLayer.order == 2);
    DraftingLayer fallbackLayerName = makeDraftingLayer("measurements", "", 3);
    EDI_CHECK(fallbackLayerName.name == "measurements");
    EDI_CHECK(fallbackLayerName.order == 3);
    DraftingLayer emptyLayer = makeDraftingLayer("", "", 4);
    EDI_CHECK(emptyLayer.id.empty());
    EDI_CHECK(emptyLayer.name.empty());
    EDI_CHECK(emptyLayer.order == 4);
    DraftingObject helperObject = makeDraftingObject("helper_line", DraftingShapeKind::Line, LineGeometry{{1.0, 2.0}, {3.0, 4.0}});
    EDI_CHECK(helperObject.id == "helper_line");
    EDI_CHECK(helperObject.kind == DraftingShapeKind::Line);
    EDI_CHECK(helperObject.layerId == "default");
    EDI_CHECK(helperObject.bounds.width == 0.0);
    auto builtLine = buildDraftingObject("built_line", DraftingShapeKind::Line, LineGeometry{{0.0, 0.0}, {5.0, 0.0}});
    EDI_CHECK(builtLine.ok);
    EDI_CHECK(builtLine.code == DraftingResultCode::None);
    EDI_CHECK(builtLine.object.id == "built_line");
    EDI_CHECK(builtLine.object.kind == DraftingShapeKind::Line);
    EDI_CHECK(builtLine.object.bounds.width == 0.0);
    auto emptyBuildId = buildDraftingObject("", DraftingShapeKind::Line, LineGeometry{{0.0, 0.0}, {5.0, 0.0}});
    EDI_CHECK(!emptyBuildId.ok);
    EDI_CHECK(emptyBuildId.code == DraftingResultCode::EmptyObjectId);
    auto mismatchedBuild = buildDraftingObject("bad_kind_build", DraftingShapeKind::Line, PointGeometry{{0.0, 0.0}});
    EDI_CHECK(!mismatchedBuild.ok);
    EDI_CHECK(mismatchedBuild.code == DraftingResultCode::KindGeometryMismatch);
    auto invalidGeometryBuild = buildDraftingObject("bad_geometry_build", DraftingShapeKind::Circle, CircleGeometry{{0.0, 0.0}, -1.0});
    EDI_CHECK(!invalidGeometryBuild.ok);
    EDI_CHECK(invalidGeometryBuild.code == DraftingResultCode::InvalidGeometry);
    auto helperObjectValidation = validateDraftingObjectShape(helperObject);
    EDI_CHECK(helperObjectValidation.ok);
    EDI_CHECK(layerIndexById(document, "default") == 0);
    EDI_CHECK(findLayer(document, "default") == &document.layers[0]);
    EDI_CHECK(containsLayer(document, "default"));
    EDI_CHECK(layerIndexById(document, "missing_layer") == std::nullopt);
    EDI_CHECK(findLayer(document, "missing_layer") == nullptr);
    EDI_CHECK(!containsLayer(document, "missing_layer"));

    DraftingDocument layerDocument = makeDraftingDocument("layer_doc");
    EDI_CHECK(layerDocument.activeLayerId == "default");
    const auto layerRevisionBeforeAdd = layerDocument.revision;
    auto addLayerResult = addLayer(layerDocument, makeDraftingLayer("sketch", "Sketch", 1), true);
    EDI_CHECK(addLayerResult.ok);
    EDI_CHECK(layerDocument.layers.size() == 2);
    EDI_CHECK(layerDocument.activeLayerId == "sketch");
    EDI_CHECK(layerDocument.revision == layerRevisionBeforeAdd + 1);
    auto duplicateLayer = addLayer(layerDocument, makeDraftingLayer("sketch", "Duplicate", 2));
    EDI_CHECK(!duplicateLayer.ok);
    EDI_CHECK(duplicateLayer.code == DraftingResultCode::DuplicateLayerId);
    EDI_CHECK(layerDocument.layers.size() == 2);
    auto renameSketch = renameLayer(layerDocument, "sketch", "Sketch Layer");
    EDI_CHECK(renameSketch.ok);
    const DraftingLayer *sketchLayer = findLayer(layerDocument, "sketch");
    EDI_CHECK(sketchLayer != nullptr);
    EDI_CHECK(sketchLayer->name == "Sketch Layer");
    auto missingActiveLayer = setActiveLayer(layerDocument, "missing_layer");
    EDI_CHECK(!missingActiveLayer.ok);
    EDI_CHECK(missingActiveLayer.code == DraftingResultCode::LayerNotFound);
    auto setDefaultActive = setActiveLayer(layerDocument, "default");
    EDI_CHECK(setDefaultActive.ok);
    EDI_CHECK(layerDocument.activeLayerId == "default");
    auto addDuplicateOrderLayer = addLayer(layerDocument, makeDraftingLayer("notes", "Notes", 1));
    EDI_CHECK(addDuplicateOrderLayer.ok);
    EDI_CHECK(layerDocument.layers.size() == 3);
    EDI_CHECK(layerDocument.layers[0].id == "default");
    EDI_CHECK(layerDocument.layers[0].order == 0);
    EDI_CHECK(layerDocument.layers[1].id == "sketch");
    EDI_CHECK(layerDocument.layers[1].order == 1);
    EDI_CHECK(layerDocument.layers[2].id == "notes");
    EDI_CHECK(layerDocument.layers[2].order == 2);
    const auto revisionBeforeMoveLayer = layerDocument.revision;
    auto moveDefaultLayerUp = moveLayer(layerDocument, "default", 1);
    EDI_CHECK(moveDefaultLayerUp.ok);
    EDI_CHECK(layerDocument.layers[0].id == "sketch");
    EDI_CHECK(layerDocument.layers[0].order == 0);
    EDI_CHECK(layerDocument.layers[1].id == "default");
    EDI_CHECK(layerDocument.layers[1].order == 1);
    EDI_CHECK(layerDocument.layers[2].id == "notes");
    EDI_CHECK(layerDocument.layers[2].order == 2);
    EDI_CHECK(layerDocument.revision == revisionBeforeMoveLayer + 1);
    const auto revisionBeforeNoopLayerMove = layerDocument.revision;
    auto moveNotesLayerUpAtTop = moveLayer(layerDocument, "notes", 1);
    EDI_CHECK(moveNotesLayerUpAtTop.ok);
    EDI_CHECK(layerDocument.revision == revisionBeforeNoopLayerMove);
    auto moveMissingLayer = moveLayer(layerDocument, "missing_layer", 1);
    EDI_CHECK(!moveMissingLayer.ok);
    EDI_CHECK(moveMissingLayer.code == DraftingResultCode::LayerNotFound);
    LayerPlotStyle sketchPlot;
    sketchPlot.plotEnabled = false;
    sketchPlot.penId = "pen_blue";
    sketchPlot.strokeColor = "#75c7ff";
    sketchPlot.strokeWidth = 1.0;
    const auto revisionBeforePlotStyle = layerDocument.revision;
    auto updateSketchPlot = updateLayerPlotStyle(layerDocument, "sketch", sketchPlot);
    EDI_CHECK(updateSketchPlot.ok);
    sketchLayer = findLayer(layerDocument, "sketch");
    EDI_CHECK(sketchLayer != nullptr);
    EDI_CHECK(!sketchLayer->plot.plotEnabled);
    EDI_CHECK(sketchLayer->plot.penId == "pen_blue");
    EDI_CHECK(sketchLayer->plot.strokeColor == "#75c7ff");
    EDI_CHECK(sketchLayer->plot.strokeWidth == 1.0);
    EDI_CHECK(layerDocument.revision == revisionBeforePlotStyle + 1);
    const auto revisionBeforeSamePlotStyle = layerDocument.revision;
    auto sameSketchPlot = updateLayerPlotStyle(layerDocument, "sketch", sketchPlot);
    EDI_CHECK(sameSketchPlot.ok);
    EDI_CHECK(layerDocument.revision == revisionBeforeSamePlotStyle);
    LayerPlotStyle badPlot = sketchPlot;
    badPlot.strokeColor = "blue";
    auto invalidPlot = updateLayerPlotStyle(layerDocument, "sketch", badPlot);
    EDI_CHECK(!invalidPlot.ok);
    EDI_CHECK(invalidPlot.code == DraftingResultCode::InvalidGeometry);
    auto missingLayerPlot = updateLayerPlotStyle(layerDocument, "missing_layer", sketchPlot);
    EDI_CHECK(!missingLayerPlot.ok);
    EDI_CHECK(missingLayerPlot.code == DraftingResultCode::LayerNotFound);

    auto layerObjectAdd = addObject(layerDocument, makeLine("layer_line"));
    EDI_CHECK(layerObjectAdd.ok);
    auto lockSketchLayer = updateLayerFlags(layerDocument, "sketch", true, true);
    EDI_CHECK(lockSketchLayer.ok);
    auto moveToLockedLayer = moveObjectToLayer(layerDocument, "layer_line", "sketch");
    EDI_CHECK(!moveToLockedLayer.ok);
    EDI_CHECK(moveToLockedLayer.code == DraftingResultCode::InvalidSelectionTarget);
    auto unlockSketchLayer = updateLayerFlags(layerDocument, "sketch", false, true);
    EDI_CHECK(unlockSketchLayer.ok);
    auto moveToSketchLayer = moveObjectToLayer(layerDocument, "layer_line", "sketch");
    EDI_CHECK(moveToSketchLayer.ok);
    const DraftingObject *layerLine = findObject(layerDocument, "layer_line");
    EDI_CHECK(layerLine != nullptr);
    EDI_CHECK(layerLine->layerId == "sketch");
    lockSketchLayer = updateLayerFlags(layerDocument, "sketch", true, true);
    EDI_CHECK(lockSketchLayer.ok);
    auto moveFromLockedLayer = moveObjectToLayer(layerDocument, "layer_line", "default");
    EDI_CHECK(!moveFromLockedLayer.ok);
    EDI_CHECK(moveFromLockedLayer.code == DraftingResultCode::InvalidSelectionTarget);

    auto add = addObject(document, makeLine("line_1"));
    EDI_CHECK(add.ok);
    EDI_CHECK(document.objects.size() == 1);
    EDI_CHECK(document.objects.front().bounds.width == 10.0);
    EDI_CHECK(objectIndexById(document, "line_1") == 0);

    auto addSecond = addObject(document, makeLine("line_2"));
    EDI_CHECK(addSecond.ok);
    auto addThird = addObject(document, makeLine("line_3"));
    EDI_CHECK(addThird.ok);
    EDI_CHECK(document.objects.size() == 3);
    EDI_CHECK(document.objects[0].id == "line_1");
    EDI_CHECK(document.objects[1].id == "line_2");
    EDI_CHECK(document.objects[2].id == "line_3");
    EDI_CHECK(objectIndexById(document, "line_2") == 1);
    EDI_CHECK(findObject(document, "line_2") == &document.objects[1]);
    EDI_CHECK(containsObject(document, "line_2"));
    EDI_CHECK(objectIndexById(document, "missing") == std::nullopt);
    EDI_CHECK(findObject(document, "missing") == nullptr);
    EDI_CHECK(!containsObject(document, "missing"));

    auto duplicate = addObject(document, makeLine("line_1"));
    EDI_CHECK(!duplicate.ok);
    EDI_CHECK(duplicate.code == DraftingResultCode::DuplicateObjectId);
    EDI_CHECK(document.objects.size() == 3);
    EDI_CHECK(document.objects[0].id == "line_1");
    EDI_CHECK(document.objects[1].id == "line_2");
    EDI_CHECK(document.objects[2].id == "line_3");
    const auto revisionAfterDuplicate = document.revision;

    DraftingObject emptyId = makeLine("");
    auto emptyIdResult = addObject(document, emptyId);
    EDI_CHECK(!emptyIdResult.ok);
    EDI_CHECK(emptyIdResult.code == DraftingResultCode::EmptyObjectId);
    EDI_CHECK(document.revision == revisionAfterDuplicate);

    DraftingObject invalidCircle;
    invalidCircle.id = "bad_circle";
    invalidCircle.kind = DraftingShapeKind::Circle;
    invalidCircle.geometry = CircleGeometry{{0.0, 0.0}, -4.0};
    auto invalidCircleResult = addObject(document, invalidCircle);
    EDI_CHECK(!invalidCircleResult.ok);
    EDI_CHECK(invalidCircleResult.code == DraftingResultCode::InvalidGeometry);
    EDI_CHECK(invalidCircleResult.code == invalidGeometryBuild.code);
    EDI_CHECK(document.revision == revisionAfterDuplicate);

    DraftingObject badKind;
    badKind.id = "bad_kind";
    badKind.kind = DraftingShapeKind::Line;
    badKind.geometry = PointGeometry{{0.0, 0.0}};
    auto badKindResult = addObject(document, badKind);
    EDI_CHECK(!badKindResult.ok);
    EDI_CHECK(badKindResult.code == DraftingResultCode::KindGeometryMismatch);
    EDI_CHECK(badKindResult.code == mismatchedBuild.code);
    EDI_CHECK(document.revision == revisionAfterDuplicate);

    DraftingObject missingLayer = makeLine("missing_layer_object");
    missingLayer.layerId = "missing_layer";
    auto missingLayerResult = addObject(document, missingLayer);
    EDI_CHECK(!missingLayerResult.ok);
    EDI_CHECK(missingLayerResult.code == DraftingResultCode::LayerNotFound);
    EDI_CHECK(document.revision == revisionAfterDuplicate);
    EDI_CHECK(objectIndexById(document, "missing_layer_object") == std::nullopt);

    auto move = moveObject(document, "line_1", 2.0, 3.0);
    EDI_CHECK(move.ok);
    const auto *line = findObject(document, "line_1");
    EDI_CHECK(line != nullptr);
    EDI_CHECK(line->bounds.x == 2.0);
    EDI_CHECK(line->bounds.y == 3.0);
    const auto revisionAfterMove = document.revision;
    DraftingObject movedCandidate = *line;
    movedCandidate.geometry = translateGeometry(movedCandidate.geometry, 1.0, 1.0);
    auto movedCandidateShape = validateDraftingObjectShape(movedCandidate);
    EDI_CHECK(movedCandidateShape.ok);
    auto secondMove = moveObject(document, "line_1", 1.0, 1.0);
    EDI_CHECK(secondMove.ok);
    const auto *secondMovedLine = findObject(document, "line_1");
    EDI_CHECK(secondMovedLine != nullptr);
    EDI_CHECK(secondMovedLine->bounds.x == 3.0);
    EDI_CHECK(secondMovedLine->bounds.y == 4.0);
    EDI_CHECK(document.revision == revisionAfterMove + 1);
    auto updateLine = updateObjectGeometry(document, "line_1", LineGeometry{{10.0, 10.0}, {20.0, 10.0}});
    EDI_CHECK(updateLine.ok);
    const auto *updatedLine = findObject(document, "line_1");
    EDI_CHECK(updatedLine != nullptr);
    const Bounds2D boundsBeforeFailedUpdate = updatedLine->bounds;
    const DraftingGeometry geometryBeforeFailedUpdate = updatedLine->geometry;
    const auto revisionBeforeFailedUpdate = document.revision;

    DraftingObject invalidLineCandidate = *updatedLine;
    invalidLineCandidate.geometry = LineGeometry{{0.0, 0.0}, {std::numeric_limits<double>::quiet_NaN(), 0.0}};
    auto invalidLineShape = validateDraftingObjectShape(invalidLineCandidate);
    auto invalidUpdate = updateObjectGeometry(document, "line_1", invalidLineCandidate.geometry);
    EDI_CHECK(!invalidLineShape.ok);
    EDI_CHECK(!invalidUpdate.ok);
    EDI_CHECK(invalidUpdate.code == invalidLineShape.code);
    EDI_CHECK(document.revision == revisionBeforeFailedUpdate);
    const auto *afterInvalidUpdate = findObject(document, "line_1");
    EDI_CHECK(afterInvalidUpdate != nullptr);
    EDI_CHECK(afterInvalidUpdate->bounds.x == boundsBeforeFailedUpdate.x);
    EDI_CHECK(afterInvalidUpdate->bounds.y == boundsBeforeFailedUpdate.y);
    EDI_CHECK(afterInvalidUpdate->bounds.width == boundsBeforeFailedUpdate.width);
    EDI_CHECK(std::get<LineGeometry>(afterInvalidUpdate->geometry).a.x == std::get<LineGeometry>(geometryBeforeFailedUpdate).a.x);

    DraftingObject mismatchedUpdateCandidate = *updatedLine;
    mismatchedUpdateCandidate.geometry = PointGeometry{{0.0, 0.0}};
    auto mismatchedUpdateShape = validateDraftingObjectShape(mismatchedUpdateCandidate);
    auto mismatchedUpdate = updateObjectGeometry(document, "line_1", mismatchedUpdateCandidate.geometry);
    EDI_CHECK(!mismatchedUpdateShape.ok);
    EDI_CHECK(!mismatchedUpdate.ok);
    EDI_CHECK(mismatchedUpdate.code == mismatchedUpdateShape.code);
    EDI_CHECK(document.revision == revisionBeforeFailedUpdate);

    const Bounds2D boundsBeforeFailedMove = afterInvalidUpdate->bounds;
    const DraftingGeometry geometryBeforeFailedMove = afterInvalidUpdate->geometry;
    auto badMove = moveObject(document, "line_1", std::numeric_limits<double>::quiet_NaN(), 0.0);
    EDI_CHECK(!badMove.ok);
    EDI_CHECK(badMove.code == DraftingResultCode::InvalidGeometry);
    EDI_CHECK(document.revision == revisionBeforeFailedUpdate);
    const auto *afterBadMove = findObject(document, "line_1");
    EDI_CHECK(afterBadMove != nullptr);
    EDI_CHECK(afterBadMove->bounds.x == boundsBeforeFailedMove.x);
    EDI_CHECK(afterBadMove->bounds.y == boundsBeforeFailedMove.y);
    EDI_CHECK(afterBadMove->bounds.width == boundsBeforeFailedMove.width);
    EDI_CHECK(std::get<LineGeometry>(afterBadMove->geometry).a.x == std::get<LineGeometry>(geometryBeforeFailedMove).a.x);

    ObjectMetadata metadata;
    metadata.author = "tester";
    metadata.createdAt = "2026-06-08T12:30:00Z";
    metadata.toolProvenance = "drafting_store_tests";
    const Bounds2D boundsBeforeMetadata = afterBadMove->bounds;
    const DraftingGeometry geometryBeforeMetadata = afterBadMove->geometry;
    const auto revisionBeforeMetadata = document.revision;
    auto metadataUpdate = updateObjectMetadata(document, "line_1", metadata);
    EDI_CHECK(metadataUpdate.ok);
    const auto *afterMetadata = findObject(document, "line_1");
    EDI_CHECK(afterMetadata != nullptr);
    EDI_CHECK(afterMetadata->metadata.author == "tester");
    EDI_CHECK(afterMetadata->metadata.toolProvenance == "drafting_store_tests");
    EDI_CHECK(afterMetadata->bounds.x == boundsBeforeMetadata.x);
    EDI_CHECK(afterMetadata->bounds.y == boundsBeforeMetadata.y);
    EDI_CHECK(afterMetadata->bounds.width == boundsBeforeMetadata.width);
    EDI_CHECK(std::get<LineGeometry>(afterMetadata->geometry).a.x == std::get<LineGeometry>(geometryBeforeMetadata).a.x);
    EDI_CHECK(document.revision == revisionBeforeMetadata + 1);

    ObjectMetadata badTimestampMetadata = metadata;
    badTimestampMetadata.createdAt = "June 8";

    const auto revisionBeforeBadMetadata = document.revision;
    const Bounds2D boundsBeforeBadMetadata = afterMetadata->bounds;
    auto rejectedMetadataUpdate = updateObjectMetadata(document, "line_1", badTimestampMetadata);
    EDI_CHECK(!rejectedMetadataUpdate.ok);
    EDI_CHECK(rejectedMetadataUpdate.code == DraftingResultCode::InvalidMetadata);
    const auto *afterRejectedMetadata = findObject(document, "line_1");
    EDI_CHECK(afterRejectedMetadata != nullptr);
    EDI_CHECK(afterRejectedMetadata->metadata.createdAt == "2026-06-08T12:30:00Z");
    EDI_CHECK(afterRejectedMetadata->bounds.x == boundsBeforeBadMetadata.x);
    EDI_CHECK(afterRejectedMetadata->bounds.y == boundsBeforeBadMetadata.y);
    EDI_CHECK(document.revision == revisionBeforeBadMetadata);

    const auto revisionBeforeMissingMetadata = document.revision;
    auto missingMetadataUpdate = updateObjectMetadata(document, "missing", metadata);
    EDI_CHECK(!missingMetadataUpdate.ok);
    EDI_CHECK(missingMetadataUpdate.code == DraftingResultCode::ObjectNotFound);
    EDI_CHECK(document.revision == revisionBeforeMissingMetadata);

    const auto revisionBeforeFlags = document.revision;
    auto flagUpdate = updateObjectFlags(document, "line_1", true, false);
    EDI_CHECK(flagUpdate.ok);
    const auto *flaggedLine = findObject(document, "line_1");
    EDI_CHECK(flaggedLine != nullptr);
    EDI_CHECK(flaggedLine->locked);
    EDI_CHECK(!flaggedLine->visible);
    EDI_CHECK(document.revision == revisionBeforeFlags + 1);

    const auto revisionBeforeLockedMove = document.revision;
    auto lockedMove = moveObject(document, "line_1", 1.0, 0.0);
    EDI_CHECK(!lockedMove.ok);
    EDI_CHECK(lockedMove.code == DraftingResultCode::InvalidSelectionTarget);
    EDI_CHECK(document.revision == revisionBeforeLockedMove);
    auto lockedGeometryUpdate = updateObjectGeometry(document, "line_1", LineGeometry{{0.0, 0.0}, {2.0, 0.0}});
    EDI_CHECK(!lockedGeometryUpdate.ok);
    EDI_CHECK(lockedGeometryUpdate.code == DraftingResultCode::InvalidSelectionTarget);
    EDI_CHECK(document.revision == revisionBeforeLockedMove);

    auto unlockLine = updateObjectFlags(document, "line_1", false, true);
    EDI_CHECK(unlockLine.ok);
    flaggedLine = findObject(document, "line_1");
    EDI_CHECK(flaggedLine != nullptr);
    EDI_CHECK(!flaggedLine->locked);
    EDI_CHECK(flaggedLine->visible);

    const auto revisionBeforeMissingFlags = document.revision;
    auto missingFlags = updateObjectFlags(document, "missing", true, false);
    EDI_CHECK(!missingFlags.ok);
    EDI_CHECK(missingFlags.code == DraftingResultCode::ObjectNotFound);
    EDI_CHECK(document.revision == revisionBeforeMissingFlags);

    const auto revisionBeforeLayerFlags = document.revision;
    auto layerFlagUpdate = updateLayerFlags(document, "default", true, false);
    EDI_CHECK(layerFlagUpdate.ok);
    const DraftingLayer *defaultLayerState = findLayer(document, "default");
    EDI_CHECK(defaultLayerState != nullptr);
    EDI_CHECK(defaultLayerState->locked);
    EDI_CHECK(!defaultLayerState->visible);
    EDI_CHECK(document.revision == revisionBeforeLayerFlags + 1);

    const auto revisionBeforeLockedLayerMutation = document.revision;
    auto lockedLayerAdd = addObject(document, makeLine("locked_layer_line"));
    EDI_CHECK(!lockedLayerAdd.ok);
    EDI_CHECK(lockedLayerAdd.code == DraftingResultCode::InvalidSelectionTarget);
    auto lockedLayerObjectFlags = updateObjectFlags(document, "line_1", true, true);
    EDI_CHECK(!lockedLayerObjectFlags.ok);
    EDI_CHECK(lockedLayerObjectFlags.code == DraftingResultCode::InvalidSelectionTarget);
    auto lockedLayerMove = moveObject(document, "line_1", 1.0, 0.0);
    EDI_CHECK(!lockedLayerMove.ok);
    EDI_CHECK(lockedLayerMove.code == DraftingResultCode::InvalidSelectionTarget);
    auto lockedLayerGeometry = updateObjectGeometry(document, "line_1", LineGeometry{{0.0, 0.0}, {2.0, 0.0}});
    EDI_CHECK(!lockedLayerGeometry.ok);
    EDI_CHECK(lockedLayerGeometry.code == DraftingResultCode::InvalidSelectionTarget);
    EDI_CHECK(document.revision == revisionBeforeLockedLayerMutation);

    auto unlockLayer = updateLayerFlags(document, "default", false, true);
    EDI_CHECK(unlockLayer.ok);
    defaultLayerState = findLayer(document, "default");
    EDI_CHECK(defaultLayerState != nullptr);
    EDI_CHECK(!defaultLayerState->locked);
    EDI_CHECK(defaultLayerState->visible);

    const auto revisionBeforeMissingLayerFlags = document.revision;
    auto missingLayerFlags = updateLayerFlags(document, "missing_layer", true, false);
    EDI_CHECK(!missingLayerFlags.ok);
    EDI_CHECK(missingLayerFlags.code == DraftingResultCode::LayerNotFound);
    EDI_CHECK(document.revision == revisionBeforeMissingLayerFlags);

    selectOnly(document, "line_2");
    auto removeMiddle = removeObject(document, "line_2");
    EDI_CHECK(removeMiddle.ok);
    EDI_CHECK(document.objects.size() == 2);
    EDI_CHECK(document.objects[0].id == "line_1");
    EDI_CHECK(document.objects[1].id == "line_3");
    EDI_CHECK(objectIndexById(document, "line_3") == 1);
    EDI_CHECK(document.selectedObjectIds.empty());
    EDI_CHECK(!document.activeObjectId);

    const auto revisionAfterRemoveMiddle = document.revision;
    auto missingRemoveBeforeOrder = removeObject(document, "missing");
    EDI_CHECK(!missingRemoveBeforeOrder.ok);
    EDI_CHECK(missingRemoveBeforeOrder.code == DraftingResultCode::ObjectNotFound);
    EDI_CHECK(document.revision == revisionAfterRemoveMiddle);
    EDI_CHECK(document.objects[0].id == "line_1");
    EDI_CHECK(document.objects[1].id == "line_3");

    auto remove = removeObject(document, "line_1");
    EDI_CHECK(remove.ok);
    auto removeLastRemaining = removeObject(document, "line_3");
    EDI_CHECK(removeLastRemaining.ok);
    EDI_CHECK(document.objects.empty());

    auto missingRemove = removeObject(document, "line_1");
    EDI_CHECK(!missingRemove.ok);
    EDI_CHECK(missingRemove.code == DraftingResultCode::ObjectNotFound);

    return 0;
}
