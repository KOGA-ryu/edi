#pragma once

#include "drafting/DraftingCanvasDims.h"
#include "drafting/DraftingDocument.h"

#include <functional>
#include <string>
#include <utility>
#include <vector>

namespace edi::drafting {

// The four edges of a rectangular room, named by compass corner. The edges run
// CLOCKWISE — N(NW->NE) · E(NE->SE) · S(SE->SW) · W(SW->NW) — so an opening's
// `center` (below) is always measured from the edge's first-named corner.
enum class RoomEdge {
    North,
    East,
    South,
    West
};

// An opening hosted in one edge, given as an INTERVAL along that edge: `center`
// is the distance from the edge's start corner and `width` the gap size (canvas
// units). The wall owns the gap (cleaner than two free endpoints — see the
// Seam-A discussion). `type` is a NEUTRAL tag the drafting layer does not
// interpret — door / corridor / secret / window — leaving the rule (does a
// secret door block sight?) to the game engine downstream. v1 realises an
// opening as a literal GAP in the band; the door leaf / secret marker is M2.2.
struct RoomOpening {
    RoomEdge edge = RoomEdge::North;
    double center = 0.0;
    double width = 0.0;
    std::string type;
};

// A plug authored on an edge: a NEUTRAL named attachment point at a single offset
// along the edge (like an opening, but a point — no width). `at` is the distance
// from the edge's start corner. The drafting layer realises it as a Point marker;
// the map graph's plug record (DraftingPlug) then anchors to that marker. `type`
// is a neutral tag (door/portal/threshold/…) edi does not interpret.
struct RoomPlugSpec {
    RoomEdge edge = RoomEdge::North;
    double at = 0.0;
    std::string name;
    std::string type;
    std::vector<std::string> flags; // neutral open-vocabulary tags (mandate: no meaning)
};

// A declared connection authored between two plugs, named by their plug `name`s
// (the controller resolves names to the plug ids it minted). NEUTRAL — it records
// that two plugs are joined, nothing about whether the join is passable. `type` is
// a free tag (corridor/portal/…). Authored at map level because a connection can
// span rooms; in a single-room file both ends are that room's plugs.
struct RoomConnectionSpec {
    std::string from;
    std::string to;
    std::string type;
};

// An INTERIOR point feature: a neutral marker placed freely inside the room, the
// complement to the edge-locked RoomPlugSpec (a plug is pinned to a wall by
// edge+at; a feature floats at a free x,y — e.g. centre rubble, a statue).
//
// COORDINATE FRAME: x,y are a ROOM-LOCAL offset from the room's NW `origin`, in
// AUTHORED FEET (south-positive, same axes as origin/width/height). So the
// realized ABSOLUTE authored position is `origin + {x,y}`. NOTE this differs from
// the rest of RoomSpec, which the parser stores already-scaled to CANVAS units —
// features stay in authored feet and the authoring controller applies the
// authored→canvas scale to the OFFSET when it mints the marker (see
// createMapFromSpec). `type` is a neutral open-vocabulary tag edi does not interpret.
struct RoomFeature {
    double x = 0.0;   // room-local offset from NW origin, authored feet
    double y = 0.0;   // (south-positive, same axes as origin/width/height)
    std::string type; // neutral open-vocabulary tag, e.g. "rubble", "statue"
    std::string name; // authored label, optional
    // NEUTRAL identity + free-form bag the ENGINE interprets — edi acts on NEITHER.
    // `id` is a stable identifier: a pickup's id IS the door's key_id, and an npc
    // marker references a patrol path BY id. Default "" ⇒ no behavioural change for
    // any existing feature (RoomFeature is never MessagePack-serialized, so adding
    // these can break no on-disk byte identity — they only widen the in-memory value).
    std::string id;
    // Free-form neutral k/v the engine reads, never edi: e.g. an npc's `patrol=<id>`
    // or a chest's `key_id=gold_key`. A vector<pair> (not a map) because authoring is
    // an ORDERED list and the count is tiny — the export joins it in author order, and
    // an order-preserving container makes the TOON projection deterministic for free.
    std::vector<std::pair<std::string, std::string>> metadata;
};

// A NEUTRAL patrol path recorded at MAP level (beside connections/blocks). This is
// NOT a DraftingGeometry variant — like a plug or a node it is a RELATION/annotation,
// not a renderable shape, so it lives as plain data the engine reads, never a drawn
// primitive edi owns.
//
// CLOSED-LOOP INVARIANT: `closed == true` means the waypoints form a LOOP, and the
// first waypoint is NOT repeated at the end — the ENGINE closes the loop (Tiled
// polygon convention). edi records the ordered points and the closed flag; it does
// NOT walk, step, or simulate the path. An npc marker references this path by `id`
// through its `metadata` (patrol=<id>).
struct MapPatrolPath {
    std::string id;                 // referenced by an npc marker's metadata patrol=<id>
    std::vector<Point2D> waypoints; // ordered; stored already-scaled to CANVAS units
    bool closed = true;             // LOOP (default) vs open path — neutral geometry tag
};

// A rectangular room in CANVAS units (the grid projection maps these to physical
// feet/squares). origin is the NW corner. The room is pure spatial data plus a
// neutral material tag — no game semantics, by design.
struct RoomSpec {
    Point2D origin;
    double width = 0.0;
    double height = 0.0;
    double wallThickness = kDefaultRoomWallThickness;
    std::string wallMaterial = "stone"; // neutral tag carried on every wall
    std::vector<RoomOpening> openings;
    std::vector<RoomPlugSpec> plugs;
    std::vector<RoomConnectionSpec> connections; // edges between plugs (by name)
    std::vector<RoomFeature> features;           // interior point markers (authored feet, room-local)

