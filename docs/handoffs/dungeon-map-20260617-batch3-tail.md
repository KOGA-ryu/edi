# Handoff — dungeon-map-20260617-batch3-tail

> The short tail after the corridors+doors closeout: the ratified `active_plug_type`
> chrome-support key + an honest backlog-exhaustion assessment (the hub's NOTHING-LEFT
> off-ramp). Builds on the closed corridors-doors batch (tip `980cc67`).

- **Campaign**: dungeon-map-20260617-batch3-tail
- **Department**: edi-dungeon-map
- **Hub directive**: continue autonomously with the batch-3/parked backlog; if the
  tool-first backlog is GENUINELY EXHAUSTED → `bus-hub NOTHING-LEFT` (the hub stands the
  dept down or fetches the user's next direction).
- **Guard**: rebase ONLY onto LOCAL master (planner-only); builders no git remote.

## Slices
### `037` — `active_plug_type` projection key (RATIFIED) — DISPATCHED
- edi-ui's door-type picker pre-selects the current type (hub-ratified UX) → add the key
  (mirror `active_object_is_plug`). Small, clearly-needed. ▶ builder.

### `038` — reviewer backlog-exhaustion ASSESSMENT — DISPATCHED
- Honest BUILD/PARK/DROP read on the remaining items (`syncGraphForMovedObject`, the
  snapshot-`m_activeConnectionId` refactor, the locked-layer NIT, anything else tool-first).
  The cost-steward call on whether genuine value remains or the backlog is EXHAUSTED. ▶ reviewer.

## Next
- 037 lands → confirm `active_plug_type` to edi-ui.
- 038 returns → if it finds BUILD-worthy items, gate+build them; if EXHAUSTED →
  bus-hub the hub **NOTHING-LEFT**. Either way, honest (no scraping).
