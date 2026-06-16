# Architecture — the drafting core (`src/drafting`, `src/core`)

> How the drafting core is structured, what talks to what, and how it is wired.
> Maintained by the `edi-drafting` planner; kept current as the builder changes
> things. First draft folded from the reviewer-gate map of campaign
> `drafting-20260616-cartography` (2026-06-16) — citations are file:line at that
> commit and drift as code moves; re-grep before trusting an exact line.

## 1. The two layers

- **`src/drafting/` → `edi_drafting_core`** — pure C++20, **no Qt types**. Plain
  structs + free functions (`Drafting*Ops`). Plan functions return `ok`+payload
  structs; mutations are a `DraftingCommand` variant applied via
  `applyDraftingCommand`. This is where all the LOGIC lives.
- **`src/core/` → Qt orchestration** — `DrawingDocumentController` (declared in
  `DrawingCore.h`, defined in the 129 KB `DrawingDocumentController.cpp`): the thin
  layer that resolves inputs → delegates planning → applies a command → emits
  `modelChanged`. `DrawingDocumentProjection.*` turns a `DraftingDocument` into the
  `QVariantMap` the canvas/inspector painters consume. `DrawingCoreInternal.h` +
  the thin `DrawingCore*.cpp` TUs are split stubs of the controller.

## 2. The core types

| Type | Where | Role |
| --- | --- | --- |
| `DraftingGeometry` | `DraftingTypes.h:324-338` | `std::variant` of the 14 geometry kinds; count-guarded by `static_assert` (`:340`) |
| `DraftingShapeKind` | `DraftingTypes.h` | the 14-kind enum; `shapeKindOf<>` map at `:369-382` |
| `always_false_v` | `DraftingTypes.h:321` | the exhaustiveness terminal for `std::visit` overload sets |
| `DraftingObject` / `Layer` / `Document` | `DraftingDocument.h:12,27,108` | the document model + find/index/id helpers |
| `StrokeStyle` / `FillStyle` | `DraftingTypes.h:123,139` | paint metadata on an object |
| `DraftingCommand` | `DraftingCommands.h:186-220` | `std::variant` of **33** command arms |
| Plot family | `DraftingPlotPlan.h` (Plan/Segment/Fill), `DraftingPlotJob.h` (Job) | export plan/stream |

### The 14 `DraftingGeometry` arms
Point, Line, Rectangle, Circle, Arc, Ellipse, Polygon, Polyline, Guide,
ConstructionLine, Dimension, TextAnnotation, Spline, Wall.

### `std::visit` over `DraftingGeometry` — 19 sites, exhaustiveness state
- **GUARDED** (terminal `always_false_v`): `DraftingGeometry.cpp` geometryKind(:303),
  validateGeometry(:394), computeBounds(:519), translateGeometry(:616),
  handleAnchors(:825); `DraftingHitTest.cpp:62`; `DraftingNumericEdit.cpp:69`;
  `DraftingSnap.cpp:267`; `DraftingObjectEdit.cpp:257,499`; `DraftingSerialize.cpp:137`;
  `DrawingDocumentProjection.cpp:313,530`.
- **NOW GUARDED (cartography batch 002, commit `2a1be77`)** — formerly unguarded;
  made compile-exhaustive via explicit per-kind arms + `always_false_v` terminal,
  behavior-preserving (each new arm reproduces the prior fall-through; reviewer
  diff-audit ACCEPT): `DraftingMirror.cpp` `mirrorGeometry` (7 transform + 7
  explicit pass-through; see the two-list note below); `DraftingQuickMeasure.cpp`
  `quickMeasureAt` (5 measured + 9 explicit `Unsupported` — the old `else` returned
  `Unsupported`, NOT a base measure); `DraftingPlotPlan.cpp` `appendPlotSegments` +
  `closedFillRing` (explicit no-segment / empty-ring arms).
  - **Mirror two-list note:** the mirrorable set lives in TWO hand-kept places by
    design — the `mirrorGeometry` visit (keys on geometry TYPE) and `supportsMirror`
    (keys on `DraftingShapeKind`). They AGREE today; `mirrorDraftingObject` gates on
    `supportsMirror` BEFORE the visit, so the pass-through arms are inert in practice.
    Not unified (unifying would be behavior-risking); the visit guard now prevents
    forgetting a kind on the visit side, and a sync comment flags `supportsMirror`.

