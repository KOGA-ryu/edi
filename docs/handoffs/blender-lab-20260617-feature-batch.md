# Handoff — blender-lab-20260617-feature-batch

> The recipe-lab feature batch (HOLD released, hub dispatch `FEATURE-DISPATCH.md`).
> 15 tasks BL-01..15 from `~/dept-bus/work-batch-plan.md` (edi-blender-lab section).
> Build the OP/controller verbs only; **edi-ui wires chrome separately** from the
> surface specs. Commit to `dept/blender-lab`; do NOT merge (hub routes to edi-ui).
> Report to hub every ~3 tasks / on blocker / at batch-done.

- **Campaign**: blender-lab-20260617-feature-batch
- **Department**: edi-blender-lab
- **Green gate (per task):** `cmake --build build && ctest --test-dir build -E edi_shell_window_tests --output-on-failure` + scan + the 3 `edi_craft` cross-language checks. Rebase on master at the START of each task.

## Ratified forks (from the hub dispatch — do not re-litigate)
- **BL-03 carrier = Option A:** a NEW concrete `AddPrismOp` arm is what extrude lowers
  to (parallel to lathe→AddMoulding). Bumps the variant 11→12.
- **BL-02 COLLAPSES:** extrude is REFUSE-ASCII (OBJ-only proof like the lathe/Script),
  NOT drawn. The freed slot → a new **Bevel** depth verb (edge chamfer/round param on
  the prism carrier, `bevelWidth`/`segments`).
- **Arm-adders are mutually serial:** BL-01 (→11) → BL-03 AddPrismOp (→12) →
  BL-08 AddSweepProfile (→13) → BL-11 AddBoolean (→14). Never two at once (they all
  rewrite the exhaustive-visit table + the `variant_size_v` tripwire at
  RecipeOpsStore.cpp:597).
- Extrude/sweep/boolean stay AUTHORED (ref-bearing), NOT one-click palette — like the
  lathe (§9.2 of the arch doc).

## Order (the hub's: spine first)
Wave 0 spine: **BL-01 → BL-03 → BL-04**. Then BL-05; Wave-1 independents BL-06, BL-12,
BL-14 can interleave (no arm collision); then BL-02→Bevel, BL-08, BL-10, BL-11, BL-07,
BL-13, BL-09, BL-15. Arm-adders (BL-01/03/08/11) serialized as above.

## Task ledger
| Task | Title | Size | Status | Commit(s) |
|---|---|---|---|---|
| BL-01 | AddExtrudedProfile arm (→11 visits) | L | **SHIPPED** (audit clean) | `cd646ab` |
| BL-03 | Resolve-lowering → new AddPrismOp (→12) | L | **SHIPPED** (keystone audit clean) | `e166709` |
| BL-04 | edi_craft.py prism build + OBJ golden | M | **SHIPPED** — SPINE COMPLETE | `914a473` |
| BL-05 | Push/Pull height authoring + bind | M | **SHIPPED** (tests-only, no gap) | `7d0a73d` |
| BL-06 | Lathe sweepDegrees param | M | builder done, reviewer audit | `6a62c8f` |
| BL-07 | Lathe screw/helix params | M | dep BL-06 | — |
| BL-02→Bevel | Bevel depth verb on prism carrier | M | blocked on BL-04 | — |
| BL-08 | Follow-Me sweep op (→13) | L | dep BL-04 + arm-serial | — |
| BL-09 | Taper-along-sweep param | M | dep BL-08 | — |
| BL-10 | Inset + normalOffset params | M | dep BL-04 | — |
| BL-11 | Solid boolean op (→14) | L | dep BL-04 + arm-serial | — |
| BL-12 | Craftsman radial_petal | S | ready (no dep) | — |
| BL-13 | Craftsman nfold_star | S | dep BL-12 | — |
| BL-14 | Named-recipe library + chaining | M | ready (no dep) | — |
| BL-15 | TOON handoff of resolved stream | M | dep BL-05 | — |

## Gate log
### BL-01 — builder DONE 2026-06-17 (`cd646ab`), reviewer audit OPEN
Builder report `replies/005`. 11 sites filled, `static_assert == 11`, refused-before-
build (compile/resolve/ascii), round-trip + refusal tests. Build green, ctest 95/95,
scan clean, cross-language unchanged. Flagged extension: a `negative_extruded_profile_
base_z` warning (not briefed) — reviewer to keep/drop. Audit brief `briefs/006`.

