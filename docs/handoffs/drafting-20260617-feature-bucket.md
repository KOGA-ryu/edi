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

## INTEGRATION ROUTING (2026-06-17, corrected) — two different "edi-ui" things
- **edi-ui (BUILD dept)** = owns shell files + MERGES dept branches to master
  (autonomous integration cadence per HUB-LEDGER). **This is the merge owner.**
- **edi-ui-integration(-drafting)** = surface-design DESIGNER GATE, **docs only, NO
  merges** — produces `~/edi/docs/ui-surface/drafting/DR-surfaces.md`. I mistakenly
  bused it the merge cadence; corrected.
- **H6 RESOLVED (hub, 2026-06-17):** the merge owner is **`edi-ui`** (the integration
  dept, RUNNING). Route merge/integration signals (batch tips, closeouts) to the
  `edi-ui` tmux session — NOT `edi-ui-integration-drafting` (docs-only designer). edi-ui
  pulls verified dept branches to master.
- **What I do:** keep building on `dept/drafting` (commit, do NOT self-merge); rebase
  each slice onto LOCAL `master` (currently ~`163a00a`, advancing as edi-ui merges;
  `origin/master` is STALE `591e92c`). Report verified batch tips to `edi-ui`.
- **Reported to edi-ui:** DR-04..07 (tip `0a8e943`) — DR-04/05/06 green + DR-07 chamfer
  green & audited ACCEPT. edi-ui pulls.

## POLICY (ratified 2026-06-17) — do NOT commit docs/handoffs/LEDGER.md on dept/drafting
edi-ui owns the master LEDGER (PROTOCOL.md). Track department state in THIS per-campaign
handoff doc (conflict-free, department-specific) + `bus-hub`. This kills the
cross-department rebase conflict on the shared LEDGER. **Amended rebase rule for the
builder:** on a `docs/handoffs/LEDGER.md` conflict during `git rebase origin/master`,
take MASTER's version and DROP our hunks (do NOT re-apply) — this is folded into builder
briefs from DR-06 on. (Historical note: dept/drafting carries +33 LEDGER lines across 5
pre-policy commits; the hub/edi-ui discard these at integration.)

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

### DR-03 — SHIPPED 2026-06-17 (SHA `d9c7aca`, 96/96) → one small amendment in flight
- Reply: `~/dept-bus/edi-drafting/replies/013-DR03-relative-snaps-builder.md`.
  `Tangent`/`Perpendicular` added; new `relativeSnapCandidatesForDocument(doc,
  fromPoint, settings)` (anchor-free path untouched); tangent = Thales contacts
  (sweep-filtered, perpendicular-to-radius asserted); perpendicular feet for
  line/cline/wall/polyline/polygon edges.
- **Planner decisions:** (a) generous "edge" scope CONFIRMED (keep). (b) SEND-BACK
  amendment `~/dept-bus/edi-drafting/briefs/014-DR03-perpendicular-onsegment-amendment.md`:
  the builder used the CLAMPED foot (`projectOnSegment`), which returns the endpoint
  when the true foot is off-segment → a mislabeled `Perpendicular` that isn't
  perpendicular (and duplicates Endpoint). v1 = emit Perpendicular ONLY when the
  unclamped foot lies on the segment (`t∈[0,1]`); infinite-line deferred-perp is a
  future variant. Not consumed yet → cheap to settle now before edi-ui wiring.
- DR-04 HELD until the amendment lands (single builder, sequential).

### DR-03 — CLOSED 2026-06-17 ✅ (amendment SHA `d6b1738`, 96/96 green)
- `~/dept-bus/edi-drafting/replies/014-DR03-perpendicular-onsegment-builder.md`.
  `perpendicularFootOnSegment` (unclamped, `t∈[0,1]`, else nullopt); `projectOnSegment`
  still backs OnCurve. Off-segment skip asserted. Perpendicular candidates are now
  genuinely perpendicular. Infinite-line deferred-perp left as a noted future variant.

### DR-04 — point-along-entity — builder BRIEFED 2026-06-17
- Brief: `~/dept-bus/edi-drafting/briefs/015-DR04-point-along-builder.md`. Deps none.
  Boundary spec-settled (pure op, Result idiom) → no reviewer gate up front.

### DR-04 — CLOSED 2026-06-17 ✅ (SHA `4429a16`, 97/97 green)
- `~/dept-bus/edi-drafting/replies/015-DR04-point-along-builder.md`.
  `DraftingPointAlongResult` + `pointAlongEntity(geometry, distance, fromEnd)` in
  `DraftingSnap.{h,cpp}` (line lerp / arc via distance·radius / polyline+polygon edge
  walk; rejects overshoot/negative/non-finite/unsupported). Judgment calls accepted:
  arc advances `sign(end−start)`; polygon `fromEnd` reverses the loop; distance==length
  (±1e-9) lands on the far end. Reuses `distance`/`arcPointAtAngle` (no re-derived trig).
