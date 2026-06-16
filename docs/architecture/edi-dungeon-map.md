# Architecture — the dungeon-map subsystem

> **Status: first draft (verified)** — folded from the reviewer-gate survey of
> campaign `dungeon-map-20260616-cartography` (reply 002, read-only, anchored to
> the `dept/dungeon-map` working tree). This is the durable map of the subsystem;
> keep it current as slices land. `file:line` anchors are as-surveyed and drift
> with edits — trust the symbol name over the number if they disagree.

The map subsystem turns an authored `.map.toml` into a neutral drafting document
(rooms + walls + corridors + doors + blocks) and exports it across **Seam B/C** as
a neutral TOON map for the user's OWN game engine. **Layered law: edi records
geometry + neutral tags; it does NOT simulate. No game rules, no generation.**
The reviewer confirmed this law holds end-to-end: no `passable`/`weight`/
`direction`/`blocksMovement` on any persisted struct, and no WFC/procedural
generation present — corridor routing is deterministic L/Z + A* obstacle
avoidance from authored door pairs (authoring, not generation).

## 1. Ownership boundary — ours vs the drafting core's — **HUB-RULED (H2)**

### The ruling (HUB H2, user-ruled 2026-06-16 — verbatim)
**By-domain, SINGLE document.** Do NOT split `DraftingDocument` or
`DraftingCommand`. Keep the one-document data model — a plug/room/connection is a
relation over objects in the SAME drawing.
- **edi-drafting owns:** the core geometry types + ops + the CORE command arms +
  the core regions of the shared headers — the `DraftingGeometry` variant,
  geometry ops, plot/export, the controller spine.
- **edi-dungeon-map owns:** the map graph — the whole-file set (`DraftingGraphOps`,
  `DraftingRoom`, `DraftingCorridor`, `DraftingPathfind`, `DraftingAsciiMap`,
  `DraftingBlockOps`), the 7 map command arms (`CreatePlug`, `DeletePlug`,
  `DeclareConnection`, `DeleteConnection`, `CreateBlock`, `DeleteBlock`,
  `CreateMapRoom`) + their semantics, and the map STRUCT/ENUM definitions
  (`DraftingPlug`/`DeclaredConnection`/`MapRoom`/`Block`; `DraftingPlugId`/
  `ConnectionId`/`BlockId`; `ObjectRole`, `WallType`, `WallVisualMetadata`,
  `BlockPlacementMetadata`). **`WallGeometry` stays CORE** — it rides every
  geometry visit; it is shared geometry, not map-only.
- **Shared headers** (`DraftingDocument.h`, `DraftingTypes.h`,
  `DraftingCommands.*`) are co-edited **by REGION, not by file**: drafting edits
  the CORE regions, dungeon-map edits the MAP regions, neither edits the other's.
  Disjoint lines ⇒ master merges stay clean.

### Our deliverable from H2 — **LANDED** (commit `8e82c41`, shape a)
The map struct/enum DEFINITIONS now live in a dungeon-map-owned
**`src/drafting/DraftingMapTypes.h`** that `DraftingTypes.h`/`DraftingDocument.h`
include; the document KEEPS its plug/connection/room/block vectors (still one
document). Pure behavior-preserving code motion (95/95 green, snapshot identical).
**Shape (a):** one header, included once mid-`DraftingTypes.h` (namespace
close/reopen) just before `ObjectMetadata`. The `DraftingBlock`↔`DraftingObject`
cycle is broken by a `struct DraftingObject;` forward-decl in the map header —
valid because since C++17 `std::vector` may be instantiated with an incomplete
element type (`DraftingObject` is completed by `DraftingDocument.h` before any
`DraftingBlock` op is odr-used). Result: `DraftingDocument.h`'s entire map surface
is now just the four vectors; `DraftingTypes.h`'s is the one include line.
**Frozen in `docs/closeouts/h2-src-drafting-map-boundary.md`.**

