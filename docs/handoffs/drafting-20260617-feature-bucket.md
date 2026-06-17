# Handoff — drafting-20260617-feature-bucket (DR-01..15)

> Per-campaign state for the dispatched 15-task feature bucket. One gate log per
> task. Source of tasks: `~/dept-bus/work-batch-plan.md` (edi-drafting section);
> surfaces `~/edi/docs/ui-surface/drafting/DR-surfaces.md`; dispatch
> `~/dept-bus/FEATURE-DISPATCH.md`.

- **Campaign**: drafting-20260617-feature-bucket
- **Department**: edi-drafting
- **Goal (one line)**: build the compass-and-straightedge construction kit DR-01..15
  as pure-ops slices (+ thin controller wiring) over the existing Result-struct +
  DraftingCommand pipeline — no generation, no rules. Commit to `dept/drafting`; do
  NOT merge (hub routes verified work to edi-ui).
- **Run order**: DR-01 FIRST (keystone — dungeon-map DM-14/15 blocked on it; bus the
  SHA the instant it's green). Then DR-02…15 per the plan's dependency order.
- **Ratified forks**: DR-13 angular dim = INFER the vertex from the two source lines
  (NO new `DimensionGeometry` field). DR-15 region fill = fill-the-selected-closed-
  object (true flood-fill parked).
- **Gates**: reviewer boundary ONLY where a task genuinely needs one (most are
  spec-settled); builder; green gate `cmake --build build && ctest --test-dir build
  -E edi_shell_window_tests` + scan. Rebase on master at the start of each task.

## Gate log

### DR-01 transformGeometry — builder BRIEFED 2026-06-17
- Brief: `~/dept-bus/edi-drafting/briefs/008-DR01-transformgeometry-builder.md`
- Boundary: SETTLED by spec — signature + per-kind rules + idiom all fixed in the
  work-batch-plan DR-01 entry; no reviewer gate needed up front.
- **Contract-file note:** the spec cites `/tmp/dept-bus-stage/002-transformGeometry-
  contract.md`, which does NOT exist on this box (Mac-side staging path, not
  transferred). The work-batch-plan DR-01 entry reproduces the full signature + all
  14 per-kind rules inline, so it is treated as authoritative. Reported to hub as an
  FYI in case the canonical contract adds anything.
- Plan: builder → green gate → reviewer diff-audit (cross-dept keystone: verify all
  14 arms + the pinned Ellipse/Text v1 limitations) → bus the SHA to hub.
- **Canonical contract delivered to box 2026-06-17**
  (`~/dept-bus/edi-drafting/briefs/002-transformGeometry-contract.md`). RECONCILED vs
  brief 008: signature + `mapPoint` + all 14 per-kind rules + test spec MATCH (brief
  008 is more precise on field names — `PointGeometry.point`). 3 additive deltas sent
  as amendment `~/dept-bus/edi-drafting/briefs/009-DR01-contract-reconcile-amendment.md`:
  (1) document the DEFERRED Ellipse/Text `rotationDeg` follow-up at the code site;
  (2) exact `static_assert` message; (3) sharpen the rotation-SENSE/sign verification
  (CCW in the model's degree convention; point-rotation must agree with field-rotation
  per `computeBounds:519`).

## Backlog (deferred, recorded so it isn't lost)
- **Ellipse/Text lossless rotation** — add an additive `rotationDeg` field to
  `EllipseGeometry` + `TextAnnotationGeometry` (mirroring `RectangleGeometry`) so
  `transformGeometry` rotation is lossless for them. Separate model slice: touches
  every ellipse/text visit + serialize + painter. NOT part of DR-01 (per contract).

### DR-01 — BLOCKER raised + RULED 2026-06-17 (rectangle anchor)
- Builder (`~/dept-bus/edi-drafting/replies/008-DR01-transformgeometry-builder.md`)
  stopped before busing a wrong keystone: the contract's Rectangle rule
  (`mapPoint(origin)` + `rotationDeg += θ`) is geometrically WRONG. The model rotates
  rectangles about their CENTER (`rectangleCorners` :81-95 → `computeBounds:526`; the
  painter `translate(center)/rotate/translate(-center)`; `DraftingObjectEdit` about
  `rectangleCenter`). Mapping the origin corner + bumping rotationDeg leaves the box's
  center un-orbited. Counterexample: square `origin=(0,0) w=h=2`, transform
  `pivot=(0,0) θ=90° scale=1` → correct footprint `[-2,0]×[0,2]`, literal rule keeps
  `[0,2]²`. Also contradicts the contract's own Arc rule (Arc correctly maps its
  rotation-center).
