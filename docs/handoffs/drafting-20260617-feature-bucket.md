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
- **MERGED 2026-06-17 (edi-ui):** DR-04..07 on master @`f53b00b` (99/99 green). Local
  `master` has since advanced to `9ebfb7f` (also carries dungeon-map DM-09..13). DR-08's
  STEP-0 `git rebase master` lands on `9ebfb7f` — DR-01..07 drop as already-applied; only
  DR-08 replays on top. (My branch is "ahead" of the stale `origin/dept/drafting`, which
  we never push — edi-ui integrates the local branch.)

## POLICY (ratified 2026-06-17) — do NOT commit docs/handoffs/LEDGER.md on dept/drafting
edi-ui owns the master LEDGER (PROTOCOL.md). Track department state in THIS per-campaign
handoff doc (conflict-free, department-specific) + `bus-hub`. This kills the
cross-department rebase conflict on the shared LEDGER. **Amended rebase rule for the
builder:** on a `docs/handoffs/LEDGER.md` conflict during `git rebase origin/master`,
take MASTER's version and DROP our hunks (do NOT re-apply) — this is folded into builder
briefs from DR-06 on. (Historical note: dept/drafting carries +33 LEDGER lines across 5
pre-policy commits; the hub/edi-ui discard these at integration.)

## ▶ RESUMED 2026-06-17 — staged live test of the new toolbelt + Sonnet builder
- New protocol absorbed: `edi-gate` (build+ctest+scan, auto-excludes shell golden),
  `bus-reply`/`bus-cat`/`bus-next`; model tiering (builder=Sonnet → brief PRECISELY,
  re-prime fresh windows). LEDGER is edi-ui's on master only (already adopted).
- DR-11+fix (`db71c1b`) still NOT merged (master `5712648`); per hub, build DR-12 on
  `dept/drafting` anyway (edi-ui integrates separately). DR-12 deps DR-01 ✅ + DR-04 ✅
  (both on master). Opened DR-12 as the live-test slice (pure op, well-scoped for Sonnet).

## ⏸ PAUSED — HUB CONTEXT SWAP (2026-06-17) — clean checkpoint
- **State:** clean. Worktree clean, no builder mid-task. `dept/drafting` tip `11d8cea`
  (docs); last CODE tip `db71c1b` (DR-11 + canonical-mirror rect fix).
- **In flight / awaiting:** DR-11 + the canonical-mirror fix are CLOSED and reported to
  edi-ui as a batch (pull to `db71c1b`); awaiting edi-ui's merge-confirm. Hub was flagged
  on the shared `mirrorGeometry` rotated-rect change (dungeon-map consumes).
- **Held (NOT scooped):** DR-12 array-along-curve — deliberately held until edi-ui confirms
  the `db71c1b` batch merged (post-merge sequencing).
- **RESUME for a fresh session:** (1) read this handoff + the INTEGRATION ROUTING / CADENCE /
  POLICY blocks above. (2) Check whether edi-ui merged `db71c1b` (look for a reply in
  `~/dept-bus/drafting/replies/` and `git merge-base --is-ancestor db71c1b master`). (3) If
  merged → open **DR-12** (deps DR-01 ✅ + DR-04 ✅): brief the builder to `git rebase master`
  (the REF) then implement `arrayAlongCurve` per `~/dept-bus/work-batch-plan.md` DR-12. (4)
  Remaining after DR-12: DR-13 angular dim (ratified: INFER vertex, no new DimensionGeometry
  field; CORE-region serialize-touching — brief carefully, consider a reviewer note), DR-14
  arc-sweep dim (deps DR-13), DR-15 fill authoring (ratified: fill-selected-closed-object).
  Bucket progress: DR-01..10 merged; DR-11+fix reported/pending-merge; DR-12..15 remain.

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

