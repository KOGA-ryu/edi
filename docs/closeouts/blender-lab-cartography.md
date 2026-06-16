# Closeout — blender-lab-20260616-cartography

> Freezes the boundary so future work does not re-litigate it. Campaign:
> **map + document + behavior-preservingly refactor the recipe lab** (Seam A)
> before features land. Closed 2026-06-16 on `dept/blender-lab`.

## What this campaign settled (do not re-derive)

**The recipe lab is mapped.** The authoritative architecture is
`docs/architecture/edi-blender-lab.md` (§1–§10). Read it before touching
`src/recipe` / `tools/blender`. Frozen facts:

- **`RecipeOp` = exactly 10 arms** (`RecipeOps.h:192`): AddBox, AddCylinder,
  AddSphere, AddRing, AddMoulding, AddProfileMoulding, AddRevolvedProfile, CutFlutes,
  AddLabel, Script.
- **Interpreter map (the "exhaustive visit" reality):** **10 compiler-exhaustive
  `std::visit` call sites (9 distinct visitors)** — namer, store WRITER, validate
  (OpChecker + NameGetter), ascii (ProjectionDrawer), bind (FieldVisit ×2 + FieldList),
  schema (appendExtras + setExtra). Plus **2 roles that are NOT compiler-exhaustive**:
  the store **reader** (`recipeOpsFromToml`, a string if-ladder with a refusing
  default) and **resolve** (lathe-only `get_if`). After this campaign, `estimateBounds`
  (ascii framing) IS now exhaustive (was the 3rd silent `get_if`, 5/10 ops).
- **C++↔Python TOML contract: NO DRIFT** — `OpWriter` (RecipeOpsStore.cpp) ↔
  `parse_ops` (edi_craft.py) match key-for-key, every op. The guard is the
  cross-language smoke (`--obj-out`, `tests/edi_craft_smoke.py`), not a compiler —
  run it on every store/`edi_craft.py` change.
- **Proof tiers:** ASCII (silhouette), dry-run (build lines), compiled (moulding
  expansion), OBJ (`--obj-out` mesh). **`ScriptOp` is invisible in ASCII *and* dry-run
  — OBJ is its only proof** (deliberate; a craftsman's shape is its Python `proof_mesh`).
- **Seams:** lab CONSUMES drafting (read-only, via `RecipeMeasure` at resolve time);
  PRODUCES opaque asset ids (Seam B expansion is downstream). `ProcessRunStore`
  (`src/io`) + `BlenderRunPlan` (`src/scripting`) + the EdiShellWindow wiring are
  ADJACENT seams — the lab owns panel CONTENT, not those files.

## Refactors landed (behavior-preserving; reviewer-audited clean)

- **`4a561e8`** — `estimateBounds` (RecipeOpsAscii.cpp) converted from a 5/10
  `get_if` ladder to a compiler-exhaustive `std::visit` (`BoundsEstimator` overload
  struct templated on the `include` lambda; 5 ops keep identical extents, 5 explicit
  no-ops). A future op arm can no longer silently lose ASCII framing.
- **`b6af915`** — `static_assert(std::variant_size_v<RecipeOp> == 10)` tripwire at the
  top of `recipeOpsFromToml`, naming both obligations (reader branch + matching
  `parse_ops` arm) — a compile-time guard for the one interpreter role the language
  can't make exhaustive.

Diff audit verdict (`replies/004`): **both behavior-preserving, high confidence, no
defects.** Reviewer independently reproduced the green gate.

## Frozen decisions

- The charter line "every `std::visit` is exhaustive" was an overstatement; corrected
  to name which sites the compiler does/does not catch and to point at the
  architecture doc. **Do not revert.**
- **Deferred candidates (NOT bugs; out of this campaign's behavior-preserving scope):**
  - dry-run `plan_lines` emits no `Script` line (header miscounts) — folds with the
    backlog `ScriptOp` ASCII/proof work (roadmap M1 slice 2), since it touches the
    dry-run *output*.
  - craftsman `param.type` default divergence (C++ "text" vs Python "number") — only
    reachable on a hand-built registry TOML; low value.
  These are logged in architecture doc §10; revisit deliberately, do not rediscover.

## Verification state at close

- **Recipe-slice green gate: GREEN.** `ctest -R recipe` 7/7; scan clean; the 3
  `edi_craft` cross-language checks green (`--obj-out`, `--list-craftsmen`, smoke).
- **Full-build gate: BLOCKED by a non-lab issue** — `tests/drafting_room_tests.cpp`
  fails to compile (missing `#include <memory>` for `std::make_shared`), introduced by
  edi-drafting (`e61c638`/`2938d7a`), confirmed pre-existing and independent of this
  batch. **Escalated to the hub; edi-drafting owns the one-line fix.** Analogous to the
  documented `edi_shell_window_tests` golden-PNG environmental exclusion.

## Not re-opened by this campaign

No features (no extrude op, no palette change, no new craftsman). The recipe-lab
feature roadmap (extrude seam M2, craftsmen library M4, etc.) lives in
`~/dept-bus/ROADMAPS-DRAFT.md` and is a separate, future campaign.
