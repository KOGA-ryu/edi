# Adding a drafting / drawing tool — the complete build context

A self-contained recipe for adding a tool to edi's drafting surface. A builder can
execute from this doc plus the two reference commits; everything is grounded in the
real tree at `115fc42`.

**Reference implementations — read these first:**
- `git show 36fdcc7` — **the arc primitive** (22 files). The canonical "new shape."
- `git show 2f7f287` — **the arrow** (6 files). The canonical "decoration on an
  existing shape."
- `git show 340fa7b` — **stroke opacity end-to-end**. The canonical "property on a
  selection" (fill follows this, not the tool recipe).

---

## Step 0 — decide which of four shapes your tool is

The build differs enormously by shape. Classify first.

| shape | examples | cost | recipe |
|---|---|---|---|
| **A. New primitive** (its own geometry) | ellipse, spline, text* | ~22 core + gate sites | §A, ref `36fdcc7` |
| **B. Decoration** (a flag on an existing geometry) | arrow, dashed-leader | ~6 files, no geometry ripple | §B, ref `2f7f287` |
| **C. Property on a selection** (not a "tool" at all) | fill, line style, color | inspector + render seams, **no belt** | §C, ref `340fa7b` |
| **D. Modify verb** (acts on existing objects) | trim, fillet, chamfer | pure op + command + pick interaction | §D |

\* Text is a primitive but heavier (content string + font metrics + an edit mode that
can lean on `edi_text_core`). Treat it as Shape A with an extra content field.

A prerequisite that several gaps share: a **pick-a-point interaction mode** (no such
mode exists today — the radial-array core already accepts a `center` the UI can't
supply by click, `DraftingArray.cpp`). Shapes C/D and user-picked centers need it;
build it as its own infrastructure slice before the verbs.

---

## §A — the new-primitive recipe (~22 core sites + the gate sites in Layer 2b)

Work top-down through the layers. The dispatch sites come in THREE shapes — `if constexpr`
visitors, `switch`-on-`DraftingShapeKind` (marked ⚠sw), and a couple of flat
`object.kind == Xxx` blocks (marked ⚠kf, e.g. `handleEditPlan`). **The compiler now guards
the first two.** Every `std::visit` over `DraftingGeometry` ends in
`else { static_assert(always_false_v<Geometry>, "<func>: unhandled geometry kind"); }`, and
the drafting core builds under `-Werror=switch` — so a forgotten visitor arm or a missing
switch case is a BUILD ERROR that NAMES the function (e.g. `computeBounds: unhandled geometry
kind — add an arm`). Add the variant arm, then let the red builds walk you through every
site. The exceptions the compiler can't see: the `⚠kf` flat blocks and the string name-maps
(`shapeKindFromName`) — but `drafting_shape_kind_exhaustive_tests` covers the name-maps by
construction, so a forgotten entry fails a test. The checklist below is your map of what the
compiler (and that test) will make you fill in.

### Layer 1 — the data · `src/drafting/DraftingTypes.h`
1. The geometry struct: `struct XxxGeometry { … };` (plain fields, sane defaults).
2. `enum class DraftingShapeKind` — add `Xxx`.
3. `using DraftingGeometry = std::variant<…>` — add `XxxGeometry` as an arm.
4. `template <> constexpr DraftingShapeKind shapeKindOf<XxxGeometry>()` specialization.
5. **Bump** `static_assert(std::variant_size_v<DraftingGeometry> == N)` beside the variant
   (it exists now, currently `== 10`). This is the TRIPWIRE: you can't add an arm without
   the build failing here, which kicks off the guided process — each `std::visit` site then
   fails its own named `static_assert(always_false_v<Geometry>, …)`, and `-Werror=switch`
   flags the enum switches.

### Layer 2 — the pure ops (the exhaustive sites)
- `DraftingGeometry.h` — declare any **shared sampler/helpers** (e.g. the arc's
  `sampleArc` / `arcPointAtAngle`). **Rule:** one helper owns the tessellation + angle
  convention; bounds, projection, painter, and plot-flatten all call it, or they drift.
- `DraftingGeometry.cpp` — `shapeKindName` (⚠sw), **`shapeKindFromName` (a name→kind `if`-chain, ~line 117 —
  it lives HERE, moved from Serialize post-arc)**, `geometryKind`, `validateGeometry`
  (finite + domain checks, reject by name), `computeBounds` (exact, not the loose
  sampled box where a closed form exists — see `arcBounds`), `translateGeometry`,
  `handleAnchors`, plus the shared helper bodies.
- `DraftingHitTest.cpp` — `hitDistance` arm.
- `DraftingSnap.cpp` — `snapCandidatesForObject` arm (which snap sources the shape
  offers — center, endpoints, etc., each gated by its `settings.*Enabled`).
