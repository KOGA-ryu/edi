# Surface-design spec — drafting batch DR-01..15

> The UI-Integration GATE deliverable for the drafting domain. Every feature names a
> mechanism from `docs/ui-surface/INFRA.md` (M1..M10), the exact interaction, and the
> existing widget/symbol it reuses. Verified against live `src/widgets` +
> `src/core/DrawingCore.h` + `src/drafting/DraftingInspectorPlan.cpp` on 2026-06-17
> (`integrate-cartography`). `file:line` anchors drift — trust the symbol name.
>
> **Surface now vs deferred:** several DR ops ship "op + tests only" this slice (the
> batch leaves the chrome to a later edi-ui hand). Each spec below still names the
> *eventual* surface so the wiring hand has no UX decision to make, and is tagged
> **SURFACE NOW** (the op slice includes the controller arm/chrome) or **SURFACE
> DEFERRED** (op lands first; this surface is the edi-ui follow-up).

---

## Open UX forks to escalate (hub/user decisions)

1. **DR-10 rotate-vs-placement toggle (UX fork).** The rotate-copies rosette and the
   existing placement-only radial array (`radialArrayButton` →
   `beginRadialArrayCenterPick`, `DraftingFeaturePanels.cpp:1618`) share one center-pick
   gesture. Decision: **add a "Rotate copies" toggle to the Repeat fold** (M2-style
   tool-option in `buildRepeatControls`) that flips which op the *same* Radial button
   runs — vs. a second button, vs. a controller-only flag. **Recommendation:** one
   `makeToggle("rotateCopiesToggle", …)` in the Repeat fold + a total-angle spin; the
   existing placement behavior stays the default (toggle off). Confirm the toggle lives
   in tool-options (this spec) and not buried as a controller flag.

2. **DR-13 Angular-dimension representation (DATA fork, not a surface fork).** Whether
   `DimensionGeometry` gains a field is a core/serialize decision (batch open question,
   planner leans option 3 = infer vertex from the two source lines). **No UI fork:** the
   surface is identical either way — one new `"Angular"` entry in `dimensionKindCombo`
   (`DraftingFeaturePanels.cpp:933`) + one belt cell. Flag forwarded so the gate is
   explicit; the surface does not block on the data decision.

3. **DR-15 bucket-fill scope (confirm, not a surface fork).** This batch ships
   *fill-the-selected-closed-object* only; true boundary-tracing bucket-fill is deferred.
   The surface (style group fill fields) is unaffected. Forwarded for the user to confirm
   fill-the-closed-object is the intended "region fill".

**NEW infra needed:** **none.** Every surface below is a new *instance* of an existing
mechanism — a new belt cell in `kDraftingTools[]` (M1), a new `toolOptionsTable()` row +
`beginInspectorGroup` builder (M2), a new `makeActionButton`/`makeToggle`/`makeDataCombo`
entry (M3/M4), a new snap `makeToggle` (M10 Snap popup), or a new `PointCaptureIntent`
value with a `begin/apply` pair (M6, which INFRA.md classifies as established plumbing,
not new infra). The minimal on-pattern additions are itemized per feature.

---

## DR-01 · transformGeometry primitive — **NO OWN SURFACE**

- **Mechanism:** none. Pure geometry primitive (rotate + uniform scale about a pivot).
- **Interaction:** none directly.
- **Reuses:** n/a — it is the *engine* consumed by DR-10 (rotate-copies), DR-11
  (kaleidoscope), DR-12 (array-along-curve), and dungeon-map DM-14/DM-15 (placed-block
  transform). It has no belt cell, no inspector control, no projection key of its own.
- **NEW infra?** No. **Recorded explicitly as "no surface"** so the gate is closed:
  transformGeometry surfaces only through the DR-10/11/12 transform actions below.

---

## DR-02 · Quadrant / nearest-on-curve snap sources — **SURFACE NOW (chrome only)**

- **Mechanism:** **M10** — the Snap chrome popup (`buildChromePanels`). Two new snap
  toggles mirroring the existing endpoint/center toggles.
