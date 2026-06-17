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
| BL-03 | Resolve-lowering → new AddPrismOp (→12) | L | builder briefed | — |
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

## Next
- BL-03 builder → reviewer diff audit (carrier shape + footprint projection + the
  variant→12 pass are the keystone joint) → BL-04 (Python prism build + OBJ golden).
- Arch doc: I (scribe) refresh §1/§2 to add arms 11 (AddExtrudedProfile) + 12
  (AddPrismOp) once the spine BL-01/03/04 lands as a coherent unit (avoid mid-spine
  churn).