### DR-08 — CLOSED 2026-06-17 ✅ (SHA `e309f25`, 98/98 green) — validated-pattern reuse
- Reply: `~/dept-bus/edi-drafting/replies/020-DR08-extend-builder.md`. Big rebase onto
  local `master` replayed CLEAN (DR-01..07 dropped as already-applied; no LEDGER/CMake
  conflict). Post-rebase gate green BEFORE building. Then: chamfer shallow-corner guard
  (Part A, closes the DR-07 audit note; reject when bevel ≤ 1e-6), `extendLineToBoundary`
  (Part B, mirror of trim — target treated infinite, boundary as segment, nearest
  crossing beyond the picked end), controller `ExtendPoint` verb (Part C, single
  `applyCommandAndEmit`, pointer-capture-before-apply). Existing paths byte-unchanged.
  (98 vs master's 99 = the box `-E`'s the edi-ui shell golden test.)
- **Observed on master:** H2 `DraftingMapTypes.h` extraction has LANDED (dungeon-map);
  map structs reach `DraftingDocument.h` transitively — no action our side. Arch doc §6
  updated.
- Reported DR-08 (`e309f25`) to edi-ui to pull.

### DR-09 — break/divide line or polyline (op + controller) — builder BRIEFED 2026-06-17
- Brief: `~/dept-bus/edi-drafting/briefs/021-DR09-break-builder.md`. Deps none. Atomic
  2-object edit (UpdateGeometry original→first + CreateObject second) — reuses the
  validated controller pattern (like chamfer's atomic bracket). Lighter scrutiny.

- **MERGED (edi-ui):** DR-08 on master @`9e9a1ab` (100/100). DR-09 reported next.

### DR-09 — CLOSED 2026-06-17 ✅ (SHA `d1d1501`, 99/99 green) — validated-pattern reuse
- Reply: `~/dept-bus/edi-drafting/replies/021-DR09-break-builder.md`. Rebase clean.
  `breakGeometryAtPoint` (Line clamped-foot split; Polyline segment split with the vertex
  in BOTH halves; near-endpoint epsilon 1e-6 on whole-shape ends only; unsupported reject)
  + `BreakPoint` controller verb (chamfer atomic 2-object pattern: UpdateGeometry original
  + CreateObject piece, id/layer/style/kind captured before the bracket). Existing paths
  byte-unchanged.
- **RATIFIED convention (planner):** selection-after-edit is PER-VERB by what the user
  most likely wants next — **break keeps the ORIGINAL (first piece) selected**;
  chamfer/fillet select the NEW object. No functional issue; trivial to flip if edi-ui
  wants uniformity later. Recorded so it isn't re-litigated.
- Reported DR-09 (`d1d1501`) to edi-ui.

### DR-10 — rotate-copies rosette (op + controller) — builder BRIEFED 2026-06-17
- Brief: `~/dept-bus/edi-drafting/briefs/022-DR10-rotate-copies-builder.md`. Deps DR-01 ✅
  (first real transformGeometry CONSUMER). **Controller fork RESOLVED:** SIBLING capture
  intent `RotateCopiesCenter` (not a flag on the existing radial path) — leaves
  `radialArrayDraftingObject`/`RadialArrayCenter` byte-unchanged; reuses
  `createArrayFromActiveObject`. Lighter scrutiny (validated array pattern).

- **MERGED (edi-ui):** DR-09 on master @`4103dab` (100/100). edi-ui flagged DR-09 carried
  a stale-base dup of DR-08 (timing race). **CADENCE FIX:** open slice N+1's build only
  AFTER edi-ui confirms slice N merged → the builder always rebases onto a master that
  carries the prior tip. (Local master is updated in-place by edi-ui on the box; it was
  never stale persistently — just a parallel-open race.)

### DR-10 — CLOSED 2026-06-17 ✅ (SHA `6d0cc01`, 99/99 green) — first transformGeometry consumer
- Reply: `~/dept-bus/edi-drafting/replies/022-DR10-rotate-copies-builder.md`. Rebase CLEAN
  (base `f65e972` current — DR-07/08 cherry-pick-skipped; no dup this time). Sibling
  `RotateCopiesCenter` intent (radial path byte-unchanged); `rotateCopiesDraftingObject`
  uses `transformGeometry` per copy. Arm distribution mirrors radial exactly (step =
  totalAngle/(copies+1); 3 copies/360 → spokes 90/180/270). `m_rotateCopiesTotalAngleDeg`
  default 360, edi-ui surfaces it.
- **RATIFIED (planner):** center==source-centre is ALLOWED for rotate-copies (valid rosette;
  intentional difference from radial's zero-arm reject). Guides rejected (would stack).
- Reported DR-10 (`6d0cc01`) to edi-ui. **DR-11 HELD until edi-ui confirms DR-10 merged**
  (new post-merge sequencing).

- **MERGED (edi-ui):** DR-10 on master @`281e93a` (100/100). edi-ui FIX: rebase onto the
  `master` REF (`git rebase master`), never a pinned SHA — folded into the builder rule.

### DR-11 — kaleidoscope / multi-axis radial mirror (op + controller) — builder BRIEFED 2026-06-17
- Brief: `~/dept-bus/edi-drafting/briefs/023-DR11-kaleidoscope-builder.md`. Deps DR-01 ✅.
  Opened AFTER DR-10 merge-confirm (new sequencing — builder rebases onto a master with
  DR-10). **Higher-risk slice:** extends `mirrorGeometry` to an arbitrary axis line (the
  guarded visit + the two-hand-kept-lists note, arch §2) + per-kind reflection correctness
  (arc sweep / rectangle rotationDeg flip orientation under reflection). **Plan: diff-audit
  after build** (canonical H/V path byte-identical; supportsMirror sync; angle-flip
  correctness per kind).

### DR-11 — SHIPPED `02fd45e`, 101/101 → diff-audit OPEN (rectangle-reflection question)
- Reply: `~/dept-bus/edi-drafting/replies/023-DR11-kaleidoscope-builder.md`. Compose approach
  `R(+θ)∘Mx∘R(−θ)` (transformGeometry rotations + canonical mirror via zero-size-bounds-at-C);
  canonical path untouched. Builder corrected my brief: Arc is NOT in `supportsMirror` (7
  mirrorable kinds per arch §2) → flip pinned on Dimension offset instead. Sibling
  `KaleidoscopeCenter` verb.
- **Diff-audit OPEN** (`~/dept-bus/edi-drafting/briefs/024-DR11-audit-reviewer.md`) with a
  PRIORITY correctness question: the canonical rectangle arm does NOT flip `rotationDeg` under
  mirror, so the conjugation nets `rotationDeg` UNCHANGED — but a true reflection maps `r → −r+2θ`.
  Tracing suggests the canonical mirror is ALREADY latently wrong for ROTATED rectangles
  (even at orthogonal axes), and DR-11 inherits it. Audit to adjudicate the math + recommend
  fix-now (flip `rotationDeg` in the canonical arm) vs accept-v1-limitation, with blast radius.
### DR-11 audit — DONE 2026-06-17 — VERDICT: ACCEPT-with-FOLLOWUP
- Reply: `~/dept-bus/edi-drafting/replies/006-DR11-audit.md`. DR-11 itself CORRECT + clean
  (conjugation verified numerically — all 4 reflected positions recomputed; canonical path
  +202/−0 untouched; supportsMirror gate + Arc reject + Dimension-offset flip all confirmed).
- **Math CONFIRMED:** the canonical rectangle mirror arm never flips `rotationDeg` → reflection
  result off by `2(θ−r)`. Pre-existing latent bug (single-axis mirror of a rotated rect is
  already wrong: r=30°→30°, should be −30°); DR-11 inherits it. Position is correct; ONLY
  `rotationDeg` is wrong.
- **FIX-NOW (audit-prescribed, one line):** `rotationDeg = -rotationDeg` in the canonical rect
  arm, OUTSIDE the H/V branch — fixes single-axis AND DR-11 (nets `−r+2θ`). Zero existing-test
  breakage (only rect mirror test is axis-aligned, `−0==0`). It's a behavior CHANGE → own
  "fix" commit + 2 new tests. mirrorGeometry is shared (dungeon-map consumes) → FLAG to hub.

### DR-11-fix — builder BRIEFED 2026-06-17
- Brief: `~/dept-bus/edi-drafting/briefs/025-DR11fix-mirror-rect-rotation-builder.md`. One-line
  canonical-arm fix + 2 tests. Lands on top of DR-11. Accept inline when green (audit
  pre-verified the exact change).
- **Plan:** when the fix lands → report DR-11 (`02fd45e`) + fix as ONE batch to edi-ui, and
  FLAG the hub re: the shared canonical-mirror rotated-rect semantics change (dungeon-map
  consumes; position unaffected, orientation-only — a fix they want). THEN open DR-12.

### DR-11 + canonical-mirror fix — CLOSED 2026-06-17 ✅ (tips `02fd45e` + `db71c1b`, 101/101)
- Fix reply: `~/dept-bus/edi-drafting/replies/025-DR11fix-mirror-rect-rotation-builder.md`.
  One-line rect `rotationDeg` negation in the canonical arm; 2 new tests (single-axis r=30°→−30°;
  kaleidoscope axis-aligned across 30° axis → 60°); old axis-aligned test still green. Accept
  inline (audit pre-verified the exact change). Arch doc §2 mirror note updated.
- Reported DR-11 + fix as ONE batch to edi-ui (tip `db71c1b`).
- **FLAGGED to hub:** shared canonical-mirror semantics change — rotated-rect orientation under
  reflection now corrected (`−r`); dungeon-map consumes `mirrorGeometry`; position unaffected,
  axis-aligned rects unchanged. A fix dungeon-map wants.

### DR-12 — array-along-curve (PURE op) — builder BRIEFED 2026-06-17 (live toolbelt test)
- Brief: `~/dept-bus/edi-drafting/briefs/026-DR12-array-along-curve-builder.md` (fresh Sonnet
  builder — precise brief, fresh-window re-prime, concrete test numbers). Deps DR-01 ✅ + DR-04
  ✅. Built on `dept/drafting` regardless of the DR-11 merge state (edi-ui integrates
  separately). Uses the new toolbelt: builder runs `edi-gate` + replies via `bus-reply`.
- When green + replied → bus-hub the result (staged live-test of the tooling). Reviewer
  optional (per hub); planner reviews the report + edi-gate is the safety net.

### DR-12 — CLOSED 2026-06-17 ✅ (SHA `e111398`, GREEN 101/101 via edi-gate) — LIVE-TOOLBELT TEST PASS
- Reply: `~/dept-bus/edi-drafting/replies/026-DR12-array-along-curve-builder.md`. Fresh
  Sonnet builder, precise brief → clean result: `arrayAlongCurve` (pure op) with
  `curveArcLength`/`curveStationPoint`/`curveTangentDeg` helpers; Line/Polyline/Arc via
  `pointAlongEntity`, Spline via a local `walkPath` on `sampleSpline` (pointAlongEntity's
  walker isn't accessible); tangent via `transformGeometry(pivot=station)`; Polygon
  rejected (not in brief). Tests: line centers 0/0.5/1.0, 45°→rotationDeg 45, arc stations
  on-arc, 3 rejections. Rebase clean (9 commits replayed). `edi-gate` GREEN, `bus-reply`
  used — the new toolbelt worked end-to-end.
- **Minor follow-up noted (not blocking):** the Spline path branch has no NUMERIC test
  (builder judged a known-sample assertion over-speccing). Add a spline-path test in a
  later cleanup; the walk mirrors the tested line/arc algorithm.
- **Live-test verdict:** PASS — precise brief + Sonnet builder + edi-gate + bus-reply +
  planner review all exercised; builder context OK (no recycle needed).

### DR-13 — angular dimension — opened at the REVIEWER GATE 2026-06-17 (representation)
- Brief: `~/dept-bus/edi-drafting/briefs/027-DR13-angular-representation-reviewer.md`.
  **Why a gate, not straight to the Sonnet builder:** the representation is genuinely
  unsettled. `DimensionGeometry` persists only `{kind,a,b,offset}` (5 scalars); an angular
  dim needs vertex(2)+2 rays(2)+arc-radius(1)=5 dof, but the ratified "store two ray
  endpoints as a/b, no new field" leaves only `offset` for the vertex+2nd-ray and CAN'T
  encode the vertex. A Sonnet builder must not improvise a PERSISTENT format. Reviewer
  (Opus, on-target, 183k — no recycle) settles the exact `{a,b,offset}` encoding + how the
  renderer recovers the vertex/angle + feasibility of "no new field" (escalate to hub if
  infeasible) + bounded slices. Then I brief the builder precisely.
- Fleet/ctx (resume duty): dept-status checked; my reviewer opus/183k OK, builder
  sonnet/fresh OK. (Researcher off-target but idle — converges at next use.)

### DR-13 reviewer gate — SETTLED 2026-06-17 — Candidate A, no new field
- Reply: `~/dept-bus/edi-drafting/replies/007-DR13-angular-representation.md`. **Representation
  = Candidate A:** `a` = vertex V (`lineIntersection(l1,l2)` at plan time), `b` = point on
  ray1 (`|b−a|` = arc radius), `offset` = signed included angle (deg) ray1→ray2 — repurposed
  for Angular kind ONLY (doesn't corrupt other kinds). 5 dof exact; recoverable from
  `{a,b,offset}` alone; round-trips verbatim. **"No new field" FEASIBLE — no hub fork
  re-open.** B rejected (vertex unrecoverable). No version bump; old-reader degrades
  angular→Distance (same additive-tolerance as prior kind adds, accepted).
- **CROSS-DEPT SEAM FLAGGED to edi-ui** (coordination, not a fork): the canvas ARC painter
  (`src/widgets/DrawingCanvas*`, edi-ui's file) must gain an Angular branch AND guard the
  linear-dim painter from mis-drawing `offset`-as-angle — must land with/before Angular is
  user-visible. DR-13 has no tool/controller wiring this slice, so no user-visible mis-render
  ships now; the flag is for when edi-ui wires the tool + arc.

### DR-13 core — builder BRIEFED 2026-06-17 (Sonnet, S1–S7)
- Brief: `~/dept-bus/edi-drafting/briefs/028-DR13-angular-core-builder.md`. The 7 settled
  core slices (enum+kind maps, planAngularDimension, dimensionMeasuredAngle, serialize
  round-trip, reject kind-change-to-Angular, projection/inspector guard, op tests). Painter
  arc + measurement-text formatting explicitly OUT. Builder sonnet/on-target/low-ctx (no cycle).

- **MERGED (edi-ui):** DR-11 + canonical-mirror fix + DR-12 on master @`f8bed78` (101/101);
  master HEAD since advanced (`ef9bf0a`). DR-01..12 now all integrated.

### QUEUED — DR-10-fix · rotate-copies total-angle clamp gap (edi-ui chrome-reviewer nit)
- `setRotateCopiesTotalAngle` (~`DrawingDocumentController.cpp:1927`) silently ignores
  `|angle| < 1.0`, but the shell's rosette spin allows `0.0` → silent field↔state divergence
  (spin shows 0, controller keeps prior angle). **Planner semantics decision:** setter STORES
  faithfully (drop the silent <1.0 swallow; keep only a non-finite guard); `rotateCopiesDraftingObject`
  (op) REJECTS a degenerate near-zero total angle with a visible code+message (surfaced via
  finishEdit). Data-oriented (validation in the op), no silent divergence, no degenerate ring.
- Small independent slice; QUEUED as the NEXT builder slice AFTER DR-13 (single builder, mid-DR-13).
  Rebase that slice onto master REF (now `ef9bf0a`+).

## Open questions / blockers
- DR-13 painter ARC is an edi-ui seam (flagged) — rides with the eventual Angular tool wiring.
- DR-10-fix (rotate-copies clamp) queued behind DR-13.

## Next
- Builder implements DR-01; planner buses the green SHA to the hub so dungeon-map
  unblocks, then opens DR-02.
