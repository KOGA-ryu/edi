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

// Removes every ConstructionLine-kind object from the document in one
// undoable step — the "clear the scaffolding" action.  Mirrors
// DeleteAllGuidesCommand exactly; commands are transient (not serialised),
// so adding a new arm here has no persistent-format impact.
struct DeleteAllConstructionLinesCommand {
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

// Map-graph commands (S2). Each carries the WHOLE pre-minted record — the same
// shape as CreateObjectCommand{DraftingObject} — so the arm is a one-line
// delegate to the matching DraftingGraphOps free function. The id is already on
// the record (decision #4 of the work order: the caller mints, the op validates),
// exactly as a DraftingObject arrives at CreateObjectCommand already carrying its
// id; the deletes name a record by id like DeleteObjectCommand.
struct CreatePlugCommand {
    DraftingPlug plug;
};

struct DeletePlugCommand {
    DraftingPlugId plugId;
};

struct DeclareConnectionCommand {
    DraftingDeclaredConnection connection;
};

struct DeleteConnectionCommand {
    DraftingConnectionId connectionId;
};

// Block-library commands (Phase C). Same shape as the map-graph commands above:
// CreateBlockCommand carries the WHOLE pre-built definition (the caller mints the
// id and normalizes the objects; the op only validates), DeleteBlockCommand names
// one by id — so each arm is a one-line delegate to a DraftingBlockOps free
// function, riding DocumentSnapshot undo for free.
struct CreateBlockCommand {
    DraftingBlock block;
};

struct DeleteBlockCommand {
    DraftingBlockId blockId;
};

// Seam C: record a named map room. Same shape as CreatePlugCommand — the caller
// builds the whole record, the arm delegates to addMapRoom (validate + append).
struct CreateMapRoomCommand {
    DraftingMapRoom room;
};

using DraftingCommand = std::variant<
    CreateObjectCommand,
    CreateObjectsCommand,
    DeleteObjectCommand,
    DeleteAllGuidesCommand,
    DeleteAllConstructionLinesCommand,
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
    SelectObjectsCommand,
    CreatePlugCommand,
    DeletePlugCommand,
    DeclareConnectionCommand,
    DeleteConnectionCommand,
    CreateBlockCommand,
    DeleteBlockCommand,
    CreateMapRoomCommand>;

struct DraftingCommandResult {
    bool ok = false;
    DraftingResultCode code = DraftingResultCode::None;
    std::string message;

    static DraftingCommandResult accepted();
    static DraftingCommandResult rejected(DraftingResultCode code, std::string message);
};

DraftingCommandResult applyDraftingCommand(DraftingDocument &document, const DraftingCommand &command);

} // namespace edi::drafting
