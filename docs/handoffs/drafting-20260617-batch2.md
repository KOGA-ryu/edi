# Handoff — drafting-20260617-batch2 (roadmap batch-2: M-tasks)

> Per-campaign state for batch-2 of the drafting roadmap (`~/dept-bus/ROADMAPS-DRAFT.md`).
> AUTONOMOUS run-ahead (user call 2026-06-17): run the queue ahead, do NOT wait for the hub
> per slice; bus-hub ONLY on milestones (closeout / blocker / cross-dept need / green tip
> ready to merge). Reviewer at checkpoints; dept-cycle workers at ticks.

- **Campaign**: drafting-20260617-batch2
- **Department**: edi-drafting
- **Queue (this run)**: M1 (delete-all-construction-lines) → M8 (motif library / flash sheet).

## Standing rules (carry into every brief)
- ⛔ **REBASE GUARD** (hub, still on until ALL-CLEAR): builders rebase ONLY onto the LOCAL
  `master` ref (`git rebase master`); NO `git fetch`, NO `origin/master` (stale `591e92c`,
  corrupts the branch). Origin reconcile in flight.
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

## Open questions / blockers
- Deferred: harden the absent-`"motifs"`-key serialize test (non-blocking).
- (Not pausing for the dogfood/use-report fork — user chose autonomous.)

## Next
- M1 lands → spot-check + report green tip to edi-ui. M8 gate settles → brief M8 Slice 1
  (capture + serialize) to the builder. Continue the batch-2 queue ahead.