- `DraftingObjectEdit.h` — `enum class DraftingObjectEditKind` — add the per-handle
  edits (e.g. `MoveArcCenter`, `SetArcRadius`, …).
- `DraftingObjectEdit.cpp` — three sites: `draftingHandlesForObject` (handle id + role
  + position), `handleEditPlan` (⚠kf — a flat `object.kind == Xxx` block, NOT an
  if-constexpr; handle → edit-kind + value), `applyObjectEdit` (edit-kind → mutate).
- `DraftingNumericEdit.cpp` — `applyNumericGeometryEdit` arm (field id → geometry field;
  reject unknown fields by name).
- `DraftingPlotPlan.cpp` — `appendPlotSegments` arm (flatten to pen segments via the
  shared sampler; open chain vs closed loop).
- `DraftingSerialize.cpp` — **three** sites: `isKnownShapeKindName`, `geometryValue`
  (write the fields), `readGeometry` (⚠sw; read with **tolerant defaults** so an
  old/short record still loads). NOTE: `shapeKindFromName` is NOT here — it lives in
  `DraftingGeometry.cpp` (above).

### Layer 2b — gate / verify sites (the `CircleGeometry` cousins)
Five files special-case closed/curved shapes by referencing `CircleGeometry`; a new
primitive must EITHER implement them OR confirm it falls through gracefully (refuses by
name). **The arc gated OUT of all of these** — for those, this is verify-the-fall-through,
not new code:
- `src/drafting/DraftingMirror.cpp` (`supportsMirror` + the mirror arm)
- `src/drafting/DraftingMeasurement.cpp`
- `src/drafting/DraftingCalibration.cpp`
- `src/drafting/DraftingQuickMeasure.cpp`
- `src/recipe/RecipeMeasure.cpp`
These are beyond the ~22-file core ripple: +1 file each if you implement, or
verify-the-gate if you don't. Implementing mirror for the new shape = editing
`DraftingMirror.cpp` (the 23rd file). Confirm the gate with a test, not by eye.

### Layer 3 — the tool (creation) · `src/drafting/DraftingToolCreation.{h,cpp}`
- `.h` — `enum class DraftingToolKind` add `Xxx`; add any tool-option field to
  `DraftingToolCreationRequest` (e.g. `arcSweepDeg`).
