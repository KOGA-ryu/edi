# Handoff — dungeon-map-20260619-cleanup-d15-d18

> Per-campaign state. Each gate appends its result; the NEXT gate reads this first.

- **Campaign**: dungeon-map-20260619-cleanup-d15-d18
- **Department**: edi-dungeon-map
- **Source**: the repo-wide cleanup clipboard `~/dept-bus/hub/briefs/cleanup-clipboard.md` — this
  department's two items, hub-ordered D15 first, then D18.
- **Goal (one line)**: settle + close the Seam B/Seam C lock-tag asymmetry (D15), then tighten the
  determinism test (D18).

## Why NO research gate (deliberate, per the workflow)
The workflow opens the research gate ONLY when the missing input is EXTERNAL/reference knowledge.
Both items are INTERNAL: D15 is an ownership/contract-intent question (where do lock tags live in
edi's own model), D18 is a test-architecture question. The only external angle (do mature formats
round-trip lock tags through a saved document? — Tiled custom properties survive save/load: YES) is
already captured in `docs/research/marker-patrol-lock-prior-art.md`. So this campaign runs
**reviewer → builder** directly.

## D15 — Seam B / Seam C lock-tag asymmetry (· M ·, clipboard line 92)
**The gap (verified on synced master @8da3bdb):**
- Seam B overload `exportMapToToon(const MapSpec&)` (MapToonExport.cpp:268-280) emits
  `connections[...]{from,to,type[,locked,key_id]}` CONDITIONALLY — carries the lock. ✅
- Seam C overload `exportMapToToon(const DraftingDocument&)` (MapToonExport.cpp:480-483) HARDCODES
  `connections[...]{from,to,type}` via `writeConnectionRow` (3 fields) — silently drops any lock.
- The document twin `DraftingDeclaredConnection` (DraftingMapTypes.h) has NO `locked`/`keyId` fields
  (only `type`, `level`); its MessagePack codec `connectionValue`/`readConnection`
  (DraftingSerialize.cpp:653-674) writes `{id,plugA,plugB,type,level}`. So the document model CANNOT
  HOLD a lock ⇒ the same authored locked map exports differently depending on whether it round-trips
  through the document (author→parse→Seam B keeps it; author→createMapFromSpec→document→Seam C loses
  it). This is the twin I DEFERRED in the proving-ground campaign — the deferral is now the debt.

**Planner's lean (reviewer to ratify/adjust): Option A — FULL PARITY.** Make the document a faithful
neutral source so both seams agree:
1. Add `bool locked = false;` + `std::string keyId;` to `DraftingDeclaredConnection`; rewrite the
   "deliberately no locked" comment to "engine-interpreted TAGS, not edi rules" (same wording M1 used
   for MapConnectionSpec).
2. Extend the codec with CONDITIONAL emission (the `flags`/`bounded_by` precedent, NOT the always-write
   `level` precedent): write `locked` only when true, `key_id` only when non-empty; read missing ⇒
   false/empty. **Byte-identical for every existing `.edidraw` connection** (the trap the proving-ground
   reviewer flagged — must be honored here).
3. Seam C export: append conditional `locked,key_id` columns mirroring Seam B (pre-scan
   `document.connections`; absent when none locked → reference dungeon Seam C byte-identical).
4. Carry the lock end-to-end: where `createMapFromSpec` mints a `DraftingDeclaredConnection` from a
   `MapConnectionSpec`, copy `locked`/`keyId` so author→document→Seam C preserves it (closes the
   asymmetry for real, not just adds dormant plumbing).
*Why A over B (document-the-divergence + warn):* the document is meant to be "the single
self-describing source the engine export reads" (DraftingMapRoom's own comment); a lock is neutral map
data; parity removes the asymmetry instead of papering it, and the codec change is the low-risk
conditional pattern edi already uses. **Reviewer decides; if it picks B, the builder instead documents
the divergence in the backlog and makes Seam C WARN (not silently omit).**

