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

// --- Map graph (Phase 2: plugs + declared connections) -----------------------
// A plug is a NAMED, NEUTRAL attachment point — a door / portal / threshold
// socket. It is not geometry of its own: it rides on an existing document object
// (a Point marker at an opening's gap-center) BY ID, the way a label rides on the
// thing it names. So a plug lives here as plain document-level data, not as a
// DraftingGeometry variant — it is a relation, not a shape, and modelling it as a
// shape would drag it through the whole geometry-visitor recipe for nothing.
//
// edi only RECORDS the plug (where it sits, what it is called, what KIND of thing
// it is as a free-form tag). It assigns NO meaning: whether a "secret" door blocks
// sight is a rule the game engine owns past Seam B.
struct DraftingPlug {
    DraftingPlugId id;               // opaque, minted like object ids ("plug_0001")
    DraftingObjectId anchorObjectId; // the doc object (a Point marker) this plug rides on
    std::string name;                // authored label, e.g. "north_doorway"
    std::string type;                // neutral open vocabulary: "door"/"portal"/...
    Point2D anchor;                  // cached gap-center, so draw/export need not re-derive
};

// A declared connection is a NEUTRAL edge: "plug A links to plug B". It references
// plugs BY ID only (never raw coordinates), so moving or renaming a plug never
// orphans the edge. It is a DECLARATION, nothing more — there is deliberately no
// `passable`, no `weight`, no `direction`, no `locked`: reachability and access
// rules are the engine's job, and that absence is the design, not an omission.
struct DraftingDeclaredConnection {
    DraftingConnectionId id;         // opaque, minted ("conn_0001")
    DraftingPlugId plugA;
    DraftingPlugId plugB;
    std::string type;                // neutral role tag, default empty ("corridor"/...)
};

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
    std::vector<DraftingObjectId> selectedObjectIds;
    std::optional<DraftingObjectId> activeObjectId;
    std::uint64_t revision = 0;
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
