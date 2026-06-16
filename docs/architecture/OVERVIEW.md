# Architecture — edi, the whole engine

> **The entry-point doc.** How edi is wired top to bottom: the four departments,
> what each owns, how data flows, and the seams that connect them. Read this first,
> then drop into the per-department detail docs linked in §7.
>
> Folded 2026-06-16 from the three department architecture surveys, independently
> verified against live source. `file:line` anchors drift as code moves — trust the
> symbol name over the number, and re-grep before relying on an exact line.

edi is a Qt6/C++20 2D drafting (CAD) application — a widget-based UI with **no QML,
no JSON, no `.js`**. It is, in purpose, a sacred-geometry / drafting construction
engine: you draft figures on a canvas, and that drafted geometry feeds two
downstream paths — a neutral **dungeon-map** document for the user's own game engine,
and a **recipe lab** that lowers canvas geometry into Blender solids.

The whole thing is held together by one discipline (§6): **closed `std::variant`s
visited by free functions**, where adding a kind/arm *fails to compile* at every
interpreter that forgot it. There is no behavior-by-subclassing anywhere in the core.

---

## 1. The four departments

edi is developed by four departments over **one shared codebase and one shared
document model**. Three are domain departments (each with a git worktree); the
fourth, **edi-ui**, owns the shell that the other three depend on.

| Department | Owns | Lives in |
| --- | --- | --- |
| **edi-drafting** | The pure-C++20 geometry CORE: the `DraftingGeometry` variant + `Drafting*Ops` free functions, the `DraftingCommand` dispatch, plot/export, and the thin Qt controller spine. | `src/drafting/` (core regions), `src/core/` |
| **edi-dungeon-map** | The map graph: rooms/plugs/connections/blocks, the 7 map command arms, the map struct definitions (`DraftingMapTypes.h`), the six map ops files, and TOON export. | `src/drafting/` (map files + map regions), `src/io/MapToonExport.*`, `RoomSpecStore.*` |
| **edi-blender-lab** | The recipe lab (Seam A): the `RecipeOp` variant, its interpreters, the C++↔Python TOML op-stream contract, and the Blender panel content. | `src/recipe/`, `tools/blender/` |
| **edi-ui** | The widget SHELL: `EdiShellWindow*`, `src/widgets/DrawingCanvas*`, `app/main.cpp`, `CMakeLists.txt` — the integration-line files every department depends on but none of the three domain depts own. | `src/widgets/`, `app/` |

**The H2 ruling (settled 2026-06-16) — by-domain, SINGLE document.** A plug/room/
connection is a *relation over objects in the same drawing*, so `DraftingDocument`
and `DraftingCommand` are **NOT split**. Instead the shared headers are co-edited
**by REGION**: edi-drafting edits the CORE regions, edi-dungeon-map edits the MAP
regions, and the regions are disjoint lines so master merges stay clean. (Details
and the file-by-file ownership table are in §5.)

---

## 2. The two layers under drafting

Everything sits on a two-layer spine:

- **`src/drafting/` → `edi_drafting_core`** — pure C++20, **no Qt types**. Plain
  structs + free functions. Plan functions return `ok`+payload structs; all mutation
  is a `DraftingCommand` variant applied via the single `applyDraftingCommand`
  visitor. **All the LOGIC lives here.**
- **`src/core/` → Qt orchestration** — `DrawingDocumentController` (declared in
  `DrawingCore.h`, defined in the ~129 KB `DrawingDocumentController.cpp`): the thin
  layer that resolves inputs → delegates planning → applies a command → emits
  `modelChanged`. `DrawingDocumentProjection.*` turns a `DraftingDocument` into the
  `QVariantMap` the canvas/inspector painters consume.