- `.cpp` — `draftingToolKindFromId` (`"xxx_tool"` → kind; **this string id is the
  bridge to the belt**), `draftingToolKindName`, and `buildDraftingObjectForTool`
  (turn the request's `start`/`end` clicks into the geometry).
- `src/widgets/DrawingCanvasWidget.cpp` — if it's a two-click tool, add `"xxx_tool"`
  to `isTwoClickCreationTool` (one line). **The controller gesture is generic** —
  `clickCanvasNormalized` already routes single-click / two-click / polyline tools
  through `buildDraftingObjectForTool`; do **not** add a controller branch unless the
  gesture is genuinely new (a third click, a drag, etc.). If the tool has options,
  carry them into `m_pendingCreation` beside `polygonSides`/`rectCornerRadius`/
  `fixedRadius` (see `DrawingDocumentController.cpp` ~line 2172).

### Layer 4 — projection + render
- `src/core/DrawingDocumentProjection.cpp` — three sites: `numericFieldsForObject`
  (the inspector geometry-editor fields — this is why a new primitive's fields appear
  in the inspector **automatically**, no `DraftingInspectorPlan` change needed),
  `physicalGeometryForObject` (measured/physical values), and
  `draftingObjectToCanvasProjection` (the canvas map; for a curved/closed shape also
  emit a flattened `points` list via the shared sampler so the painter draws it
  directly). ⚠sw `numericFieldsForObject`. NOTE: these are the per-object GEOMETRY/canvas
  projection; per-object STYLE keys (`own_stroke_*`, and fill in §C) are inserted in a
  DIFFERENT function, `draftingDocumentToModelProjection` — don't conflate them. Also
  check any field-id allow-list that gates physical-unit conversion (circle's `radius`
  sits in one such list) and add the new shape's field ids.
- `src/widgets/DrawingCanvasObjectPainter.cpp` — `drawObject` (committed) and
  `drawPreviewObject` (live preview). Reuse the open-chain/`points` path where you can
  (the arc did). **Decorations and handles are sized in SCREEN space, geometry in data
  space.**

### Layer 5 — belt surfacing (CURRENT tree) · `src/widgets/DraftingFeaturePanels.cpp`
- One row in `kDraftingTools[]`: `{"xxx_tool", "Xxx", "Xx", <beltRow>}`.
- One branch in `draftingToolFace` authoring the cell icon as unit-space coordinates
  ([0,1]², y-down: `polylines` / `ellipses` / `dots`). Skip it → automatic text-glyph
  fallback.
- **Nothing else.** `toolInventory()` (settings checklist), `defaultBeltLayout()`, and
  `beltLayoutForTools()` all derive from `kDraftingTools`. ⚠ The arc commit edited
  `EdiShellWindow.cpp buildLeftPanel` — **that left-panel tool list was retired**
  (`9fd569be`); ignore that hunk, the belt is the surface now.

### Layer 6 — build + tests
- `CMakeLists.txt` — `add_edi_contract_test(drafting_xxx_tests tests/drafting_xxx_tests.cpp edi_drafting_core)`.
- `tests/drafting_xxx_tests.cpp` (new) — validate / bounds / translate / hit / handles /
  edit / flatten, each **mutation-pinned** (a deliberate break must abort the suite).
- Extend: `drafting_serialize_tests.cpp` (add to the every-kind round-trip),
  `drafting_tool_creation_tests.cpp` (the tool builds the geometry),
  `drafting_document_query_tests.cpp` (kind enumeration),
  `drawing_canvas_widget_tests.cpp` (click-create + drag a handle).
- **The belt pin** (new, current tree): assert the tool id resolves to a real belt cell
  (not `"?"`) and appears in `toolInventory()` and `defaultBeltLayout()` — so "in the
  table ⇒ on all three surfaces" can't silently break.

---

## §B — the decoration recipe (a flag on an existing shape)

Reuses an existing geometry — **no** bounds/hit-test/snap/edit/plot ripple. Touch:
- `DraftingTypes.h` — add the flag to the relevant `*VisualMetadata` struct (e.g.
  `LineVisualMetadata.endArrow`), beside the other per-object visual flags.
- `DraftingToolCreation.{h,cpp}` — a tool kind whose creation sets the flag on an
  otherwise-normal base shape.
- `DraftingSerialize.cpp` — serialize the flag with a tolerant default.
- The **visual** (drawing the decoration) is its own small slice in
  `DrawingCanvasObjectPainter` + the projection, screen-space sized (the arrowhead
  shipped separately in `89e1a55`).
- Belt + tests as in §A Layers 5–6.

---

## §C — the property recipe (fill, line style, color) — NOT a belt tool

A property on a *selected* object. No `kDraftingTools`, no geometry, no creation
gesture. Follow `340fa7b` (stroke opacity), mirroring the seams stroke already travels:
- a command (`UpdateFillStyleCommand`) applied via `applyDraftingCommand`; locked objects
  reject it — and **add `FillStyle::operator==`** (stroke has one at `DraftingTypes.h:107`,
  FillStyle does NOT) so that arm has its no-op guard;
- controller setters (`setSelectedObjectFill*`) through the resolve→plan→apply→emit helper;
- add `fillColor` / `fillOpacity` to the `DrawingCanvasProjectedStyle` struct
  (`DrawingCanvasProjectedObject.h`) and extract + clamp them in `projectedObjectStyle`
  (`DrawingCanvasProjectedObject.cpp` — the THIRD clamp site, beside the painter and
  `readFill`); then the painter reads them into a `QBrush`, **gated to CLOSED shapes only**
  (no `isClosed` helper — gate by kind: rectangle / circle / polygon — in both the painter
  and the command). The painter is currently fill-less, so the brush is **net-new**, not a
  re-wire;
- **SVG / plot fill is a SEPARATE, harder slice — NOT a key addition.** Unlike stroke
  opacity (which rode the existing path-group key), the SVG/plot model is a flat list of
  OPEN stroke segments (`DraftingSvgOut.cpp`, `job.strokeSegments`, `fill="none"` hardcoded)
  with no closed-region representation. A filled region is net-new closed-path emission. Ship
  canvas-brush fill as v1 and DEFER SVG/plot fill to its own slice (pen plotters can't fill);
- the inspector Style group is TWO files: BUILD the fill color + opacity rows in
  `DraftingFeaturePanels.cpp`, AND refresh them back (push `own_fill_*` into the spins on
  selection) in `DraftingFeatureInspector.cpp` — miss the second and the spin never updates
  on selection. (A property does NOT auto-surface like a primitive's numeric fields.);
- byte-identity is **stronger than a sentinel**: the `fill` key is ALREADY written and read
  today (`DraftingSerialize.cpp`), so §C adds NO new serialized field — existing `.edidraw`
  files already carry `fill:{0.0, …}` and stay byte-identical because the format is unchanged.
  `readFill` already clamps. Fill #0 in `docs/drafting-gaps.md` is this recipe.

---

## §D — the modify-verb recipe (trim, fillet, chamfer)

- A pure free function in `src/drafting/` (`DraftingTrimOps` etc.), value-in/value-out,
  golden-testable — the geometry math (line–line intersection, fillet arc) lives here.
- A `DraftingCommand` to apply the result; orchestration grows the existing
  kind-and-callable controller helper, not a new inlined sequence.
- A **pick interaction** (the prerequisite mode): the verb is modal — pick the tool,
  then click the object(s) it acts on. Decide per verb whether it's a belt tool (modal)
  or an Edit-menu action.

---

## The discipline (every shape, non-negotiable)

- **Behavior-preserving by default.** New serialized fields default so every existing
  `.edidraw` loads byte-identically; decode is tolerant (missing field → default).
- **Variation as data.** Pay the variant tax only for a true new *shape*; a decoration
  is a flag. Add the `static_assert` on variant size.
- **Mutation-pinned.** No assert ships until a deliberate break makes it fail (exit
  134). Render/paint changes need a **pixel probe** (`edi --snapshot --probe`), not a
  property read — a stylesheet/paint can be silently wrong while every property looks
  right.
- **Numbers are a contract.** `from_chars`/`to_chars`, deterministic formatting, byte
  goldens for serialization (see `docs/lessons/08-numbers-are-a-contract.md`).
- **Gate gracefully.** Features that don't support the new primitive (mirror, measure,
  calibration) should refuse it by name, not half-implement it (the arc did this).
- **Build loop before commit:** `cmake --build build` && `ctest --test-dir build
  --output-on-failure` green, plus the format scan (no `.js`/`.qml`, no `.json` outside
  `.claude/`, no QtQml). In mutate/restore cycles, **delete the `.o`** first — mtime
  granularity will run a stale object through a green suite (field note; it has bitten
  five times).

## Acceptance (per tool)

1. The tool appears on the belt *and* the settings Tool Belt checklist (pinned).
2. Its gesture creates the shape; it auto-selects; the live preview renders.
3. Round-trips through `.edidraw` byte-stable; pre-existing files are unaffected.
4. Hit-test, snap, handles, numeric edit, plot, and SVG all work for the new shape.
5. The new per-kind test is green and mutation-pinned; the four extended tests pass.
6. Full suite green; format scan clean.

## Traps that bit here (don't pay twice)

- The exhaustive `if constexpr` chains fall through **silently** — use the Layer-2
  checklist, don't trust the compiler.
- One shared sampler/angle helper, or bounds/painter/plot disagree.
- A new serialized field must join every grouping/dedup key (the SVG path-group bug).
- Screen-space for handles/decorations, data-space for geometry.
- The belt surface is `kDraftingTools`, not the retired left panel.
- `.o` deletion in mutate/restore cycles.

---

## Worked example — ellipse (Shape A), the template filled in

- **Geometry:** `struct EllipseGeometry { Point2D center; double rx = 0.0; double ry =
  0.0; };` (axis-aligned for v1; rotation is a later field, defaulted 0 → existing
  files unaffected).
- **Tool:** `ellipse_tool`, two-click (center → corner). `buildDraftingObjectForTool`:
  `rx = |end.x - start.x|`, `ry = |end.y - start.y|`, clamped like circle's
  `std::min(1.0, …)`. Add to `isTwoClickCreationTool`.
- **Ops:** bounds = `center ± (rx, ry)`; hit = distance to the sampled outline; snap =
  center + four quadrant points; handles = center + an rx handle + an ry handle; flatten
  = `sampleEllipse` (new shared helper). Validate finite + non-negative radii.
- **Render:** committed via `QPainter::drawEllipse(center, rx*board, ry*board)`; preview
  via the flattened `points` (mirror circle/arc).
- **Serialize:** `center`, `rx`, `ry`. **Numeric fields:** `cx`, `cy`, `rx`, `ry`.
- **Belt:** `{"ellipse_tool", "Ellipse", "El", 4}` (shares circle's row 4) + a
  `draftingToolFace` branch `face.ellipses = {QRectF(0.1, 0.3, 0.8, 0.4)}`.
- **Gate (Layer 2b):** for v1, gate the ellipse OUT of mirror / measure / calibration /
  quick-measure exactly as the arc did (refuse by name) — verify-the-fall-through, not new
  code. If you choose to implement mirror, that's a +1 file (`DraftingMirror.cpp`: an
  `EllipseGeometry` arm + a `Circle`-style `supportsMirror` entry) beyond the core ripple.
- **Tests:** `tests/drafting_ellipse_tests.cpp` + the four extensions + the belt pin.

This is one slice. Build it, mutation-pin it, run the loop, then the next tool reuses
the same checklist.