## D18 — map_determinism_tests runs both runs in one process (· S nit ·, clipboard line 109)
`tests/map_determinism_tests.cpp:78-99` builds `ctrlA` and `ctrlB` in the SAME `main()` and diffs
their TOON. Its header claims to own cross-process/ASLR nondeterminism, but same-process runs cannot
catch it. **Planner's lean:** the lighter, robust fix (Option b) — document the in-process limit
honestly in the test header and lean on `map_regression_lock_tests`' fixed sha256 (which pins the
exact bytes) for the real guarantee; OR (Option a) re-exec a print-TOON helper as a separate process
and diff. Reviewer picks a vs b by cost/value (it's a NIT — prefer the cheapest correct option).

## Gate log

### Reviewer gate — 2026-06-19 — edi-dungeon-map-reviewer (CLOSED ✅ BOUNDARY SETTLED: YES)
- **D15 → Option A (full parity)** — ratified, all 4 sub-parts in scope. Document must be the
  self-describing source; Option B would freeze the lossiness. Neutrality confirmed (no code branches
  on the lock except serialize/export — pure record/emit).
- **Exact break point pinned:** `createMapFromSpec` (`DrawingDocumentController.cpp:3461`) copies only
  `request.type`, DROPPING `locked`/`keyId` — that's sub-part A3, the line that actually closes the
  asymmetry end-to-end.
- **CRITICAL catch the plan missed:** there is currently **NO golden test for the Seam C (document)
  export** — `map_regression_lock_tests` pins only Seam B; the Seam C path is only diffed against
  itself in `map_determinism_tests`. So A4 MUST ADD a reference-dungeon Seam C golden (in
  `map_regression_lock_tests.cpp`, which already builds the document) — otherwise the byte-identity
  claim ships unverified.
- **Byte-identity trap reconfirmed:** A2 codec MUST use the `flags`/`bounded_by` CONDITIONAL pattern
  (`DraftingSerialize.cpp` 616-626, 727-737), NOT the always-write `level` pattern (line 661). NO
  version bump.
- **D18 → Option b** — document the in-process limit in the test header (lines 1-30, scope the
  over-claim at line 13), point the cross-build guarantee at `map_regression_lock_tests`' sha256
  `6c632293…b0e3`. Comment-only; no new CLI/fork harness for a nit.

### Builder batch — 2026-06-19 — edi-dungeon-map-builder (COMPLETE ✅) + planner verification
SHAs: **S1 `b6a5b24`** · **S2 `8fc0a25`** · **S3 `0102b79`** · **S4 `5f14e37`** · **S5 `3d02f9e`**.
- **Planner-verified (independent re-run):** Debug 118/118 green; Release 111/118 (ONLY the 7
  pre-existing D01 drafting-core segfaults — confirmed identical to clean master @8da3bdb; ZERO new
  failures; all map/serialize/regression/determinism tests pass in BOTH build types); Seam B golden
  sha256 `6c632293…b0e3` UNCHANGED (not re-blessed); NEW Seam C golden pins `connections[12]{from,to,
  type}` (no-columns) + a locked round-trip shows columns turn on; neutrality grep clean (lock read
  only at parse-record / createMapFromSpec carry / serialize / export — NO behavioral branch); scan
  clean; reference dungeon renders (152KB).
- **D15 closed end-to-end** (parity): break was `createMapFromSpec:3461` copying only `type`; now
  carries the lock through codec + Seam C. **D18 closed** (header rescoped).
- Closeout: `docs/closeouts/dungeon-map-lock-tag-seam-parity.md`.

### Builder batch — ratified slices (delivered)
- **S1 (D15-A1) model+comment:** add `locked`/`keyId` to `DraftingDeclaredConnection`
  (`DraftingMapTypes.h:155-166`); rewrite the "deliberately no locked" comment to "engine-interpreted
  TAGS, not edi rules". *Accept:* builds; existing tests green (defaults change nothing).
- **S2 (D15-A2) codec:** conditional `locked`(only if true)/`key_id`(only if non-empty) in
  `connectionValue`/`readConnection`; tolerant read defaults. *Accept:* round-trip test + a no-lock
  connection serializes BYTE-IDENTICAL (key absent); all `.edidraw` tests green. NO version bump.
- **S3 (D15-A3) carry-through:** at `DrawingDocumentController.cpp:3461` copy `request.locked`/`keyId`.
  *Accept:* author locked MapSpec → `createMapFromSpec` → document connection carries the lock.
- **S4 (D15-A4) Seam C export + NEW golden:** mirror Seam B's conditional `locked,key_id` columns in
  the document overload (`MapToonExport.cpp:480-484`, `hasLock` pre-scan); ADD the reference-dungeon
  Seam C golden to `map_regression_lock_tests.cpp` (no lock ⇒ `connections[12]{from,to,type}`).
  *Accept:* (a) Seam C ref golden = no-columns shape; (b) locked round-trip author→document→Seam C
  shows columns; (c) existing Seam B golden UNCHANGED (re-run, do not re-bless).
- **S5 (D18) header doc:** edit `map_determinism_tests.cpp` header only — scope to in-process
  determinism, defer the byte guarantee to the regression sha256. *Accept:* comment-only; green.

## Open questions / blockers
- RESOLVED: D15 → Option A (full parity); D18 → Option b. No open blockers.
- NOTE the Release green leg: the clipboard's D02 adds a Release ctest leg; this campaign verifies
  Debug AND Release locally per commit regardless (the lock work is pure logic, but Release is the
  honest gate).

## Next
- Builder batch S1→S5 (boundary settled). Commit per slice; Debug+Release ctest green + scan + both
  goldens byte-identical (Seam B unchanged, new Seam C no-columns) per commit → closeout → report
  SHAs to hub. NO push.
</content>
