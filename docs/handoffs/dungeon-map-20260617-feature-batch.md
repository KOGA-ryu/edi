# Handoff — dungeon-map-20260617-feature-batch

> Per-campaign state + the live DM task board. Each gate appends its result; the
> board is the source of truth a fresh session recovers from.

- **Campaign**: dungeon-map-20260617-feature-batch
- **Department**: edi-dungeon-map
- **Goal**: the usable-map polish ring past the stop-line — frame-on-load,
  interior features, neutral plug flags, Seam-C edited round-trip, region fill,
  map browser content, block rotation/scale record+export. DM-01..13 NOW;
  DM-14/15 (per-instance geometry transform) WAIT for drafting's
  `transformGeometry` (DR-01) — hub signals when merged to master.

## Operating rules (HUB dispatch 2026-06-17)
- I build **OP/controller verbs + pure plans + data/parse/persist/export ONLY**.
  **edi-ui wires the chrome** from the surface spec (`~/edi/docs/ui-surface/
  dungeon-map/DM-surfaces.md`) separately. Do NOT edit the edi-ui-owned shell
  files (`EdiShellWindow*`, `app/main.cpp`, `CMakeLists.txt`,
  `DrawingCanvasWidget`, `DraftingFeaturePanels.cpp`) — that's a hub/edi-ui hand.
- Autonomous through gates: reviewer boundary ONLY where genuinely needed → builder
  → green gate (`cmake --build build && ctest -E edi_shell_window_tests` + scan +
  reference-dungeon snapshot) → next. Rebase on master at each task start.
- Commit to `dept/dungeon-map`; do NOT merge/push — the hub routes verified work
  to edi-ui. Report to hub every ~3 tasks / on a blocker / at batch-done.
