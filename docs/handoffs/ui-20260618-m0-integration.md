# Handoff — ui-20260618-m0-integration

> edi-ui acting as the M0 INTEGRATION HUB. Merges the generator (dungeon-map)
> + realizer (blender-lab) to LOCAL master as each goes green; keeps master
> green (edi-gate). Brief: `~/dept-bus/M0-CRYPT-SLICE.md`.

- **Campaign**: ui-20260618-m0-integration
- **Department**: edi-ui (integration role only — NO chrome for M0)
- **Goal (one line)**: Integrate the M0 crypt slice — generator (C++ hardcoded
  crypt MapSpec → createMapFromSpec → Seam-B TOON w/ block instances) +
  realizer (standalone bpy: TOON → greybox → Cycles OptiX render on 5090) —
  to LOCAL master, keeping every merge green.
- **Boundary (what integration must hold)**: master stays green (edi-gate:
  build + ctest + scan) after EVERY merge; merges happen ONLY on a
  department's confirmed-green tip; the M0 socket contract is owned by
  dungeon-map (settled at its reviewer gate) — integration does not arbitrate
  it, only verifies both sides agree before declaring the seam done.

## Baseline (campaign start — 2026-06-18 ~13:50)

- master tip: `b3e2932` (edi-ui LEDGER sync, docs-only, on top of FINAL line
  `0e0df11` = 104/104 green + scan). Treated as the green baseline.
- dept/dungeon-map tip `eb864d3` — 1 docs-only commit ahead of master, no M0
  code yet (kicked off 13:48).
- dept/blender-lab tip `0e0df11` — nothing new yet (kicked off 13:48).
- dept/drafting tip `15f22d1` — idle (support only, if MapSpec/DraftingRoom
  struct tweaks are needed).

## Integration policy (this campaign)

- Merge dungeon-map (generator) + blender-lab (realizer) tips to LOCAL master
  as they report green. NO `git fetch` / origin ops — local master is the
  integration line (standing fleet rule from the batch-2 rebase incidents).
- After each merge: run `edi-gate` from `~/edi`. Green → keep; red → revert the
  merge, bus-hub the blocker to the owning dept, do NOT sit on a red master.
- The realizer is a standalone bpy program invoked via the EXISTING
  BlenderRunPlan subprocess seam — no new edi-ui chrome required.

## Merge log

_(append one row per merge: tip, what, edi-gate result)_

| When | Dept | Tip merged | What | edi-gate |
| --- | --- | --- | --- | --- |
| — | — | — | (none yet — awaiting first green M0 tip) | — |

## Open questions / blockers
- None yet. Watching: M0 socket contract settlement (dungeon-map reviewer gate)
  and the 5090/OptiX render-log evidence (blender-lab) — both are dept-owned
  gate items, integration only confirms the seam lines up.

## Next
- Poll the hub inbox + dept branch tips for the first green M0 tip; merge it.