### `std::visit` over `DraftingCommand`
Exactly ONE: `applyDraftingCommand` (`DraftingCommands.cpp:68-340`). Most arms are a
one-line delegate to a `*Ops` free function via `fromStoreResult`;
CreateObjects/MoveSelection/Align/Distribute/Select* inline a validate→stage→commit
block (justified by their batch O(N²)-avoidance comments). The terminal arm is now
`static_assert(always_false_v<Command>)` (commit `985e200`, `DraftingCommands.cpp:336-342`)
— so the command variant IS compile-exhaustive like the geometry visits (a missing
arm fails the build). The inline blocks MoveSelection/Align/Distribute remain
near-duplicates → dedup slice still pending (§5).

## 3. The ops slices (free functions over plain structs)
`DraftingGeometry` (validate/computeBounds/translateGeometry/handleAnchors/area +
the shared samplers `sampleArc`/`sampleEllipse`/`sampleSpline` + segment intersection),
`DraftingHitTest`, `DraftingSnap`, `DraftingMirror`, `DraftingModify`,
`DraftingOffset`, `DraftingArray`, `DraftingAlign`, `DraftingNudgeOps`,
`DraftingNumericEdit`, `DraftingObjectEdit`, `DraftingPhysicalEdit`/`PhysicalGeometry`,
`DraftingSelection`, `DraftingClipboard`, `DraftingConstructionOps`,
`DraftingDimensionOps`, `DraftingGuideOps`, `DraftingGrid`, `DraftingMetadata`,
`DraftingLayerOps`, `DraftingMeasurement`(+`Format`), `DraftingQuickMeasure`,
`DraftingCalibration`, `DraftingInspectorPlan`, `DraftingBuildPlan`,
`DraftingToolCreation`, `DraftingStore`. Serialize: `DraftingSerialize.*`
(MessagePack value codec). Export: `DraftingPlotPlan/Job/JobReport/Bounds`,
`DraftingSvgOut`, `DraftingHpglOut`, `DraftingGcodeOut`.

### plan*/build* (`ok`+payload) functions
planDraftingAlignment, buildBlockFromObjects, buildPlanNoteForObject(Checked)/
buildPlanDocumentForObjects, buildDraftingCalibrationPattern/planDraftingCalibrationCorrection,
buildDraftingGuideObject, buildDraftingPlotPlan/buildDraftingPlotJob,
planDraftingInspector, planCreateDraftingLayer/planLayer{Locked,Visible}Update,
planDimensionKindChange, planNudgeDelta/planNudgeInsideDrawable/planSelectionDrawableMove,
planPhysicalGeometryEdit, buildDraftingObjectForTool, planDraftingPaste,
planGuideVisual*/planDimension*/planObjectRole/WallType/Material/ExportGroup/Tags,
build/validate/makeDraftingObject; **(map)** planAsciiMapGeometry, planDraftingRoom.

## 4. The controller spine (`DrawingCore.h:312-364`) and the call path

Every mutation funnels through **`applyCommandAndEmit`** (`:312`): apply one
`DraftingCommand`, `beginEdit`/`commitEdit` undo bookkeeping, emit `modelChanged`.
The kind-and-callable helpers each do resolve→plan→apply→emit and funnel here:
- `applyActiveObjectMetadataUpdate` (`:339` kind-keyed / `:344` any-kind) — resolve
  active object → run a `DraftingMetadataUpdatePlan` callable → `UpdateMetadataCommand`.
- `applyActiveObjectGeometryUpdate` (`:346`) + template `applyActiveGeometryPlan<Geometry>`
  (`:351`) — resolve active object of a kind, unwrap the variant, run a geometry plan
  → `UpdateGeometryCommand`.
- `applyLayerFlagsUpdate` (`:355`), `applyActiveLayerPlotStyleUpdate` (`:359`),
  `applyFieldEdit` (`:329`), `applySelectionDrawablePlacement` (`:338`),
  `applyGuideDrawablePlacement` (`:354`), `createTransformedActiveObject` (`:361`) —
  same shape. `beginEdit`/`commitEdit`/`pushUndoState` (`:317-326`) do undo
  snapshots; `commitEdit` takes a compile-time `selectionOnly` hint to skip the
  encode-twice fallback.

