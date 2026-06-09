#include "drafting/DraftingStore.h"

#include "drafting/DraftingDocument.h"
#include "drafting/DraftingGeometry.h"
#include "drafting/DraftingSelection.h"

#include <cassert>
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
    assert(document.title == "doc");
    DraftingDocument explicitTitle = makeDraftingDocument("doc_with_title", "Plan A");
    assert(explicitTitle.title == "Plan A");
    DraftingDocument emptyDraftingDocument = makeDraftingDocument("");
    assert(emptyDraftingDocument.id.empty());
    assert(emptyDraftingDocument.title.empty());
    assert(isValidDraftingDocumentId("doc"));
    assert(!isValidDraftingDocumentId(""));
    assert(isValidDraftingDocumentTitle("Plan"));
    assert(!isValidDraftingDocumentTitle(""));
    assert(isValidDraftingObjectId("line_1"));
    assert(!isValidDraftingObjectId(""));
    assert(isValidLayerId("default"));
    assert(!isValidLayerId(""));
    assert(isValidLayerName("Default"));
    assert(!isValidLayerName(""));
    DraftingLayer defaultLayer = makeDefaultLayer();
    assert(defaultLayer.id == "default");
    assert(defaultLayer.name == "Default");
    assert(defaultLayer.order == 0);
    DraftingLayer explicitLayer = makeDraftingLayer("overlay", "Overlay", 2);
    assert(explicitLayer.id == "overlay");
    assert(explicitLayer.name == "Overlay");
    assert(explicitLayer.order == 2);
    DraftingLayer fallbackLayerName = makeDraftingLayer("measurements", "", 3);
    assert(fallbackLayerName.name == "measurements");
    assert(fallbackLayerName.order == 3);
    DraftingLayer emptyLayer = makeDraftingLayer("", "", 4);
    assert(emptyLayer.id.empty());
    assert(emptyLayer.name.empty());
    assert(emptyLayer.order == 4);
    DraftingObject helperObject = makeDraftingObject("helper_line", DraftingShapeKind::Line, LineGeometry{{1.0, 2.0}, {3.0, 4.0}});
    assert(helperObject.id == "helper_line");
    assert(helperObject.kind == DraftingShapeKind::Line);
    assert(helperObject.layerId == "default");
    assert(helperObject.bounds.width == 0.0);
    auto builtLine = buildDraftingObject("built_line", DraftingShapeKind::Line, LineGeometry{{0.0, 0.0}, {5.0, 0.0}});
    assert(builtLine.ok);
    assert(builtLine.code == DraftingResultCode::None);
    assert(builtLine.object.id == "built_line");
    assert(builtLine.object.kind == DraftingShapeKind::Line);
    assert(builtLine.object.bounds.width == 0.0);
    auto emptyBuildId = buildDraftingObject("", DraftingShapeKind::Line, LineGeometry{{0.0, 0.0}, {5.0, 0.0}});
    assert(!emptyBuildId.ok);
    assert(emptyBuildId.code == DraftingResultCode::EmptyObjectId);
    auto mismatchedBuild = buildDraftingObject("bad_kind_build", DraftingShapeKind::Line, PointGeometry{{0.0, 0.0}});
    assert(!mismatchedBuild.ok);
    assert(mismatchedBuild.code == DraftingResultCode::KindGeometryMismatch);
    auto invalidGeometryBuild = buildDraftingObject("bad_geometry_build", DraftingShapeKind::Circle, CircleGeometry{{0.0, 0.0}, -1.0});
    assert(!invalidGeometryBuild.ok);
    assert(invalidGeometryBuild.code == DraftingResultCode::InvalidGeometry);
    auto helperObjectValidation = validateDraftingObjectShape(helperObject);
    assert(helperObjectValidation.ok);
    assert(layerIndexById(document, "default") == 0);
    assert(findLayer(document, "default") == &document.layers[0]);
    assert(containsLayer(document, "default"));
    assert(layerIndexById(document, "missing_layer") == std::nullopt);
    assert(findLayer(document, "missing_layer") == nullptr);
    assert(!containsLayer(document, "missing_layer"));

    DraftingDocument layerDocument = makeDraftingDocument("layer_doc");
    assert(layerDocument.activeLayerId == "default");
    const auto layerRevisionBeforeAdd = layerDocument.revision;
    auto addLayerResult = addLayer(layerDocument, makeDraftingLayer("sketch", "Sketch", 1), true);
    assert(addLayerResult.ok);
    assert(layerDocument.layers.size() == 2);
    assert(layerDocument.activeLayerId == "sketch");
    assert(layerDocument.revision == layerRevisionBeforeAdd + 1);
    auto duplicateLayer = addLayer(layerDocument, makeDraftingLayer("sketch", "Duplicate", 2));
    assert(!duplicateLayer.ok);
    assert(duplicateLayer.code == DraftingResultCode::DuplicateLayerId);
    assert(layerDocument.layers.size() == 2);
    auto renameSketch = renameLayer(layerDocument, "sketch", "Sketch Layer");
    assert(renameSketch.ok);
    const DraftingLayer *sketchLayer = findLayer(layerDocument, "sketch");
    assert(sketchLayer != nullptr);
    assert(sketchLayer->name == "Sketch Layer");
    auto missingActiveLayer = setActiveLayer(layerDocument, "missing_layer");
    assert(!missingActiveLayer.ok);
    assert(missingActiveLayer.code == DraftingResultCode::LayerNotFound);
    auto setDefaultActive = setActiveLayer(layerDocument, "default");
    assert(setDefaultActive.ok);
    assert(layerDocument.activeLayerId == "default");

    auto layerObjectAdd = addObject(layerDocument, makeLine("layer_line"));
    assert(layerObjectAdd.ok);
    auto lockSketchLayer = updateLayerFlags(layerDocument, "sketch", true, true);
    assert(lockSketchLayer.ok);
    auto moveToLockedLayer = moveObjectToLayer(layerDocument, "layer_line", "sketch");
    assert(!moveToLockedLayer.ok);
    assert(moveToLockedLayer.code == DraftingResultCode::InvalidSelectionTarget);
    auto unlockSketchLayer = updateLayerFlags(layerDocument, "sketch", false, true);
    assert(unlockSketchLayer.ok);
    auto moveToSketchLayer = moveObjectToLayer(layerDocument, "layer_line", "sketch");
    assert(moveToSketchLayer.ok);
    const DraftingObject *layerLine = findObject(layerDocument, "layer_line");
    assert(layerLine != nullptr);
    assert(layerLine->layerId == "sketch");
    lockSketchLayer = updateLayerFlags(layerDocument, "sketch", true, true);
    assert(lockSketchLayer.ok);
    auto moveFromLockedLayer = moveObjectToLayer(layerDocument, "layer_line", "default");
    assert(!moveFromLockedLayer.ok);
    assert(moveFromLockedLayer.code == DraftingResultCode::InvalidSelectionTarget);

    auto add = addObject(document, makeLine("line_1"));
    assert(add.ok);
    assert(document.objects.size() == 1);
    assert(document.objects.front().bounds.width == 10.0);
    assert(objectIndexById(document, "line_1") == 0);

    auto addSecond = addObject(document, makeLine("line_2"));
    assert(addSecond.ok);
    auto addThird = addObject(document, makeLine("line_3"));
    assert(addThird.ok);
    assert(document.objects.size() == 3);
    assert(document.objects[0].id == "line_1");
    assert(document.objects[1].id == "line_2");
    assert(document.objects[2].id == "line_3");
    assert(objectIndexById(document, "line_2") == 1);
    assert(findObject(document, "line_2") == &document.objects[1]);
    assert(containsObject(document, "line_2"));
    assert(objectIndexById(document, "missing") == std::nullopt);
    assert(findObject(document, "missing") == nullptr);
    assert(!containsObject(document, "missing"));

    auto duplicate = addObject(document, makeLine("line_1"));
    assert(!duplicate.ok);
    assert(duplicate.code == DraftingResultCode::DuplicateObjectId);
    assert(document.objects.size() == 3);
    assert(document.objects[0].id == "line_1");
    assert(document.objects[1].id == "line_2");
    assert(document.objects[2].id == "line_3");
    const auto revisionAfterDuplicate = document.revision;

    DraftingObject emptyId = makeLine("");
    auto emptyIdResult = addObject(document, emptyId);
    assert(!emptyIdResult.ok);
    assert(emptyIdResult.code == DraftingResultCode::EmptyObjectId);
    assert(document.revision == revisionAfterDuplicate);

    DraftingObject invalidCircle;
    invalidCircle.id = "bad_circle";
    invalidCircle.kind = DraftingShapeKind::Circle;
    invalidCircle.geometry = CircleGeometry{{0.0, 0.0}, -4.0};
    auto invalidCircleResult = addObject(document, invalidCircle);
    assert(!invalidCircleResult.ok);
    assert(invalidCircleResult.code == DraftingResultCode::InvalidGeometry);
    assert(invalidCircleResult.code == invalidGeometryBuild.code);
    assert(document.revision == revisionAfterDuplicate);

    DraftingObject badKind;
    badKind.id = "bad_kind";
    badKind.kind = DraftingShapeKind::Line;
    badKind.geometry = PointGeometry{{0.0, 0.0}};
    auto badKindResult = addObject(document, badKind);
    assert(!badKindResult.ok);
    assert(badKindResult.code == DraftingResultCode::KindGeometryMismatch);
    assert(badKindResult.code == mismatchedBuild.code);
    assert(document.revision == revisionAfterDuplicate);

    DraftingObject missingLayer = makeLine("missing_layer_object");
    missingLayer.layerId = "missing_layer";
    auto missingLayerResult = addObject(document, missingLayer);
    assert(!missingLayerResult.ok);
    assert(missingLayerResult.code == DraftingResultCode::LayerNotFound);
    assert(document.revision == revisionAfterDuplicate);
    assert(objectIndexById(document, "missing_layer_object") == std::nullopt);

    auto move = moveObject(document, "line_1", 2.0, 3.0);
    assert(move.ok);
    const auto *line = findObject(document, "line_1");
    assert(line != nullptr);
    assert(line->bounds.x == 2.0);
    assert(line->bounds.y == 3.0);
    const auto revisionAfterMove = document.revision;
    DraftingObject movedCandidate = *line;
    movedCandidate.geometry = translateGeometry(movedCandidate.geometry, 1.0, 1.0);
    auto movedCandidateShape = validateDraftingObjectShape(movedCandidate);
    assert(movedCandidateShape.ok);
    auto secondMove = moveObject(document, "line_1", 1.0, 1.0);
    assert(secondMove.ok);
    const auto *secondMovedLine = findObject(document, "line_1");
    assert(secondMovedLine != nullptr);
    assert(secondMovedLine->bounds.x == 3.0);
    assert(secondMovedLine->bounds.y == 4.0);
    assert(document.revision == revisionAfterMove + 1);
    auto updateLine = updateObjectGeometry(document, "line_1", LineGeometry{{10.0, 10.0}, {20.0, 10.0}});
    assert(updateLine.ok);
    const auto *updatedLine = findObject(document, "line_1");
    assert(updatedLine != nullptr);
    const Bounds2D boundsBeforeFailedUpdate = updatedLine->bounds;
    const DraftingGeometry geometryBeforeFailedUpdate = updatedLine->geometry;
    const auto revisionBeforeFailedUpdate = document.revision;

    DraftingObject invalidLineCandidate = *updatedLine;
    invalidLineCandidate.geometry = LineGeometry{{0.0, 0.0}, {std::numeric_limits<double>::quiet_NaN(), 0.0}};
    auto invalidLineShape = validateDraftingObjectShape(invalidLineCandidate);
    auto invalidUpdate = updateObjectGeometry(document, "line_1", invalidLineCandidate.geometry);
    assert(!invalidLineShape.ok);
    assert(!invalidUpdate.ok);
    assert(invalidUpdate.code == invalidLineShape.code);
    assert(document.revision == revisionBeforeFailedUpdate);
    const auto *afterInvalidUpdate = findObject(document, "line_1");
    assert(afterInvalidUpdate != nullptr);
    assert(afterInvalidUpdate->bounds.x == boundsBeforeFailedUpdate.x);
    assert(afterInvalidUpdate->bounds.y == boundsBeforeFailedUpdate.y);
    assert(afterInvalidUpdate->bounds.width == boundsBeforeFailedUpdate.width);
    assert(std::get<LineGeometry>(afterInvalidUpdate->geometry).a.x == std::get<LineGeometry>(geometryBeforeFailedUpdate).a.x);

    DraftingObject mismatchedUpdateCandidate = *updatedLine;
    mismatchedUpdateCandidate.geometry = PointGeometry{{0.0, 0.0}};
    auto mismatchedUpdateShape = validateDraftingObjectShape(mismatchedUpdateCandidate);
    auto mismatchedUpdate = updateObjectGeometry(document, "line_1", mismatchedUpdateCandidate.geometry);
    assert(!mismatchedUpdateShape.ok);
    assert(!mismatchedUpdate.ok);
    assert(mismatchedUpdate.code == mismatchedUpdateShape.code);
    assert(document.revision == revisionBeforeFailedUpdate);

    const Bounds2D boundsBeforeFailedMove = afterInvalidUpdate->bounds;
    const DraftingGeometry geometryBeforeFailedMove = afterInvalidUpdate->geometry;
    auto badMove = moveObject(document, "line_1", std::numeric_limits<double>::quiet_NaN(), 0.0);
    assert(!badMove.ok);
    assert(badMove.code == DraftingResultCode::InvalidGeometry);
    assert(document.revision == revisionBeforeFailedUpdate);
    const auto *afterBadMove = findObject(document, "line_1");
    assert(afterBadMove != nullptr);
    assert(afterBadMove->bounds.x == boundsBeforeFailedMove.x);
    assert(afterBadMove->bounds.y == boundsBeforeFailedMove.y);
    assert(afterBadMove->bounds.width == boundsBeforeFailedMove.width);
    assert(std::get<LineGeometry>(afterBadMove->geometry).a.x == std::get<LineGeometry>(geometryBeforeFailedMove).a.x);

    ObjectMetadata metadata;
    metadata.author = "tester";
    metadata.createdAt = "2026-06-08T12:30:00Z";
    metadata.toolProvenance = "drafting_store_tests";
    const Bounds2D boundsBeforeMetadata = afterBadMove->bounds;
    const DraftingGeometry geometryBeforeMetadata = afterBadMove->geometry;
    const auto revisionBeforeMetadata = document.revision;
    auto metadataUpdate = updateObjectMetadata(document, "line_1", metadata);
    assert(metadataUpdate.ok);
    const auto *afterMetadata = findObject(document, "line_1");
    assert(afterMetadata != nullptr);
    assert(afterMetadata->metadata.author == "tester");
    assert(afterMetadata->metadata.toolProvenance == "drafting_store_tests");
    assert(afterMetadata->bounds.x == boundsBeforeMetadata.x);
    assert(afterMetadata->bounds.y == boundsBeforeMetadata.y);
    assert(afterMetadata->bounds.width == boundsBeforeMetadata.width);
    assert(std::get<LineGeometry>(afterMetadata->geometry).a.x == std::get<LineGeometry>(geometryBeforeMetadata).a.x);
    assert(document.revision == revisionBeforeMetadata + 1);

    ObjectMetadata badTimestampMetadata = metadata;
    badTimestampMetadata.createdAt = "June 8";

    const auto revisionBeforeBadMetadata = document.revision;
    const Bounds2D boundsBeforeBadMetadata = afterMetadata->bounds;
    auto rejectedMetadataUpdate = updateObjectMetadata(document, "line_1", badTimestampMetadata);
    assert(!rejectedMetadataUpdate.ok);
    assert(rejectedMetadataUpdate.code == DraftingResultCode::InvalidMetadata);
    const auto *afterRejectedMetadata = findObject(document, "line_1");
    assert(afterRejectedMetadata != nullptr);
    assert(afterRejectedMetadata->metadata.createdAt == "2026-06-08T12:30:00Z");
    assert(afterRejectedMetadata->bounds.x == boundsBeforeBadMetadata.x);
    assert(afterRejectedMetadata->bounds.y == boundsBeforeBadMetadata.y);
    assert(document.revision == revisionBeforeBadMetadata);

    const auto revisionBeforeMissingMetadata = document.revision;
    auto missingMetadataUpdate = updateObjectMetadata(document, "missing", metadata);
    assert(!missingMetadataUpdate.ok);
    assert(missingMetadataUpdate.code == DraftingResultCode::ObjectNotFound);
    assert(document.revision == revisionBeforeMissingMetadata);

    const auto revisionBeforeFlags = document.revision;
    auto flagUpdate = updateObjectFlags(document, "line_1", true, false);
    assert(flagUpdate.ok);
    const auto *flaggedLine = findObject(document, "line_1");
    assert(flaggedLine != nullptr);
    assert(flaggedLine->locked);
    assert(!flaggedLine->visible);
    assert(document.revision == revisionBeforeFlags + 1);

    const auto revisionBeforeLockedMove = document.revision;
    auto lockedMove = moveObject(document, "line_1", 1.0, 0.0);
    assert(!lockedMove.ok);
    assert(lockedMove.code == DraftingResultCode::InvalidSelectionTarget);
    assert(document.revision == revisionBeforeLockedMove);
    auto lockedGeometryUpdate = updateObjectGeometry(document, "line_1", LineGeometry{{0.0, 0.0}, {2.0, 0.0}});
    assert(!lockedGeometryUpdate.ok);
    assert(lockedGeometryUpdate.code == DraftingResultCode::InvalidSelectionTarget);
    assert(document.revision == revisionBeforeLockedMove);

    auto unlockLine = updateObjectFlags(document, "line_1", false, true);
    assert(unlockLine.ok);
    flaggedLine = findObject(document, "line_1");
    assert(flaggedLine != nullptr);
    assert(!flaggedLine->locked);
    assert(flaggedLine->visible);

    const auto revisionBeforeMissingFlags = document.revision;
    auto missingFlags = updateObjectFlags(document, "missing", true, false);
    assert(!missingFlags.ok);
    assert(missingFlags.code == DraftingResultCode::ObjectNotFound);
    assert(document.revision == revisionBeforeMissingFlags);

    const auto revisionBeforeLayerFlags = document.revision;
    auto layerFlagUpdate = updateLayerFlags(document, "default", true, false);
    assert(layerFlagUpdate.ok);
    const DraftingLayer *defaultLayerState = findLayer(document, "default");
    assert(defaultLayerState != nullptr);
    assert(defaultLayerState->locked);
    assert(!defaultLayerState->visible);
    assert(document.revision == revisionBeforeLayerFlags + 1);

    const auto revisionBeforeLockedLayerMutation = document.revision;
    auto lockedLayerAdd = addObject(document, makeLine("locked_layer_line"));
    assert(!lockedLayerAdd.ok);
    assert(lockedLayerAdd.code == DraftingResultCode::InvalidSelectionTarget);
    auto lockedLayerObjectFlags = updateObjectFlags(document, "line_1", true, true);
    assert(!lockedLayerObjectFlags.ok);
    assert(lockedLayerObjectFlags.code == DraftingResultCode::InvalidSelectionTarget);
    auto lockedLayerMove = moveObject(document, "line_1", 1.0, 0.0);
    assert(!lockedLayerMove.ok);
    assert(lockedLayerMove.code == DraftingResultCode::InvalidSelectionTarget);
    auto lockedLayerGeometry = updateObjectGeometry(document, "line_1", LineGeometry{{0.0, 0.0}, {2.0, 0.0}});
    assert(!lockedLayerGeometry.ok);
    assert(lockedLayerGeometry.code == DraftingResultCode::InvalidSelectionTarget);
    assert(document.revision == revisionBeforeLockedLayerMutation);

    auto unlockLayer = updateLayerFlags(document, "default", false, true);
    assert(unlockLayer.ok);
    defaultLayerState = findLayer(document, "default");
    assert(defaultLayerState != nullptr);
    assert(!defaultLayerState->locked);
    assert(defaultLayerState->visible);

    const auto revisionBeforeMissingLayerFlags = document.revision;
    auto missingLayerFlags = updateLayerFlags(document, "missing_layer", true, false);
    assert(!missingLayerFlags.ok);
    assert(missingLayerFlags.code == DraftingResultCode::LayerNotFound);
    assert(document.revision == revisionBeforeMissingLayerFlags);

    selectOnly(document, "line_2");
    auto removeMiddle = removeObject(document, "line_2");
    assert(removeMiddle.ok);
    assert(document.objects.size() == 2);
    assert(document.objects[0].id == "line_1");
    assert(document.objects[1].id == "line_3");
    assert(objectIndexById(document, "line_3") == 1);
    assert(document.selectedObjectIds.empty());
    assert(!document.activeObjectId);

    const auto revisionAfterRemoveMiddle = document.revision;
    auto missingRemoveBeforeOrder = removeObject(document, "missing");
    assert(!missingRemoveBeforeOrder.ok);
    assert(missingRemoveBeforeOrder.code == DraftingResultCode::ObjectNotFound);
    assert(document.revision == revisionAfterRemoveMiddle);
    assert(document.objects[0].id == "line_1");
    assert(document.objects[1].id == "line_3");

    auto remove = removeObject(document, "line_1");
    assert(remove.ok);
    auto removeLastRemaining = removeObject(document, "line_3");
    assert(removeLastRemaining.ok);
    assert(document.objects.empty());

    auto missingRemove = removeObject(document, "line_1");
    assert(!missingRemove.ok);
    assert(missingRemove.code == DraftingResultCode::ObjectNotFound);

    return 0;
}
