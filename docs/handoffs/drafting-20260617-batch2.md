# Handoff — drafting-20260617-batch2 (roadmap batch-2: M-tasks)

> Per-campaign state for batch-2 of the drafting roadmap (`~/dept-bus/ROADMAPS-DRAFT.md`).
> AUTONOMOUS run-ahead (user call 2026-06-17): run the queue ahead, do NOT wait for the hub
> per slice; bus-hub ONLY on milestones (closeout / blocker / cross-dept need / green tip
> ready to merge). Reviewer at checkpoints; dept-cycle workers at ticks.

- **Campaign**: drafting-20260617-batch2
- **Department**: edi-drafting
- **Queue (this run)**: M1 (delete-all-construction-lines) → M8 (motif library / flash sheet).

## ⏻ POWER-DOWN CHECKPOINT (2026-06-17) — CLEAN, nothing in flight
- **State:** clean. Worktree clean, NO slice in flight, no rebase active. `dept/drafting` tip
  `889099d` (docs); local `master` (integration line) `6bb6c14`. All drafting work GREEN
  (`edi-gate` 104/104) and merged or merging via edi-ui.
- **DONE this program:** DR-01..15 feature bucket ✅ (merged) · M1 delete-construction-lines ✅ ·
  M8 motif library (S1+S2) ✅ · DR-13 angular tool-arm ✅ (angular dim LIVE end-to-end) · M2
  intersection nodes + Node snap (S1+S2+S3) ✅. Closeouts: `drafting-fill-side-channel`,
  `drafting-cartography`, `drafting-feature-bucket`, `drafting-motif-library`,
  `drafting-intersection-nodes` (+ ruling H2 recorded in `docs/architecture/edi-drafting.md`).
- **NEXT (awaiting hub direction — named queue EXHAUSTED):** the rest of drafting batch-2 is
  done/duplicate of the DR bucket; M4 tangent-circle (Apollonius) PARKED. No genuinely-new ready
  drafting item. **Available in-charter follow-ups** (small): (a) snap-settings TOML persistence
  (own slice — NO snap flag persists today); (b) absent-`"motifs"`-key serialize test hardening;
  (c) M8-S3 motif transform-on-place fork (needs the oriented-stamping UX decision — user/edi-ui).
- **edi-ui seams pending (theirs):** Angular arc painter + linear-painter guard + `isAngleField`
  for the Angular offset field; Node snap inspector checkbox (`node_enabled` published); motif
  placement chrome (routed to the user; controller API ready); the deferred tool/belt surfaces.
- **RESUME recipe for a fresh session:** (1) read this handoff + `docs/architecture/edi-drafting.md`
  + `~/dept-bus/PROTOCOL.md`. (2) Rebase target = LOCAL `master` ref (planner-synced; origin
  reconciled, now a backup). (3) If the hub assigns a campaign → open it (reviewer gate first for
  any persistent-format/representation question, e.g. DR-13/M8 pattern). (4) Else if told "take
  the cleanups" → do (a)/(b)/(c) above as small builder slices. (5) Report green tips to `edi-ui`
  (merge owner); bus-hub on milestones only.

## Standing rules (carry into every brief)
- **REBASE PRACTICE** (ALL-CLEAR 2026-06-17 — origin reconciled): builders rebase onto the
  LOCAL `master` ref (`git rebase master`) — it is the planner-synced, always-current
  integration line. The stale-origin corruption trap is GONE (origin/master fast-forwarded to
  `17c716a`, all batch work; now a current backup). Standing practice UNCHANGED: target LOCAL
  `master` (no ancient-conflict risk); `git fetch`/origin is no longer dangerous but local
  remains the authoritative rebase target.
- LEDGER is edi-ui's (master only) — never commit it here. Toolbelt: `edi-gate` + `bus-reply`.
- edi-ui = merge owner (route green tips there). Painter/tool chrome = edi-ui seams.

## Gate log

### M1 — delete-all-construction-lines — builder BRIEFED 2026-06-17
- Brief: `~/dept-bus/edi-drafting/briefs/032-M1-delete-construction-lines-builder.md`.
  **Planner call (resolves a stale spec premise):** no generic multi-delete command exists
  (only `DeleteObjectCommand` + the per-kind `DeleteAllGuidesCommand`), so M1 ADDS
  `DeleteAllConstructionLinesCommand` mirroring `DeleteAllGuidesCommand` EXACTLY (struct
  `DraftingCommands.h:27`, variant arm `:190`, handler `DraftingCommands.cpp:140`) +
  `deleteAllConstructionLines()` mirroring `deleteAllGuides()` (`:3249`). Commands aren't
  serialized → contained. Sonnet, single slice.