**Representative call-graph (a numeric inspector edit):**
```
widget(objectName field) → controller.applyFieldEdit("numeric", …, value, planEdit)
  → resolve active object → planPhysicalGeometryEdit() → plan struct
  → NumericGeometryEditCommand → applyCommandAndEmit
    → beginEdit() snapshot → applyDraftingCommand → visit arm → applyNumericGeometryEdit (ops)
    → updateObjectGeometry (store, ++revision) → commitEdit(selectionOnly=false) pushes undo
    → emit modelChanged → projection rebuilds QVariantMap → painter
```
**Rule: extend the helpers, never re-inline resolve→plan→apply→emit.**

## 5. Refactor backlog (behavior-preserving; from the reviewer gate)
Ranked; tracked in campaign `drafting-20260616-cartography`. None depends on the
ownership fork (§6).

- ✅ **DONE (HIGH, `985e200`)** — `applyDraftingCommand` terminal `else` →
  `static_assert(always_false_v)`. Behavior-preserving; reviewer ACCEPT.
- ✅ **DONE (MED, `2a1be77`)** — the four unguarded geometry visits (Mirror,
  QuickMeasure, PlotPlan ×2) made compile-exhaustive by making each kind's current
  behavior EXPLICIT, then guarding. Behavior-preserving; reviewer ACCEPT + targeted
  ctest green. (See §2 for the per-visit detail + the Mirror two-list note.)
- ✅ **DONE (LOW, `cae383a`)** — `circleSegments=32` → `constexpr kCircleSegments`
  (one name for both the stroke-outline count and the fill-ring count; they must
  stay equal).
- ✅ **DONE (MED, `818736e` + fix `9ab72aa`)** — dedup the translation commands.
  `applyTranslationPlan(DraftingDocument&, const vector<DraftingTranslation>&)`
  (file-local) now runs the shared copy→loop-moveObject→commit; **AlignSelection +
  DistributeSelection delegate to it** (their old blocks were byte-identical —
  reviewer-confirmed). Variation is pure DATA (the translation vector), no
  subclassing. **MoveSelection deliberately KEEPS its own interleaved loop** and does
  NOT share the helper: its per-id `containsObject` guard must stay INTERLEAVED with
  the move because the FIRST failing id in selection order decides the rejection
  **message** — a `[present+locked, missing]` selection must surface "object is
  locked" (from `moveObject`), not the pre-scan's "selection target does not exist".
  message is observable upstream (`finishEdit`), so the interleaved order is
  load-bearing. (The first dedup attempt hoisted the guard and was sent back on
  exactly this; the regression is now pinned by a test in `drafting_commands_tests`.)
  Also note: the `containsObject` guard intercepts a missing id with
  `InvalidSelectionTarget` BEFORE `moveObject` would return `ObjectNotFound` — that is
  what distinguishes a stale selection target from a generic not-found.
- **DEFERRED to edi-dungeon-map (MAP region per ruling H2)** — the
  `highestDocumentIdSerial` rooms-comment clarification (`DraftingDocument.h:150-153`):
  not ours to edit.
- **NOTE (not a bug)** — circle is analytic on screen (`DrawingDocumentProjection.cpp:569-573`)
  but faceted at 32 segments on export (`DraftingPlotPlan.cpp:171,233`); ellipse is 64
  both places. SVG is internally consistent (stroke 32 == fill 32). A known
  screen-vs-plot fidelity choice, not a fix.

No dead code, no subclassing-for-behavior, no stateful logic objects, no JSON/qml
leakage found in the scanned scope.

## 6. Seams to other departments

### The `src/drafting` ownership boundary — HUB RULING H2 (2026-06-16), SETTLED
**By-domain, SINGLE document.** Do NOT split `DraftingDocument` or `DraftingCommand`
— a plug/room/connection is a relation over objects in the SAME drawing, so the
one-document data model stays.
- **edi-drafting owns:** the core geometry types + ops + the CORE command arms + the
  CORE regions of the shared headers — the `DraftingGeometry` variant, geometry ops,
  plot/export, the controller spine.
- **edi-dungeon-map owns:** the map graph — the whole-file set (`DraftingGraphOps`,
  `DraftingRoom`, `DraftingCorridor`, `DraftingPathfind`, `DraftingAsciiMap`,
  `DraftingBlockOps`), the 7 map command arms (CreatePlug, DeletePlug,
  DeclareConnection, DeleteConnection, CreateBlock, DeleteBlock, CreateMapRoom) +
  their semantics, and the map STRUCT/ENUM definitions (`DraftingPlug`/
  `DeclaredConnection`/`MapRoom`/`Block`; `DraftingPlugId`/`ConnectionId`/`BlockId`;
  `ObjectRole`, `WallType`, `WallVisualMetadata`, `BlockPlacementMetadata`).
  `WallGeometry` stays CORE — it rides every geometry visit; shared geometry, not
  map-only.
