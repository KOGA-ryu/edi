# Tool change-log — what each tool addition actually touched

The empirical record of every site a tool addition changes, per recipe shape —
built by adding tools **by hand** so a future `tools/scaffold_tool.py` can be
designed from evidence, not guesswork. Each entry is the exact file → site list,
plus the one thing that matters most for a generator: **which sites the compiler
guards, and which it does not.**

## The compiler-guard map (why it matters for a scaffolder)

| site class | guarded? | so a generator… |
|---|---|---|
| a new `DraftingGeometry` variant arm (bounds, hit, snap, edits, serialize-write, …) | **YES** — `always_false_v` static_assert per `std::visit` + `static_assert(variant_size==N)` | can be best-effort; a miss is a named build error |
| a `switch` over `DraftingShapeKind` / `DraftingToolKind` (shapeKindName, draftingToolKindName, readGeometry, numericFieldsForObject) | **YES** — `-Werror=switch` on edi_drafting_core | a miss is a build error (in the core) |
| the string name-maps (shapeKindFromName, isKnownShapeKindName, draftingToolKindFromId) | **partial** — `drafting_shape_kind_exhaustive_tests` covers shape names by construction; tool ids are unguarded | must generate these; only shape-name misses fail a test |
| serialize fields, projection keys, the painter, the belt, the inspector, metadata flags, the `DraftingCommand` apply arm | **NO** | **must be complete — a miss is silent** (stores but never draws/loads) |

**The rule that falls out:** generate the rote half for every recipe, but hold the
*non-geometry* paths (properties, decorations, the belt/painter/inspector) to a
higher completeness bar — there is no compiler backstop there.

---

## Ellipse — new primitive (Shape A) · commit `cc6c78d`

The full ~22-site ripple. **11 of these the compiler named for me** (the
`std::visit`/`switch` sites); the rest are rote.

- **DraftingTypes.h** — struct `EllipseGeometry{center,rx,ry}`; `DraftingShapeKind::Ellipse`; variant arm; `shapeKindOf<>`; bump `variant_size` 10→11.
- **DraftingGeometry.{h,cpp}** — `sampleEllipse` (shared); `shapeKindName`⚠, `shapeKindFromName`, `geometryKind`, `validateGeometry`, `computeBounds`, `translateGeometry`, `handleAnchors`, `area`.
- **DraftingHitTest.cpp** — `hitDistance` arm.
- **DraftingSnap.cpp** — `snapCandidatesForObject` arm.
- **DraftingObjectEdit.{h,cpp}** — edit-kind enum; `draftingHandlesForObject`, `handleEditPlan`, `applyObjectEdit`.
- **DraftingNumericEdit.cpp** — `applyNumericGeometryEdit` arm.
- **DraftingPlotPlan.cpp** — `appendPlotSegments` arm.
- **DraftingSerialize.cpp** — `isKnownShapeKindName`, `geometryValue`, `readGeometry`⚠.
- **DraftingToolCreation.{h,cpp}** — `DraftingToolKind::Ellipse`; `draftingToolKindFromId`, `draftingToolKindName`⚠, `buildDraftingObjectForTool`.
- **DrawingCanvasWidget.cpp** — `isTwoClickCreationTool`.
- **DrawingDocumentProjection.cpp** — `numericFieldsForObject`⚠, `physicalGeometryForObject`, `draftingObjectToCanvasProjection` (+ flattened `points`).
- **DrawingCanvasObjectPainter.cpp** — `drawObject` + `drawPreviewObject` (closed polygon via points).
- **DraftingFeaturePanels.cpp** — `kDraftingTools` row + `draftingToolFace` branch.
- **CMakeLists.txt** + **tests/drafting_ellipse_tests.cpp** (new) + serialize/shell-count test bumps.

(⚠ = the site is a `switch` guarded by `-Werror=switch`.)

---

## Fill — property on a selection (Shape C) · commit `ae312ba`

~11 sites, **none compiler-guarded** (no geometry variant; the `DraftingCommand`
variant has no exhaustiveness guard). Almost entirely rote (mirror stroke).

