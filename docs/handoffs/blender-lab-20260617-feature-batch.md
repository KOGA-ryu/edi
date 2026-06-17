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
| BL-01 | AddExtrudedProfile arm (→11 visits) | L | builder briefed | — |
| BL-03 | Resolve-lowering → new AddPrismOp (→12) | L | blocked on BL-01 | — |
| BL-04 | edi_craft.py prism build + OBJ golden | M | blocked on BL-03 | — |
| BL-05 | Push/Pull height authoring + bind | M | blocked on BL-04 | — |
| BL-06 | Lathe sweepDegrees param | M | ready (no dep) | — |
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
### BL-01 — builder briefed 2026-06-17
Brief `~/dept-bus/edi-blender-lab/briefs/005-bl01-extrude-arm.md`. Add-the-arm +
round-trip + refusals only (no lowering, no Python). Diff → reviewer audit after.

## Next
- BL-01 builder → reviewer diff audit (risky joint: 11 visit sites + cross-lang shape)
  → BL-03.
