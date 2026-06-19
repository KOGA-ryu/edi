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
| 2026-06-18 ~14:10 | blender-lab | `03b8cc1` → merge `ac3bf96` | M0 REALIZER (tools/blender/edi_realize.py + smoke test + sample evidence). Two-tier bpy: pure parse_toon/plan_greybox (GPU-free, ctest #100) + Blender OptiX render. | GREEN 105/105 + scan |
| 2026-06-18 ~14:15 | drafting (support) | `366f23c` → merge `fb6ca25` | M0 PROPS SUPPORT: MapBlockSpec carrier on MapSpec (DraftingRoom.h) + createMapFromSpec block-stamp arm + e2e TOON-row assertion. The field the generator needs to place crypt props. | GREEN 105/105 + scan |
| 2026-06-18 ~15:1x | drafting (COHERENCE) | `3ab8033` → merge `4273171` | no-magic-dims sweep: retire hardcoded kCorridorWidth=0.045 (corridor width DERIVES from room scale) + DraftingCanvasDims.h core spine naming canvas/door/wall dims. src/core+src/drafting only, disjoint from the fit slice. | GREEN 105/105 + scan |
| 2026-06-18 ~15:35 | edi-ui (P1 sweep) | `fa26860` (own) | P1 canvas-chrome dim-literal sweep → src/widgets/DrawingCanvasChromeDims.h (named k…Px, byte-identical, golden 0-diff). | GREEN 105/105 + scan |
| 2026-06-18 ~15:40 | drafting (no-magic-dims) | `ee5bf15` → merge `1254bcb` | 050+051 kCanvasBoardExtent: one named source for the radius/setback/thickness/spacing max-clamp 1.0. Sweep COMPLETE (map/canvas path). NIT comment verified survived. | GREEN 105/105 + scan |

## SCALE-POLICY slice (edi-ui PRIMARY — `~/dept-bus/SCALE-POLICY.md`)
The user's scale directive: the 2D canvas must FIT THE SCREEN at any dungeon size.
- **`d5a5c40`** — added the **"no hardcoded dimensions" HARD RULE** to CLAUDE.md
  (every dimension is named DATA; reviewer-enforced, no mechanical scan; exempt
  epsilons/tolerances + unset 0.0). Green-gated.
- **`6520a6d`** — canvas **auto-fit-to-content** for ANY dungeon size, all dims as
  NAMED CONFIG: `kViewportFitPaddingFraction` (~10% breathing room, replaces the
  magic 48px), `kViewportFitMinZoom` (fit-only floor so a 5× dungeon still frames
  while interactive [0.2,16] is unchanged). Root-caused + fixed the MAP-workspace
  right-panel clipping: the feature panels are OVERLAYS over a full-width canvas,
  so the shell now pushes overlay occlusion to the canvas as view-insets and the
  fit frames into the VISIBLE sub-rect. Verified single/doubled/5× in both
  workspaces; default_shell golden 0-diff. **In edi-ui reviewer gate now** (also
  doing the src/widgets dimension-literal sweep + the fit-padding coherence check
  vs drafting's kAsciiBoardFillFraction).

| 2026-06-18 ~16:xx | dungeon-map (GENERATOR) | `1b3d4ab` → merge `a0078bc` | M0 crypt generator: buildCryptMapSpec(double scale) + G2 props + exportMapToToon(sceneScale) advisory scale: header. Rebased clean on master, disjoint from edi-ui work. | GREEN 106/106 + scan |
| 2026-06-18 ~16:xx | edi-ui (CLI) | `533d95e` (own) | `--generate-crypt <out> --scale <S>` in app/main.cpp (brief 048) + CryptGenerator in the edi target. The M0 one-command terminus. | GREEN 106/106 + scan |

## M0 one-command chain — CORRECTED

**Correction (2026-06-18, hub dogfood):** an earlier note here claimed
`edi --generate-crypt --scale N` met the whole-chain gate. That was OVERSTATED —
that handler is a **TOON terminus only**; it does NOT invoke the realizer. The
first "M0 GATE MET" render was made by chaining the realizer SEPARATELY. The
TOON/scale half is correct (S=1 omits the `scale:` header; S=2/S=4 produce the
doubled 30×30/50×50 and 60×60/100×100 geometry), but one-command→PNG was not met.

**Fix — `tools/m0/render-crypt.sh`** (the orchestration tier; keeps edi and
Blender decoupled per the three-tier law — neither imports the other):
```
tools/m0/render-crypt.sh --out <png> [--scale S] [--reference] [--samples N]
```
tier 1 `edi --generate-crypt` (headless/offscreen) → TOON; tier 2 the bpy
realizer → render. VERIFIED genuinely end-to-end from ONE command:
`render-crypt.sh --out /tmp/m0/oneshot_s2.png --scale 2` →
**Cycles OPTIX on NVIDIA GeForce RTX 5090 (GPU CONFIRMED, no CPU fallback)** →
1920×1080 PNG in **3.2s** (GATE timing PASS <120s).

**`--reference` status:** the wrapper forwards `--scale` + `--reference` to the
realizer. `--scale` is real now. `--reference` (the fixed-6ft-figure + floor-checker
scale overlay) lives in blender-lab's realizer (`dept/blender-lab`, NOT yet on
master), so on master's realizer it is currently a harmless no-op — it activates
the moment blender-lab's realizer-with-`--reference` is merged. **Coordination
requested with blender-lab** (rebase dept/blender-lab onto master + hand off the
realizer-with-reference). Until then: one-command→PNG = DONE; `--reference` overlay = PENDING.

## Pending coordination
- **`--generate-crypt <out> --scale <S>` CLI** (dungeon-map brief 048): edi-ui
  owns the ~20-line headless flag in app/main.cpp (mirrors --export-map). Replied
  ACK (reply 048). BLOCKED until dungeon-map brief 047 lands buildCryptMapSpec(scale)
  + exportMapToToon sceneScale on master — merge that first, then build the flag.

**Realizer gate evidence verified on the merged line** (render.log L288–294):
`compute_device_type: OPTIX`, `[X] OPTIX NVIDIA GeForce RTX 5090`,
`GPU CONFIRMED: rendering on OPTIX device(s): ['NVIDIA GeForce RTX 5090']`
(CUDA box unchecked → no CPU fallback), 128 samples Finished `Time:00:03.14`,
peak `1553M` (~1.5 GB), `crypt.png` 1920×1080. All 4 gate criteria PASS for the
realizer half. Blender 4.5.9 (box version) — edi_craft seam re-verified clean.

## Seam notes (for the final converge-check)
- 2026-06-18 ~13:57 — dungeon-map pushed a socket-contract PROPOSAL (v0) on its
  branch (`e33cc3c`, docs-only, `docs/dungeon-map-m0-socket-contract.md`). HELD,
  not merged — still in dungeon-map's reviewer gate; merging an unsettled
  contract could mislead the realizer. Seam understood:
  - Wire = existing `exportMapToToon` output (Seam C): 4 flat arrays
    rooms/plugs/connections/blocks. NO new columns for M0.
  - Split: STRUCTURE (10 piece types) is expanded from the graph by the
    realizer; only PROPS (`crypt.sarcophagus`, `crypt.brazier`) ride as block
    `asset_ref`s. asset_ref form `<theme>.<piece>`.
  - Frame: feet, 5 ft grid module, min-corner room origin, block origin =
    centre, M0 scale=1/rotation=0.
  - **CROSS-DEPT CONFIRM POINT**: 2D→3D handedness (`map x→world X`,
    `map y→world −Y`, +Z up). Contract flags the realizer must confirm this
    matches its bpy build — watch for convergence before declaring the seam done.

- 2026-06-18 ~14:20 — dungeon-map FROZE the contract v1 (`d79970e`, docs-only,
  reconciled vs the merged realizer + live exporter). HELD (not merged) — will
  ride in with the generator code on the same branch. The freeze CORRECTED the
  handedness: `map-y → world Y` (the proposal's `−Y` was stale). It lists 3
  realizer-confirm items.
  - **HUB CROSS-CHECK (from the merged `edi_realize.py` on master)** — I can see
    both halves, so I verified the 3 confirm items at the code level to de-risk
    the re-render (blender-lab still owns the authoritative confirm):
    1. Handedness: realizer uses `blk.x/blk.y` + `_plug_anchor` (edge midpoint)
       directly as world coords, `location=(p.x,p.y,p.z)` (L288/421-438/600-608)
       — NO negation. Frozen contract's `map-y→world Y` MATCHES. ✓
    2. `crypt.stair`: `PIECE_STAIR` resolve exists (L433). ✓ (plausible)
    3. Straight corridor: single-bend L-router degenerates to straight when
       `start.y==end.y` (L458). ✓ (plausible)
  - No TRUE divergence found between the frozen contract and the merged realizer.

## Open questions / blockers
- None blocking. Watching: the GENERATOR code tip (merge it + the frozen-contract
  docs together); blender-lab's formal ack of the 3 confirm items; then the
  one-command whole-chain converge (the M0 finish line). All dept-owned gate
  items — integration confirms the seam lines up + keeps master green.

## Next
- REALIZER merged ✓. Awaiting the GENERATOR (dungeon-map): hardcoded crypt
  MapSpec → createMapFromSpec → Seam-B TOON. Merge when green.
- THE CLOSING CONVERGE (gate criterion #4 — one-command whole chain): the
  realizer ran on its own `samples/crypt_m0/crypt.toon`. Once the generator
  lands, confirm the generator-emitted TOON feeds the realizer and renders —
  i.e. the two halves agree on the wire + the 2D→3D handedness. That end-to-end
  confirm is the M0 finish line; flag any divergence to both depts.
