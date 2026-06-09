#pragma once

#include "drafting/DraftingAlign.h"
#include "drafting/DraftingDocument.h"

#include <string>
#include <variant>

namespace edi::drafting {

struct CreateObjectCommand {
    DraftingObject object;
};

struct DeleteObjectCommand {
    DraftingObjectId objectId;
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

struct UpdateLayerFlagsCommand {
    LayerId layerId;
    bool locked = false;
    bool visible = true;
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
    DeleteObjectCommand,
    MoveObjectCommand,
    MoveSelectionCommand,
    EditObjectHandleCommand,
    UpdateGeometryCommand,
    UpdateMetadataCommand,
    UpdateObjectFlagsCommand,
    MoveObjectToLayerCommand,
    CreateLayerCommand,
    RenameLayerCommand,
    SetActiveLayerCommand,
    UpdateLayerFlagsCommand,
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