**The controller spine (`DrawingCore.h`).** Every mutation funnels through
**`applyCommandAndEmit`** (apply one command, do `beginEdit`/`commitEdit` undo
bookkeeping, emit `modelChanged`). The kind-and-callable helpers
(`applyActiveObjectMetadataUpdate`, `applyActiveObjectGeometryUpdate`,
`applyFieldEdit`, `applyLayerFlagsUpdate`, …) each do resolve→plan→apply→emit and
funnel here. **Rule for all departments: extend the helpers, never re-inline the
resolve→plan→apply→emit sequence.**

---

## 3. The shared core types

| Type | Where | Role |
| --- | --- | --- |
| `DraftingGeometry` | `DraftingTypes.h:324` | `std::variant` of **14** geometry kinds; count-guarded by `static_assert` at `:340` |
| `DraftingShapeKind` | `DraftingTypes.h` | the 14-kind enum; `shapeKindOf<>` map |
| `always_false_v` | `DraftingTypes.h` | the exhaustiveness terminal for every `std::visit` overload set |
| `DraftingObject` / `Layer` / `Document` | `DraftingDocument.h` | the document model + find/index/id helpers |
| `DraftingCommand` | `DraftingCommands.h:186` | `std::variant` of **33** command arms (26 core + 7 map) |
| `DraftingMapTypes.h` | `src/drafting/DraftingMapTypes.h` | **(dungeon-map-owned)** the map record DEFINITIONS, included mid-`DraftingTypes.h` |
| `RecipeOp` | `RecipeOps.h:192` | `std::variant` of **10** recipe-op arms (blender-lab-owned) |

The 14 geometry kinds: Point, Line, Rectangle, Circle, Arc, Ellipse, Polygon,
Polyline, Guide, ConstructionLine, Dimension, TextAnnotation, Spline, **Wall**.
`WallGeometry` is map-MOTIVATED but is genuine shared geometry — it stays CORE and
rides every geometry visit.

---

## 4. The core data flows

There is **one document** (`DraftingDocument`). It is the hub of four flows.

### A. Document → projection → canvas paint (the live editing loop)
```
widget (objectName field) → controller.applyFieldEdit / apply*Update
  → resolve active object → plan* (ok+payload) → DraftingCommand
  → applyCommandAndEmit
      → beginEdit snapshot → applyDraftingCommand (the ONE command visit)
      → ops free function mutates the document (++revision)
      → commitEdit pushes undo → emit modelChanged
  → DrawingDocumentProjection rebuilds the QVariantMap → canvas/inspector painter
```

### B. Document → plot plan → SVG / HPGL / G-code (vector export)
```
DraftingDocument → buildDraftingPlotPlan / buildDraftingPlotJob
  → DraftingPlotPlan (segments + fill rings)
  → DraftingSvgOut / DraftingHpglOut / DraftingGcodeOut
```
(Circle is analytic on screen but faceted at 32 segments on export, ellipse at 64
both places — a known, internally-consistent screen-vs-plot fidelity choice.)

### C. Document → map TOON export (Seam B/C, to the user's game engine)
```
.map.toml → parseMapSpecToml → MapSpec (neutral, validated)
  → controller.createMapFromSpec
      → per room: planDraftingRoom → wall segments + plug markers
      → per plug: mint DraftingPlug + door leaf
      → per connection: mint DraftingDeclaredConnection
           → routeCorridorCenterline / corridorWalls (L/Z + grid A*)
      → createObjectsAndSelect (atomic) → commitEdit → modelChanged
  → exportMapToToon(DraftingDocument)  →  kind: map + rooms/plugs/connections/blocks
```
The map document records **geometry + neutral tags only** — no `passable`/`weight`/
`direction`/`locked`, no rules, no generation. `WallType` (Solid/Door/Window/Secret)
is a *render* classification that carries no behavior. The engine downstream owns all
game rules. Export is **TOON, never JSON/UVTT.**

