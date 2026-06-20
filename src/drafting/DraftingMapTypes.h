#pragma once

#include <string>
#include <vector>

namespace edi::drafting {

// Forward-declared CORE type: DraftingBlock stores std::vector<DraftingObject> by
// value. Since C++17 std::vector may be instantiated with an incomplete element
// type, so this header need not (and must not — it would cycle) include
// DraftingDocument.h. DraftingObject is completed by DraftingDocument.h before any
// DraftingBlock operation is used. (Bounds2D / Point2D are likewise provided by the
// includer — this header is intentionally NOT standalone; it is included once
// mid-DraftingTypes.h, after Point2D/Bounds2D are defined and at file scope.)
struct DraftingObject;

// Map-graph handles. Opaque strings like the object/layer ids above — a plug and
// a declared connection are identified, referenced, and validated exactly the way
// objects are, so they reuse the same alias shape rather than inventing a new id
// type. (Phase 2 / docs/dungeon-map-graph-work-order.md.)
using DraftingPlugId = std::string;
using DraftingConnectionId = std::string;
// Block-library handle (Phase C / "flash sheet"). A block DEFINITION is a named,
// saved group of objects; its id is minted and validated like an object id (one
// monotonic serial, distinct "block_" prefix), so it reuses the same alias shape.
using DraftingBlockId = std::string;
// Connector-node handle (Phase-1 / inverted model, decision 1/11). A node is a
// small labeled point minted as "node_NNNN"; the "node_" prefix is distinct from
// plug_/block_/conn_ so highestDocumentIdSerial can scan all vectors without
// ambiguity. Same opaque-string alias shape as the ids above.
using DraftingNodeId = std::string;

// N3: the legacy object "role" — a semantic tag the 3D/export pipeline reads
// (a wall extrudes, a cutout subtracts, a collider is invisible but solid).
// An enum, not a free string, because the set is closed and downstream code
// switches on it; material / export_group / tags below stay free-form
// because those vocabularies are open and user-defined.
enum class ObjectRole {
    None,
    Wall,
    Floor,
    Cutout,
    Collider
};

// M1.3 dungeon-map: a wall's NEUTRAL classification — it changes only how the
// wall DRAWS (a secret door reads flush, a window thin), never what it means.
// Whether a secret door blocks sight is the game engine's rule across Seam B;
// this enum carries no behaviour. Closed enum (the painter switches on it),
// exactly like ObjectRole above; the open vocabulary rides ObjectMetadata.tags.
enum class WallType {
    Solid,
    Door,
    Window,
    Secret
};

// M1.3: per-wall render classification. Default Solid keeps every existing wall
// byte-identical. The painter reads `type` to vary the band (Secret dashed,
// Window thin/double, Door distinct); the open vocabulary stays in tags.
struct WallVisualMetadata {
    WallType type = WallType::Solid;
};

// Seam B: provenance stamped on each object a block placement FLATTEN-stamps.
// FLATTEN keeps no LIVE link to the definition, so the asset is SNAPSHOT here — it
// survives deleting the block. `instanceId` groups the N stamped objects of ONE
// placement so the engine export (Seam C) can re-form the instance from the
// flattened geometry. An empty instanceId means an ordinary object, not a placement.
struct BlockPlacementMetadata {
    DraftingBlockId blockId;   // the definition this object was stamped from
    std::string assetRef;      // snapshot of the block's asset (what Seam C emits)
    std::string instanceId;    // groups the objects of one placement
    // Neutral per-instance placement transform record (Seam C reads these). Identity
    // by default — DM-14 will write real values via transformGeometry; this slice is
    // identity-valued plumbing, so every existing placement stays byte-identical.
    double rotationDeg = 0.0;  // placement rotation, degrees CCW; 0 = identity
    double scale = 1.0;        // uniform placement scale; 1 = identity
};

const char *objectRoleName(ObjectRole role);
// Inverse of objectRoleName; unknown names fall back to None.
ObjectRole objectRoleFromName(const std::string &name);
const char *wallTypeName(WallType type);
// Inverse of wallTypeName; unknown names fall back to Solid.
WallType wallTypeFromName(const std::string &name);

// How a room's footprint is determined. Placed = classic room-as-rectangle (NW
// origin + size, today's only model); SpanDerived = the inverted model where the
// footprint is DERIVED from bounding connector nodes (Phase 2+). Closed enum so
// the painter and exporter switch on it; the open vocabulary stays in neutral tags.
// Default Placed keeps every existing room byte-identical — COEXIST (decision 1).
enum class RoomDerivation { Placed, SpanDerived };
const char *roomDerivationName(RoomDerivation d);          // "placed" / "span_derived"
// Inverse of roomDerivationName; unknown names fall back to Placed (safe default).
RoomDerivation roomDerivationFromName(const std::string &name);

// A room's enclosure kind — drives DRAWING only (which perimeter segments
// planDraftingRoom emits), mirroring how WallType varies the band visual.
// NOT a game rule: whether an "open" room blocks movement is the engine's call.
// Closed enum, default Enclosed so every existing room draws identically.
// (Phase 2 will make planDraftingRoom honour kind and per-edge walls; this slice
// adds the type + field only, with no drawing-logic change — data-spine first.)
enum class RoomKind { Enclosed, Open };
const char *roomKindName(RoomKind k);            // "enclosed" / "open"
// Inverse of roomKindName; unknown strings fall back to Enclosed (safe default).
RoomKind roomKindFromName(const std::string &name);

// What the parser / controller should do when two room footprints overlap.
// PickOne = keep the first (the current hard-reject becomes a pick, decision 12);
// Merge = combine into a union footprint (Phase 3); Allow = record both as-is.
// Default PickOne so every existing map is byte-identical (missing ⇒ PickOne).
// Closed enum: the resolver switches on it; the open vocabulary stays in tags.
enum class OverlapPolicy { PickOne, Merge, Allow };
const char *overlapPolicyName(OverlapPolicy p);          // "pick_one"/"merge"/"allow"
// Inverse of overlapPolicyName; unknown strings fall back to PickOne (safe default).
OverlapPolicy overlapPolicyFromName(const std::string &name);

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
    std::vector<std::string> flags;  // neutral OPEN vocabulary of string tags — edi
                                     // RECORDS them, assigns NO rule meaning (mandate):
                                     // no passable/weight/direction interpretation here.
    Point2D anchor;                  // cached gap-center, so draw/export need not re-derive
    // `anchor` is seeded at plug-creation time and kept live by syncGraphForMovedObject()
    // (DraftingGraphOps.h), which DrawingDocumentController::applyCommandAndEmit() calls
    // after every geometry-mutation command.  deriveEdge() (MapToonExport.cpp) reads
    // this field, so a moved plug anchor now exports the correct wall-edge.
    // Ratified as Phase-1 decision 7 / brief 052.
    // Discrete elevation band (Phase-1 decision 8/10). 0 = ground; same additive
    // template as DraftingMapRoom::level (slice 3a). Every existing plug is level 0.
    // NOT on the TOON wire this slice — Phase-2 "level + manifest" item.
    int level = 0;
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
    // Discrete elevation band (Phase-1 decision 8/10). 0 = ground; same additive
    // template as DraftingMapRoom::level and DraftingPlug::level (slice 3a/3f).
    // A future vertical-transition connection will carry differing endpoint levels
    // (NOT modelled this slice — default 0 keeps all existing connections identical).
    // NOT on the TOON wire this slice — Phase-2 "level + manifest" item.
    int level = 0;
};

