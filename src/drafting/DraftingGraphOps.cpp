#include "drafting/DraftingGraphOps.h"

#include <algorithm>
#include <utility>
#include <vector>

namespace edi::drafting {

std::optional<std::size_t> plugIndexById(const DraftingDocument &document, const DraftingPlugId &id)
{
    for (std::size_t index = 0; index < document.plugs.size(); ++index) {
        if (document.plugs[index].id == id) {
            return index;
        }
    }
    return std::nullopt;
}

std::optional<std::size_t> connectionIndexById(const DraftingDocument &document, const DraftingConnectionId &id)
{
    for (std::size_t index = 0; index < document.connections.size(); ++index) {
        if (document.connections[index].id == id) {
            return index;
        }
    }
    return std::nullopt;
}

std::optional<std::size_t> mapRoomIndexByName(const DraftingDocument &document, const std::string &name)
{
    for (std::size_t index = 0; index < document.rooms.size(); ++index) {
        if (document.rooms[index].name == name) {
            return index;
        }
    }
    return std::nullopt;
}

DraftingStoreResult addMapRoom(DraftingDocument &document, DraftingMapRoom room)
{
    // A room is identified by its name (what plugs reference as "room.plug"), so the
    // name must be present and unique — the addPlug shape, keyed by name not id.
    if (room.name.empty()) {
        return DraftingStoreResult::rejected(DraftingResultCode::EmptyObjectId, "map room name is required");
    }
    if (mapRoomIndexByName(document, room.name)) {
        return DraftingStoreResult::rejected(DraftingResultCode::DuplicateObjectId, "map room name already exists");
    }
    document.rooms.push_back(std::move(room));
    ++document.revision;
    return DraftingStoreResult::accepted();
}

DraftingStoreResult addPlug(DraftingDocument &document, DraftingPlug plug)
{
    // A plug id is an object-style id, so reuse the same non-empty validity check.
    // (We pair a coarse result code with a specific message, as addObject does for
    // the locked-layer case — the code groups, the message pinpoints.)
    if (!isValidDraftingObjectId(plug.id)) {
        return DraftingStoreResult::rejected(DraftingResultCode::EmptyObjectId, "plug id is required");
    }
    if (plugIndexById(document, plug.id)) {
        return DraftingStoreResult::rejected(DraftingResultCode::DuplicateObjectId, "plug id already exists");
    }
    // The plug must ride on a real document object (a Point marker, a wall, …).
    // edi records the reference; it does not care WHAT kind of object it is.
    if (!containsObject(document, plug.anchorObjectId)) {
        return DraftingStoreResult::rejected(DraftingResultCode::ObjectNotFound, "plug anchor object does not exist");
    }

    document.plugs.push_back(std::move(plug));
    ++document.revision;
    return DraftingStoreResult::accepted();
}

DraftingStoreResult updatePlug(DraftingDocument &document, const DraftingPlugId &id,
                                const std::string &type)
{
    // Sibling of addPlug/removePlug: validate first, mutate second, bump revision
    // only on success — the same three-step pattern the whole store layer follows.
    // Only `type` is written; anchor/anchorObjectId/id/name/flags stay untouched
    // (the door leaf re-mint is the controller's concern, not the graph op's).
    const auto idx = plugIndexById(document, id);
    if (!idx) {
        return DraftingStoreResult::rejected(DraftingResultCode::ObjectNotFound, "plug does not exist");
    }
    document.plugs[*idx].type = type;
    ++document.revision;
    return DraftingStoreResult::accepted();
}

DraftingStoreResult removePlug(DraftingDocument &document, const DraftingPlugId &id)
{
    const auto index = plugIndexById(document, id);
    if (!index) {
        return DraftingStoreResult::rejected(DraftingResultCode::ObjectNotFound, "plug does not exist");
    }

    // Cascade: drop every connection that named this plug, so the graph never
    // holds an edge to a plug that is gone. (S4 closes the *object*-delete hole;
    // this closes the *plug*-delete one.) erase-remove over both endpoints.
    const DraftingPlugId removedId = document.plugs[*index].id;
    document.connections.erase(
        std::remove_if(document.connections.begin(), document.connections.end(),
                       [&removedId](const DraftingDeclaredConnection &connection) {
                           return connection.plugA == removedId || connection.plugB == removedId;
                       }),
        document.connections.end());

    document.plugs.erase(document.plugs.begin() + static_cast<std::ptrdiff_t>(*index));
    ++document.revision;
    return DraftingStoreResult::accepted();
}

DraftingStoreResult declareConnection(DraftingDocument &document, DraftingDeclaredConnection connection)
{
    if (!isValidDraftingObjectId(connection.id)) {
        return DraftingStoreResult::rejected(DraftingResultCode::EmptyObjectId, "connection id is required");
    }
    if (connectionIndexById(document, connection.id)) {
        return DraftingStoreResult::rejected(DraftingResultCode::DuplicateObjectId, "connection id already exists");
    }
    // Both endpoints must name plugs that exist — an edge to a missing plug is a
    // dangling declaration, which edi refuses to record. (A self-loop where
    // plugA == plugB is left to the caller: that is a neutral declaration, not an
    // error edi gets to rule on.)
    if (!plugIndexById(document, connection.plugA) || !plugIndexById(document, connection.plugB)) {
        return DraftingStoreResult::rejected(DraftingResultCode::ObjectNotFound, "connection references a plug that does not exist");
    }

    document.connections.push_back(std::move(connection));
    ++document.revision;
    return DraftingStoreResult::accepted();
}

DraftingStoreResult undeclareConnection(DraftingDocument &document, const DraftingConnectionId &id)
{
    const auto index = connectionIndexById(document, id);
    if (!index) {
        return DraftingStoreResult::rejected(DraftingResultCode::ObjectNotFound, "connection does not exist");
    }

    document.connections.erase(document.connections.begin() + static_cast<std::ptrdiff_t>(*index));
    ++document.revision;
    return DraftingStoreResult::accepted();
}

std::optional<DraftingPlugId> plugAtAnchorObject(const DraftingDocument &document,
                                                  const DraftingObjectId &objectId)
{
    // Linear scan — the plug list is short (tens to low hundreds even for a large
    // dungeon map) and this runs only on a user click, not per frame. There is at
    // most ONE plug per anchor object (the document never anchors two plugs to the
    // same marker), so we return the first match without continuing the scan.
    for (const DraftingPlug &plug : document.plugs) {
        if (plug.anchorObjectId == objectId) {
            return plug.id;
        }
    }
    return std::nullopt;
}

// NOTE(dungeon-map): this handles object DELETE (cascade plugs/connections). The
// object MOVE counterpart — re-syncing DraftingPlug::anchor when an anchoring
// object moves — does NOT exist yet; see the TODO on DraftingPlug::anchor in
// DraftingDocument.h. A syncGraphForMovedObject() sibling is the deferred fix.
bool pruneGraphForRemovedObject(DraftingDocument &document, const DraftingObjectId &removedObjectId)
{
    // Collect the ids of plugs anchored to the now-removed object.
    std::vector<DraftingPlugId> doomed;
    for (const DraftingPlug &plug : document.plugs) {
        if (plug.anchorObjectId == removedObjectId) {
            doomed.push_back(plug.id);
        }
    }
    if (doomed.empty()) {
        return false; // common case (and every pre-graph document): nothing to do.
    }

    const auto isDoomed = [&doomed](const DraftingPlugId &id) {
        return std::find(doomed.begin(), doomed.end(), id) != doomed.end();
    };
    // Drop every edge that named a doomed plug first, then the doomed plugs —
    // same two-step cascade removePlug uses, but for a whole anchor at once.
    document.connections.erase(
        std::remove_if(document.connections.begin(), document.connections.end(),
                       [&isDoomed](const DraftingDeclaredConnection &connection) {
                           return isDoomed(connection.plugA) || isDoomed(connection.plugB);
                       }),
        document.connections.end());
    document.plugs.erase(
        std::remove_if(document.plugs.begin(), document.plugs.end(),
                       [&removedObjectId](const DraftingPlug &plug) {
                           return plug.anchorObjectId == removedObjectId;
                       }),
        document.plugs.end());
    return true;
}

} // namespace edi::drafting