### M8 — motif library (flash sheet) — REVIEWER GATE SETTLED 2026-06-17
- Reply: `~/dept-bus/edi-drafting/replies/008-M8-motif-representation.md`. **SETTLED:**
  `DraftingMotif{name, vector<DraftingObject> objects, Bounds2D bounds}` — a document-level CORE
  field on `DraftingDocument` (beside objects/layers, NOT the map region); a deliberate TWIN of
  the map-owned `DraftingBlock` minus `id`/`assetRef`/Seam-B (name-keyed, no id mint). Serialize
  = additive `"motifs"` key, no version bump, reuse the `DraftingObject` codec, default-empty
  when absent. FLATTEN-on-place via the EXISTING plural `CreateObjectsCommand` (no new place
  arm); capture EXCLUDES guides, INCLUDES construction lines. Capture rides a transient
  `CreateMotifCommand`.
- **S3 transform-on-place: DEFERRED behind a fork** (planner accepts the reviewer rec — S2
  ships translate-only; oriented stamping decided after S2, gated on the placement-UX cost).
- **Hub RECORD (scope, sent):** `DraftingMotif` is an INTENTIONAL core twin of the map-owned
  `DraftingBlock` — conscious duplication, NO shared base now (revisit only if a 3rd consumer
  appears). Recorded with the hub re: the H2 boundary; not a fork re-open.
- **M8-S1 brief WRITTEN + ready** (`~/dept-bus/edi-drafting/briefs/034-M8-S1-motif-capture-builder.md`):
  struct+field, `buildMotifFromObjects`/`addMotif` (mirror block ops), `CreateMotifCommand`,
  `"motifs"` serialize + round-trip. QUEUED behind M1 (single builder) — FIRE when M1 lands.
- M8-S2 (place/FLATTEN + `MotifPlacement` intent) follows S1; reviewer diff-audit on S1 (it
  touches the persistent format) before/with S2.

### M1 — CLOSED 2026-06-17 ✅ (SHA `eeba231`, GREEN 102/102)
- Reply: `~/dept-bus/edi-drafting/replies/032-M1-delete-construction-lines-builder.md`. Mirrors
  `DeleteAllGuidesCommand` exactly (`DeleteAllConstructionLinesCommand` + `deleteAllConstructionLines()`);
  mixed-doc test pins (only CLs removed, others byte-identical, one undo, no-CL no-op). Rejected
  an over-general `DeleteAllOfKindCommand` (YAGNI). Accepted inline; reported `eeba231` to edi-ui.

### M8-S1 — DONE 2026-06-17 ✅ (SHA `940c4d1`, GREEN 103/103) → S1 audit + S2 in parallel
- Reply: `~/dept-bus/edi-drafting/replies/034-M8-S1-motif-capture-builder.md`. `DraftingMotif` +
  `motifs` field (CORE, beside objects — H2 respected, not the map region); `DraftingMotifOps`
  (build/add/remove/indexByName); `CreateMotifCommand`; serialize `"motifs"` key (additive,
  reuse object codec) + round-trip + absent-key tests. Name-keyed (no id-mint). `removeMotif`
  added (needed for tests); `DeleteMotifCommand` deferred to when a verb needs it. Reported
  `940c4d1` to edi-ui.
- **PARALLEL (run-ahead):** reviewer **diff-audits S1** (persistent-format checkpoint — serialize
  symmetry/round-trip/additive-tolerance/H2; brief 035) WHILE the builder builds **M8-S2**
  (place/FLATTEN + `MotifPlacement` intent; brief 036). S2 uses the struct, not the serialize,
  so it's independent of the audit. (DG/MotifDG context OK — no recycles.)
- **DEFERRED:** S3 transform-on-place (behind the fork).

## DG (DraftingMotif) DESIGN NOTE
DraftingMotif is core-owned, name-keyed, FLATTEN-on-place — a deliberate twin of the map block
(hub-recorded). Place = fresh ordinary objects (new ids via m_nextObjectSerial), one
CreateObjectsCommand = one undo.

### M8-S1 audit — DONE 2026-06-17 — VERDICT: ACCEPT
- Reply: `~/dept-bus/edi-drafting/replies/009-M8-S1-audit.md`. Persistent-format surface VERIFIED:
  `motifs` encode/decode are exact inverses (reuse object codec; `bounds` derived, not
  serialized); additive-tolerant, no version bump; capture excludes guides + resets
  lock/visible + normalizes to (0,0); H2 clean (core struct, name-keyed, no id/assetRef);
  `CreateMotifCommand` arm satisfies the static_assert; scope clean. The new `"motifs"` key is
  safe to land in real `.edidraw` files.