- Forks RATIFIED (follow the spec's recommendations): plug-flags = open token run;
  region-fill arm = inspector `fillRegionButton` (A); DM-14 spins = Left panel;
  DM-11 golden = edi-ui co-bless when it lands.

## transformGeometry contract (for DM-14/15 — RECORD, hub signal 2026-06-17)
DR-01 keystone is BUILT (drafting `167768e`) and edi-ui is MERGING it to master.
**DM-14/15 stay parked until it lands on MASTER** — then rebase and build them.
Verify with `git grep transformGeometry master -- src/drafting/DraftingGeometry.*`
before dispatching DM-14/15. (As of this note: NOT on master; tip still `f87bc1b`.)

**Signature:** `transformGeometry(g, pivot, rotationDeg /*deg CCW*/, scale /*uniform*/)`.
- **Rectangle = CENTER-anchored** under transform (Ruling A) — not corner-anchored.
- **LOSSY cases (do NOT assume fidelity):** a rotated non-circular **Ellipse drops
  its axis tilt**; rotated **Text drops baseline angle**; **Guide = identity** (a
  rotate/scale is a no-op on guides). So a block whose objects include ellipses/
  text/guides will NOT perfectly preserve tilt under per-instance rotation — record
  this as a known v1 fidelity note in the DM-14/15 brief + acceptance (test with
  shapes that DO transform faithfully: line/rect/polygon/circle/arc/spline).
- It is **drafting-owned** (`DraftingGeometry.{h,cpp}`); dungeon-map CONSUMES it via
  `placeBlockInstance` (DM-14) and `transformBlockInstance` (DM-15). Do not modify it.

## Master state note
Rebased onto master `f87bc1b` (edi-drafting's `<memory>` test fix — my identical
`c9d6156` was auto-skipped). My 22 cartography commits ride on top, still unmerged
(hub routes them to edi-ui). `origin/dept/dungeon-map` diverged (box vs Mac, 6
commits) — hub is settling; I do not push.

## Task board (DM-01..15)
Legend: ✅ done · ▶ in gate · ◻ queued · ⏸ waiting (DR-01) · 🟦 edi-ui-coordinated (I build only the OP/controller sliver)

| Task | What (my sliver) | Wave/dep | Status |
| --- | --- | --- | --- |
| DM-04 | plug flags: `DraftingPlug.flags` + `RoomPlugSpec` parse (comma-split) | W1 | ✅ `29be7a7` |
| DM-05 | persist plug flags: additive `plugValue`/`readPlug`, no bump | W2 ←04 | ✅ `620a3c2` |
| DM-06 | plug flags → TOON: `writePlugRow` `flags` column, both overloads | W3 ←05 | ✅ `e429af5` |
| DM-02 | interior features: `RoomSpec.features` data + parse | W1 | ✅ `b330269` |
| DM-03 | features → Point markers in `createMapFromSpec` (+`feature:<type>` tag) | W2 ←02 | ✅ `4cfc7dd` |
| DM-07 | Seam C edited round-trip: store room footprint+name in `document.rooms` | W1 | ✅ verified intact (no code) |
| DM-08 | Seam C round-trip regression test | W2 ←07 | ✅ `67c608e` |
| DM-12 | block rotation/scale fields: `BlockPlacementMetadata.rotationDeg/scale` + persist | W1 | ✅ `30ee3b3` |
| DM-13 | export reads rotation/scale (replace 1/0 placeholders) | W3 ←12 | ✅ `d9022b9` |
| DM-09 | region-fill boundary trace: pure `planRegionFill` (`DraftingRegionFill`), algo (a) footprint | W1 | ✅ `55e5264` (rebased) |
| DM-10 | region-fill controller verb: `PointCaptureIntent::RegionFill` → filled Polygon | W2 ←09 | ✅ `012d48f` (rebased) |
| DM-14 | place rotated/scaled block (`placeBlockInstance` extended) | ←12,DR-01 | ✅ `419db06` (rebased) |
| DM-15 | transform placed instance (`transformBlockInstance`) | ←14,DR-01 | ✅ `4beb3b5` (rebased) |
| DM-01 | view-auto-fit sliver: `computeDocumentBounds()` getter | 🟦 W1 | ✅ `32b297b` |
| DM-11 | map browser sliver: shared `DraftingMapQuery` (`deriveEdge`+`connected`) | 🟦 W1 | ✅ `1b8dacb` |

## Gate log

### Kickoff — 2026-06-17 — edi-dungeon-map-planner
- Read dispatch + DM bucket + surface spec. Rebased on master. Dispatched
  batch-1 (plug-flags spine DM-04/05/06) to builder + a region-fill reviewer
  boundary gate (DM-09/10) to reviewer in parallel.

### Reviewer boundary — region fill (DM-09/10) — 2026-06-17 — edi-dungeon-map-reviewer (SETTLED YES)
- Reply: `~/dept-bus/edi-dungeon-map/replies/008-reviewer-region-fill-boundary.md`
- **DISTINCT from drafting's DR-15** (DR-15 recolors a SELECTED object's
  FillStyle; ours mints a NEW Polygon from a seed CLICK). No overlap, no hub
  escalation — provided the builder holds to (a)/(b) and does NOT do (c).
- **Algorithm: (a) room-footprint lookup for v1** (find the `DraftingMapRoom` whose
  footprint contains the seed via `boundsContainsPoint`, emit its 4 corners as a
  closed Polygon). **(b) general wall-loop trace PARKED** (forward-compat: add a
  `walls` param later); **(c) closed-object containment REJECTED** (≈ DR-15).
- **Home: `src/drafting/DraftingRegionFill.{h,cpp}` — NEW, wholly OURS** (map
  domain — reads `DraftingMapRoom`). Test `tests/drafting_region_fill_tests.cpp`
  in `tests/CMakeLists.txt`.
- Signature: `RegionFillPlan planRegionFill(Point2D seed, const
  std::vector<DraftingMapRoom>& rooms)`. Controller verb:
  `PointCaptureIntent::RegionFill` + `beginRegionFillPick()` (REFUSE when
  `m_document.rooms` empty) + `fillEnclosedRegion(Point2D)`, wired into the
  `resolvePointCapture` switch (no default → exhaustiveness enforced).
- **Neutral CONFIRMED:** emits a Polygon + `FillStyle` only; NO `ObjectRole`/tag.
- v1 fidelity (documented): fills the AUTHORED footprint (ignores wall
  half-thickness); a click with no containing room footprint = the no-op refusal.
- **Note:** DM-09/10 reads `document.rooms`, which `createMapFromSpec` already
  populates on map load (so it works pre-DM-07); DM-07 just makes it survive
  save/reload. No hard dependency, but DM-07 (batch-3) lands before batch-5.
- Cross-dept flag: `fillRegionButton` arming widget is edi-ui's — controller verb
  is ours. Recorded for the hub.