### The map subsystem is cleanly separable
Map data lives in wholly-owned files plus a small, well-marked set of arms
threaded through shared files. This table is the contract that keeps the two
departments out of each other's way.

### Wholly-OURS files (map-specific, no drafting-core role)
| File | Role |
| --- | --- |
| `src/drafting/DraftingMapTypes.h` | **(H2 extraction)** the map record DEFINITIONS: id aliases `DraftingPlugId`/`ConnectionId`/`BlockId`; enums `ObjectRole`/`WallType`; `WallVisualMetadata`/`BlockPlacementMetadata`; structs `DraftingPlug`/`DeclaredConnection`/`MapRoom`/`Block`; the name⇄enum free-func decls. Included by `DraftingTypes.h` (mid-file) |
| `src/drafting/DraftingRoom.{h,cpp}` | Authoring structs (`RoomSpec`, `RoomPlugSpec`, `MapSpec`, `RoomEdge`) + `planDraftingRoom` |
| `src/drafting/DraftingCorridor.{h,cpp}` | `CorridorSpec` → centerline → wall geometry (door↔door routing) |
| `src/drafting/DraftingPathfind.{h,cpp}` | grid A* for v2 corridor obstacle avoidance |
| `src/drafting/DraftingGraphOps.{h,cpp}` | plug/connection/room **mutation ops** + cascade prune |
| `src/drafting/DraftingAsciiMap.{h,cpp}` | ASCII-map parse path (`createMapFromAscii`) |
| `src/drafting/DraftingBlockOps.{h,cpp}` | block-library ops (Phase C) |
| `src/io/MapToonExport.{h,cpp}` | Seam B/C TOON export (both overloads) |
| `src/io/RoomSpecStore.{h,cpp}` | `parseRoomSpecToml` / `parseMapSpecToml` (Seam A authoring) |
| `tests/` | `drafting_room_`, `drafting_corridor_`, `drafting_graph_ops_`, `drafting_ascii_map_`, `drafting_block_ops_`, `room_spec_store_`, `map_spec_store_`, `map_toon_export_` |

### SHARED files — map-specific symbols vs the file's core role
| File | Map-specific symbols (OURS) | Core role (drafting's) |
| --- | --- | --- |
| `DraftingTypes.h` | **(post-H2)** just the `#include "drafting/DraftingMapTypes.h"` line (mid-file, before `ObjectMetadata`) — the map id/enum/metadata DEFINITIONS moved OUT into that header | other ids, geometry variant, stroke/fill/layer/measurement metadata, `ObjectMetadata` (which embeds the moved map metadata) |
| `DraftingDocument.h` | **(post-H2)** just the four doc vectors `plugs`/`connections`/`rooms`/`blocks` (`:52-58`) + `canvasPerAuthoredUnit` + the plug/conn/block clauses of `highestDocumentIdSerial` — the four record STRUCTS moved OUT into `DraftingMapTypes.h` (reached transitively via the `DraftingTypes.h` include) | `DraftingObject`, `DraftingLayer`, objects/layers vectors, find/index helpers |
| `DraftingCommands.{h,cpp}` | arms `CreatePlug`/`DeletePlug`/`DeclareConnection`/`DeleteConnection` (`.h:151-165`), `CreateBlock`/`DeleteBlock` (`:172-178`), `CreateMapRoom` (`:182`); visit clauses (`.cpp:322-335`) | the other 26 arms + the visitor scaffold |
| `DraftingSerialize.cpp` | `plugValue`/`readPlug` (`:588-607`), `connectionValue`/`readConnection` (`:610-627`), `mapRoomValue`/`readMapRoom` (`:632-651`), `blockValue`/`readBlock` (`:660-703`), doc-level plugs/connections/rooms/blocks emit+read (`:722-744`,`:843-884`), `canvas_per_authored_unit` (`:767`,`:818`) | layer/object/geometry codecs, envelope, version gate |
| `DrawingDocumentController.cpp` | `createMapFromSpec` (`:2019-2198`), `createMapFromAscii` (`:2200+`), 4-arg `createObjectsAndSelect` (`:2271-2325`) | the entire non-map controller |
| `DrawingCore.h` | `createMapFromSpec`/`createMapFromAscii` decls (`:241-245`), plug/conn/room overload of `createObjectsAndSelect` (`:370`) | controller class surface |
| `EdiShellWindowIo.cpp` | `buildMapBrowserPanel` (`:700-772`) | all other panel builders |
| `EdiShellWindow.cpp` | `mapWorkspaceLayout` (`:118`), the Map layout-selector branch (`:586-587`) | shell window |
| `app/AppState.cpp` | `WorkspaceMode::Map` arms (`:40,61,82,103,129,145`) | workspace enum machinery |
| `app/main.cpp` | `--map-file` (`:117`,`:325-337`), `--export-map` (`:130`,`:157-214`) | CLI entry |

> **Note on ownership of shared shell files:** `EdiShellWindow.cpp`,
> `app/main.cpp`, `CMakeLists.txt` are **edi-ui's** integration-line files (the
> rebase contract). Our map *content* there (the Map browser panel, the
> `--map-file`/`--export-map` CLI arms) is map-specific logic, but edits to those
> files are a hub/edi-ui coordination, not a local dept/dungeon-map edit.

