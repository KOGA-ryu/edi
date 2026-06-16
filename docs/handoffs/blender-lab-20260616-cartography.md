# Handoff — blender-lab-20260616-cartography

> The per-campaign state. Each gate appends its result; the NEXT gate reads this
> first. Agents hand off THROUGH this file — they cannot message each other.

- **Campaign**: blender-lab-20260616-cartography
- **Department**: edi-blender-lab
- **Goal (one line)**: MAP + DOCUMENT + behavior-preservingly REFACTOR the recipe
  lab so its architecture is understood and clean before features land.
- **Boundary (the question the reviewer gate must settle)**: What exactly is the
  recipe lab's architecture today — the `RecipeOp` variant and its full set of
  exhaustive-visit sites, the C++↔Python TOML contract key-for-key, the
  resolve/lowering path, the proof tiers, the craftsmen scan, the Blender
  subprocess wiring, click→PNG call graph, the seams in/out — and which
  behavior-preserving refactors are warranted (duplication, dead/empty arms,
  drift from data-oriented rules, un-commented wiring)?

## Gate log

### Reviewer gate — 2026-06-16 — edi-blender-lab-reviewer (COMPLETE)
Brief: `briefs/002-cartography-reviewer.md`; findings:
`~/dept-bus/edi-blender-lab/replies/002-cartography-reviewer.md`. Planner
spot-verified the load-bearing line cites (estimateBounds 5/10, palette 4 types,
store-reader default, empty Script ascii arm — all confirmed).
- **RecipeOp = exactly 10 arms.**
- **Interpreter map CORRECTED:** the "7 visit sites" prior was 7 *roles*. Reality:
  **10 compiler-exhaustive `std::visit` call sites (9 distinct visitors)** + **2
  roles that are NOT exhaustive** (store reader = string if-ladder w/ refusing
  default; resolve = lathe-only `get_if`) + a **3rd non-exhaustive `get_if`**:
  `estimateBounds` (ascii framing) covers only 5/10 ops, silent fall-through.
- **C++↔Python TOML contract: NO DRIFT** (every op key-for-key; live `--obj-out`,
  `--dry-run`, `--list-craftsmen` green on the doric sample).
- **Script invisible in TWO proof tiers** (ASCII + dry-run); OBJ is its only proof.
- All 3 planner priors CONFIRMED (lathe-only bridge; lathe absent from palette;
  empty Script ascii arm — deliberate + commented).
- No data-oriented-rule violations. ProcessRunStore/BlenderRunPlan are src/io +
  src/scripting (seams we record, not edit).
- **Boundary settled? YES.** Architecture mapped; refactor candidates ranked.

### Scribe — 2026-06-16 — planner
Wrote `docs/architecture/edi-blender-lab.md` (first draft, §1–§10). Corrected the
charter's "every visit is exhaustive" overstatement to point at the real map.

## Open questions / blockers
- Worktree has NO `build/` dir yet — `cmake -S . -B build` needed once before the
  green gate runs (told builder to configure it in slice A).

## Decided — behavior-preserving builder batch (this campaign)
- **Slice A** (MED–HIGH): convert `estimateBounds` (RecipeOpsAscii.cpp:81–116) to a
  compiler-exhaustive `std::visit` overload set — no-op arms for the 5 draw-nothing
  ops, identical extents for the 5 it already frames. Comment the AddLabel
  divergence from Python `bounds_of`. Behavior-preserving.
- **Slice B** (MED): `static_assert(std::variant_size_v<RecipeOp> == 10)` + teaching
  comment beside the store-reader if-ladder (RecipeOpsStore.cpp:~605) — a tripwire
  for the one non-compiler-enforced interpreter role. Compile-time only.
- Deferred (NOT this campaign): #4 dry-run Script line (touches behavior; folds with
  backlog Script-ASCII work), #5 craftsman param-type default (hand-built-TOML-only).

### Builder batch — 2026-06-16 — edi-blender-lab-builder (COMPLETE)
Brief `briefs/003`; report `replies/003-cartography-hardening.md`.
- **Slice A** `4a561e8` — `estimateBounds` → `std::visit` over named overload struct
  `BoundsEstimator` (templated on the `include` lambda, by-member, inlinable). All 10
  arms explicit: 5 keep identical extent math, 5 explicit no-ops, AddLabel comment re
  Python `bounds_of` divergence.
- **Slice B** `b6af915` — `static_assert(std::variant_size_v<RecipeOp> == 10)` at top
  of `recipeOpsFromToml` + teaching comment. `<variant>` transitive; no new include.
- **Gate (recipe slice):** recipe targets build clean; `ctest` recipe slice **7/7**
  (incl. `recipe_ops_ascii_tests` + doric goldens unchanged); scan clean; all 3
  cross-language checks green (`--obj-out` 306482-byte OBJ, `--list-craftsmen`, smoke).
- **Cross-dept BLOCKER surfaced:** full `cmake --build build` fails compiling
  `tests/drafting_room_tests.cpp` (missing `#include <memory>` for `std::make_shared`)
  — confirmed pre-existing via `git stash`, edi-drafting's ownership, untouched by
  this batch. Analogous to the documented `edi_shell_window_tests` exclusion.
  Escalated to hub.

### Reviewer diff-audit gate — 2026-06-16 — edi-blender-lab-reviewer (COMPLETE)
Brief `briefs/004`; verdict `replies/004-cartography-audit.md`.
- **Slice A: behavior-preserving — YES** (high confidence). Per-op extents
  byte-equivalent (only `->`→`.`); empty-profile early-out preserved (`return`≡old
  `continue`); 5 no-op arms frame nothing new; templated struct holds `include` by
  const-ref (mutations land on the real `bounds`, no dangle/copy); 10 explicit arms,
  no `auto` catch-all → real exhaustiveness; goldens unchanged.
- **Slice B: behavior-preserving — YES** (high confidence). Assert value correct,
  `<variant>` in scope, declaration-only (no runtime/logic effect), message names
  both obligations.
- **No defects. Reviewer independently REPRODUCED green:** `ctest -R recipe` 7/7,
  `--obj-out` byte-stable OBJ, smoke ok.
- Confirms `drafting_room_tests.cpp` failure independent + edi-drafting's (intro'd by
  `e61c638`/`2938d7a`), correctly escalated, NOT ours.
- **Verdict: CLOSE THE CAMPAIGN.**

### Closeout — 2026-06-16 — planner
`docs/closeouts/blender-lab-cartography.md` written; LEDGER row → Closed; closeout
reported to hub. Recipe-slice gate green; full-build gate blocked only by the
external drafting `<memory>` issue (hub/edi-drafting owns it).

## Next
- (none — campaign closed) Future: the feature roadmap (extrude M2, craftsmen M4…)
  is a separate campaign per `~/dept-bus/ROADMAPS-DRAFT.md`.