### Builder batch-5 (region fill DM-09/10) — QUEUED
- Brief: `~/dept-bus/edi-dungeon-map/briefs/009-builder-region-fill.md` (written;
  dispatch after batches 2–4 free the builder).

### Builder batch-1 (plug-flags DM-04/05/06) — 2026-06-17 — DONE
- Reply: `~/dept-bus/edi-dungeon-map/replies/007-builder-plug-flags-spine.md`
- Commits `29be7a7`/`620a3c2`/`e429af5`. Gate GREEN 95/95, scan clean, snapshot
  identical, export rooms[12]/plugs[26]/connections[12]. Neutral law honored
  (`flags` = bare `vector<string>`); additive codec, NO bump (v2); TOON golden
  re-blessed (new `flags` column, empty→`""`). Builder resolved: `RoomPlugPlacement`
  is the mint carrier (mirrored `type`); `splitCommaTokens` file-local helper.
- Noted (not built): connections could carry the same neutral flags vocabulary
  symmetrically — out of scope, parked.

### Reviewer diff-audit of batch-1 — 2026-06-17 — edi-dungeon-map-reviewer (CLEAN)
- Reply: `~/dept-bus/edi-dungeon-map/replies/010-reviewer-batch1-audit.md`
- **batch-1 audit CLEAN.** Additive codec verified: `plugValue` APPENDS `flags`
  only when non-empty (no-flags plug is byte-identical to pre-DM-05); `readPlug`
  tolerant (absent⇒empty), mirrors `tags`; no bump (v2). Neutral law: grep proved
  NO flag-value branching anywhere — bare `vector<string>` end to end. TOON: 6-cell
  header both overloads, empty⇒`""`, `·`-run bare (U+00B7 not a quote trigger),
  golden covers both overloads × empty/non-empty. `splitCommaTokens` locale-safe,
  all edge cases. Thread-through faithful (`type` mirror); ASCII path mints no plugs
  (no drop). No scope-creep.
- **Carry to closeout (non-blocking notes):**
  - NOTE 1 — the DM-05 "four corners" = 3 executed + 1 (old-binary-reads-new-file)
    guaranteed structurally by the unknown-key-ignore contract, NOT an in-tree
    fixture. Record so a future reader doesn't expect an old-binary test.
  - NOTE 2 — a space-bearing flag token (forces `cell()` to quote the flags column)
    is correct-by-construction but not golden-exercised. Optional 1-line golden add;
    LOW value (cell()-quoting already golden-covered on origin/size). DEFERRED.

### Builder batch-2 (interior features DM-02/03) — 2026-06-17 — DONE
- Reply: `~/dept-bus/edi-dungeon-map/replies/011-builder-interior-features.md`
- Commits `b330269` (data+parse), `4cfc7dd` (Point-marker realization). Gate GREEN
  95/95, scan clean, snapshot identical, object count byte-unchanged (reference
  dungeon authors no features).
- **Planner coordinate verification (the one real risk — VERIFIED CORRECT myself):**
  the builder found my brief's premise wrong and deviated correctly.
  `parseRoomFields` (RoomSpecStore.cpp:108-116) scales `origin`/`width`/`height`/
  `plug.at` by `canvasPerUnit` at PARSE — so `RoomSpec.origin` is CANVAS units.
  Features are deliberately stored UNSCALED (authored feet, `:227-228` comment), so
  the mint `at = origin(canvas) + feature.{x,y}(feet) × canvasPerAuthoredUnit` is
  numerically correct (same 0.02 scale; test pins the absolute positions). The
  neutral law is honored + test-asserted (`ObjectRole::None`, `feature:<type>` +
  optional `name:<name>` tags; `toolProvenance="feature"`).
- **PLANNER DECISION (the asymmetry the builder flagged):** ACCEPT features-in-feet
  while the rest of RoomSpec is canvas. It is behavior-correct, well-commented, and
  the alternative (scale-at-parse) isn't clearly better — a room-LOCAL offset is
  most naturally stored as raw authored feet and resolved at the mint site where the
  origin is known. No refactor. No reviewer gate: the one real risk (coordinates) is
  planner-verified directly; batch-2 touches neither the additive codec nor adds
  rule-meaning, so the charter's two risky joints don't apply.
- Noted (not built): features are ordinary Point objects (round-trip via the object
  codec) — NOT a first-class TOON `features[]` export section. A future Seam-C
  consumer wanting that is a separate slice.

