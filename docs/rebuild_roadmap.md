# Rebuild roadmap

Implementation spec for restoring (and surpassing) the scrapped version's
capabilities. Written to be executed cold by a model with no prior context.
Behavioral reference: `docs/legacy_inventory.md` (mined from git history at
`ce0b751` — read old files with `git show ce0b751:<path>`, never check out).

## Ground rules (read before any phase)

1. Read `CLAUDE.md` first. Hard rules: **no JSON, no `.js`/`.qml`,
   data-oriented design** (plain structs + free functions in `src/drafting/`
   style; variation as data or plan callables; no subclassing for behavior).
2. Confirm repo identity before working: `git rev-parse --show-toplevel` must
   print `/Users/kogaryu/edi`. Never touch
   `/Users/kogaryu/draft/draftsman_STALE_PARENT_DO_NOT_USE`.
3. Verify loop per commit: `cmake --build build` clean →
   `ctest --test-dir build --output-on-failure` fully green → no `.js`/`.qml`
   anywhere, no `.json` outside `.claude/` → read the diff. Commit style:
   `claude: <imperative summary>` + body + Co-Authored-By trailer.
4. Every new test target gets one **mutation check**: sabotage the code under
   test, confirm the test aborts, restore. Force hard rebuilds around the
   mutate/restore (delete the target's `.o` and binary) — fast cycles land
   within make's mtime granularity and silently run stale binaries.
5. Shell-test gotcha: `rebuildGeometryEditor` retires widgets with
   `deleteLater()`; flush `QEvent::DeferredDelete` before `findChildren` lookups
   (see `geometryFieldSpin` in `tests/edi_shell_window_tests.cpp`).
6. These phases are **features**: behavior-additive is expected, but never
   change existing behavior silently — when a phase forces a change to existing
   semantics, say so in the commit body.

Architecture orientation (current state): pure data core in `src/drafting/`
(`DraftingDocument` + `DraftingCommand` variant applied by
`applyDraftingCommand`); thin Qt controller `DrawingDocumentController`
(declared in `src/core/DrawingCore.h`) that resolves → plans → applies → emits
`modelChanged`; projections to `QVariantMap` in
`src/core/DrawingDocumentProjection.cpp`; widgets shell `EdiShellWindow` +
`DrawingCanvasWidget` (+ `DrawingCanvas*` family). Tests: 51 targets, plain
`assert` + `int main`, registered via `add_edi_contract_test` or explicit
`add_executable` blocks in `CMakeLists.txt`; widget tests run on
`QT_QPA_PLATFORM=offscreen`.

---

## Phase R1 — MessagePack value codec + drawing save/open

**Goal:** durable drawing documents. Ctrl+S / Ctrl+O round-trips the full
document through a versioned MessagePack file.

**Critical context:** `src/formats/` MessagePack code is an *envelope
scaffold*, not a general codec — `MessagePackRecordSet` carries only
schema/version/recordCount with magic-byte framing (`EDIM`, see
`MessagePackInspector.h`). There is no value encoder. Build one.

**Read first:** `src/formats/MessagePack*.{h,cpp}`, `src/formats/FormatResult.h`,
`src/drafting/DraftingDocument.h`, `src/drafting/DraftingTypes.h`,
`src/io/DrawingDocumentStore.{h,cpp}` (stub with working `writeTextFile` /
`localPath` helpers), `tests/format_messagepack_tests.cpp`.

**Design:**
- `src/formats/MessagePackValue.{h,cpp}`: a `MsgPackValue` plain-data variant
  (nil, bool, int64, double, string, vector<MsgPackValue>,
  vector<pair<string, MsgPackValue>> as map) + `encodeMessagePack(value) ->
  ByteBuffer` and `decodeMessagePack(bytes) -> FormatResult<MsgPackValue>`
  implementing the real msgpack wire format (at minimum: nil, true/false,
  int64 family, float64, str8/16, array16/32, map16/32, bin8 if needed). Pure
  free functions, zero Qt. Extend `edi_format_core` in CMake.
- `src/drafting/DraftingSerialize.{h,cpp}` in `edi_drafting_core`:
  `MsgPackValue draftingDocumentToValue(const DraftingDocument &)` and
  `FormatResult<DraftingDocument> draftingDocumentFromValue(const MsgPackValue &)`.
  Schema: top-level map `{schema:"edi.drawing", version:1, document:{...}}`
  wrapped in the existing EDIM envelope (reuse the magic framing from
  `MessagePackWriter.cpp`). Serialize: layers (id, name, order, visible,
  locked, plot{penId, strokeColor, strokeWidth, plotEnabled}), objects (id,
  kind name via `shapeKindName`, layerId, visible, locked, geometry per
  variant alternative, bounds recomputed on load not stored, metadata fields
  — enumerate `ObjectMetadata` in `DraftingTypes.h:119`), activeLayerId,
  selectedObjectIds, activeObjectId, revision. Unknown-field policy: ignore on
  read (forward compatibility); missing required → rejected FormatResult.
- Controller: `bool saveDocument(const QUrl &url)` / `bool openDocument(const
  QUrl &url)` on `DrawingDocumentController` — serialize via the above, file
  I/O through `DrawingDocumentStore` (replace its stubbed `save`/`open`; binary
  write needs a `writeBinaryFile` sibling of `writeTextFile` using `QSaveFile`).
  On open: replace `m_document`, reset `m_nextObjectSerial` above the highest
  numeric suffix found in object ids, clear pending creation/preview, emit.
- Shell: wire the left panel's "Project files" placeholder into Save / Save As
  / Open buttons via the `makeActionButton` factory + `QFileDialog`
  (`getSaveFileName`/`getOpenFileName`, filter `*.edidraw`), plus
  `QShortcut(QKeySequence::Save/Open)` on the window. Track current file path
  + dirty state (document revision vs last-saved revision) in the window title.

**Tests:** `format_messagepack_value_tests` (encode/decode round-trip per type,
boundary ints, truncated-buffer rejection); `drafting_serialize_tests`
(document round-trip equality — every field; unknown-field tolerance; bad
schema rejected). Both via `add_edi_contract_test`. Shell test: save to a
QTemporaryDir, mutate, open, assert projection equals saved state. Mutation
check: corrupt one encoded byte → decode rejects.

**DoD:** round-trip of a document containing every object kind + 2 layers +
selection is byte-stable across save/open/save; suite green; mutation-checked.

---

## Phase R2 — Undo/redo

**Goal:** Ctrl+Z / Ctrl+Y (and Edit buttons) across every document mutation.

**Read first:** `DrawingDocumentController.cpp` — grep `applyDraftingCommand(`;
mutations flow through `applyCommandAndEmit` plus a handful of direct calls
(`createObjectsAndSelect`, `createTransformedActiveObject`,
`applyGuidePreset`, `recordCalibrationMeasurement`, `clickCanvasNormalized`,
`applyFieldEdit`, `selectObjectsInBoundsNormalized`, `moveSelectionNormalized`).

**Design (snapshot stack — the document is plain copyable data, exploit it):**
- Controller members: `std::vector<DraftingDocument> m_undoStack`,
  `m_redoStack`; cap 100 (drop oldest).
- One choke point: private `DocumentEditScope` RAII guard (or begin/commit pair)
  — captures a copy of `m_document` on entry; on exit, if
  `m_document.revision` changed, push the captured copy onto undo and clear
  redo. Wrap every public mutating entry point (the grep list above) — NOT the
  internal helpers, so one user action = one undo step even when it applies
  several commands (e.g. `applyGuidePreset`, repeat's create loop).
  Selection-only commands (`SelectObjectCommand` etc.) bump revision? Check
  `applyDraftingCommand` — if selection changes don't bump revision, decide and
  document: recommended to EXCLUDE pure selection changes from undo (matches
  most CAD).
- `bool undo()` / `bool redo()`: move current document to the opposite stack,
  restore, clear pending creation/preview/lastEditStatus, emit `modelChanged`.
  `bool canUndo()/canRedo()` for UI enablement.
- Shell: Undo/Redo buttons (conditional-button registry pattern in
  `EdiShellWindow` — see `makeConditionalButton`; add a non-projection enable
  source or refresh from `canUndo()` in `refreshInspector`), shortcuts
  `QKeySequence::Undo/Redo`.
- Undo interacts with R1 dirty tracking: dirty = revision != lastSavedRevision
  still works since undo restores the old revision value.

**Tests:** controller test (extend `drawing_document_controller_tests`):
create→undo→empty→redo→restored; nudge twice→undo once→one nudge left;
guide preset = single undo step; cap behavior; redo cleared by new edit.
Mutation check: make the guard never push → assertions abort.

**DoD:** every mutating public method is undoable as one step; 100-step cap;
suite green; mutation-checked.

---

## Phase R3 — Zoom, pan, and the keyboard map

**Goal:** restore old navigation semantics (legacy_inventory §lost): Ctrl/Cmd+
scroll zoom at cursor (factor `pow(1.0015, angleDelta)`), plain-scroll or
middle-drag pan, and the keyboard map: Esc cancel, Del/Backspace delete,
Ctrl+D duplicate, arrow nudge (plain=grid, Alt=fine, Shift=large).

**Read first:** `src/widgets/DrawingCanvasViewport.{h,cpp}` (+ its tests — the
pure layer where zoom/pan math lives), `DrawingCanvasWidget.cpp` mouse
handlers, `DrawingCanvasGestureState.{h,cpp}` (an unused `Panning` mode +
`beginPan`/screen-point tracking already exist), `nudgeSelection` in the
controller (step modes already exist: "grid"/"fine" — check exact mode ids in
`draftingNudgeScaleForMode`).

**Design:**
- Viewport (pure, tested): `DrawingCanvasViewportInput` gains `zoom` (default
  1.0, clamp [0.2, 16.0]) and `panXPx`, `panYPx`. `viewportBoardRect` applies:
  fit-rect as today, then scale by zoom and offset by pan. Add
  `zoomAtPoint(input, factor, anchorPx) -> input` keeping the anchor's canvas
  point stationary (solve pan from the anchor invariance equation — pure
  function, unit-test it).
- Widget: members `m_zoom`, `m_pan`; `wheelEvent` (Ctrl/Cmd → zoomAtPoint at
  cursor; plain scroll → pan by pixelDelta/angleDelta); middle-button press →
  `beginPan` gesture, move → update pan, release → finish. All repaint via
  `update()`; controller untouched (navigation is view state, not document
  state — do NOT put zoom/pan in DraftingDocument).
- Keyboard: widget `keyPressEvent` (focus policy is already ClickFocus):
  - Esc: cancel pending two-click creation + preview. Add controller
    `void cancelPendingCreation()` (resets `m_pendingCreation`,
    `m_previewObject`, emits if either was set).
  - Del/Backspace: controller `bool deleteSelectedObject()` — generalize the
    existing guide-only delete: `DeleteObjectCommand` on the active object id
    (verify the command handles non-guide kinds; if it is guide-specialized,
    extend it in `DraftingCommands.cpp` and cover in `drafting_commands_tests`).
  - Ctrl+D: controller `bool duplicateSelectedObject()` — copy active object,
    fresh id via `nextObjectId(QStringLiteral("copy"), ...)`, small offset
    (+0.02, +0.02), create+select (reuse `createTransformedActiveObject`).
  - Arrows: `nudgeSelection(direction, mode)` with mode from modifiers
    (plain → "grid", Alt → "fine", Shift → large — if no large mode exists in
    `draftingNudgeScaleForMode`, add one, scale 4.0).
- Status overlay and rulers can come later; not in this phase.

**Tests:** viewport unit tests for zoom clamp, pan offset, anchor invariance
under zoomAtPoint. Canvas widget test (extend
`drawing_canvas_widget_tests.cpp`): synthesize `QWheelEvent` with Ctrl →
assert a known canvas point maps to the same screen point before/after zoom;
synthesize `QKeyEvent`s: Escape clears `preview_object` from the model,
Delete removes the selected object, Ctrl+D adds one, arrow moves selection by
the grid step. Mutation check: break anchor invariance → test aborts.

**DoD:** zoom/pan/keyboard work in tests; document state never contains
navigation; suite green; mutation-checked.

---

## Phase R4 — Arc and regular polygon tools

**Goal:** the two lost geometry tools (legacy defaults: arc start 15°, end
120°; polygon 6 sides, rotation 30°).

**Read first:** `src/drafting/DraftingTypes.h` (geometry variant),
`DraftingToolCreation.{h,cpp}` (tool ids → `DraftingToolKind`,
`creationRequest`, `buildDraftingObjectForTool`),
`clickCanvasNormalized`/`isTwoClickCreationTool` flow, and one full "ripple"
example: git log the commits that added DimensionGeometry, or grep a kind
through every switch.

**Design — polygon first (no new variant alternative; pure data win):**
- Tool `regular_polygon_tool` ("Polygon" in the left panel): two clicks =
  center then radius point. Creation computes `PolygonGeometry` vertices:
  `sides` from a tool-options control (default 6, range 3–24), vertex i at
  `center + radius * (cos, sin)(rotationDeg + i*360/sides)`, rotation default
  30°. Everything downstream (paint, hit test, plot, projection points) already
  handles PolygonGeometry. Tool options UI: a small spin box in the left panel
  ("Sides"), value passed through a controller setter + used at build time —
  follow the snap-spinner precedent in `buildLeftPanel`.
- **Arc (new variant alternative — the full ripple checklist):**
  `ArcGeometry {Point2D center; double radius; double startAngleDeg; double
  endAngleDeg;}` added to the `DraftingGeometry` variant +
  `DraftingShapeKind::Arc`. Then update every exhaustive site — grep for a
  sibling kind (`CircleGeometry`/`Circle`) and mirror at each hit:
  `shapeKindName`, `geometryKind`, `kindMatchesGeometry`, `shapeKindOf<>`
  specialization (+ static_asserts in `drafting_document_query_tests`),
  `validateDraftingObjectShape`, bounds computation, translate/move,
  `draftingHandlesForObject` (center / radius-at-mid-angle / two endpoint-angle
  handles), `handleEditPlan`, hit test (distance to arc within angular span),
  `numericFieldsForObject` + numeric edit (cx, cy, radius, start_angle_deg,
  end_angle_deg), physical projection, canvas painter (`QPainter::drawArc` —
  note Qt angles are 1/16° and y-down; convert explicitly and comment the
  convention), plot flattening (sample to segments at ~2° steps in
  `DraftingBuildPlan`/plot plan — find where circles flatten and mirror), SVG
  path when R5 lands. The compiler is the checklist: after adding the variant
  alternative, every non-exhaustive `if constexpr` chain still compiles, so
  ALSO grep `std::is_same_v<Geometry,` and `DraftingShapeKind::Circle` to
  enumerate sites — do not trust compilation alone.
- Tool: `arc_tool`, two clicks = center then radius/start-angle point; end
  angle from tool-option (default sweep 105° matching legacy 15→120) — or
  three-click (center, start, end) if cleaner; pick one, document it.
- Left panel: add both to the `tools` spec table in `buildLeftPanel`.

**Tests:** new `drafting_arc_tests` (validate/bounds/move/hit/handles/flatten);
extend tool-creation, numeric-edit, projection, and canvas-widget tests
(click-create arc + polygon, drag an arc handle). Mutation check: flatten step
sabotage → arc test aborts.

**DoD:** both tools creatable by mouse in the widget test, editable via
handles + numeric fields, plot-flattened, all exhaustive sites covered;
suite green.

---

## Phase R5 — Plot output: SVG export + HPGL emitter

**Goal:** the app can finally produce output a plotter (or browser) consumes.

**Read first:** `src/drafting/DraftingPlotJob.h` (ordered
`strokeSegments`/`travelSegments`, layer/pen stats, bounds, calibration —
90% of the pipeline exists), `src/core/DrawingSvgExport.cpp` (11-line stub),
`DrawingDocumentStore::exportSvg` (file plumbing already works),
`src/tools/EdiPlotJobReport.cpp` (CLI consumer of the plot job — pattern for a
second emitter), grid unit handling in `DraftingGrid.*`.

**Design:**
- `src/drafting/DraftingSvgOut.{h,cpp}`: `std::string svgFromPlotJob(const
  DraftingPlotJob &job, const DraftingGridProjection &grid)` — one `<path>`
  per pen/layer group from stroke segments (travel segments excluded),
  `viewBox` from grid physical size, stroke colors from layer plot styles.
  Pure function, no Qt. Wire `DrawingSvgExport` + a shell "Export SVG" button
  (Project files section) through `DrawingDocumentStore::exportSvg`.
- `src/drafting/DraftingHpglOut.{h,cpp}`: `std::string hpglFromPlotJob(const
  DraftingPlotJob &job, const DraftingHpglSettings &settings)`.
  `DraftingHpglSettings {double plotterUnitsPerMm = 40.0; bool returnHome =
  true;}`. Emission: `IN;` → per pen group `SP<n>;` (pen index = order of
  first appearance in `penStats`, document the mapping in a leading comment
  line? No comments in HPGL — put the mapping in the report instead) → for
  each stroke chain `PU<x>,<y>;PD<x>,<y>[,<x>,<y>...];` chaining consecutive
  segments that share endpoints → final `PU;SP0;` (+ `IN;` if returnHome).
  Coordinates: normalized → physical mm via grid width/height and unit
  conversion (find the existing unit-to-mm conversion in `DraftingGrid` /
  physical projection; if only labels exist, add a
  `double millimetersPerUnit(DraftingGridUnit)` table — canvas_unit maps 1:1
  and must be documented as device-dependent) → × `plotterUnitsPerMm`,
  rounded to integers, y-axis NOT flipped (HPGL origin bottom-left vs screen
  top-left — flip y: `y_hpgl = heightMm*40 - y`; verify against a known-good
  HPGL viewer and say which convention was chosen in the commit).
- Shell: "Export HPGL" button beside Export SVG, extension `.hpgl`, via a new
  `DrawingDocumentStore::exportText(url, text)` (rename/generalize the
  existing `exportSvg` plumbing).

**Tests:** `drafting_svg_out_tests` + `drafting_hpgl_out_tests` — golden
small-document checks (one line + one circle on two pens): assert exact
emitted string; pen mapping; y-flip; segment chaining. Mutation check: break
the y-flip → golden test aborts.

**DoD:** exporting a two-layer document produces SVG that renders and HPGL
with correct pen separation and travel moves; suite green.

---

## Phase R6 — TOML settings persistence

**Goal:** grid/snap/plot settings, recent files, and window geometry survive
restarts. (Theme/panel-layout settings wait until those UIs exist.)

**Read first:** `src/formats/TomlReader.h` (`StaticConfig =
std::map<std::string,std::string>` — flat string map; reader/writer exist and
are tested), controller settings members (`m_gridSettings`, `m_snapSettings`,
`m_plotSettings`), `DrawingRecentFilesStore` stub.

**Design:**
- `src/io/SettingsStore.{h,cpp}`: load/save a `StaticConfig` at
  `<QStandardPaths::AppConfigLocation>/edi.toml` via the existing
  reader/writer. Dotted keys, stringified values:
  `grid.preset`, `grid.unit`, `grid.width`, ... `snap.grid_enabled`,
  `snap.object_tolerance_preset`, `plot.order_mode`, `plot.direction_mode`,
  `window.width/height`, `recent.0..recent.9`. Typed get/set helpers
  (`double settingsDouble(config, key, fallback)`) as free functions —
  tolerate missing/garbage values with fallbacks (same spirit as
  `finiteNumber`).
- Controller: `QVariantMap settingsSnapshot()` / `void applySettings(...)` or
  individual existing setters driven from the shell at startup; saving
  triggered on `modelChanged` debounce is overkill — save on window close +
  after each settings-affecting action (the setters are cheap; a 250ms
  QTimer debounce in the shell is acceptable view-side state).
- Recent files: maintained by R1's save/open, persisted in the same file;
  surface as buttons in the Project files section (most recent 5).

**Tests:** `settings_store_tests` — round-trip, missing-file defaults,
garbage-value fallbacks, recent-list ordering/dedup/cap. Shell test: change a
snap toggle, save, rebuild window from the file, assert the toggle state.
Mutation check: drop the fallback path → garbage-value test aborts.

**DoD:** settings round-trip across a simulated restart in tests; no JSON
anywhere; suite green.

---

## Sequencing and dependencies

R1 → R2 (dirty-tracking interplay) → R3 (independent of R1/R2 but keyboard
Delete/duplicate want R2 so mistakes are undoable) → R4 → R5 (R4's arcs must
flatten before R5 goldens include them — or land R5 first with line/circle
goldens and extend) → R6 (depends on R1's recent files). Each phase is
several commits: land core (pure functions + tests) before controller before
shell, verifying the full loop at every commit.