- **Interaction:** user opens the **Snap** popup in the top chrome, ticks **Quadrant**
  and/or **On-curve**; thereafter the canvas snap marker
  (`drawPointerSnapMarker`, `DrawingCanvasWidget.cpp:717`) lands on circle/arc cardinal
  points and projects onto nearby curves. On-curve loses to keypoint snaps (op-side
  priority), so no UI ordering control is needed.
- **Reuses:** `makeToggle("snapToggle", …)` exactly as `m_endpointSnap` / `m_centerSnap`
  (`DraftingFeaturePanels.cpp:472–483`); add `m_quadrantSnap` / `m_onCurveSnap` members
  (`DraftingFeature.h:181–186` sibling) bound to new controller setters
  `setQuadrantSnapEnabled` / `setOnCurveSnapEnabled` (mirror `setEndpointSnapEnabled`).
  Backed by the op's `quadrantEnabled` / `onCurveEnabled` flags in `DraftingSnapSettings`.
- **NEW infra?** No — two new `makeToggle` instances + two controller setters. The snap
  ops land in this slice; the toggle is the only chrome and is a later edi-ui hand.

---

## DR-03 · Tangent / perpendicular snap candidates — **SURFACE NOW (chrome only)**

- **Mechanism:** **M10** — two more Snap-popup toggles (**Tangent**, **Perpendicular**),
  identical pattern to DR-02.
