# edi

Qt6/C++20 2D drafting (CAD) application. CMake build, widget-based UI — deliberately no QML.

## Hard rules

- **No JSON** in project source — no config files, no serialization. (`.claude/` harness files are exempt.)
- **No `.js` and no `.qml`** — the codebase must stay free of QtQml/QtQuick references.
- **Data-oriented design.** Pure logic lives in `src/drafting/` as free functions over plain
  structs (`Drafting*Ops`). Variation points are data — enums, kinds, plan structs, member
  pointers — or plan callables. No subclassing for behavior, no stateful logic objects.
- **No hardcoded dimensions — every dimension is DATA.** Any dimension — width, height,
  thickness, radius, scale factor, fit-padding, spacing, greybox envelope — is a named
  field in a spec / config / constants table, derived or parameterizable; never a magic
  literal baked into logic. Where a dimension varies (scale, theme) it is a spec field or
  parameter, not a frozen constant. *Exempt:* epsilons / tolerances (numerical, not
  dimensions) and "unset" defaults (`= 0.0`). Reviewer-ENFORCED — a dimension is not
  grep-distinguishable from any number, so there is no mechanical scan; each reviewer
  audits its domain's diffs for magic dimension literals, builders introduce dimensions as
  named data, and existing magic dims are defects to sweep. (See `~/dept-bus/SCALE-POLICY.md`
  for the canvas↔feet↔Blender application.)
- **Behavior-preserving refactors only** unless a change is explicitly requested.

## Architecture

- `src/drafting/` — pure C++ core (`edi_drafting_core`), no Qt types. Plan functions return
  plan structs (`ok` + payload); commands are a `DraftingCommand` variant applied via
  `applyDraftingCommand`.
- `src/core/DrawingDocumentController.*` (class in `DrawingCore.h`) — thin Qt orchestration:
  resolve inputs → delegate planning → apply command → emit `modelChanged`. Shared
  orchestration goes through kind-and-callable helpers (`applyActiveObjectMetadataUpdate`,
  `applyActiveObjectGeometryUpdate`, `applyCommandAndEmit`, `applyLayerFlagsUpdate`, …) —
  extend these rather than re-inlining the resolve/plan/apply/emit sequence.
- `src/widgets/` — Qt widgets shell (`EdiShellWindow`, `DrawingCanvasWidget` + the
  `DrawingCanvas*` family; shared numeric parsing in `DrawingCanvasValues`).
  `edi_shell_window_tests` covers the window's wiring (offscreen platform, widgets
  driven by objectName, controller state asserted) and `drawing_canvas_widget_tests`
  covers canvas mouse paths via synthesized QMouseEvents; extend them when adding
  controls or gestures. Test gotcha: `rebuildGeometryEditor` retires spins with
  `deleteLater()` — flush `QEvent::DeferredDelete` before widget lookups.
- `src/io/` — persistence is live. `DrawingDocumentStore` does binary drawing
  I/O (save/open `.edidraw`) and text export (SVG/HPGL); `SettingsStore` is a
  free-function module over `StaticConfig` persisting `edi.toml`. Format
  decisions (2026-06-10): **TOML** for configurable settings, **MessagePack**
  for document data (the real value codec + `EDIM` envelope live in
  `src/formats/MessagePackValue.*` and `src/drafting/DraftingSerialize.*`),
  **TOON** for AI handoffs. Never JSON. (`DrawingRecentFilesStore`,
  `ShellLayoutStore`, `TextEditorStore` remain stubs.)
- `tests/` — one focused test file per ops slice, registered in `CMakeLists.txt`.

## Build & verify

Every change must pass this loop before commit:

```
cmake --build build
ctest --test-dir build --output-on-failure   # must be fully green
```

Plus the scan: no `.js`/`.qml` files anywhere, no `.json` outside `.claude/`, no
`QtQml`/`QtQuick` references in sources or CMake.

## Commits

Style: `claude: <imperative summary>` plus a short body explaining what was consolidated and
why it is behavior-preserving. One vein/slice per commit; working tree clean between slices.
(History before 2026-06-10 uses the older `codex:` prefix.)

**This is a C++ learning codebase.** The user is using it to learn C++ and
data-oriented design. Commit bodies must teach: explain WHY the design was
chosen (data layout, dispatch mechanism, ownership), what the alternative was,
and why it lost. Add short in-code comments where a choice would be non-obvious
to a C++ learner. Optimization for its own sake is not a goal; clarity of
reasoning is.

If `.git/index.lock` or `.git/HEAD.lock` blocks a commit, verify it is a zero-byte stale file
(only `fsmonitor--daemon` git processes running) before removing it.

## Gotchas

- `DraftingGeometry` is a `std::variant`; converting a concrete geometry into
  `std::optional<DraftingGeometry>` needs an explicit `DraftingGeometry{...}` (two implicit
  user-defined conversions won't chain).
- Plan structs carry concrete geometry types; `UpdateGeometryCommand` takes the variant.
- The `/goal` command (`.claude/commands/goal.md`) encodes the refactor-campaign protocol.

## Agent workflow

Work is organized into departments — `edi-drafting`, `edi-blender-lab`, `edi-ui`,
`edi-dungeon-map` — each with a planner + builder/reviewer/researcher under
`.claude/agents/edi-<dept>/`, a charter in `docs/departments/`, and (for the three
domain departments) a git worktree. The main session is the HUB (the user's one
window); it runs work in GATES — research → reviewer → builder → closeout, in that
order — and tracks every thread in `docs/handoffs/LEDGER.md`. Subagents are
one-shot and workers report only to their planner; the durable handoff is files
(the ledger, `docs/handoffs/`, `docs/closeouts/`), not agent memory. Full protocol:
`docs/agent-workflow.md`.

## Compact Instructions

When compacting this conversation, PRESERVE: the active campaign ids + their status
from `docs/handoffs/LEDGER.md`; any boundary currently being settled (the open
reviewer-gate question); the green-gate state of the last build (build/ctest/scan);
and any unpushed-commit / unmerged-worktree state. Do NOT spend compaction budget
restating the agent workflow, the gate definitions, or the department charters —
those live in files (`docs/agent-workflow.md`, `docs/departments/`,
`.claude/agents/`) and are re-read on demand.
