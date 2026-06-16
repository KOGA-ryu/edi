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
    // TODO(dungeon-map): `anchor` is a cache computed at AUTHORING time (when the
    // plug is created from its anchorObjectId's geometry) and is NOT re-synced when
    // that anchoring object is later moved or edited. The Seam C export reads it —
    // MapToonExport.cpp deriveEdge() picks the room side nearest `anchor` — so an
    // interactive move of a plug-anchored object would DRIFT the exported edge while
    // the live geometry has moved on. The real fix is a syncGraphForMovedObject()
    // sibling to pruneGraphForRemovedObject() (recompute anchors for plugs whose
    // anchorObjectId moved). DEFERRED: that is a feature, out of the cartography
    // campaign's behavior-preserving mandate — recorded here, not built.
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

// A named room as a NEUTRAL map entity (Seam C). The document keeps walls + plugs
// + connections, but loses the room's identity (name + footprint) once authored —
// only plug-name prefixes survive. Storing the room makes the document the single
// self-describing source the engine export reads (rooms keyed by NAME, what plugs
// reference as "room.plug"). Footprint is the AUTHORED rectangle (NW corner +
// size), stored directly — it is authored data, not derived from the walls.
struct DraftingMapRoom {
    std::string name;                // unique within the map ("entrance", "room1")
    Point2D origin;                  // NW corner, authored units
    double width = 0.0;
    double height = 0.0;
    std::string material;            // neutral tag, e.g. "stone"
};

// --- Block library (Phase C: the "flash sheet") ------------------------------
// A BLOCK is a named, saved group of objects — a reusable symbol (a table, a
// door, a girih tile). It lives here as plain document-level data, beside the
// objects/plugs/connections vectors, NOT as a DraftingGeometry variant: a block
// is a stored TEMPLATE, not a shape on the canvas, so modelling it as a geometry
// kind would drag it through the whole geometry-visitor recipe for nothing.
//
// FLATTEN fork (resolved Phase-C design): an INSTANCE is not modelled here at
// all. Placing a block mints independent transformed COPIES of `objects` as
// ordinary DraftingObjects (paste-with-a-transform), so there is no instance
// struct, no back-reference, and no propagate-on-edit engine — every placed
// object stays a first-class, directly-editable shape. The block's `objects` are
// stored NORMALIZED to the origin (lower-left of their union at 0,0); `bounds` is
// the cached union extent (origin-based), recomputed on load like object bounds.
struct DraftingBlock {
    DraftingBlockId id;                  // opaque, minted like object ids ("block_0001")
    std::string name;                    // authored label, e.g. "tavern_table"
    // Seam B (the pipeline link): the Blender asset/recipe this block DEPICTS, as a
    // NEUTRAL opaque reference — edi records which asset a symbol stands for without
    // interpreting it, the way a plug records a neutral `type`. Empty for a purely
    // hand-drawn block. This is what carries an authored placement across to the
    // game engine (Seam C): a placed instance can be traced back to its asset.
    std::string assetRef;
    std::vector<DraftingObject> objects; // the saved group, normalized to the origin
    Bounds2D bounds;                     // cached union extent; derived, recomputed on load
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
