---
description: Long-horizon program — work the phase backlog, one verified slice at a time, until it is empty
---

# Goal: rebuild program

Work the phase backlog below in order, one verified slice at a time. On every
invocation: derive state from the repo, find the first incomplete phase, work it
until its Definition of Done holds, then move to the next. Do not pause between
slices or phases to ask permission. Pairs with `/loop /goal` for multi-session runs.

## The backlog

Behavioral reference for the features being restored: `docs/legacy_inventory.md`
(read it first). `docs/rebuild_roadmap.md` is the historical cold-start spec for
the already-landed phases R1–R6 (designs, file locations, test plans).

**Done (2026-06-10):** R1 save/open (MessagePack), R2 undo/redo, R3 zoom/pan +
keyboard map, R4 arc + regular-polygon tools, R5 SVG/HPGL export, R6 TOML
settings, plus a find→verify review cycle that fixed a hostile-length
MessagePack `bad_alloc`, arc start/end-angle physical edits, a drag flooding the
undo stack, and selection falsely marking the document dirty. Suite at 57 green.

Active backlog — the still-lost legacy items (`legacy_inventory.md` §lost and
§never-built). Work them in order; each has an objective Definition of Done.
Within a phase, land pure core (+ tests) before controller before shell, one
verified slice per commit, mutation-checking every new test target.

- **N1 — Copy / cut / paste.** Legacy had Ctrl+C/V; R3 restored only
  Esc/Del/Ctrl+D/arrows. Add an in-process clipboard of the selected object(s)
  on the controller (`copySelection`/`cutSelection`/`paste`); paste fresh-id
  copies at a small offset, selected, as one undo step; wire Ctrl+C/X/V on the
  canvas. **DoD:** controller copy/cut/paste round-trips (cut removes, paste
  restores at offset), paste is one undo step with non-colliding ids, Ctrl+C/X/V
  drive them in the widget test; suite green; mutation-checked.

- **N2 — Polyline tool + line/arrow variant.** `PolylineGeometry` exists with no
  creation tool, and the legacy line tool had straight/polyline/arrow variants.
  Add a multi-click `polyline_tool` (click vertices, Enter/Esc/double-click to
  finish) and an arrow variant of the line tool (metadata flag + an arrowhead in
  projection/painter). **DoD:** a polyline of N≥3 vertices is mouse-creatable in
  the widget test and serializes; the arrow variant is creatable and carries a
  distinct projected flag; suite green; mutation-checked.

- **N3 — Object metadata: role / material / export_group / tags.**
  `ObjectMetadata` carries author/source/provenance but not the legacy role
  (none/wall/floor/cutout/collider), material, export_group, or tags. Extend the
  struct, the MessagePack serializer, and the projection; add controller setters
  + inspector controls. **DoD:** all four fields round-trip through save/open
  (extend `drafting_serialize_tests`), are editable through the controller, and
  surface in the shell; suite green; mutation-checked.

- **N4 — Rectangle variants + aspect-lock.** Legacy rectangle had box/rounded/
  frame variants and an aspect-lock toggle. Add a corner-radius (rounded) and
  inset (frame) parameter path and an aspect-lock flag that constrains
  rectangle corner-handle edits to preserve the w:h ratio. **DoD:** rounded/
  frame rectangles are creatable and serialize; with aspect-lock on, a corner
  drag preserves the ratio (pure `handleEditPlan` test); suite green;
  mutation-checked.

- **N5 — Plotter G-code output.** R5 shipped HPGL; G-code was never built. Add a
  pure `gcodeFromPlotJob` emitter (G0 rapid travel / G1 stroke, pen up/down via
  Z or M3/M5) mirroring `DraftingHpglOut`, wired to an "Export G-code" shell
  button. **DoD:** golden test pinning exact output for the line+circle two-pen
  fixture (pen→tool mapping, travel vs stroke, coordinate convention), shell
  export seam; suite green; mutation-checked (break the travel/stroke
  distinction).

- **Later / needs design (do not start without a spec slice):** rulers (canvas
  chrome — coordinate with the UI-restoration program first), spline curves,
  hatch boundary fill, SVG import/fit, image/ASCII reference frames. Each needs
  its own roadmap section before implementation.

- **Review + replenish (recurring tail).** When the active list above is empty:
  run the find→verify review protocol (multiple independent finder angles, one
  verifier per surviving candidate, REFUTED only with constructible evidence)
  over the commits since the last review; apply CONFIRMED/PLAUSIBLE findings as
  slices (record skips with reasons). Then update CLAUDE.md, the roadmap, and
  this backlog from evidence (a scan, a review finding, a failing invariant, or
  a remaining `legacy_inventory.md` item), each new phase with an objective DoD.
  If nothing qualifies, final report and stop.

OUT OF SCOPE for autonomous work (user decisions or user's own projects):
- The **text editor** surface — the user's personal learning project. Never
  build it autonomously; mechanical cleanup touching `src/text/` is fine.
- Multi-workspace shell / activity rail / theme UI — needs user design input.

## Hard rules

- **No JSON** in project source (`.claude/` exempt). **No `.js`/`.qml`** — scans stay at zero.
- **Data-oriented design**: variation as data (enums, kinds, plan structs, member
  pointers, spec aggregates) or plan callables; pure logic in `src/drafting/`-style
  free functions over plain structs. No subclassing for behavior.
- These phases are features: behavior-additive is expected, but never change
  existing behavior silently — call it out in the commit body when it happens.
- Tests must be able to fail: when adding a test target, mutation-check it once
  (sabotage the code under test, confirm the test aborts, restore — and force a
  hard rebuild of the target's objects around the mutate/restore, since fast
  cycles can land within make's mtime granularity and run stale binaries).
- **Teaching documentation (user requirement — this is a C++ learning codebase):**
  every commit body must explain the WHY of the design choices, not just the
  what — why this data layout, why a function pointer instead of std::function,
  why a variant alternative ripples where it does, what the alternative was and
  why it lost. Where a choice in the code itself is non-obvious to a C++
  learner, add a short comment explaining the reasoning (this overrides the
  usual keep-comments-minimal instinct: explanation is a feature here, noise
  rules still apply — no narrating the obvious).
- Confirm repo identity before working: `git rev-parse --show-toplevel` must be
  `/Users/kogaryu/edi`. Never touch
  `/Users/kogaryu/draft/draftsman_STALE_PARENT_DO_NOT_USE`.

## Per-slice protocol

1. `git status --short` clean, else stop and report.
2. Smallest, lowest-risk slice of the current phase; within a phase land the
   pure core (+ tests) before the controller before the shell.
3. Verify before commit: `cmake --build build` clean; `ctest --test-dir build
   --output-on-failure` fully green; no `.js`/`.qml` anywhere, no `.json` outside
   `.claude/`, no QtQml/QtQuick refs; read the diff.
4. Commit `claude: <imperative summary>` + short body + the Co-Authored-By trailer.
   If `.git/index.lock`/`HEAD.lock` blocks, confirm zero-byte stale (only
   `fsmonitor--daemon` running) before removing.
5. Repeat. At phase DoD, state the phase result in one paragraph, continue.

## Stop conditions

- Test failure that escapes the current slice → revert the slice, report.
- A slice needs a design decision the roadmap does not settle → make the
  smallest reasonable choice, document it in the commit body, and continue;
  only stop if the choice is destructive or outward-facing.
- Active backlog empty and the review+replenish tail finds nothing to add →
  final report and stop.