### Boundary risks (where future edits could collide with edi-drafting)
1. **`DraftingTypes.h` variant `static_assert`** — a new geometry kind from
   edi-drafting bumps the variant; map code adds NO geometry kinds (plug/
   connection/block are document vectors by design — §2), so the two departments
   touch *different regions*. Low collision risk; the one shared header both edit.
2. **`DraftingCommands` variant list** — both departments append arms to the same
   `std::variant` + the same if-constexpr chain. Append-only but textually
   adjacent → merge friction. Small slices + frequent rebase is the mitigation.
3. **`createObjectsAndSelect`** has two overloads (objects-only vs
   +plugs/conns/rooms); the map overload is the richer one, and any drafting-side
   change to the base batch path must keep it working.

## 2. The plug / connection graph model — **CONFIRMED neutral**

- **Two document-level vectors** (plus `rooms`, `blocks`): `DraftingDocument::plugs`
  and `::connections` (`DraftingDocument.h:116-117`), `rooms` (`:120`), `blocks`
  (`:122`). A plug is a RELATION, not a `DraftingGeometry` variant — `DraftingPlug`
  (`:47`) anchors to a doc object **by id** (`anchorObjectId`, `:49`); the header
  comment (`:36-46`) states the rationale.
- **`DraftingPlug` fields** (`:47-53`): `id`, `anchorObjectId`, `name`, `type`
  (open vocab), `anchor` (cached `Point2D` — see B1 staleness note in §7).
  **No passable/weight/direction.**
- **`DraftingDeclaredConnection` fields** (`:60-65`): `id`, `plugA`, `plugB`,
  `type` (neutral role tag). Comment (`:57-59`) calls out the deliberate ABSENCE
  of `passable`/`weight`/`direction`/`locked`.
- `WallType` (`DraftingTypes.h:96`) is a *render* classification only
  (Solid/Door/Window/Secret); comment `:91-94` confirms it "carries no behaviour."

## 3. Map arms of `DraftingCommand` + visit sites

**Arms (7):** `CreatePlugCommand`, `DeletePlugCommand`, `DeclareConnectionCommand`,
`DeleteConnectionCommand`, `CreateBlockCommand`, `DeleteBlockCommand`,
`CreateMapRoomCommand` (`DraftingCommands.h:151-184`; variant list `:214-220`).

**Dispatch sites:**
- `applyDraftingCommand` if-constexpr chain — all 7 handled, each a one-line
  delegate to a `DraftingGraphOps`/`DraftingBlockOps` free function
  (`DraftingCommands.cpp:322-335`). ✓
