# Thread ledger — who is doing what

The hub (the main session) maintains this. One row per active campaign/thread.
Because spawned agents are one-shot and sessions reset, THIS FILE — not anyone's
memory — is the source of truth for state. Read it to recover where things stand;
update it when a gate opens, advances, blocks, or closes.

## Active

| Campaign id | Department | Gate | Status | Handoff doc | Session id |
| --- | --- | --- | --- | --- | --- |
| drafting-20260617-feature-bucket | edi-drafting | builder | open (DR-01..07 on master @f53b00b; DR-08 extend-to-boundary in flight, rebases on master) | [docs/handoffs/drafting-20260617-feature-bucket.md](drafting-20260617-feature-bucket.md) | edi-drafting-planner |
| ui-20260617-master-integration | edi-ui | builder | open (master integration line + chrome bucket) | [docs/handoffs/ui-20260617-master-integration.md](ui-20260617-master-integration.md) | edi-ui-planner |
| blender-lab-20260617-feature-batch | edi-blender-lab | builder | open (BL-01/03/04 merged to master @0041783; rebase + next batch) | [docs/handoffs/blender-lab-20260617-feature-batch.md](blender-lab-20260617-feature-batch.md) | edi-blender-lab-planner |
| dungeon-map-20260617-feature-batch | edi-dungeon-map | builder | open (DM-02..13 + region-fill on master @9ebfb7f; DM-14/15 building on top; DM-01/DM-11 chrome slivers → edi-ui) | [docs/handoffs/dungeon-map-20260617-feature-batch.md](dungeon-map-20260617-feature-batch.md) | edi-dungeon-map-planner |

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