- **DraftingTypes.h** — `FillStyle::operator==` (for the no-op guard).
- **DraftingCommands.{h,cpp}** — `UpdateFillStyleCommand` struct + variant arm; `applyDraftingCommand` arm (exists/locked/no-op/assign).
- **DrawingCore.h** + **DrawingDocumentController.cpp** — `setSelectedObjectFillColor/Opacity` (one #rrggbb gate, clamp).
- **DrawingDocumentProjection.cpp** — `own_fill_color`/`own_fill_opacity` keys.
- **DrawingCanvasProjectedObject.{h,cpp}** — `fillColor`/`fillOpacity` on the style struct + extract/clamp in `projectedObjectStyle`.
- **DrawingCanvasObjectPainter.cpp** — a `QBrush` gated to CLOSED shapes when opacity>0.
- **DraftingFeature.h** + **DraftingFeaturePanels.cpp** + **DraftingFeatureInspector.cpp** — Style-group fill rows **built in one file, refreshed in another** (the easy site to miss).
- **tests/drawing_document_controller_tests.cpp** — fill setter pins (clamp / junk / non-finite / no-op-guard).
- Deferred: SVG/plot fill (a region has no representation in the segment model).

---

## Double-arrow — decoration on an existing shape (Shape B) · commit `55e65bd`

A flag on `LineVisualMetadata` + a tool that sets it. ~10 sites; **only
`draftingToolKindName` is compiler-guarded** (the `-Werror=switch` case). Reuses
`LineGeometry` entirely — no geometry/bounds/hit/snap/edit ripple.

- **DraftingTypes.h** — `startArrow` flag in `LineVisualMetadata`.
- **DraftingToolCreation.{h,cpp}** — `DraftingToolKind::DoubleArrow`; `draftingToolKindFromId`, `draftingToolKindName`⚠; `buildDraftingObjectForTool` (add to the Line/Arrow geometry case + set both flags).
- **DraftingSerialize.cpp** — write `start_arrow` + read `start_arrow`.
- **DrawingDocumentProjection.cpp** — emit the `start_arrow` key.
- **DrawingCanvasObjectPainter.{h,cpp}** — `startArrow` on the scene item; read the key; draw a head on the reversed line.
- **DraftingFeaturePanels.cpp** — `kDraftingTools` row + `draftingToolFace` branch.
- **DrawingCanvasWidget.cpp** — `isTwoClickCreationTool` (it's a two-click line).
- **tests** — tool-creation (both flags, start/end mix-up pin); serialize round-trip; shell belt-count bump.

---

## Text annotation — string-field primitive (Shape A) · commit `fc6d40f`

A new primitive, BUT carrying a `std::string content` — the first **non-numeric
field**, and the evidence a scaffolder most needed. The geometry-visitor ripple was
compiler-guided (same 11 named sites as ellipse); the STRING-specific machinery
below was all silent.

- **DraftingTypes.h** — `TextAnnotationGeometry{position, content, height}`; enum; variant; `shapeKindOf`; bump 11→12.
- **DraftingGeometry.{h,cpp}** — name maps + `geometryKind` / `validate` / `computeBounds` (a box sized from `content.size()`) / `translate` / `handleAnchors`.
- **HitTest / Snap / ObjectEdit / NumericEdit / PlotPlan** — box-distance hit; position snap; a `MoveTextAnnotation` edit; px/py/height numeric (NOT content); plot is a deliberate no-op (a pen can't draw glyphs).
- **DraftingSerialize.cpp** — **the string field**: `geometryValue` writes `content` via `MsgPackValue::text`, `readGeometry` reads it via `asString`. (A scaffolder's field codec must branch on field type: text vs number vs point.)
- **DraftingToolCreation.{h,cpp}** — tool kind + maps + build (single-click → `request.start`, default content `"Text"`).
- **single-click gesture** — added to the controller's single-click set, NOT `isTwoClickCreationTool`. A different gesture path than every prior tool.
- **content editing** — `setSelectedObjectTextContent` rides **`UpdateGeometryCommand`** (content is geometry → the whole-geometry replace, not a style command).
- **DrawingDocumentProjection.cpp** — px/py/height numeric fields + physical + a `content` string key for the painter.
- **DrawingCanvasObjectPainter.{h,cpp}** — a `text` scene struct + extractor + a **`drawText`** render mode (QFont sized to height×board, baseline-offset by ascent). A genuinely new paint path.
- **inspector** — a STRING control (QLineEdit), built in `DraftingFeaturePanels`, refreshed in `DraftingFeatureInspector`, **visibility-gated to text objects** (numeric fields auto-surface via the plan; a string field does not).
- **tests** — drafting_text_tests (geometry / edit / tool + a UTF-8 string round-trip).

**New scaffolder lessons from text:** (a) field codecs must be type-aware (string ≠ double ≠ point) across serialize / numeric / projection; (b) a string field needs a hand-built, visibility-gated inspector control, not the auto numeric editor; (c) content-as-geometry edits ride `UpdateGeometryCommand`; (d) single-click vs two-click vs multi-click is a per-tool gesture choice the generator must take as input; (e) `drawText` is a paint mode of its own.

---

## Spline — sampled-curve primitive + multi-click reuse (Shape A) · commit `16fc71a`

A new primitive `SplineGeometry{controlPoints}`, BUT the first whose **drawn form
is a computed sampling** of its stored data (Catmull-Rom through the knots), and
the first to **reuse an existing gesture** (the polyline's multi-click). Two new
axes of evidence the four prior tools couldn't give.

- **DraftingTypes.h** — `SplineGeometry{std::vector<Point2D> controlPoints}`; enum; variant; `shapeKindOf`; bump 12→13.
- **DraftingGeometry.{h,cpp}** — `sampleSpline` (shared Catmull-Rom, declared like `sampleEllipse`); name maps; `geometryKind` / `validate` (≥2 points) / `computeBounds` **(of the SAMPLED curve)** / `translate` (all knots) / `handleAnchors` (`control_N`).
- **HitTest** — `distanceToVertexList(sampleSpline(...), point, false)` — hit the curve, not the control polygon.
- **Snap** — control points (front/back endpoints, interior as vertices); own arm over `controlPoints`.
- **ObjectEdit** — read-only `control_N` knot descriptors (own arm); joins the polygon/polyline "no editable handles yet" reject arm.
- **NumericEdit** — joins the polygon/polyline reject arm (no scalar fields).
- **DraftingPlotPlan.cpp** — `appendVertexSegments(..., sampleSpline(geometry), false)`. **SILENT** (this visit has no terminal static_assert).
- **DraftingSerialize.cpp** — `control_points` via `verticesValue`/`readVertices`; `readGeometry` case⚠; `isKnownShapeKindName` += "spline".
- **DraftingToolCreation.{h,cpp}** — kind + id-map + name⚠ + build (`request.vertices` → controlPoints).
- **multi-click gesture (REUSED)** — controller's multi-click branch and `updateCreationPreview` broadened with `|| Spline`; `finishPendingPolyline` → **`finishPendingMultiClick`** (guard accepts Polyline OR Spline). The gesture machinery already existed — spline added ZERO new gesture code, only widened three predicates.
- **DrawingDocumentProjection.cpp** — `numericFieldsForObject` switch case⚠; physical empty arm (joined); canvas `points` from **`sampleSpline`** (own arm).
- **DrawingCanvasObjectPainter.cpp** — `"spline"` added to the extract group, the draw group (open chain), and the preview (with arc). Three SILENT strings; no new scene struct (reuses `item.polygon`).
- **DraftingFeaturePanels.cpp** — `kDraftingTools` row + a curved `draftingToolFace`.
- **DraftingMirror.cpp** — **no edit**: the visit's terminal else returns the geometry unchanged and `supportsMirror` excludes it (matches polyline).
- **tests** — drafting_spline_tests (sampler-through-knots / bounds-of-curve / hit / handles / tool / serialize) + a controller multi-click block.

**New scaffolder lessons from spline:** (a) a **sampled** geometry has two data
shapes — stored knots vs the computed curve — and the generator must route
bounds/hit/plot/projection/paint to the SAMPLER while translate/handles/serialize
stay on the raw points; (b) a vector-of-points geometry whose member isn't named
`vertices` (here `controlPoints`, the honest term) **cannot** merge into the
polygon/polyline `||` arms — but at the sampling sites it shouldn't anyway, since
the operation differs; (c) **gestures are reusable** — the multi-click path is
shared infrastructure, so a generator adding a multi-click tool *widens the
existing branch/finish guard*, it does not emit a new gesture; (d) generalizing a
tool-specific verb (`finishPendingPolyline` → `finishPendingMultiClick`) is the
cost of the second tool of a kind — a rename + its test call-site updates.

---

## Friction that recurred (a scaffolder should pre-empt)

1. **The belt-inventory count assertion** (`edi_shell_window_tests`, hardcoded
   `checkedCount == N`) broke on *every* tool addition. **FIXED this session** — it
   now derives from `DraftingFeature::toolInventory().size()`, so no future tool
   touches it.
2. **The serialize test's order-dependent `objects[N]` indices** break if a new
   sample object is inserted mid-list — append at the end.
3. **The painter splits extraction (`buildCanvasSceneItem`) from drawing
   (`drawSceneItem`)** — a new shape/flag must be added on BOTH sides or it draws
   nothing (no compile error).
4. **The inspector splits build (`DraftingFeaturePanels`) from refresh
   (`DraftingFeatureInspector`)** — miss the refresh and the control never
   updates on selection.
5. **The model-vs-canvas projection** — per-object style lives in
   `draftingDocumentToModelProjection` (feeds both painter and inspector), not the
   base `draftingObjectToCanvasProjection`. Easy to put a key in the wrong one.