- **Shared headers** (`DraftingDocument.h`, `DraftingTypes.h`, `DraftingCommands.*`)
  are co-edited **by REGION, not by file**: drafting edits CORE regions, dungeon-map
  edits MAP regions, neither touches the other's region (disjoint lines → clean
  master merges). The `highestDocumentIdSerial` map-id scan is a MAP region.
- **dungeon-map's pending behavior-preserving slice:** extract the map struct/enum
  DEFINITIONS into a dungeon-map-owned `DraftingMapTypes.h` (included by
  `DraftingTypes.h`/`DraftingDocument.h`); the document KEEPS its vectors — only the
  definitions move. Shrinks the shared-edit surface to the include line + the vectors.

Map-specific citations (dungeon-map's region; listed so we know what NOT to touch):
  - Types in `DraftingTypes.h`: `DraftingPlugId`/`DraftingConnectionId` (:18-19),
    `ObjectRole` (:83-89), `WallType` (:96-101), `WallVisualMetadata` (:186-188),
    `BlockPlacementMetadata` (:195-199), `DraftingBlockId` (:23). (`WallGeometry`
    :310-314 is map-MOTIVATED but genuine shared geometry — stays core; it rides
    every geometry visit.)
  - `DraftingDocument.h` structs: `DraftingPlug` (:47-53), `DraftingDeclaredConnection`
    (:60-65), `DraftingMapRoom` (:73-79), `DraftingBlock` (:95-106); the document
    vectors plugs/connections/rooms/blocks (:116-122); `canvasPerAuthoredUnit` (:130);
    `highestDocumentIdSerial` (:150-153).
  - `DraftingCommand` arms (`DraftingCommands.h:151-184`, variant :214-220):
    CreatePlug, DeletePlug, DeclareConnection, DeleteConnection, CreateBlock,
    DeleteBlock, CreateMapRoom — dispatch at `DraftingCommands.cpp:322-335`.
  - Whole files: `DraftingGraphOps.*`, `DraftingRoom.*`, `DraftingCorridor.*`,
    `DraftingPathfind.*`, `DraftingAsciiMap.*`, `DraftingBlockOps.*`.
  - (The map graph is not file-separable — `DraftingDocument` embeds the map vectors
    and `DraftingCommand` embeds the 7 map arms — which is exactly why ruling H2 chose
    co-edit-by-region over a file split. Document/Command are NOT split.)
- **edi-blender-lab (reads, never writes our core):** `MeasurementMetadata` +
  `MeasurementUnit` (`DraftingTypes.h:156-160,68-76`), `ScaleCalibration`/
  `MeasurementCalibrationResult` (`DraftingMeasurement.h`), `DraftingQuickMeasureResult`
  (`DraftingQuickMeasure.h`), and Seam-B asset refs `DraftingBlock.assetRef`
  (`DraftingDocument.h:103`) + `BlockPlacementMetadata.assetRef` + ObjectRole/material/
  exportGroup (`DraftingTypes.h:213-219`).
- **edi-ui (shell belt/menu):** `DraftingToolKind` (`DraftingToolCreation.h:10`) +
  `DraftingInspectorPlan` (`DraftingInspectorPlan.h`) + the projection `QVariantMap`.
  Canvas interaction lives in `src/widgets/DrawingCanvas*` (our behavior, edi-ui's
  file ownership — coordinate).

## 7. Missing primitive — `transformGeometry` (DRAFTING-owned, per ruling H2)
CONFIRMED ABSENT — only a forward-looking comment at `DrawingCore.h:287`
("transformGeometry slice"). Rotate/scale over the 14 kinds belongs in
`DraftingGeometry.{h,cpp}` beside `translateGeometry` (a sibling guarded visit) and
is **edi-drafting-owned**; dungeon-map CONSUMES it (room/block placement transforms).
It is a FEATURE (parked in `~/dept-bus/ROADMAPS-DRAFT.md`) — do NOT build it during
the cartography campaign; this entry records the ownership only.