### Builder batch-3 (Seam C edited round-trip DM-07/08) — 2026-06-17 — DONE
- Reply: `~/dept-bus/edi-dungeon-map/replies/012-builder-seam-c-roundtrip.md`
- **DM-07: round-trip VERIFIED INTACT (no code).** Builder traced all 4 links —
  `createMapFromSpec`→`CreateMapRoomCommand`→`document.rooms` (5 fields);
  `mapRoomValue` save; `readMapRoom` open (tolerant); Seam C `writeRoomRow` export.
  All carry name+footprint+material. No gap. Matches arch §4/§5/§6.
- **DM-08: pinned** `67c608e` (tests only) — persistence leg (encode→decode
  byte-faithful) + export-fidelity leg (Seam C document export's `rooms[...]`
  byte-identical to Seam B spec export). Guard-checked (scratch-dropped `material`
  → test fails, reverted). Split across two existing test targets to avoid a
  CMakeLists edit (edi-ui-owned). Gate GREEN 95/95, snapshot/export unchanged.

### HUB RATIFICATION (2026-06-17) — LEDGER fix accepted; integration in progress
- **LEDGER policy CONFIRMED:** stop committing `docs/handoffs/LEDGER.md` on the
  dept branch — per-campaign handoff docs + bus-hub only. (Adopted.)
- **edi-ui is MERGING `dept/dungeon-map` → master** (resolving the LEDGER conflict).
- **DM-14/15 trigger + procedure:** when master carries (my work + transformGeometry),
  **RESET onto master** (the hub said reset, NOT rebase — because the merge puts my
  commits ON master, so reset aligns my branch to the integrated tip), THEN build
  DM-14/15. ⚠ **SAFEGUARD (do not lose batch-4/5):** edi-ui merges the branch at a
  SNAPSHOT; batch-4 (DM-12/13) and batch-5 (DM-09/10) commits made AFTER that
  snapshot will NOT be in master. So the reset is NOT a blind `reset --hard master`:
  1. `git fetch` + verify master has transformGeometry
     (`git grep transformGeometry master -- src/drafting/DraftingGeometry.h`) AND my
     merged work.
  2. `git log master..HEAD` → list MY commits not yet in master (batch-4/5 etc.).
  3. `git reset --hard master`, then `git cherry-pick` (or rebase) ONLY those
     not-yet-in-master commits back on top. Verify the green gate. THEN DM-14/15.
  4. If a cherry-picked commit conflicts on LEDGER, drop the LEDGER hunk (policy).
- I'll confirm with the hub at reset time which snapshot was merged, so batch-4/5
  reconciliation is unambiguous.

### ⚠ BLOCKER (HUB-escalated → IN PROGRESS) — dept-branch ↔ master integration + shared LEDGER
- The builder's `git rebase master` could NOT complete: **local `master` (bd3d99d)
  is the real integration line** (edi-ui integration + DR/BL merges, INCLUDING
  `transformGeometry`/DR-01) — **86 commits ahead** of this branch; my branch is 34
  ahead of it. The rebase **conflicts on the shared `docs/handoffs/LEDGER.md`** (21
  master commits + ~10 of mine touch it). (`origin/master` 591e92c is a stale box-
  vs-Mac ref, behind my branch — ignore it.)
- **This is hub-owned** (integration + LEDGER reconciliation). Escalated via bus-hub.
- **POLICY CHANGE I'm adopting now (to stop worsening it):** I will NO LONGER commit
  the shared `LEDGER.md` on `dept/dungeon-map`. Campaign state lives in THIS
  dept-local handoff doc (not cross-contended) + bus-hub reports; the hub owns the
  master LEDGER. This removes the recurring conflict source going forward.
- **DM-14/15 unblock depends on this:** `transformGeometry` is on local master, so
  once the hub integrates my branch with master, DM-14/15 can rebase + build.
- **Meanwhile:** batches 4 (DM-12/13) and 5 (DM-09/10) proceed on the CURRENT dept
  tip (the builder proved work is correct on the tip; no rebase needed for them). I
  tell the builder to SKIP the master rebase until the hub resolves integration.