### BL-01 — SHIPPED 2026-06-17 (reviewer audit `replies/006`: SHIP, no defects)
Reviewer reproduced green (full app builds, recipe ctest 7/7); confirmed all 11 sites
real (no catch-all), reader key-for-key BL-04-ready, refused-before-build complete.
**Integration decision:** KEEP the `negative_extruded_profile_base_z` warning — it is
exact parity with the lathe/moulding `negative_*_base_z` warnings; omitting it was the
inconsistency. No code change needed; accepted as-is.
**Seam recorded (closes in BL-03):** `saveResolvedOpsToToml` (EdiShellWindowIo.cpp,
edi-ui-host file, NOT ours) gates only on `resolved.ok`, so today it could serialize an
UNLOWERED extrude into a "resolved" export — harmless (downstream compile/ascii/python
still refuse). It closes naturally once BL-03 makes resolve LOWER the extrude (no raw
extrude survives resolve). No action for us; BL-03 dissolves it.

### BL-03 — builder briefed 2026-06-17
Brief `briefs/007-bl03-prism-carrier.md`. New `AddPrismOp` arm (→12) + resolve-lowering
AddExtrudedProfile→AddPrismOp via a new `resolveExtrudeProfilePoints` footprint
projector. AddPrismOp = OBJ-only-proof (mirrors Script's ASCII: no-op bounds, empty
draw, NOT refused — it is the buildable carrier).

### BL-03 — SHIPPED 2026-06-17 (keystone audit `replies/008`: SHIP, no defects)
Reviewer reconstructed old-vs-new `resolveProfilePoints` line-by-line → lathe
**byte-identical** (the `ProfileSource.ok=false` default is the load-bearing invariant);
all 12 sites real; carrier passes every buildable consumer; no raw extrude survives
resolve (BL-01 seam dissolved); reader key-for-key BL-04-ready. **Flags resolved:**
(a) bindable prism fields KEEP (AddMoulding precedent), (b) degenerate wording ACCEPTABLE.
**Deferred (recorded, not a blocker):** the footprint gate checks distinct-count (≥3)
but not collinearity/zero-area — CONSISTENT with the family (lathe/moulding guard
neither). A 3+ collinear-distinct figure lowers to a flat prism (no crash). NOT adding a
prism-only guard (would diverge from the family); parked as a possible future
"degenerate-geometry guards across the carrier family" polish item.

### BL-04 — builder briefed 2026-06-17
Brief `briefs/009-bl04-prism-python.md`. The Python prism half: ARCHITECTURAL_OPS +
parse_ops (key-for-key) + `_prism_world` mesh + obj/plan/bounds/build + raw-extrude
refusal + a new sample + OBJ golden + smoke. Completes the extrude spine.

### BL-04 — SHIPPED 2026-06-17 (audit `replies/010`: SHIP, spine sound end-to-end)
Reviewer reproduced green; cross-language contract EXACT key-for-key (defaults
included — the table in §1 of the verdict), mesh honest+pure (signed height, deduped
loop, shared with the bpy build), raw extrude refused both sides, doric byte-identical,
prism golden byte-pinned. **The north-star extrude spine is COMPLETE: a drafted figure
→ AddExtrudedProfile → AddPrism → OBJ/Blender.**

### Spine closeout — 2026-06-17 — planner
Arch doc refreshed for the spine (§1 arms 11+12 + PrismPoint; §2 →12 arms +
static_assert 12; §3 prism contract; §4 extrude lowering; §5 AddPrism OBJ tier; §9
"no extrude" SUPERSEDED). Line-anchor full sweep deferred to batch close (arms 13/14
still incoming). Reported SPINE COMPLETE to hub.

## LEDGER policy (ratified 2026-06-17)
Do NOT commit `docs/handoffs/LEDGER.md` on `dept/blender-lab` (kills rebase conflicts;
edi-ui owns the master LEDGER per PROTOCOL.md). State lives HERE (this handoff doc) +
`bus-hub` reports. This doc is the department's source of truth.

## Master integration
- 2026-06-17: spine (BL-01/03/04) MERGED to master by edi-ui (master `0041783`, tip
  `efb6032`). Rebased `dept/blender-lab` onto master `dd226c4` (carries drafting
  transformGeometry+snaps + dungeon-map DM-02..08). Clean (recipe work isolated). Full
  integration line GREEN: build clean, ctest 96/96, recipe 7/7, prism/smoke ok. Rebased
  tip `bf9bae3`. Report closeouts to edi-ui for the (master-owned) ledger.

### BL-06 — builder briefed 2026-06-17
Brief `briefs/012-bl06-sweep.md`. Field-add `sweepDegrees` (default 360) on
AddRevolvedProfile, SURVIVES lowering to AddMoulding, threads both languages +
`moulding_rings`. Behavior-preserving (default 360 = doric golden byte-identical).
Unblocks BL-07. No arm collision.

## Next
- BL-06 builder → reviewer audit (survives-lowering + doric-byte-identical are the
  joints) → BL-07 (screw/helix, dep BL-06) OR a Wave-1 independent (BL-12/BL-14).
- Arm-adders still serial: BL-08 (→13), BL-11 (→14) — one at a time, later.