    // Phase-1 decision 13: enclosure + per-edge wall presence.  These drive DRAWING
    // only (which perimeter segments planDraftingRoom emits), like WallType varies the
    // band; they carry NO game-rule semantics past Seam B.
    //
    // Default Enclosed + all-walls-present keeps every existing RoomSpec byte-for-byte
    // identical to the current behaviour: planDraftingRoom still emits four solid walls
    // for a plain rectangular room with no explicit edge-wall configuration.
    //
    // Phase-2 will make planDraftingRoom READ these fields; this slice adds the data
    // spine only — no drawing-logic change means the canary stays byte-identical.
    RoomKind kind  = RoomKind::Enclosed;
    // Named per-edge booleans: named fields avoid index-order confusion
    // (e.g. was index 0 North or East?) that an array would introduce.
    bool wallN = true; // North (top) wall present?
    bool wallE = true; // East  (right) wall present?
    bool wallS = true; // South (bottom) wall present?
    bool wallW = true; // West  (left) wall present?
};

// --- multi-room map authoring (a whole dungeon from one file) ----------------
// A reference to a plug in a multi-room map, by (room name, plug name). Plug names
// are unique only WITHIN a room, so a cross-room connection needs both halves.
struct MapPlugRef {
    std::string roomName;
    std::string plugName;
};

// A connection authored at MAP level: it can span rooms, so each endpoint is a
// (room, plug) pair, not a bare plug name. Neutral like RoomConnectionSpec. A hub
// of degree N (e.g. a 3-way junction) is an N-plug room with N of these edges —
// a single connection joins exactly two plugs, never fans to three.
struct MapConnectionSpec {
    MapPlugRef from;
    MapPlugRef to;
    std::string type;
    // NEUTRAL engine-interpreted TAGS — NOT a rule edi enforces. `locked` is the lock
    // STATE the engine reads; `keyId` names which key opens it (matching a pickup
    // marker's `id`). edi records the tag and simulates NOTHING: it still "lets"
    // anything through because it never models movement. (The connection's older
    // "deliberately no locked" guard meant no RULE; a TAG the engine interprets is
    // squarely inside the mandate.) Defaults false/"" keep every existing connection
    // unchanged. The DraftingDeclaredConnection document twin is DEFERRED (this
    // campaign ships only the Seam B / MapSpec authoring path; the twin would need the
    // fiddly conditional `.edidraw` codec proof).
    bool locked = false;
    std::string keyId;
};

// One room in a multi-room map, named so connections can find its plugs. The
// geometry is just a RoomSpec — the multi-room file reuses the single-room shape.
struct NamedRoomSpec {
    std::string name;
    RoomSpec spec;
};

// MapSpec-level declared PROP instance (M0 additive): a placement by asset_ref at a point —
// NO DraftingBlock definition needed. These props have no in-edi geometry; they are pure asset
// references (like a plug records a neutral type). edi RECORDS asset_ref + transform + position;
// the realizer downstream owns the mesh. A deliberate TWIN of the saved DraftingBlock definition,
// not a reuse (same H2 discipline as motif-vs-block).
struct MapBlockSpec {
    std::string assetRef;          // "<theme>.<piece>", e.g. "crypt.sarcophagus"
    Point2D     position;          // placement CENTRE, ABSOLUTE authored feet (+x east / +y south)
    double      rotationDeg = 0.0; // identity default; plumbs BlockPlacementMetadata.rotationDeg
    double      scale       = 1.0; // identity default; plumbs BlockPlacementMetadata.scale
    std::string name;             // optional label; stamped as a `name:<name>` tag
};

// A whole map: many named rooms in one coordinate space + the cross-room
// connections between their plugs. The neutral authoring product of a .map.toml.
struct MapSpec {
    std::vector<NamedRoomSpec> rooms;
    std::vector<MapConnectionSpec> connections;
    // M0 additive: MapSpec-level prop instances (asset_ref placements). Default-empty,
    // so every existing MapSpec stays byte-identical and behaviour-unchanged.
    std::vector<MapBlockSpec> blocks;
    // NEUTRAL patrol paths (an npc marker references one by id). Default-empty so any
    // map without patrols projects identically. Lives at MAP level, beside blocks,
    // because a patrol is a map-wide annotation, not a per-room shape.
    std::vector<MapPatrolPath> patrols;
};

// A plug the room emitted, paired with the marker object it rides on. The marker
// is in DraftingRoomPlan::objects; this records WHICH plug it is (neutral name +
// type) and the marker's id as the anchor, so the caller mints a plug id and
// issues a CreatePlugCommand. planDraftingRoom mints the marker's OBJECT id; the
// caller mints the PLUG id (work-order decision #4: caller mints, op validates).
struct RoomPlugPlacement {
    DraftingObjectId anchorObjectId;
    std::string name;
    std::string type;
    std::vector<std::string> flags;  // neutral tags, threaded from RoomPlugSpec like `type`
    Point2D anchor;
    RoomEdge edge = RoomEdge::North; // which wall the plug sits on (its outward normal)
};

struct DraftingRoomPlan {
    bool ok = false;
    DraftingResultCode code = DraftingResultCode::None;
    std::string message;
    std::vector<DraftingObject> objects; // the perimeter wall segments + plug markers
    std::vector<RoomPlugPlacement> plugs; // one per authored plug, anchored to a marker

    static DraftingRoomPlan accepted(std::vector<DraftingObject> objects);
    static DraftingRoomPlan rejected(DraftingResultCode code, std::string message);
};

// Build a room's perimeter as WallGeometry segments: each edge MINUS its
// openings, so a solid edge is one segment and an edge with a centred door is
// two. Adjacent segments share corner endpoints, so the existing wall-join pass
// miters them for free; an opening at a corner simply leaves that corner open.
// `mintId` supplies a fresh object id per emitted segment — the caller owns id
// minting so this stays Qt-free and the segment count (opening-dependent) need
// not be known in advance.
DraftingRoomPlan planDraftingRoom(const RoomSpec &spec,
                                  const std::function<DraftingObjectId()> &mintId);

} // namespace edi::drafting