### Builder batch-4 (block transform record/export DM-12/13) — 2026-06-17 — DONE
- Reply: `~/dept-bus/edi-dungeon-map/replies/013-builder-block-transform-record.md`
- Commits `30ee3b3` (fields+persist), `d9022b9` (export). Gate GREEN 95/95;
  TOON `blocks[]` golden **byte-identical** (identity `scale 1`→`"1"`, `rot 0`→`"0"`
  via `%g` = old placeholders → proves the column mapping; header
  `{room,asset,origin,scale,rotation}`, col4=scale col5=rotation). Additive codec:
  `rotation_deg`/`scale` appended only when non-default, tolerant read, NO bump (v2).
  Did NOT consume transformGeometry (identity record only). On current tip (no rebase).
- → Reviewer diff-audit dispatched (additive-codec joint).

### Reviewer diff-audit of batch-4 — 2026-06-17 — edi-dungeon-map-reviewer (CLEAN)
- Reply: `~/dept-bus/edi-dungeon-map/replies/014-reviewer-batch4-audit.md`
- **batch-4 audit CLEAN.** Additive codec: `block_placement` map appends
  `rotation_deg`/`scale` LAST only when non-default (existing 3 keys unshifted →
  identity placement byte-identical to pre-DM-12); tolerant read (absent⇒0/1); no
  bump (v2). Float guard safe (`-0.0 == 0.0` so no spurious emit; no NaN path in
  identity plumbing). Export mapping correct (scale=col4, rotation=col5; `%g`
  identity = old `"1"`/`"0"` → golden legitimately UNCHANGED, not silent re-bless).
  First-object-of-group sound (loop skips empty `instanceId`; FLATTEN shares
  metadata). Neutral law honored; no `transformGeometry` call (only a comment); no
  scope-creep.
- **DM-14 REQUIREMENT (carry into the DM-14 brief — NOTE 2):** the exact-compare
  emit guard means a computed transform landing exactly on 0/1 is (correctly)
  omitted as identity — fine. BUT DM-14 must ensure computed rotation/scale are
  **never NaN** (NaN != 0/1 ⇒ would spuriously emit + is invalid geometry). Validate
  before writing.