### D. Canvas geometry → recipe bindings → bpy extrude/lathe (Seam A, to Blender)
```
click → mutate m_opsStream → opsStreamChanged → panels re-render
recipe authoring → RecipeOpStream (10-arm RecipeOp variant)
  → resolveRecipeOps(stream, DraftingDocument, grid)        [reads drafting READ-ONLY]
      → bindings pass: resolveMeasurementField measures the drafted object by id
      → lathe lowering: AddRevolvedProfile → AddMoulding via resolveProfilePoints
  → OpWriter emits a TOML op stream
  → tools/blender/edi_craft.py parse_ops reads it key-for-key
      → ASCII / dry-run / OBJ proofs, then (via host) the Blender subprocess → PNG
```
The recipe lab consumes the drafting document **read-only** (binding identity = the
drafted object's immutable string id) and never writes the core.

---

## 5. Cross-department seams and shared-file ownership

### What each department EXPORTS / IMPORTS (verified)

**edi-drafting exports** (the shared substrate everyone builds on):
- The `DraftingGeometry` variant + `DraftingShapeKind` + the `always_false_v`
  exhaustiveness terminal — the shared geometry vocabulary.
- `WallGeometry` — core geometry consumed by dungeon-map room/wall rendering.
- The `DraftingCommand` variant + `applyDraftingCommand` (the single command
  dispatch; the 7 map arms are dungeon-map-owned regions *inside* it).
- Geometry ops free functions (`translateGeometry` / `computeBounds` /
  `validateGeometry` / `handleAnchors` / samplers) — and the **planned, not-yet-built
  `transformGeometry` sibling**, which is drafting-owned and which dungeon-map will
  consume.
- The controller spine helpers (`applyCommandAndEmit` + the `apply*Update` family).
- Measurement types (`MeasurementMetadata`, `MeasurementUnit`, `ScaleCalibration`,
  `DraftingQuickMeasureResult`) and Seam-B asset refs (`DraftingBlock.assetRef`,
  `BlockPlacementMetadata.assetRef`) — read by edi-blender-lab.
- `DraftingToolKind` + `DraftingInspectorPlan` + the projection `QVariantMap` —
  consumed by edi-ui's belt/menu/canvas painters.

**edi-dungeon-map exports / imports:**
- Exports `DraftingMapTypes.h` (the map record vocabulary), the four
  `DraftingDocument` map vectors, the 7 map command arms + their
  `DraftingGraphOps`/`DraftingBlockOps` handlers + cascade pruning, the Seam A
  parsers (`parseMapSpecToml`/`parseRoomSpecToml`, `createMapFromSpec/FromAscii`),
  and the Seam B/C `exportMapToToon` boundary.
- Imports the entire drafting CORE (geometry variant incl. `WallGeometry`, ops,
  command visitor scaffold, document/controller spine, MessagePack codecs) and has
  one **parked forward dependency** on the future drafting-owned `transformGeometry`.

**edi-blender-lab exports / imports:**
- Exports the `RecipeOp` variant + `RecipeOpStream`, the TOML op-stream contract
  (the C++↔Python seam read by `edi_craft.py`), opaque asset ids, the
  custom-craftsman manifest contract, and the ASCII/dry-run/OBJ proof artifacts.
- Imports drafting **read-only** via `RecipeMeasure`; treats `src/io/ProcessRunStore`,
  `src/scripting/BlenderRunPlan`, and `EdiShellWindow*` as **adjacent seams it
  records, not edits** (those are edi-ui host files).

### Shared files — co-edited BY REGION (the rebase contract)

`DraftingDocument.h`, `DraftingTypes.h`, and `DraftingCommands.{h,cpp}` carry both
core and map content; the two domain depts edit **disjoint line regions**:

| Shared file | edi-drafting region (CORE) | edi-dungeon-map region (MAP) |
| --- | --- | --- |
| `DraftingTypes.h` | geometry variant, stroke/fill/layer/measurement metadata, `ObjectMetadata` | the single `#include "drafting/DraftingMapTypes.h"` line |
| `DraftingDocument.h` | `DraftingObject`/`Layer`, objects/layers vectors, find/index helpers | the four doc vectors `plugs`/`connections`/`rooms`/`blocks` + `canvasPerAuthoredUnit` |
| `DraftingCommands.{h,cpp}` | the 26 core arms + the visitor scaffold | the 7 map arms + their visit clauses |
| `DraftingSerialize.cpp` | layer/object/geometry codecs, envelope, version gate | the plug/connection/room/block codecs |

The **edi-ui shell files** (`EdiShellWindow*`, `app/main.cpp`, `CMakeLists.txt`) are
the integration-line files edi-ui owns. Map content there (the Map browser panel, the
`--map-file`/`--export-map` CLI arms) and blender-lab content (the `--list-craftsmen`
host call) are *coordinated* edits, not local dept edits. **No edi-ui session is
running yet** — these files are touched by hub/edi-ui coordination.

### Formats (no JSON, ever)
- **MessagePack** for documents — additive-tolerant (`EDIM` envelope, version gate;
  the map fields added no version bump, every missing key defaults).
- **TOML** for settings and recipe op streams.
- **TOON** for AI/engine handoffs (Seam A authoring and Seam B/C map export).

---

## 6. The discipline that holds it together: exhaustive variants

The whole engine is built on **closed `std::variant`s visited by free-function
overload sets**, each terminated by `static_assert(always_false_v<…>)`. Adding a kind
or arm makes every interpreter that forgot it **fail to compile** — the compiler is
the checklist. No behavior-by-subclassing, no stateful logic objects.

- **`DraftingGeometry` (14 kinds)** — visited at ~17 geometry-side sites
  (validate/computeBounds/translate/handleAnchors, hit-test, snap, numeric-edit,
  object-edit, serialize, projection, mirror, quick-measure, plot-plan). All of them,
  including the four formerly-unguarded visits (Mirror, QuickMeasure, PlotPlan ×2),
  now carry the `always_false_v` terminal.
- **`DraftingCommand` (33 arms)** — exactly **one** `std::visit`
  (`applyDraftingCommand`); its terminal is now `static_assert(always_false_v<Command>)`,
  so a forgotten command arm fails the build like the geometry visits do.
- **`RecipeOp` (10 arms)** — visited by `Namer`, the store writer, validators, the
  ASCII drawer, the bind tables, and the schema generators. The one non-compiler-
  enforced site is the **TOML store READER** (a string if/else ladder with a refusing
  default), now backstopped by a `static_assert(variant_size_v<RecipeOp> == 10)`
  tripwire beside it, plus the cross-language **C++↔Python TOML contract** whose only
  guard is the smoke test (`tests/edi_craft_smoke.py` / `--obj-out`).

---

## 7. Where to go next

- **drafting core detail** → `docs/architecture/edi-drafting.md` — the geometry
  variant, the ops slices, the controller spine, the plot/export path.
- **dungeon-map detail** → `docs/architecture/edi-dungeon-map.md` — the plug/
  connection graph, the 7 map arms, the `.map.toml` → objects call chain, Seam B/C
  TOON.
- **recipe lab detail** → `docs/architecture/edi-blender-lab.md` — the `RecipeOp`
  variant, the interpreter sites, the C++↔Python contract, the proof tiers.
- **process** → `docs/agent-workflow.md`, `docs/departments/`, `docs/handoffs/LEDGER.md`.

---

## 8. Known parked / not-yet-built

- **`transformGeometry`** (rotate/scale over the 14 geometry kinds) — drafting-owned,
  confirmed absent (only a forward-looking comment exists). Both drafting and
  dungeon-map (per-instance block rotation/scale) will consume it. A parked FEATURE,
  not to be built during the current map-and-clean campaigns.
- **edi-ui shell session** — not yet running; the shell files are currently maintained
  only via hub/edi-ui coordination.
- **`plug.anchor` sync on interactive move** — the cached plug anchor is authored-once
  and not yet synced when an anchored object moves; documented as a contract, the fix
  is parked alongside `transformGeometry`.
- **Seam B asset→solid expansion** — downstream of the recipe lab; out of scope of
  any current department.
