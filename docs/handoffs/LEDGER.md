# Thread ledger — who is doing what

The hub (the main session) maintains this. One row per active campaign/thread.
Because spawned agents are one-shot and sessions reset, THIS FILE — not anyone's
memory — is the source of truth for state. Read it to recover where things stand;
update it when a gate opens, advances, blocks, or closes.

## Active

| Campaign id | Department | Gate | Status | Handoff doc | Session id |
| --- | --- | --- | --- | --- | --- |
| drafting-20260617-batch2 | edi-drafting | builder | open — DR bucket (DR-01..15) COMPLETE; batch-2: M1 + M8 motif library COMPLETE (S1 record/serialize + S2 placeMotif/capture) on master @2878803. QUEUE EXHAUSTED (drafting idle, awaiting next campaign). All of DR-01..15 + M1 + M2 (incl Node snap) + M8 motif + DR-13 angular (LIVE end-to-end) on master @ed90e3d. edi-ui chrome queued for next session: DR-13 arc-painter, DR-14/15 cells, DR-07/08/09 verbs, M2-S3 node_enabled checkbox | [docs/handoffs/drafting-20260617-batch2.md](drafting-20260617-batch2.md) | edi-drafting-planner |
| ui-20260617-master-integration | edi-ui | builder — HOLD | Integration + chrome bucket COMPLETE on master @0e0df11, edi-gate GREEN 104/104 + scan. All delivered chrome reviewer-ACCEPT (DM-01/10/11/14/15, DR-10/11/13 cell+combo, M8 Motifs palette). OPEN backlog (HELD pending user decision — do NOT scoop): door-type picker (active_plug_type reader — core key not yet on master), DR-13 arc-painter canvas seam, M2-S3 node-snap checkbox, DR-14/15 dim/fill cells, DR-07/08/09 modify-verb chrome, DM-15 projection gap. | [docs/handoffs/ui-20260617-master-integration.md](ui-20260617-master-integration.md) | edi-ui-planner |
| blender-lab-20260617-feature-batch | edi-blender-lab | builder | open (P1..P4b spine taper + RD1 ScriptOp bbox on master @2878803; RD2 recipe TOON diff in flight) | [docs/handoffs/blender-lab-20260617-feature-batch.md](blender-lab-20260617-feature-batch.md) | edi-blender-lab-planner |
| ui-20260618-m0-integration | edi-ui | builder (integration) | open — M0 crypt-slice integration hub. Merges generator (dungeon-map) + realizer (blender-lab) to LOCAL master as they go green; keeps master green (edi-gate). Baseline master @b3e2932 (FINAL line). Both depts kicked off 13:48, no M0 green tips yet. NO edi-ui chrome (realizer via existing subprocess seam). | [docs/handoffs/ui-20260618-m0-integration.md](ui-20260618-m0-integration.md) | edi-ui-planner |

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
