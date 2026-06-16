# Closeout — the `src/drafting` map/core ownership boundary (HUB H2)

> Freezes a boundary so future work does not re-litigate it.

- **Boundary**: who owns what in the shared `src/drafting` headers, and the home
  of the dungeon-map record types
- **Department**: edi-dungeon-map
- **Campaign**: dungeon-map-20260616-cartography
- **Date**: 2026-06-16

## The decision

**By-domain, SINGLE document** (HUB ruling H2, user-ruled). `DraftingDocument` and
`DraftingCommand` are NOT split — a plug/room/connection is a relation over objects
in the SAME drawing.

- **edi-drafting owns:** the core geometry types + ops + the CORE command arms +
  the core regions of the shared headers — the `DraftingGeometry` variant,
  geometry ops, plot/export, the controller spine. **`WallGeometry` stays CORE**
  (it rides every geometry visit — shared geometry, not map-only).
- **edi-dungeon-map owns:** the map graph — the whole-file set (`DraftingGraphOps`,
  `DraftingRoom`, `DraftingCorridor`, `DraftingPathfind`, `DraftingAsciiMap`,
  `DraftingBlockOps`), the 7 map command arms (`CreatePlug`, `DeletePlug`,
  `DeclareConnection`, `DeleteConnection`, `CreateBlock`, `DeleteBlock`,
  `CreateMapRoom`) + their semantics, and the map STRUCT/ENUM definitions — now
  physically homed in **`src/drafting/DraftingMapTypes.h`** (commit `8e82c41`):
  `DraftingPlug`/`DeclaredConnection`/`MapRoom`/`Block`; `DraftingPlugId`/
  `ConnectionId`/`BlockId`; `ObjectRole`, `WallType`, `WallVisualMetadata`,
  `BlockPlacementMetadata`, the name⇄enum free-func decls.
- **Shared headers** (`DraftingDocument.h`, `DraftingTypes.h`,
  `DraftingCommands.*`) are co-edited **by REGION, not by file**: drafting edits
  the CORE regions, dungeon-map edits the MAP regions, neither edits the other's.
  After the H2 extraction the MAP region in `DraftingDocument.h` is just the four
  vectors, and in `DraftingTypes.h` just the one `#include` line.

## Why (the reasoning that must NOT be re-argued)

- **Single document over a split.** A plug/connection/room/block is a RELATION over
  objects in one drawing; splitting `DraftingDocument`/`DraftingCommand` would
  create a second derived object-space and duplicate the undo/serialize/command
  plumbing for nothing. The map graph already rides the document's existing free
  undo + MessagePack persistence — that is the whole reason it lives inside the
  document.
- **Shape (a) — one `DraftingMapTypes.h`, mid-file include, forward-decl.** Two
  alternatives were on the table (reviewer gate 004):
  - *(c) two headers* — cleaner layering direction (no map→core forward-decl) but
    +1 file and a deviation from H2's literal "single header."
  - *(a) one header* — chosen. It is what H2 literally specified, shrinks the
    `DraftingDocument.h` map surface to JUST the four vectors, and the one
    cross-boundary cost (a `struct DraftingObject;` forward-decl so `DraftingBlock`
    can hold `std::vector<DraftingObject>` by value) is **valid, standard C++17**:
    `std::vector` may be instantiated with an incomplete element type; the element
    is completed by `DraftingDocument.h` before any `DraftingBlock` op is odr-used.
  Settled in favor of (a); the hub veto-window toward (c) closed unused.

## The contract (what future work must respect)

- The map record DEFINITIONS live in `DraftingMapTypes.h`. New map structs/enums go
  THERE, not back into `DraftingTypes.h`/`DraftingDocument.h`. Those two shared
  headers keep ONLY the include line + the four document vectors on the map side.
- `DraftingMapTypes.h` is intentionally **NOT standalone**: it relies on its
  includer to have defined `Point2D`/`Bounds2D` first (hence the single mid-file
  include point just before `ObjectMetadata`). Do not "fix" it to self-include
  `DraftingTypes.h` — that re-creates the cycle.
- The map graph stays **NEUTRAL**: no `passable`/`weight`/`direction`/`locked` on
  any persisted struct. Rules live downstream of Seam B in the user's engine.
- MessagePack stays **additive-tolerant, no version bump** (missing key ⇒ default).
- The `applyDraftingCommand` visitor's exhaustiveness is now **compile-time
  guarded** (`static_assert(always_false_v<Command>)`, commit `4ca427e`): a future
  unhandled command arm is a NAMED BUILD ERROR. Keep it that way.

## Out of scope / explicitly NOT allowed

- **`transformGeometry`** (rotate/scale over the geometry kinds) is a
  **drafting-owned** shared primitive (lives beside `translateGeometry` in
  `DraftingGeometry.{h,cpp}`). dungeon-map CONSUMES it. It is a FEATURE (parked in
  `~/dept-bus/ROADMAPS-DRAFT.md`) — NOT built during cartography. Per-instance
  block rotation/scale depends on it.
- **`syncGraphForMovedObject`** (B1's real fix — re-sync `plug.anchor` when an
  anchored object moves) is a documented but UNBUILT correctness gap (TODO now in
  `DraftingMapTypes.h` on `DraftingPlug`). A FEATURE — not built during cartography.
- No generation (WFC/BSP/procedural). edi is the authoring bench; rules + generation
  are downstream.

## Pointers
- Code: `src/drafting/DraftingMapTypes.h` (`8e82c41`); `DraftingCommands.cpp`
  static_assert (`4ca427e`); `MapToonExport.cpp` de-dup (`fcae7eb`);
  `DraftingDocument.h`/`DraftingGraphOps.cpp` B1 docs (`1a26ee7`).
- Tests: `drafting_serialize`, `drafting_graph_ops`, `drafting_block_ops`,
  `map_toon_export`, `room_spec_store`, `map_spec_store` (all green, unchanged).
- Related: `docs/architecture/edi-dungeon-map.md` (the living map),
  `~/dept-bus/RULING-H2-src-drafting-boundary.md` (the hub ruling),
  `docs/departments/edi-dungeon-map.md` (charter).
- **Open cosmetic follow-up (edi-ui-owned):** `DraftingMapTypes.h` is NOT listed in
  `CMakeLists.txt` though its two sibling headers are (build-irrelevant; IDE
  source-group only). An edi-ui change may add it — not a dungeon-map edit.
