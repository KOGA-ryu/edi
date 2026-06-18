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

### M8 — motif library (flash sheet) — REVIEWER GATE opened 2026-06-17 (parallel to M1)
- Brief: `~/dept-bus/edi-drafting/briefs/033-M8-motif-representation-reviewer.md`. M8 is an L
  touching the PERSISTENT format (a motif record in the MessagePack envelope) with a
  representation FORK (document-level vs sidecar). Reviewer (Opus) settles the motif-record
  home + serialization + the FLATTEN-on-place decision + the transform-on-place fork, BEFORE a
  Sonnet builder touches the format (same discipline as DR-13). Runs in parallel with M1.

## Open questions / blockers
- M8 motif-record representation — under reviewer gate (document vs sidecar + serialize shape).
- (Not pausing for the dogfood/use-report fork — user chose autonomous.)

## Next
- M1 lands → spot-check + report green tip to edi-ui. M8 gate settles → brief M8 Slice 1
  (capture + serialize) to the builder. Continue the batch-2 queue ahead.
