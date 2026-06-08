#include "drafting/DraftingCommands.h"

#include "drafting/DraftingSelection.h"
#include "drafting/DraftingStore.h"

namespace edi::drafting {

namespace {

DraftingCommandResult fromStoreResult(const DraftingStoreResult &result)
{
    return {result.ok, result.message};
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
        } else if constexpr (std::is_same_v<Command, UpdateGeometryCommand>) {
            return fromStoreResult(updateObjectGeometry(document, typedCommand.objectId, typedCommand.geometry));
        } else if constexpr (std::is_same_v<Command, UpdateMetadataCommand>) {
            return fromStoreResult(updateObjectMetadata(document, typedCommand.objectId, typedCommand.metadata));
        } else {
            selectOnly(document, typedCommand.objectId);
            ++document.revision;
            return {true, {}};
        }
    }, command);
}

} // namespace edi::drafting
