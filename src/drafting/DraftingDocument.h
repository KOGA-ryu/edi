#pragma once

#include "drafting/DraftingTypes.h"

#include <cstddef>
#include <optional>
#include <unordered_set>
#include <vector>

namespace edi::drafting {

struct DraftingObject {
    DraftingObjectId id;
    DraftingShapeKind kind = DraftingShapeKind::Point;
    DraftingGeometry geometry = PointGeometry{};
    StrokeStyle stroke;
    FillStyle fill;
    StyleId styleId;
    Transform2D transform;
    Bounds2D bounds;
    LayerId layerId = "default";
    ObjectMetadata metadata;
    bool locked = false;
    bool visible = true;
};

struct DraftingLayer {
    LayerId id = "default";
    std::string name = "Default";
    int order = 0;
    bool visible = true;
    bool locked = false;
    LayerPlotStyle plot;
};

// --- Map graph + block library -----------------------------------------------
// The document-record structs DraftingPlug, DraftingDeclaredConnection,
// DraftingMapRoom and DraftingBlock moved to drafting/DraftingMapTypes.h (HUB H2 —
// shrink the dungeon-map ↔ drafting shared-edit surface). They arrive here
// transitively via the existing #include "drafting/DraftingTypes.h" above, so no
// new include is needed; the four document vectors below are the entire map surface
// DraftingDocument.h now carries.

struct DraftingDocument {
    DraftingDocumentId id;
    std::string title;
    std::vector<DraftingLayer> layers;
    LayerId activeLayerId = "default";
    std::vector<DraftingObject> objects;
    // The map graph rides INSIDE the document (beside objects), so it inherits the
    // existing DocumentSnapshot undo/redo for free — no separate undo plumbing.
    std::vector<DraftingPlug> plugs;
    std::vector<DraftingDeclaredConnection> connections;
    // Named map rooms (Seam C): the neutral footprints the engine export needs,
    // kept beside the graph so they ride the same free undo + persistence.
    std::vector<DraftingMapRoom> rooms;
    // Block library rides inside the document too (same free undo as the graph).
    std::vector<DraftingBlock> blocks;
    std::vector<DraftingObjectId> selectedObjectIds;
    std::optional<DraftingObjectId> activeObjectId;
    std::uint64_t revision = 0;
    // Seam C: the scale a map was authored at — CANVAS units per authored unit
    // (e.g. 0.02 canvas per foot). The document stores every coordinate in canvas
    // units; the engine export divides by this to recover the authored units (feet).
    // 1.0 means coordinates ARE the authored units (a hand-drawn doc / no map scale).
    double canvasPerAuthoredUnit = 1.0;
};

struct DraftingObjectBuildResult {
    bool ok = false;
    DraftingResultCode code = DraftingResultCode::None;
    std::string message;
    DraftingObject object;

    static DraftingObjectBuildResult accepted(DraftingObject object);
    static DraftingObjectBuildResult rejected(DraftingResultCode code, std::string message);
};

DraftingDocument makeDraftingDocument(DraftingDocumentId id, std::string title = {});
DraftingObject makeDraftingObject(DraftingObjectId id, DraftingShapeKind kind, DraftingGeometry geometry);
DraftingObjectBuildResult validateDraftingObjectShape(const DraftingObject &object);
DraftingObjectBuildResult buildDraftingObject(DraftingObjectId id, DraftingShapeKind kind, DraftingGeometry geometry);
DraftingObjectId draftingObjectIdForSerial(const std::string &prefix, int serial);
// The inverse of draftingObjectIdForSerial: the highest trailing serial across
// EVERY id-bearing document vector (objects, plugs, connections, blocks). All
// document ids share one monotonic serial via distinct prefixes, so a freshly
// opened document must resume minting above every id it already holds — scanning
// objects alone let a plug_/conn_/block_ id collide with a freshly minted one.
int highestDocumentIdSerial(const DraftingDocument &document);
DraftingLayer makeDraftingLayer(LayerId id, std::string name, int order = 0);
DraftingLayer makeDefaultLayer();
std::optional<std::size_t> objectIndexById(const DraftingDocument &document, const DraftingObjectId &id);
DraftingObject *findObject(DraftingDocument &document, const DraftingObjectId &id);
const DraftingObject *findObject(const DraftingDocument &document, const DraftingObjectId &id);
const DraftingObject *activeObject(const DraftingDocument &document);
const DraftingObject *activeObjectOfKind(const DraftingDocument &document, DraftingShapeKind kind);
std::optional<std::size_t> layerIndexById(const DraftingDocument &document, const LayerId &id);
DraftingLayer *findLayer(DraftingDocument &document, const LayerId &id);
const DraftingLayer *findLayer(const DraftingDocument &document, const LayerId &id);
bool containsObject(const DraftingDocument &document, const DraftingObjectId &id);
bool containsLayer(const DraftingDocument &document, const LayerId &id);
// All object ids as a hash set: build once (O(M)), O(1) membership after.
// For code that checks MANY ids against the document (batch create,
// multi-select) — containsObject per id is a fresh linear scan each time.
std::unordered_set<DraftingObjectId> objectIdSet(const DraftingDocument &document);

} // namespace edi::drafting