- **Closeout note (NOTE 1):** the DM-12 "four corners" = 3 executed + 1
  (old-reads-new) contract-guaranteed by unknown-key-ignore, not an in-tree fixture
  (same as batch-1's DM-05 note).

### Builder batch-5 (region fill DM-09/10) — 2026-06-17 — DONE
- Reply: `~/dept-bus/edi-dungeon-map/replies/009-builder-region-fill.md`
- Commits `55e5264` (pure `planRegionFill`), `012d48f` (controller verb + intent).
  96/96 green pre-rebase. New wholly-ours `DraftingRegionFill.{h,cpp}`; algo (a)
  footprint lookup; neutral fill (`role==None`, opacity 0.5 stone). v1 fills the
  authored footprint (ignores wall half-thickness — the reviewer-accepted cut).
- **⚠ CMakeLists flag (edi-ui coordination):** the new `DraftingRegionFill` source +
  `drafting_region_fill_tests` need 2 additive `CMakeLists.txt` entries (mirroring
  the `DraftingRoom`/`drafting_room_tests` pattern; no edi-ui target touched). The
  builder made them to let the test build, per reviewer-008's explicit "register the
  test." **Hand to edi-ui at merge** — they own CMakeLists; either bless the 2 lines
  or take ownership.

### ✅ INTEGRATION RESOLVED — rebased onto master, green
- edi-ui merged my **DM-02..08** to master `c6e98e3` (tip `dd226c4`/`163a00a`).
- **Rebased `dept/dungeon-map` onto master with ZERO conflicts** — DM-02..08 dropped
  as already-applied; only batch-4 (DM-12/13) + batch-5 (DM-09/10) + 3 dept-local
  docs commits replayed (edi-ui's superset merge made my hunks apply cleanly).
- **Green gate on the NEW base: build clean · ctest 97/97 (−edi_shell_window) ·
  scan clean · snapshot renders.** The batch integrates with 100 commits of master.
- LEDGER policy holds (no LEDGER commits; my old ones reconciled by edi-ui's merge).

### Builder batch-6 (DM-14/15 per-instance block transform) — 2026-06-17 — DONE
- Reply: `~/dept-bus/edi-dungeon-map/replies/015-builder-block-instance-transform.md`
- Commits (pre-rebase `176f8fe`/`7af18e5`) → rebased `419db06` (DM-14) / `4beb3b5`
  (DM-15). Identity byte-identical (short-circuit avoids transformGeometry ULP
  drift); NaN/scale guards in setters AND `transformBlockInstance`; pivots = placement
  center (DM-14) / group union-bounds center read pre-mutation (DM-15); undo-atomic
  via Update commands; lossy ellipse/text/guide tilt avoided in tests (accepted v1).
  Left the optional `has_block_instance_selection` projection key for edi-ui.
- **Rebased onto master `9e9a1ab`** (advanced to include drafting's DR-08). Conflict
  on `DrawingCore.h` + `DrawingDocumentController.cpp` — both ADDITIVE (DR-08 chamfer
  members/setters vs my block-placement members/setters); resolved keeping BOTH.
  **Green gate on new base: build clean · ctest 99/99 · scan clean · snapshot.**

### Builder batch-7 (DM-01/DM-11 slivers) — 2026-06-17 — DONE
- Reply: `~/dept-bus/edi-dungeon-map/replies/016-builder-dm01-dm11-slivers.md`
- DM-01 `32b297b`: `documentObjectsBounds(doc)→optional<Bounds2D>` (free fn in
  `DraftingDocument`) + thin `computeDocumentBounds()` getter. DM-11 `1b8dacb`: new
  no-Qt `DraftingMapQuery.{h,cpp}` (`deriveEdge` + `plugIsConnected(connections,
  plugId)`); `MapToonExport` switched to the shared helpers — **golden UNCHANGED**
  (pure extraction). 101/101 green. CMakeLists additions flagged (additive, no
  edi-ui target). **Signatures for edi-ui** recorded in the reply (the exact headers
  `buildMapBrowserPanel`/`computeFitView` include). **ALL 15 DM TASKS BUILT.**

### Builder batch-8 (composed-scale overflow guard) — DISPATCHED (final)
- Brief: `~/dept-bus/edi-dungeon-map/briefs/018-builder-scale-overflow-guard.md`
- The DM-14/15 audit nit: guard the composed `scale *= factor` against `+inf`
  (refuse the member), with a pathological 1e200×1e200 test. Last code change before
  the final edi-ui hand-off + closeout.

### Reviewer diff-audit of DM-14/15 — 2026-06-17 — edi-dungeon-map-reviewer (CLEAN, 1 nit)
- Reply: `~/dept-bus/edi-dungeon-map/replies/017-reviewer-dm1415-audit.md`
- **CLEAN.** Pivots correct (DM-14 placement center == block geometric center; DM-15
  union-bounds center frozen `const` pre-mutation); identity short-circuit gates BOTH
  transformGeometry AND computeBounds → bit-identical; NaN guard correct at both
  boundaries; cumulative metadata starts from each member's own (90+45=135°, 2×1.5=3);
  undo one-step; neutral; transformGeometry consumed-not-modified; DR-08 chamfer verb
  fully intact beside my block-placement members (rebase resolution verified).
- **FINDING (NIT, theoretical-only) — composed-scale overflow:** the guard validates
  INPUTS (delta/factor finite, factor>0) but not the COMPOSED `scale *= factor`
  (`DrawingDocumentController.cpp` ~`:3290`). Pathological finite inputs (1e200×1e200)
  → `+inf` written to metadata + the additive codec (+ a geometry/metadata desync, the
  rejected UpdateGeometry result ignored). Unreachable via the UI's bounded spins; NaN
  never reaches anything. **PLANNER DECISION: FIX it** (not just TODO) — it breaches the
  charter's "additive codec never sees non-finite" invariant, and the fix is ~1 line
  (`if (!std::isfinite(composedScale)) refuse this member` before the
  UpdateMetadataCommand). Fold into the wrap-up as a tiny hardening slice (batch-8)
  after batch-7 slivers land.
- Other NOTES (carry, no action): 360° treated as non-identity (fine, non-canonical
  angle); tests use single-object blocks (multi-member shared-pivot correct by
  inspection, unexercised); old-reads-new codec corner contract-guaranteed (carried).

## Open questions / blockers
- DM-14/15 blocked on DR-01 (`transformGeometry`) — drafting builds it first; hub
  signals merge. DM-12/13 deliberately split off so they land now (identity-valued).

## Next
- Batches 2–5 queued; dispatch each as the builder frees. Assess DM-01/DM-11
  sliver vs edi-ui ownership. Report kickoff to hub.