- **PLANNER RULING: A — center-anchored rectangle** (verified analytically incl.
  scale: rendered corners' = `mapPoint(original corners)`; consistent with Arc).
  Corrected rule = the canonical Rectangle rule going forward:
  `C=origin+(w/2,h/2); C'=mapPoint(C); origin'=C'-(w'/2,h'/2); rotationDeg'+=θ;
  w/h/cornerRadius/inset *= scale`. Builder told to ship it (brief 010) without
  waiting on hub — A is provably correct. Rotation-SENSE check already confirmed OK
  by builder (mapPoint matrix == the model's field-rotation; no sin flip).
- **Reported to hub** as a contract correction (canonical contract line 59 needs the
  center-anchor fix before dungeon-map consumes it).
- **HUB RATIFIED 2026-06-17:** ruling A confirmed; canonical contract corrected
  (Rectangle rule = center-anchor). dungeon-map will consume the corrected rule.
  Boundary settled — proceed and bus the green SHA.

### DR-01 — SHIPPED 2026-06-17 (SHA `167768e`, 96/96 green) → diff-audit OPEN
- Reply: `~/dept-bus/edi-drafting/replies/010-DR01-transformgeometry-builder.md`.
  Ruling A implemented exactly (Rectangle center-anchor, orbit asserted on
  computeBounds with the [-2,0]×[0,2] counterexample); other 13 arms per contract;
  amendment-009 deltas 1–3 all in; rotation sense confirmed (no sin flip); deferred
  Ellipse/Text follow-up flagged in comments. `cmake` clean; ctest 96/96
  (`-E edi_shell_window_tests`); scan clean.
- **Diff-audit OPEN** (cross-dept keystone — audit BEFORE busing so a bug doesn't
  reach master/dungeon-map): brief `~/dept-bus/edi-drafting/briefs/011-DR01-audit-reviewer.md`.
  Priority: the center-anchor correctness (assert on corners not stored origin), the
  13 other arms, rotation sense, and whether the lossy Ellipse/Text limitation is
  pinned by a DEDICATED assert (not masked by a self-cancelling round-trip).
- On ACCEPT → `bus-hub drafting "transformGeometry landed 167768e"` to unblock
  dungeon-map → open DR-02.

### DR-01 — CLOSED 2026-06-17 ✅ — audited ACCEPT, bused to hub
- Audit reply: `~/dept-bus/edi-drafting/replies/004-DR01-audit.md` — ACCEPT, no
  issues; center-anchor + rotation sense verified vs the REAL model matrices
  (`arcPointAtAngle`, `rectangleCorners`), test genuinely distinguishes the corrected
  rule (wrong rule → `bounds.x=0`, assert rejects), lossy Ellipse/Text pinned by
  dedicated asserts.
- **BUSED to hub** `transformGeometry landed 167768e` + the consumer contract (mapPoint
  property; Rectangle center-anchor; Ellipse/Text v1 lossy caveats; Guide identity;
  deferred additive-rotationDeg follow-up). dungeon-map DM-14/15 unblocked.

### DR-02 quadrant / nearest-on-curve snaps — builder BRIEFED 2026-06-17
- Brief: `~/dept-bus/edi-drafting/briefs/012-DR02-snaps-builder.md`. Boundary
  spec-settled (additive enum + settings + candidate emission; behavior-preserving for
  existing snaps) → no reviewer gate up front. No dependency.

### DR-02 — DONE 2026-06-17 ✅ (SHA `e405d89`, 96/96 green)
- Reply: `~/dept-bus/edi-drafting/replies/012-DR02-snaps-builder.md`. `Quadrant` +
  `OnCurve` added to the snap enum/settings/name; Circle 4-cardinal + Arc sweep-gated
  quadrant emission; OnCurve for line/circle/arc/polyline/polygon edges. Existing
  `drafting_snap_tests` green (behavior-preservation proof).
- **PRECEDENCE DECISION (confirmed by planner) — durable:** OnCurve is a **fallback
  tier** in `nearestObjectSnap`, consulted ONLY when no keypoint/guide/intersection
  snapped (`!found`). The brief's literal "append after keypoints, rely on `<=`" was
  FLAWED — selection is by distance, so a genuinely-closer OnCurve (line body 0.010)
  would outrank the existing Intersection snap (0.014) and flip an existing test. The
  fallback tier is the correct CAD semantics (nearest-on-curve only when no higher
  snap is in aperture) and behavior-preserving. Any future "OnCurve competes on
  distance with keypoints" is a DELIBERATE precedence change, not this slice.
- Follow-ups noted (not urgent): OnCurve for wall/cline/dimension/ellipse/spline is
  v1-absent; `projectOnSegment` is file-local in DraftingSnap — could be promoted to
  `DraftingGeometry` as a shared primitive if another slice needs it.

### DR-03 — tangent / perpendicular relative snaps — builder BRIEFED 2026-06-17
- Brief: `~/dept-bus/edi-drafting/briefs/013-DR03-relative-snaps-builder.md`. Deps
  DR-02 (satisfied). Boundary spec-settled; no reviewer gate up front.

## Open questions / blockers
- (none blocking — DR-02 done; DR-03 in build)

## Next
- Builder implements DR-01; planner buses the green SHA to the hub so dungeon-map
  unblocks, then opens DR-02.
