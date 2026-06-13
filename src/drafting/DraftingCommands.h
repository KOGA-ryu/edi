#pragma once

#include "drafting/DraftingAlign.h"
#include "drafting/DraftingDocument.h"

#include <string>
#include <variant>

namespace edi::drafting {

struct CreateObjectCommand {
    DraftingObject object;
};

// Atomic batch create (#30 arrays): semantically a sequence of
// CreateObjectCommand, but validated as a whole BEFORE anything lands (no
// half-committed batch) and with one duplicate-id set instead of a per-insert
// document scan — a loop of single creates is O(N^2) in batch size.
struct CreateObjectsCommand {
    std::vector<DraftingObject> objects;
};

struct DeleteObjectCommand {
    DraftingObjectId objectId;
};

struct DeleteAllGuidesCommand {
};

struct MergeDuplicateGuidesCommand {
};

struct MoveObjectCommand {
    DraftingObjectId objectId;
    double dx = 0.0;
    double dy = 0.0;
};

struct MoveSelectionCommand {
    double dx = 0.0;
    double dy = 0.0;
};

struct EditObjectHandleCommand {
    DraftingObjectId objectId;
    std::string handleId;
    Point2D point;
    bool preserveAspect = false; // N4 aspect-lock for rectangle corner drags
};

struct NumericGeometryEditCommand {
    DraftingObjectId objectId;
    std::string fieldId;
    double value = 0.0;
};

struct UpdateGeometryCommand {
    DraftingObjectId objectId;
    DraftingGeometry geometry = PointGeometry{};
};

struct UpdateMetadataCommand {
    DraftingObjectId objectId;
    ObjectMetadata metadata;
};

struct UpdateObjectFlagsCommand {
    DraftingObjectId objectId;
    bool locked = false;
    bool visible = true;
};

// Per-object stroke styling (color/width/lineStyle/opacity). Inherit
// sentinels (empty color, width 0) are legal values: "go back to the
// layer's style" is itself a styling decision.
struct UpdateStrokeStyleCommand {
    DraftingObjectId objectId;
    StrokeStyle stroke;
};

struct UpdateFillStyleCommand {
    DraftingObjectId objectId;
    FillStyle fill;
};

struct SetAllGuidesVisibleCommand {
    bool visible = true;
};

struct SetAllGuidesLockedCommand {
    bool locked = false;
};

struct MoveObjectToLayerCommand {
    DraftingObjectId objectId;
    LayerId layerId;
};

struct CreateLayerCommand {
    DraftingLayer layer;
    bool makeActive = false;
};

struct RenameLayerCommand {
    LayerId layerId;
    std::string name;
};

struct SetActiveLayerCommand {
    LayerId layerId;
};

struct MoveLayerCommand {
    LayerId layerId;
    int delta = 0;
};

struct UpdateLayerFlagsCommand {
    LayerId layerId;
    bool locked = false;
    bool visible = true;
};

struct UpdateLayerPlotStyleCommand {
    LayerId layerId;
    LayerPlotStyle plot;
};

struct AlignSelectionCommand {
    DraftingAlignmentMode mode = DraftingAlignmentMode::Left;
};

struct DistributeSelectionCommand {
    DraftingAlignmentMode mode = DraftingAlignmentMode::DistributeX;
};

struct SelectObjectCommand {
    DraftingObjectId objectId;
};

struct SelectObjectsCommand {
    std::vector<DraftingObjectId> objectIds;
};

using DraftingCommand = std::variant<
    CreateObjectCommand,
    CreateObjectsCommand,
    DeleteObjectCommand,
    DeleteAllGuidesCommand,
    MergeDuplicateGuidesCommand,
    MoveObjectCommand,
    MoveSelectionCommand,
    EditObjectHandleCommand,
    NumericGeometryEditCommand,
    UpdateGeometryCommand,
    UpdateMetadataCommand,
    UpdateObjectFlagsCommand,
    UpdateStrokeStyleCommand,
    UpdateFillStyleCommand,
    SetAllGuidesVisibleCommand,
    SetAllGuidesLockedCommand,
    MoveObjectToLayerCommand,
    CreateLayerCommand,
    RenameLayerCommand,
    SetActiveLayerCommand,
    MoveLayerCommand,
    UpdateLayerFlagsCommand,
    UpdateLayerPlotStyleCommand,
    AlignSelectionCommand,
    DistributeSelectionCommand,
    SelectObjectCommand,
    SelectObjectsCommand>;

struct DraftingCommandResult {
    bool ok = false;
    DraftingResultCode code = DraftingResultCode::None;
    std::string message;

    static DraftingCommandResult accepted();
    static DraftingCommandResult rejected(DraftingResultCode code, std::string message);
};

DraftingCommandResult applyDraftingCommand(DraftingDocument &document, const DraftingCommand &command);

} // namespace edi::drafting