- **Interaction:** tick **Tangent** / **Perpendicular** in the Snap popup. These are
  *relative* snaps (they need the in-progress segment's anchor), so they only fire during
  a multi-click draw once the controller passes the live anchor to
  `relativeSnapCandidatesForDocument` — that controller wiring is the deferred hand; the
  toggle is the only chrome this slice.
- **Reuses:** same `makeToggle("snapToggle", …)` pattern + `m_tangentSnap` /
  `m_perpendicularSnap` members + `setTangentSnapEnabled` / `setPerpendicularSnapEnabled`
  controller setters (mirror DR-02). `DraftingSnapSettings` tangent/perpendicular flags.
- **NEW infra?** No. Note the relative-anchor controller plumbing as the edi-ui follow-up.

---

## DR-04 · Snap a typed distance along an entity — **SURFACE DEFERRED**

- **Mechanism:** **M2** — a tool-options numeric field group (the typed distance + a
  from-which-end choice). Not a snap toggle: it is a typed parameter.
- **Interaction:** with a draw tool armed, the user types a **Distance along** value and
  picks **from start / from end**; the snap then offers a candidate that distance along
  the hovered line/polyline/arc from the chosen end.
- **Reuses:** a new `toolOptionsTable()` row + a `beginInspectorGroup("tool_along")`
  builder modeled on `tool_radius` (`DraftingFeaturePanels.cpp:674`,
  `DraftingInspectorPlan.cpp:53`): one `QDoubleSpinBox` ("alongDistanceSpin", mirror
  `filletRadiusSpin` at `:1057`) + a `makeToggle`/`makeDataCombo` for start/end. Controller
  field mirrors `m_filletRadius` / `setFilletRadius` (`DrawingCore.h:410`,
  `:226`). The op `pointAlongEntity` supplies the candidate point.
- **NEW infra?** No — one tool-options group + one controller param field. Batch ships the
  pure op only; the field + the snap hook are the deferred edi-ui hand.

---

## DR-05 · Circle by 3 points — **SURFACE DEFERRED**

- **Mechanism:** **M1 belt cell** (a circle-tool variant) + **M6** a 3-point capture.
- **Interaction:** click the **Circle (3 pts)** belt cell on the circle row, then click
  three points on the canvas (with the snaps above active); on the third click the unique
  circumcircle is created and selected. Collinear triple → rejection surfaced via
  `finishEdit` (no circle). A 2-point diameter variant can share the row.
- **Reuses:** one row in `kDraftingTools[]` (`DraftingFeaturePanels.cpp:55`) on `beltRow 4`
  (the circle/ellipse row) — e.g. `{"circle_3pt_tool", "Circle (3 pts)", "C3", 4}` — plus
  a `draftingToolFace("circle_3pt_tool")` arm (a tiny 3-dots-on-a-ring diagram). The
  3-click gesture is a new `PointCaptureIntent::CircleThreePoints` with a
  `beginCircleThreePoints()` / `applyCirclePointAtPoint(Point2D)` pair that accumulates 3
  points then applies `CreateObject` — modeled on the multi-pick idiom of
  `beginFilletSelectedLine` / `applyFilletAtPoint` (`DrawingDocumentController.cpp:1871`,
  intent enum `DrawingCore.h:39`). Op: `circleThroughThreePoints`.
- **NEW infra?** No — one belt cell + one belt face + one `PointCaptureIntent` value (M1 +
  M6, both on-pattern). Op lands first; arm + face are the deferred edi-ui hand.

---

## DR-06 · Divide circle into N / inscribe N-gon / {n/k} star — **SURFACE DEFERRED**

- **Mechanism:** **M1 belt cell** (polygon-row variant) + **M2** a sides/step/start-angle
  tool-options group + **M6** a circle/center pick.
- **Interaction:** click the **Inscribe** belt cell on the polygon row; set **Sides (n)**,
  **Star step (k)** (k = 1 → regular N-gon; k ≥ 2 → {n/k} star), and **Start angle** in
  the tool-options; then click to pick the bounding circle (or a center + radius). One
  closed `PolygonGeometry` object is created and selected. Invalid {n/k} (gcd≠1 etc.)
  rejects via `finishEdit`.
- **Reuses:** one `kDraftingTools[]` row on `beltRow 6` (the polygon row) e.g.
  `{"inscribe_tool", "Inscribe N-gon / Star", "In", 6}` + a `draftingToolFace` arm. The
  tool-options group reuses the existing `tool_polygon` *sides spin* pattern
  (`DraftingFeaturePanels.cpp:619`, `DraftingInspectorPlan.cpp:51`): add a `tool_inscribe`
  group with a sides `QSpinBox` (copy the polygon sides spin), a step `QSpinBox`, and a
  start-angle `QDoubleSpinBox`. Pick reuses `PointCaptureIntent` (a circle-pick or reuse
  the center pick). Op: `divideCirclePoints` / `inscribeRegularPolygon` /
  `inscribeStarPolygon`.
- **NEW infra?** No — one belt cell + one tool-options group (sibling of `tool_polygon`) +
  capture. Op lands first; surface is the deferred hand.

---

## DR-07 · Chamfer two lines — **SURFACE NOW**

- **Mechanism:** **M3** inspector action button in the Modify fold + **M2** a setback spin
  + **M6** a `PointCaptureIntent::ChamferSecondLine`.
- **Interaction:** with one line selected, set **Chamfer setback** in the Modify fold,
  click **Chamfer**; the next canvas click picks the second line + the corner, and both
  lines trim back to the setback points with a bevel line joining them (atomic). Parallel /
  overrun rejects surface via `finishEdit`.
- **Reuses:** add a `chamferButton` `makeActionButton` + a `chamferSetbackSpin`
  `QDoubleSpinBox` to the **Modify** `FoldBox` right beside `filletButton` /
  `filletRadiusSpin` (`DraftingFeaturePanels.cpp:1044–1067`) — byte-for-byte the Fillet
  pattern. Controller: `PointCaptureIntent::ChamferSecondLine` + `beginChamferSelectedLine`
  / `applyChamferAtPoint(Point2D)` mirroring `beginFilletSelectedLine` /
  `applyFilletAtPoint`; setback mirrors `m_filletRadius` / `setFilletRadius`. Op:
  `chamferLines`.
- **NEW infra?** No — a sibling button + spin + one `PointCaptureIntent` value. The
  controller arm is **in this op slice** (the batch wires the verb), so this surfaces now.

---

## DR-08 · Extend line to a boundary — **SURFACE NOW**

- **Mechanism:** **M3** Modify-fold action button + **M6**
  `PointCaptureIntent::ExtendPoint` (mirror of Trim).
- **Interaction:** with a line selected, click **Extend**; the next canvas click picks the
  end to extend and the picked end lengthens to the nearest boundary crossing of its
  infinite extension. No-crossing / already-past rejects via `finishEdit`.
- **Reuses:** an `extendButton` `makeActionButton` beside `trimButton` in the Modify fold
  (`DraftingFeaturePanels.cpp:1038`). Controller: `PointCaptureIntent::ExtendPoint` +
  `beginExtendSelectedLine` / `applyExtendAtPoint(Point2D)` mirroring `beginTrimSelectedLine`
  (`DrawingDocumentController.cpp:1827`, `TrimPoint` intent). Applies via
  `applyCommandAndEmit(UpdateGeometryCommand{…})`. Op: `extendLineToBoundary`.
- **NEW infra?** No — one button + one `PointCaptureIntent` value. Controller arm is in the
  op slice → surfaces now.

---

## DR-09 · Break / divide at a point — **SURFACE NOW**

- **Mechanism:** **M3** Modify-fold action button + **M6**
  `PointCaptureIntent::BreakPoint`.
- **Interaction:** with a line/polyline selected, click **Break**; the next canvas click
  picks the split point and the object splits into two independent objects (original keeps
  selection). Near-endpoint / unsupported kind rejects via `finishEdit`.
- **Reuses:** a `breakButton` `makeActionButton` in the Modify fold beside trim/fillet
  (`:1038–1049`). Controller: `PointCaptureIntent::BreakPoint` + `beginBreakSelectedObject`
  / `applyBreakAtPoint(Point2D)`, applied atomically (UpdateGeometry original → `first` +
  CreateObject `second`) in one `beginEdit`/`commitEdit` bracket exactly like the fillet
  path. Op: `breakGeometryAtPoint`.
- **NEW infra?** No — one button + one `PointCaptureIntent` value. Surfaces now.

---

## DR-10 · Rotate-copies rosette — **SURFACE NOW** (see UX fork #1)

- **Mechanism:** **M2** a "Rotate copies" toggle + total-angle spin in the Repeat fold,
  reusing the existing **M6** radial-array center pick.
- **Interaction:** with an object selected, set **Count** (existing `arrayCountSpin`), tick
  **Rotate copies**, set **Total angle** (default 360 = full ring); click **Radial**, then
  click the canvas to place the ring center. Each copy is rotated to its spoke
  (transformGeometry). Toggle **off** = today's placement-only behavior (default, preserved).
- **Reuses:** `buildRepeatControls` (`DraftingFeaturePanels.cpp:1564`): reuse `arrayCountSpin`
  (`:1576`) and the `radialArrayButton` → `beginRadialArrayCenterPick` flow (`:1618`); add
  a `makeToggle("rotateCopiesToggle", …)` + a `rosetteAngleSpin` `QDoubleSpinBox` (mirror
  `arraySpacingXSpin` at `:1596`) bound to new controller params. The center pick reuses
  `PointCaptureIntent::RadialArrayCenter` / `runRadialArrayAtCenter` (`DrawingCore.h:387`)
  with the mode flag selecting `rotateCopiesDraftingObject` vs `radialArrayDraftingObject`.
- **NEW infra?** No — one toggle + one spin + a controller mode flag. **UX fork #1**:
  confirm the toggle (not a hidden controller flag) is the surface and that placement-only
  stays the default.

---

## DR-11 · Kaleidoscope / dihedral mirror — **SURFACE NOW**

- **Mechanism:** **M3** inspector action button (in the Duplicate fold's mirror area) +
  **M2** an axis-count spin + **M6** a center pick.
- **Interaction:** with a mirror-supported object selected, set **Axes** (axis count),
  click **Kaleidoscope**, then click the canvas to place the symmetry center; the object is
  reflected across N axes spaced 180/N° through that center. Unsupported kind / axisCount<1
  rejects via `finishEdit`.
- **Reuses:** add a `kaleidoscopeButton` `makeActionButton` + an `axisCountSpin` `QSpinBox`
  to `buildMirrorControls()` (folded into the Duplicate section, `:1030`), styled like the
  Repeat `arrayCountSpin`. Controller: a `PointCaptureIntent::KaleidoscopeCenter` +
  `beginKaleidoscopeSelectedObject` / `applyKaleidoscopeAtPoint(Point2D)` modeled on the
  `RadialArrayCenter` center-pick pair. Op: `kaleidoscopeMirror` (gated on `supportsMirror`).
- **NEW infra?** No — one button + one spin + one `PointCaptureIntent` value (center pick
  sibling of radial array).

---

## DR-12 · Array along a curve — **SURFACE DEFERRED**

- **Mechanism:** **M3** inspector action button + **M2** count + align-to-tangent toggle +
  **M6** a *path-object* pick capture.
- **Interaction:** with an object selected, set **Count** (reuse `arrayCountSpin`), tick
  **Align to tangent**, click **Array along path**; the next canvas click picks the guide
  path object (line/polyline/arc/spline) and N copies distribute evenly along its arc
  length, optionally rotated to the path tangent.
- **Reuses:** add an `arrayAlongCurveButton` `makeActionButton` + a
  `makeToggle("alignToTangentToggle", …)` to the Duplicate/Repeat fold, reusing
  `arrayCountSpin`. The path pick is a new `PointCaptureIntent::ArrayPath` that captures the
  *object under the click* (not a free point) — mirror `BlockInstance`'s object-targeting
  capture (`PointCaptureIntent::BlockInstance`, `DrawingDocumentController.cpp:1797`). Op:
  `arrayAlongCurve` (reuses DR-04 `pointAlongEntity` for stationing).
- **NEW infra?** No new mechanism, but flag the **path-PICK capture** as the one piece that
  differs from a plain point pick — it selects an *object* like the block-instance capture.
  Batch ships op + tests only; controller wiring (the path pick) is the deferred edi-ui hand
  (called out in the batch spec). **Surface named so the hand has no UX decision.**

---

## DR-13 · Angular dimension — **SURFACE NOW** (see UX fork #2, data only)

- **Mechanism:** **M1 belt cell** on the dimension row + **M4** the `dimensionKindCombo`
  gains an `"Angular"` entry.
- **Interaction:** click the **Dimension: Angular** belt cell, pick the two lines; the
  angular dimension is created. When a dimension is selected, the inspector `dimension`
  group shows **Angular** in the kind combo (and the readout shows the angle). Parallel
  lines reject via `finishEdit`.
- **Reuses:** one `kDraftingTools[]` row on `beltRow 9` (the dimension row, which already
  carries 5 cells: distance/width/height/radius/diameter, `:75–79`) — e.g.
  `{"angular_dimension_tool", "Dimension: Angular", "Da", 9}` + a `draftingToolFace` arm.
  Add `{QStringLiteral("Angular"), QStringLiteral("angular")}` to the `dimensionKindCombo`
  `makeDataCombo` list (`DraftingFeaturePanels.cpp:933`). The kind id maps through
  `draftingDimensionKindFromId`. Op: `planAngularDimension`.
- **NEW infra?** No — one belt cell + one combo entry. **UX fork #2 is DATA-only** (whether
  `DimensionGeometry` gains a field) and does **not** change this surface. The pick gesture
  (two lines) reuses the multi-pick capture idiom.

---

## DR-14 · Arc-length / sweep & radial dimension — **SURFACE NOW (cells) / one-pick**

- **Mechanism:** **M1 belt cell** on the dimension row + **M6** a one-pick capture; the
  radius/diameter *kinds* already exist.
- **Interaction:** click the **Dimension: Arc Sweep** belt cell, then click an arc → a
  sweep (Angular) dimension; the existing **Radius** / **Diameter** dimension tools
  (`radius_dimension_tool` / `diameter_dimension_tool`, `:78–79`) gain a *one-pick* path:
  click the arc/circle once and the radial dimension anchors automatically (a = center,
  b = a point on the curve) instead of placing two endpoints by hand.
- **Reuses:** one `kDraftingTools[]` row on `beltRow 9` for arc-sweep, e.g.
  `{"arc_sweep_dimension_tool", "Dimension: Arc Sweep", "As", 9}` + a `draftingToolFace`
  arm; the sweep kind rides the DR-13 `"Angular"` combo entry. Radius/diameter reuse their
  existing belt cells + `dimensionKindCombo` entries — only the controller gains a one-pick
  capture that calls `planRadialDimensionForArc` / `planRadialDimensionForCircle`. Op:
  `planRadialDimensionForArc` / `planRadialDimensionForCircle` / `planArcSweepDimension`.
- **NEW infra?** No — one belt cell (arc-sweep) + a one-pick capture; radius/diameter are a
  planning convenience over existing kinds, not a new combo entry.

---

## DR-15 · Region fill authoring (wake FillStyle) — **SURFACE NOW**

- **Mechanism:** **M4** — the existing `style` inspector group's fill color field +
  opacity spin (already plumbed end-to-end). The DR-15 action just sets opacity>0 on a
  closed selection.
- **Interaction:** select a closed object (Circle/Ellipse/Rectangle/Polygon/closed-Arc),
  pick a **Fill color** in the style group and raise **Fill opacity** above 0 — the object
  renders filled (canvas + SVG). On an open kind (Line/Polyline/Guide) the action rejects
  with a code+message; for clarity an optional `makeConditionalButton("fillButton", "Fill",
  "has_closed_selection", …)` can default the opacity in one click.
- **Reuses:** `styleFillColorField` + `m_styleFillOpacitySpin` in the `style` group
  (`DraftingFeaturePanels.cpp:882`, refreshed `DraftingFeatureInspector.cpp:200–206`),
  bound to the `own_fill_color` / `own_fill_opacity` projection keys (M7) — *all already
  exist*. The closed-kind gating + the metadata write flow through
  `applyActiveObjectMetadataUpdate` / `UpdateMetadataCommand` (`DrawingCore.h:339`). If a
  one-click Fill button is added, it reuses `makeConditionalButton` (the
  `has_selection`-style enable-key pattern, `:729`) with a new closed-selection projection
  bool key.
- **NEW infra?** No — the fields exist; this is an authoring *action* over them. Optional
  one-click button reuses `makeConditionalButton` + a new `has_closed_selection` projection
  key (M7 add-a-key, on-pattern). Surfaces now (the controller metadata path is in slice).

---

## Summary table

| DR | Mechanism (M#) | Surface now? | Minimal addition |
|----|----------------|--------------|------------------|
| 01 | none | — | none (engine for 10/11/12) |
| 02 | M10 Snap toggle | now (chrome) | 2 `snapToggle` + setters |
| 03 | M10 Snap toggle | now (chrome) | 2 `snapToggle` + setters |
| 04 | M2 tool-options | deferred | `tool_along` group + param |
| 05 | M1+M6 | deferred | belt cell + face + capture intent |
| 06 | M1+M2+M6 | deferred | belt cell + `tool_inscribe` group + capture |
| 07 | M3+M2+M6 | now | Modify button + setback spin + intent |
| 08 | M3+M6 | now | Modify button + intent |
| 09 | M3+M6 | now | Modify button + intent |
| 10 | M2+M6 | now | Repeat toggle + angle spin + mode flag (**fork #1**) |
| 11 | M3+M2+M6 | now | Mirror button + axis spin + center intent |
| 12 | M3+M2+M6 | deferred | button + tangent toggle + path-pick intent |
| 13 | M1+M4 | now | belt cell + combo entry (**fork #2 data-only**) |
| 14 | M1+M6 | now | arc-sweep belt cell + one-pick capture |
| 15 | M4 | now | reuse fill fields; optional conditional button |
