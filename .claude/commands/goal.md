---
description: Work the DrawingDocumentController refactor, one verified slice at a time, until complete
---

# Goal: complete the controller refactor

Reduce `src/core/DrawingDocumentController.cpp` to thin delegation: every method resolves
its inputs, delegates planning to pure functions, applies a command, and emits. All pure
logic lives in `src/drafting/` as free functions over plain data, covered by focused tests.

Work slices continuously until the Definition of Done holds or a Stop Condition fires.
Do not pause to ask permission between slices.

## Hard rules

- **No JSON** in project source (config, serialization, anything). `.claude/` harness files are exempt.
- **No `.js`** and no `.qml` — keep the JS/QML scan at zero.
- **Data-oriented design**: variation points become data (enums, kinds, plan structs) or
  plan callables — never subclassing, never stateful objects. Pure functions over plain
  structs, in the `Drafting*Ops` style. Behavior stays separable and testable.
- **Behavior-preserving only.** If a slice would change observable behavior, stop and report
  instead of committing.

## State

Derive progress from the repo, not a state file:
- `git log --oneline` — completed slices are the `claude:` commits (`codex:` before 2026-06-10).
- A fresh scan of the controller — what duplication remains.

## Per-slice protocol

1. **Clean check.** `git status --short` must be clean. If not, stop and report.
2. **Scan.** Find the next-cleanest duplication vein in `DrawingDocumentController.cpp`:
   - two or more methods whose bodies differ only by a constant, enum, kind, or plan call;
   - inline validation/computation that belongs in a `Drafting*Ops` free function;
   - repeated resolve/guard sequences not yet behind a shared query or helper.
   Prefer the smallest, lowest-risk slice. One vein per commit.
3. **Implement.** Extract to data-parameterized helpers (kind + callable pattern already
   established: `applyActiveObjectMetadataUpdate`, `applyActiveObjectGeometryUpdate`,
   `applySelectionDrawablePlacement`, `applyGuideDrawablePlacement`). New pure logic goes
   in `src/drafting/` with a focused test target registered in CMake.
4. **Verify — all must pass before commit:**
   - `cmake --build build` (controller TU and `edi` must compile)
   - focused test for anything new, then `ctest --test-dir build --output-on-failure` — fully green
   - scan: no `.js`/`.qml` anywhere; no `.json` outside `.claude/`; no `QtQml`/`QtQuick` refs
   - read the diff; confirm behavior preservation (watch optional-deref guards and
     variant conversions — `DraftingGeometry{plan.geometry}` must be explicit)
5. **Commit** in the established style: `claude: <imperative summary>` plus a short body,
   ending with `Co-Authored-By: Claude Opus 4.8 <noreply@anthropic.com>`.
   If `.git/index.lock` or `.git/HEAD.lock` exists, check it is zero-byte stale (only
   `fsmonitor--daemon` processes running) before removing it.
6. Repeat from step 1.

## Definition of Done

A full scan of the controller confirms all of:
1. No two methods whose bodies differ only by a parameterizable constant, enum, or callable.
2. No inline pure logic (validation, geometry math, plan construction) that could move to a
   `Drafting*Ops` free function — only resolve → plan → apply → emit orchestration remains.
3. Full suite green, JS/QML/JSON scan clean, tree clean.

When done: report the final controller line count versus its starting ~2,137, list the veins
consolidated, and stop.

## Stop conditions

- A test fails and the fix isn't contained within the current slice → revert the slice
  (`git checkout -- .`), report the failure.
- A candidate slice requires a behavior change or an API decision → skip it, note it in the
  final report, continue with other veins.
- Definition of Done holds → final report.
