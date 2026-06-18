# Thread ledger — who is doing what

The hub (the main session) maintains this. One row per active campaign/thread.
Because spawned agents are one-shot and sessions reset, THIS FILE — not anyone's
memory — is the source of truth for state. Read it to recover where things stand;
update it when a gate opens, advances, blocks, or closes.

## Active

| Campaign id | Department | Gate | Status | Handoff doc | Session id |
| --- | --- | --- | --- | --- | --- |
| drafting-20260617-batch2 | edi-drafting | builder | open — DR bucket (DR-01..15) COMPLETE; batch-2: M1 + M8 motif library COMPLETE (S1 record/serialize + S2 placeMotif/capture) on master @2878803. ⚠ DR-13 angular needs drafting's AngularDimension tool-kind + 2-line-pick controller arm (escalated; chrome already landed pending it). edi-ui chrome: DR-11 DONE, DR-13 chrome landed, DR-14/15/motif + DR-13 arc-painter still queued | [docs/handoffs/drafting-20260617-batch2.md](drafting-20260617-batch2.md) | edi-drafting-planner |
| ui-20260617-master-integration | edi-ui | builder ⏸ PAUSED @a6bbf4b | chrome batch-1: DR-10/DM-10/DM-14 DONE+green (caafe9f/0acba08/a6bbf4b); DM-15 BLOCKED on cross-dept projection gap (has_block_instance_selection + instance_id); reviewer audit PENDING; batch-2 (DM-01/DM-11+golden) not started. RESUME: REBALANCE merges (blender-lab batch-1 + drafting) per hub. See handoff PAUSED CHECKPOINT. | [docs/handoffs/ui-20260617-master-integration.md](ui-20260617-master-integration.md) | edi-ui-planner |
| blender-lab-20260617-feature-batch | edi-blender-lab | builder | open (P1..P4b spine taper + RD1 ScriptOp bbox on master @2878803; RD2 recipe TOON diff in flight) | [docs/handoffs/blender-lab-20260617-feature-batch.md](blender-lab-20260617-feature-batch.md) | edi-blender-lab-planner |

## Conventions

- **Campaign id** — `<dept>-<YYYYMMDD>-<slug>`, e.g. `drafting-20260616-fill-svg`.
- **Department** — `edi-drafting` · `edi-blender-lab` · `edi-ui` · `edi-dungeon-map`.
- **Gate** — `research` · `reviewer` · `builder` · `closeout` (the gate in flight).
- **Status** — `open` · `in-gate` · `blocked` · `closed`.
- **Handoff doc** — `docs/handoffs/<campaign-id>.md` (created at campaign start from
  `docs/handoffs/_TEMPLATE.md`).
- **Session id** — the Claude Code session running it (from `list_sessions`), or
  `hub` when the hub runs it directly.

## Closed

_(Move a row here when its campaign closes; link the closeout doc that froze its
boundary.)_

| Campaign id | Closeout doc | Closed |
| --- | --- | --- |
| drafting-20260616-fill-svg | [docs/closeouts/drafting-fill-side-channel.md](../closeouts/drafting-fill-side-channel.md) | 2026-06-16 |
| drafting-20260616-cartography | [docs/closeouts/drafting-cartography.md](../closeouts/drafting-cartography.md) | 2026-06-16 |
| dungeon-map-20260616-cartography | [docs/closeouts/h2-src-drafting-map-boundary.md](../closeouts/h2-src-drafting-map-boundary.md) | 2026-06-16 |
| blender-lab-20260616-cartography | [docs/closeouts/blender-lab-cartography.md](../closeouts/blender-lab-cartography.md) | 2026-06-16 |
| dungeon-map-20260617-feature-batch | [docs/closeouts/dungeon-map-20260617-feature-batch.md](../closeouts/dungeon-map-20260617-feature-batch.md) | 2026-06-17 |
