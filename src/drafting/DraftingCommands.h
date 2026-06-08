#pragma once

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

struct UpdateGeometryCommand {
    DraftingObjectId objectId;
    DraftingGeometry geometry = PointGeometry{};
};

struct UpdateMetadataCommand {
    DraftingObjectId objectId;
    ObjectMetadata metadata;
};

struct SelectObjectCommand {
    DraftingObjectId objectId;
};

using DraftingCommand = std::variant<
    CreateObjectCommand,
    DeleteObjectCommand,
    MoveObjectCommand,
    UpdateGeometryCommand,
    UpdateMetadataCommand,
    SelectObjectCommand>;

struct DraftingCommandResult {
    bool ok = false;
    DraftingResultCode code = DraftingResultCode::None;
    std::string message;
};

DraftingCommandResult applyDraftingCommand(DraftingDocument &document, const DraftingCommand &command);

} // namespace edi::drafting