// --- Connector node (Phase-1 decision 1/11: the inverted model) ----------------
// A node is a SMALL LABELED POINT placed by the author at a corridor junction or
// room corner. Spans between adjacent nodes become rooms in the SpanDerived model
// (Phase 2). A node is document-level data — NOT a DraftingGeometry variant —
// for the same reason DraftingPlug is not a shape: it is a relation / anchor point,
// not a renderable object. It rides the same free undo + persistence as plugs/blocks.
//
// Default `radius` is a NAMED constant so no magic literal appears in the struct
// definition (no-hardcoded-dims rule). It represents the visual footprint of the
// node on the canvas — roughly a half-foot circle on a 5-foot-grid dungeon map,
// large enough to hit-test but small enough not to obscure nearby walls.
inline constexpr double kDefaultNodeRadius = 0.5; // canvas units (= 0.5 feet at scale 1:1)

struct DraftingNode {
    DraftingNodeId id;                      // opaque, minted ("node_0001")
    Point2D anchor;                         // connector point, canvas units
    double radius = kDefaultNodeRadius;     // authored visual footprint (DATA, not a magic literal)
    std::string type;                       // neutral open vocab: "junction"/"anchor"/...
    std::string name;                       // authored label
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
    // Discrete elevation band — 0 = ground level. edi owns NO Z coordinate; this
    // is a NEUTRAL INTEGER the realizer interprets (Phase-1 decision 8). Default
    // keeps every existing room identical (missing ⇒ 0 on decode). NOT on the
    // TOON wire yet — Phase-2 wire extension (brief 055: struct + .edidraw only).
    int level = 0;
    // How the footprint is determined (Phase-1 decision 1: COEXIST). Placed = the
    // classic NW-origin + size rectangle (every existing room); SpanDerived = the
    // inverted model where the footprint is derived from connector nodes (Phase 2+).
    // Default Placed keeps existing documents byte-identical (missing ⇒ Placed).
    RoomDerivation derivation = RoomDerivation::Placed;
    // The connector nodes that BOUND this room's footprint — non-empty for
    // SpanDerived rooms only.  Stored by node ID so the wire can resolve IDs to
    // names (the engine always reads by name; IDs are an internal opaque handle).
    // Empty for every Placed room, so the MessagePack key is omitted conditionally
    // (like plug.flags) to keep all Placed-room documents byte-identical.
    std::vector<DraftingNodeId> boundedBy;
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

} // namespace edi::drafting
