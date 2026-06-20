# Thread ledger — who is doing what

The hub (the main session) maintains this. One row per active campaign/thread.
Because spawned agents are one-shot and sessions reset, THIS FILE — not anyone's
memory — is the source of truth for state. Read it to recover where things stand;
update it when a gate opens, advances, blocks, or closes.

## Active

| Campaign id | Department | Gate | Status | Handoff doc | Session id |
| --- | --- | --- | --- | --- | --- |
| dungeon-map-20260619-provingground | edi-dungeon-map | closeout | COMPLETE ✅ (awaiting HUB FF to master — NOT pushed). NEUTRAL proving-ground map + Seam B TOON export. All gates green: research ✅ reviewer ✅ (BOUNDARY SETTLED) builder ✅ M1 8f240b5 / M2 68e3b94 / M3 1f93288 / M4 a5a4f13. Planner-verified: Debug 118/118 green, map/marker/manifest/golden green, ref-golden byte-identical, scan clean, both renders OK, TOON carries markers[6]/patrols[1]/locked goal door (gold_key gates door+chest). Closeout+manifest: docs/closeouts/dungeon-map-proving-ground.md. FORK: DraftingDeclaredConnection lock twin DEFERRED (Seam B path only). ⚠️ TWO PRE-EXISTING drafting-core issues: (1) 7-test Release-build SEGFAULT — ✅ FIXED by dept/drafting D03/D05 merge (8e1e5b6): bare assert() elided under NDEBUG; replaced with non-eliding EDI_CHECK + lint guard; Release ctest now 121/121. (2) --export-map CLI teardown abort — partially addressed by D04 headless offscreen fallback (d990313); a no-display --generate-crypt smoke is now green, but the specific --export-map teardown path was not separately re-tested — leave for HUB triage. | [docs/handoffs/dungeon-map-20260619-provingground.md](dungeon-map-20260619-provingground.md) | edi-dungeon-map-planner |
| drafting-20260617-batch2 | edi-drafting | builder | open — DR bucket (DR-01..15) COMPLETE; batch-2: M1 + M8 motif library COMPLETE (S1 record/serialize + S2 placeMotif/capture) on master @2878803. QUEUE EXHAUSTED (drafting idle, awaiting next campaign). All of DR-01..15 + M1 + M2 (incl Node snap) + M8 motif + DR-13 angular (LIVE end-to-end) on master @ed90e3d. edi-ui chrome queued for next session: DR-13 arc-painter, DR-14/15 cells, DR-07/08/09 verbs, M2-S3 node_enabled checkbox. ✅ dept/drafting FULLY MERGED to master @8e1e5b6 by HUB (D03/D05 EDI_CHECK harness + A4 resolver closeout); branch 0 ahead of master. | [docs/handoffs/drafting-20260617-batch2.md](drafting-20260617-batch2.md) | edi-drafting-planner |
| ui-20260617-master-integration | edi-ui | builder — HOLD | Integration + chrome bucket COMPLETE on master @0e0df11, edi-gate GREEN 104/104 + scan. All delivered chrome reviewer-ACCEPT (DM-01/10/11/14/15, DR-10/11/13 cell+combo, M8 Motifs palette). OPEN backlog (HELD pending user decision — do NOT scoop): door-type picker (active_plug_type reader — core key not yet on master), DR-13 arc-painter canvas seam, M2-S3 node-snap checkbox, DR-14/15 dim/fill cells, DR-07/08/09 modify-verb chrome, DM-15 projection gap. | [docs/handoffs/ui-20260617-master-integration.md](ui-20260617-master-integration.md) | edi-ui-planner |
| blender-lab-20260617-feature-batch | edi-blender-lab | builder | open (P1..P4b spine taper + RD1 ScriptOp bbox on master @2878803; RD2 recipe TOON diff in flight) | [docs/handoffs/blender-lab-20260617-feature-batch.md](blender-lab-20260617-feature-batch.md) | edi-blender-lab-planner |
| ui-20260618-m0-integration | edi-ui | closeout | M0 COMPLETE — generator + realizer + drafting-support all merged; one-command chain `tools/m0/render-crypt.sh --scale S [--reference]` renders OptiX/RTX-5090 PNG end-to-end (verified 3.3s). master @0d5ca60 GREEN 106/106. | [docs/handoffs/ui-20260618-m0-integration.md](ui-20260618-m0-integration.md) | edi-ui-planner |
| ui-20260619-cleanup-assetlink | edi-ui | closeout | COMPLETE ✅ — HUB cleanup wave. D06 ✅ (06b9e2c dead RecentFilesStore) · D04 ✅ (d990313 headless offscreen fallback + smoke) · D17 ✅ (f6725e6 controller→edi_controller_core, compile-once) · D08 ✅ (a0fec6b live AssetZoo on EdiShellWindow, empty-on-miss, no UI) · D07 ✅ (4688714 validateAssetRefs(zoo) const pull, edi_asset_link PRIVATE). Reviewer gate SETTLED + reviewer-ACCEPT all three (signal-safety PASS: zero call sites, not on modelChanged). Integrations: blender-lab (6799f74) + drafting D03/D05 (8e1e5b6) merged. master gate GREEN Debug 121/121 + Release 121/121 + scan. DEFERRED look decisions for USER (not built): (1) visible AssetZoo browser panel; (2) unresolved/uncurated asset-ref warning surface. | [docs/handoffs/ui-20260619-cleanup-assetlink.md](ui-20260619-cleanup-assetlink.md) | hub |
| ui-20260618-spatial-phase1 | edi-ui | builder (integration) | open — SOLE integrator, dungeon spatial inversion. PHASE 1 COMPLETE (data-spine, 9 slices, canary 6c632293 byte-identical throughout; master 3ce29a3 GREEN 110/110). PHASE 2 (full breadth) IN PROGRESS: merge hub-pinged dept SHAs in order, canary-gated. WIRE NOTE: P2-A extends the TOON (nodes[]/columns) — canary may change; verify DELIBERATE golden update + node-less dungeon stays byte-identical, else HALT. | [docs/handoffs/ui-20260618-spatial-phase1.md](ui-20260618-spatial-phase1.md) | edi-ui-planner |
| blender-lab-20260618-m0-realizer | edi-blender-lab | closeout — GATE PASS | REALIZER COMPLETE on dept/blender-lab @rebased. Standalone bpy realizer (tools/blender/edi_realize.py): TOON map -> greybox(10 piece types+2 props+1 light) -> Cycles OptiX. Crypt rendered 1080p, OptiX/RTX-5090 CONFIRMED in log, 3.4s, ~1.5GB (all 4 gate criteria PASS). edi_craft bpy seam re-verified clean on Blender 4.5.9 (no drift). Reviewer: no blockers, 3 SHOULD-FIX applied. Green gate 105/105 + scan. Evidence: samples/crypt_m0/{crypt.png,render.log}. ✅ MERGED to master @6799f74 by HUB cleanup-wave (incl. D09/D10/D14 cleanup +3); dept/blender-lab fully absorbed (0 ahead). master gate 120/120 + scan green. | [docs/handoffs/blender-lab-20260618-m0-realizer.md](blender-lab-20260618-m0-realizer.md) | hub |

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