- `DrawingDocumentController.cpp:1278-1279` — the selection-only undo classifier
  correctly EXCLUDES map arms (they mutate content, not just selection). ✓ No
  other variant-wide visit exists.

**⚠ Non-exhaustive-visit FLAG (Finding A1):** the chain's terminal `else`
(`DraftingCommands.cpp:336-338`) is a **runtime** `rejected("unsupported command")`,
NOT a compile-time `always_false_v` — unlike the geometry visitors
(`DraftingGeometry.cpp`: `geometryKind`/`validateGeometry`/`computeBounds`/
`translateGeometry`/`handleAnchors` all end in `always_false_v`). A forgotten
future arm therefore compiles and silently rejects at runtime instead of failing
the build. **This is the highest-value hardening (refactor slice #1).**

## 4. MessagePack codec for map data — **CONFIRMED additive-tolerant, no bump**

- Versions: `kDraftingDocumentVersion = 2`,
  `kDraftingDocumentMinReadVersion = 1` (`DraftingSerialize.h:20-21`). The v1→v2
  bump was the per-object stroke shim ONLY (`DraftingSerialize.cpp:323-328`);
  **the map fields added NO further bump** (emit comment `:756-761`; read comments
  `:843-845`,`:864-865`,`:875-876`).
- **Tolerant defaults (missing key ⇒ default):**
  - plugs/connections/rooms/blocks arrays — absent ⇒ empty vector (`:846-884`),
    each row type-gated (`type == Map`).
  - `canvas_per_authored_unit` — absent ⇒ `1.0` (`:818`).
  - `block.asset_ref` — absent ⇒ empty (the explicit `wall_visual` precedent,
    `:680-682`).
  - `wall_visual` — absent ⇒ Solid (`:469-471`); `block_placement` — absent ⇒
    empty (`:485-490`).
  - `readPlug`/`readConnection`/`readMapRoom` fall back to each struct's own
    default (`:602-651`) — every field tolerant.
- **No latent intolerant field** in the map codec. Note: a block's *nested* object
  skips a malformed member (tolerant, `:687-689`), whereas a *top-level* object is
  a hard SyntaxError (`:830-838`) — intentional asymmetry, commented.
- **Round-trip:** plug `anchor`, connection endpoints, room footprint, block
  objects all serialize/read symmetrically (`drafting_serialize_tests` covers it).

## 5. What-calls-what: `.map.toml` → rendered objects

```
app/main.cpp:325 (--map-file)
 └─ edi::io::parseMapSpecToml(text, kCanvasPerFoot)          RoomSpecStore.cpp
      └─ MapSpec (DraftingRoom.h:99) — neutral, validated (unique names,
         non-overlapping footprints, every connection endpoint resolves)
 └─ controller->createMapFromSpec(spec, kCanvasPerFoot)      DrawingDocumentController.cpp:2019
      ├─ m_document.canvasPerAuthoredUnit = scale             :2024
      ├─ per room: inject opening at each CONNECTED plug       :2082-2095
      │    └─ planDraftingRoom(roomSpec, mintId)              DraftingRoom.cpp → wall segments + plug markers
      ├─ per plug placement: mint DraftingPlug + door leaf     :2104-2138
      ├─ per connection: mint DraftingDeclaredConnection       :2147-2160
      │    └─ routeCorridorCenterline + corridorWalls         DraftingCorridor.cpp / DraftingPathfind.cpp
      └─ createObjectsAndSelect(objects, plugs, conns, rooms)  :2196 → :2271
           ├─ CreateObjectsCommand (atomic)                    :2295
           ├─ CreatePlug / DeclareConnection / CreateMapRoom    :2304-2317 (results intentionally dropped — §7 N3)
           ├─ SelectObjectsCommand                             :2318
           └─ commitEdit(false); emit modelChanged()           :2322-2323
                └─ projection → painter + buildMapBrowserPanel refresh (EdiShellWindowIo.cpp:769)
```
The Map browser (`EdiShellWindowIo.cpp:722-762`) is a **read-only** re-projection
of the live document on every `modelChanged`; footprints in authored feet
(`canvas / scale`, `:741`).

## 6. Seams in / out — Seam B/C **CONFIRMED TOON** (never JSON/UVTT)

- **Seam B** — `exportMapToToon(MapSpec)` (`MapToonExport.cpp:72-129`): `kind: map`
  + three TOON tabular arrays `rooms/plugs/connections`. `connected` is DERIVED
  from the connection set (`:76-80`), not stored. Projects the **typed MapSpec**,
  not a document reconstruction.
- **Seam C** — `exportMapToToon(DraftingDocument)` (`:161-283`): same three arrays
  + a fourth `blocks[]` re-formed by grouping placed objects on
  `BlockPlacementMetadata.instanceId` (`:247-281`). Projects the live typed
  document; divides canvas units by `canvasPerAuthoredUnit` to recover authored
  feet (`:165-166`); plug `edge` derived from anchor vs footprint (`:149-157`).
- CLI `--export-map` (`app/main.cpp:157-214`) dispatches by extension: `.edidraw`
  → decode doc → Seam C; `.map.toml` → parse → Seam B.
- Seam A into map code: `parseMapSpecToml`/`parseRoomSpecToml` — neutral parse, no
  rules.

## 7. Refactor candidates (behavior-preserving only) — ranked

| # | Severity | Where | Defect | Behavior-preserving fix |
| --- | --- | --- | --- | --- |
| A1 | BUG (latent) | `DraftingCommands.cpp:336-338` | terminal `else` is a runtime reject, not `always_false_v`; a forgotten arm silently rejects instead of failing the build | mirror the geometry-visitor `static_assert(always_false_v<Command>)`. *Accept:* all arms dispatch; deleting any one arm fails to compile |
| N1 | NIT (dup) | `MapToonExport.cpp:82-90` & `176-184` | the two overloads duplicate the TOON header + rooms/plugs/connections row shapes | hoist `writeHeader()` + a row helper. *Accept:* both overloads byte-identical; `map_toon_export_tests` green |
| B1 | BUG (latent, no trigger) | `DraftingDocument.h:52` + move path | `plug.anchor` cache is authored-once, NOT synced on object move; export `deriveEdge` (`MapToonExport.cpp:149`) would read a drifted anchor once interactive move lands | **document the contract now** (comment + TODO); the real `syncGraphForMovedObject` fix is **note-don't-build** (mandate — no feature) |
| N3 | NIT (low) | `DrawingDocumentController.cpp:2304-2317` | the plug/conn/room sub-command results are intentionally dropped 3× | optional tiny `applyTrusted(cmd)` debug-assert wrapper. Low value — defer |
| N2 | none | `MapToonExport.cpp:37` | `edgeName` unreachable `return "?"` | already commented; no action |

**Cascade integrity (checked CLEAN):** object-delete → `removeObject` →
`pruneGraphForRemovedObject` drops anchored plugs AND their edges
(`DraftingGraphOps.cpp:132-163`); plug-delete → `removePlug` cascades edges
(`:76-97`); connection refs validated on declare (`:99-118`). No dangling gap.

## 8. Known forward dependency (note, do not build)
- **`transformGeometry`** (rotate/scale over the geometry kinds) does NOT exist
  yet. **HUB H2: drafting-owned shared primitive** — it lives in
  `DraftingGeometry.{h,cpp}` beside `translateGeometry` (a guarded visit), and
  **dungeon-map CONSUMES it**. It is a FEATURE (parked in
  `~/dept-bus/ROADMAPS-DRAFT.md`) — do NOT build it during cartography. Future
  per-instance block rotation/scale depends on it. (Same box: B1's
  interactive-move `plug.anchor` sync.)