- Follow-ups noted (not built, low priority): ConstructionLine + Spline support for
  `pointAlongEntity` (cline trivial; spline needs sampled-curve arc length). DR-12
  (array-along-curve) can fall back to the samplers for spline if it needs stationing.

### DR-05 — circle through 3 / 2 points — builder BRIEFED 2026-06-17
- Brief: `~/dept-bus/edi-drafting/briefs/016-DR05-derived-circle-builder.md`. Deps
  none. New `DraftingDerived.{h,cpp}` (adds to `edi_drafting_core` sources). Boundary
  spec-settled → no reviewer gate up front.

### DR-05 — CLOSED 2026-06-17 ✅ (SHA `1dbbd4f`, 98/98 green)
- `~/dept-bus/edi-drafting/replies/016-DR05-derived-circle-builder.md`. New
  `DraftingDerived.{h,cpp}` (added to `edi_drafting_core`): `circleThroughThreePoints`
  (branch-free determinant circumcenter, collinear/non-finite reject) +
  `circleThroughTwoPoints` (diameter form, coincident reject). Epsilon 1e-9 on the
  determinant (normalized 0..1 canvas space). File structured to grow for DR-06.

### DR-06 — divide circle + inscribe N-gon/{n/k} star — builder BRIEFED 2026-06-17
- Brief: `~/dept-bus/edi-drafting/briefs/017-DR06-inscribe-builder.md`. Deps DR-05 ✅.
  Extends `DraftingDerived.{h,cpp}`. First builder brief carrying the LEDGER rebase
  rule (policy 2026-06-17). Boundary spec-settled → no reviewer gate up front.

### DR-06 — CLOSED 2026-06-17 ✅ (SHA `d3451d6`, 98/98 green)
- `~/dept-bus/edi-drafting/replies/017-DR06-inscribe-builder.md`. `divideCirclePoints`
  + `inscribeRegularPolygon` + `inscribeStarPolygon` extend `DraftingDerived`. {n/k}
  validity fully enforced (`n≥5, 2≤step<n/2, gcd=1`; pentagram order `0,2,4,1,3`).
  PolygonGeometry is implicitly closed (no flag); defensive `inscribableCircle` guard.
  No LEDGER conflict. Snap/derived front (DR-02..06) complete.

### DR-07 — chamfer (op + controller wiring) — SHIPPED `0a8e943`, 98/98 → diff-audit OPEN
- Reply: `~/dept-bus/edi-drafting/replies/018-DR07-chamfer-builder.md`. `chamferLines`
  op + `ChamferSecondLine` capture intent + begin/apply/setback verbs; atomic 3-object
  undo (UpdateGeometry×2 + CreateObject bevel in one beginEdit/commitEdit); fillet path
  byte-unchanged; pointer-lifetime handled (ids/layer captured before mutation). 98/98
  green incl. `drawing_document_controller_tests`.
- **Diff-audit OPEN** (first controller-wiring slice — validate the pattern before
  DR-08/09 reuse it): brief `~/dept-bus/edi-drafting/briefs/019-DR07-audit-reviewer.md`.
  Priority: existing paths byte-unchanged, pointer-lifetime, atomic 1-step undo, op
  correctness, scope.
- Deliberate (accepted) duplication: `chamferLines` mirrors `filletLines`'s corner
  logic rather than a shared `cornerFromPick` helper — kept to preserve fillet's bytes;
  shared-helper extraction is a noted future cleanup.

### DR-07 — CLOSED 2026-06-17 ✅ (SHA `0a8e943`, 98/98) — audit ACCEPT
- Audit `~/dept-bus/edi-drafting/replies/005-DR07-audit.md`: +318/−0 (nothing existing
  modified), pointer-capture-before-mutate + atomic 1-step undo mirror fillet exactly,
  corner logic non-divergent. **Controller-wiring pattern VALIDATED → DR-08/09 reuse it
  on lighter scrutiny (no mandatory audit unless something new arises).**
- Non-blocking follow-up (FOLDED into DR-08): chamfer accepts a near-0° wedge → a
  near-degenerate bevel; the controller doesn't check the CreateObject result, so a
  partial chamfer could commit. Add a min-bevel-length (or min-corner-angle) reject to
  `chamferLines` (cheap; same file DR-08 touches).

### DR-08 — extend-line-to-boundary (op + controller) — builder BRIEFED 2026-06-17
- Brief: `~/dept-bus/edi-drafting/briefs/020-DR08-extend-builder.md`. Deps none. Mirrors
  the TRIM path (single UpdateGeometry via applyCommandAndEmit). Reuses the validated
  controller pattern. **Carries the FIRST big rebase onto local `master`** (across 3
  depts' merged work) as STEP 0, + the chamfer min-bevel guard.

## Open questions / blockers
- (none blocking — DR-08 in build; merges flow to edi-ui as it pulls)

## Next
- Builder implements DR-01; planner buses the green SHA to the hub so dungeon-map
  unblocks, then opens DR-02.