- **Non-blocking nit (deferred test-hardening):** the absent-`"motifs"`-key decode (old files →
  empty motifs) is correct-by-inspection but only the empty-ARRAY case is directly tested. Add a
  genuinely-absent-key serialize test — fold into the next builder touch (M8-S2 accept or a tiny
  follow-up); NOT blocking (the additive-tolerance is the established wall_visual/plug pattern).

### M8-S2 — CLOSED 2026-06-17 ✅ (SHA `c566987`, GREEN 103/103) → M8 COMPLETE
- Reply: `~/dept-bus/edi-drafting/replies/036-M8-S2-motif-place-builder.md`. `placeMotif` FLATTEN
  (fresh ids, no back-ref, translate motif (0,0)→point), `defineMotifFromSelection` +
  `MotifPlacement` intent + `beginMotifPlacement`/`runMotifAtPoint`, one `CreateObjectsCommand` =
  one undo. Mirrors the validated array/capture pattern. Accepted inline. Reported `c566987` to edi-ui.
- **M8 COMPLETE** (S1+S2). Closeout: `docs/closeouts/drafting-motif-library.md`. S3
  transform-on-place DEFERRED behind the fork (decide after v1 translate-only is in use; UX is
  edi-ui's). M8-S1 merged @`fa8afb2`.

### DR-13 follow-up — AngularDimension TOOL KIND + two-line-pick arm — builder BRIEFED 2026-06-17
- Brief: `~/dept-bus/edi-drafting/briefs/037-DR13-angular-tool-arm-builder.md`. edi-ui surfaced a
  cross-dept gap (`replies/037-...`): DR-13 core shipped but the TOOL path is incomplete —
  `DraftingToolKind::AngularDimension` + the two-line-pick controller arm (→ `planAngularDimension`)
  are MINE (I'd over-deferred them as "tool wiring"). edi-ui's belt cell + combo are merged
  (`c498c9c`), waiting on this arm. PRIORITY over M2 (completes already-chromed work). Mirrors the
  fillet two-line capture. Builder told to STOP-ask if the tool-dispatch hook is ambiguous.

### M2 — materialize intersections (drop points where lines cross) — QUEUED next
- Confirmed genuinely NEW (DR-02 added the intersection SNAP source; M2 materializes Point
  OBJECTS at crossings — a create op). 3 slices (op / command+verb / Node snap source). dep:none.
  Queue AFTER the DR-13 arm (single builder). NB: roadmap M3/M5/M6/M7 OVERLAP the DR bucket
  (already done: DR-05/06, DR-10/11, DR-07/08/09, DR-13/14) — M2 is the next genuinely-new item;
  M4's tangent-circle (Apollonius) stays parked.

### DR-13 angular tool-arm — CLOSED 2026-06-17 ✅ (SHA `1fd80fb`, GREEN 103/103)
- `DraftingToolKind::AngularDimension` + `angular_dimension_tool` id; two-line-pick arm in
  `clickCanvasNormalized` + `applyAngularDimensionAtPoint` + `PointCaptureIntent::AngularDimensionSecondLine`
  + `m_pendingAngularFirstLineId` (id stored, not pointer). Tests: nominal one-undo + 3 rejection
  paths. Diff +214/−0, all my files (core/toolcreation + tests). Spot-checked, accepted. Reported
  `1fd80fb` to edi-ui → unblocks their end-to-end Angular confirmation + arc painter. **DR-13
  fully end-to-end now** (core + tool + chrome).

### M2 (S1+S2) — materialize intersections — builder BRIEFED 2026-06-17
- Brief: `~/dept-bus/edi-drafting/briefs/038-M2-intersection-points-builder.md`. Pure op
  `documentIntersectionPoints` (visible Line+ConstructionLine segments, segmentIntersection,
  dedup) + `dropIntersectionPoints()` verb (CreateObjectsCommand, auto-select, idempotent
  skip-existing, selection-scoped). dep:none; no reviewer gate (reuses settled machinery). M2-S3
  (Node snap source) is the next slice after.

### M2 (S1+S2) — CLOSED 2026-06-17 ✅ (SHA `34aa0da`, GREEN 104/104)
- New `DraftingIntersectionOps.{h,cpp}`: `documentIntersectionPoints` + `subsetIntersectionPoints`
  (selection-scoped) + `filterExistingIntersections` (idempotent skip-existing). `dropIntersectionPoints()`
  verb reuses `createObjectsAndSelect` (atomic CreateObjectsCommand + auto-select, one undo).
  New test file + controller tests. Diff +490/−0, all my files. Spot-checked, accepted. Reported
  `34aa0da` to edi-ui.

### M2-S3 — Node snap source — builder BRIEFED 2026-06-17 (completes M2)
- Brief: `~/dept-bus/edi-drafting/briefs/039-M2-S3-node-snap-builder.md`. Add `Node` snap kind +
  `nodeEnabled` flag + emission for Point objects (so the dropped intersection nodes snap) +
  controller getter/setter via `setSnapFlag` (DrawingCore.h:440) + projection `node_enabled` key.
  TOML persistence: mirror IF snap flags persist, else note deferred (DR-02/03 flags aren't
  persisted either). Inspector checkbox = edi-ui. Anchors confirmed.

### M2-S3 — CLOSED 2026-06-17 ✅ (SHA `9e52c61`, GREEN 104/104) → M2 COMPLETE
- `Node` snap kind + `nodeEnabled` + emission for Point objects (dropped intersection nodes +
  standalone Points snap); controller getter/setter via `setSnapFlag`; projection `node_enabled`.
  **Precedence:** Node emitted BEFORE Endpoint so Endpoint shadows it (the `<=` last-wins rule) —
  Node surfaces only when endpoint off (same discipline as DR-02 OnCurve); documented + tested.
  **TOML: snap flags are NOT persisted at all** → node-only would be inconsistent → deferred (own
  slice). Diff +113/−1, all my files. Accepted inline. Reported `9e52c61` to edi-ui.
- **M2 COMPLETE** (S1+S2+S3). Closeout: `docs/closeouts/drafting-intersection-nodes.md`.

## ✅ NAMED BATCH-2 QUEUE EXHAUSTED 2026-06-17
- Completed this run: M1 (delete-construction-lines), M8 (motif library, S1+S2), DR-13 angular
  tool-arm (→ angular dim LIVE end-to-end), M2 (intersection nodes + Node snap, S1+S2+S3).
- **Remaining drafting batch-2 (ROADMAPS-DRAFT.md M0-M8) is DONE or DUPLICATE:** M0=fill-svg ✅,
  M3≈DR-05/06 ✅, M4 = circle-3pt (DR-05 ✅) + tangent-circle (PARKED — L, needs an Apollonius
  solver), M5≈DR-10/11 ✅, M6=DR-07/08/09 ✅, M7=DR-13/14 ✅. No genuinely-new ready drafting item left.
- **Available in-charter follow-ups (small, deferred):** (a) snap-settings TOML persistence (own
  slice); (b) absent-`"motifs"`-key serialize test hardening; (c) the M8-S3 motif transform-on-place
  fork (needs the oriented-stamping UX decision — user/edi-ui).
- **Reported to hub** (queue-exhausted scope decision): awaiting the next campaign / a fresh
  roadmap pull, OR direction to take the cleanups. NOT inventing a large feature without direction.

## Open questions / blockers
- Awaiting hub direction: next campaign vs the deferred cleanups (a/b/c above). Parked: M4
  tangent-circle (Apollonius), M8-S3 transform fork.
- (Not pausing for the dogfood/use-report fork — user chose autonomous.)

## Next
- M1 lands → spot-check + report green tip to edi-ui. M8 gate settles → brief M8 Slice 1
  (capture + serialize) to the builder. Continue the batch-2 queue ahead.

## M0 SUPPORT (2026-06-18) — HOLD READY
- Mode: drafting is the SUPPORT dept for the M0 crypt slice (~/dept-bus/M0-CRYPT-SLICE.md).
  dungeon-map builds the generator → MapSpec → createMapFromSpec; blender-lab realizes/renders.
- My scope if bussed: MapSpec / RoomSpec / DraftingRoom struct tweaks + createMapFromSpec assist
  (src/core, src/drafting). dungeon-map will bus the exact need; I do NOT pre-build.
- State: dept/drafting rebased onto LOCAL master (tip 297d5a6, base b3e2932), edi-gate GREEN 104/104.
- READINESS NOTE (reported to hub): block-instance machinery EXISTS (DraftingBlock+assetRef in
  DraftingMapTypes.h; placeBlockInstance/defineBlockFromSelection controller verbs in DrawingCore.h),
  but MapSpec (DraftingRoom.h:119) has NO field to DECLARE a block instance (asset_ref + transform)
  for createMapFromSpec to stamp. RoomFeature carries only neutral type/name → realized as a bare
  Point marker. The crypt's sarcophagus + brazier (asset_ref crypt.sarcophagus / crypt.brazier+light)
  will most likely need a MapSpec block-instance authoring field + a createMapFromSpec stamp arm
  reusing placeBlockInstance's flatten path. Pre-scoped; awaiting the dungeon-map bus to brief builder.
- RESUME: re-read this section + ~/dept-bus/M0-CRYPT-SLICE.md + bus log tail; check
  ~/dept-bus/edi-drafting/briefs|replies + ~/dept-bus/drafting/replies for any M0 brief; hold if none.
