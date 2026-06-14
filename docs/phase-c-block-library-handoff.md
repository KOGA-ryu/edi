# Phase C handoff — block / symbol library (cold-start)

> **STATUS (2026-06-14): the design workflow §5 has been RUN; the fork is
> resolved.** Outcome → **FLATTEN-on-place** (unanimous judge panel; critique
> verdict *sound*, grafts folded in). **C1 shipped** (commit ccd14c6). The living
> plan with the full resolution + the refined C2/C3 specs is
> `docs/dungeon-map-tool-backlog.md` §Phase C — read that first now; this doc is
> the historical design brief. Do NOT re-run the workflow.

A self-contained handoff so a fresh session can run Phase C of the tool-first
dungeon-map program and continue. Phase C is the **block / symbol library** (the
"flash sheet"): save a selection as a named block, stamp transformed instances, a
tagged palette. It is the first phase that touches the widget/UI layer, so per the
plan it starts with a **design + adversarial-critique workflow** (authored below)
before any code.

---

## 0. Read first (cold start)

1. `CLAUDE.md` — the hard rules (no JSON in our data, no `.js`/`.qml`/QtQml, DOD).
2. Memories (in `~/.claude/projects/-Users-kogaryu-edi/memory/`): **edi-tool-first-mandate**
   (the program mandate), **edi-dungeon-map-research** (shipped history), **edi-coding-rules**,
   **edi-format-decisions**, **edi-figma-restructure** (the belt/palette UI idioms).
3. This doc, then `docs/dungeon-map-tool-backlog.md` (the living plan) and
   `docs/dungeon-map-seams.md` **§M** (the block seam — the one genuinely-new seam).
4. Verify the repo and the green baseline:
   ```
   git rev-parse --show-toplevel    # must be /Users/kogaryu/edi
   cmake --build build && ctest --test-dir build --output-on-failure   # 93 green
   ```
   Plus the scan: no `.js`/`.qml`, no `.json` outside `.claude/`, no `QtQml`/`QtQuick`.

## 1. Where things stand

- **Mandate (do not re-litigate):** tool-first stop-line · run-ahead-autonomously
  (surface only at phase boundaries / real forks) · **planner drives the forks** with
  recommendations.
- **Shipped on `master`:** the wall · the map graph (plugs + connections) · multi-room
  `.map.toml` authoring · corridors v1+v2 (A\* obstacle routing) · doors. Backlog
  **Phases A + B are done**; **C (this) and D (Seam B export) remain, then STOP.**
- `master` is ahead of `origin` by ~15 commits, **unpushed** (push policy is the user's
  call — ask once if it matters).
- The reference dungeon renders fully:
  `QT_QPA_PLATFORM=offscreen ./build/edi --map-file tests/data/dungeon.map.toml --snapshot /tmp/d.png`.

## 2. Phase C scope (from the backlog)

- **C1 — block definition.** Save a selection/group as a named block.
- **C2 — block instances.** Place/stamp instances (position, rotation, scale) of a
  definition, with per-instance overrides.
- **C3 — palette + tag/set taxonomy.** A browsable palette categorized by tag/set,
  click to stamp. (Widget layer.)

## 3. Forks to drive in C (planner's call — recommendations; the workflow refines)

- **Reference model — the main fork.** Instance = **FLATTEN-on-place** (an independent
  transformed copy via the paste/array machinery) vs **DEF+INSTANCE live reference**
  (AutoCAD: instances re-expand if the definition changes). *Lean:* start FLATTEN for
  the MVP — it ships the flash-sheet value on the existing clipboard/array machinery
  with the least new architecture; live-reference is a later upgrade. The judge panel
  in the workflow should confirm or overturn this.
- **Block storage.** A document-level `blocks` (definitions) vector on `DraftingDocument`,
  persisted additively in MessagePack — *mirror exactly how `plugs`/`connections` were
  added* (no document-version bump; tolerant decode). This is the precedent; reuse it.
- **Palette UI.** Reuse the belt-carousel idioms (row-per-tool, peeks, pinning — see
  edi-figma-restructure) rather than inventing a new control; mount a block palette as a
  panel/tab. Keep heavy visual polish out (the user owns the look — flag visible changes).

## 4. The §M seam (what Phase C plugs into)

There is no block system today. Closest existing relatives (reuse, don't reinvent):
- **Copy/paste:** `m_clipboard` (`DrawingCore.h`) + `planDraftingPaste` (pure id-mint +
  offset) + `CreateObjectsCommand` — an instance is "paste with a transform".
- **Array-from-active:** `createArrayFromActiveObject` + `radialArrayDraftingObject`
  (`DraftingArray.h`) — mint N transformed copies atomically.
So a block definition = a saved named group; an instance = a transformed placement via
the paste/array machinery + `createObjectsAndSelect`. The genuinely new part is
**persisting the definition** and **the palette UI**.

## 5. The design workflow — RUN THIS FIRST

Run it with the Workflow tool: paste the script below into `Workflow({ script })` (it
self-persists; iterate via the returned `scriptPath`). It grounds the three seams in
parallel, panels three block-model approaches with a judge vote, then adversarially
critiques the winner, returning a C1–C3 slice plan. Read the result, drive the fork,
then build the slices one verified commit at a time (the `/goal` discipline).

```js
export const meta = {
  name: 'block-library-design',
  description: 'Design edi Phase C block/symbol library: ground the reuse seams, panel 3 block-model approaches with a judge vote, adversarially critique the winner, emit a C1-C3 slice plan',
  phases: [
    { title: 'Ground', detail: 'paste/array, persistence, UI seams' },
    { title: 'Design', detail: '3 block-model approaches + judge panel' },
    { title: 'Critique', detail: 'adversarial review of the winner' },
  ],
}

const CONTEXT = `
PROJECT: edi — Qt6/C++20 2D drafting/CAD app + a C++/DOD learning project.
HARD RULES: no JSON in our own data; no .js/.qml/QtQml/QtQuick; data-oriented design
(pure logic = free functions over plain structs in src/drafting/, returning {ok+payload};
commands are a DraftingCommand variant via applyDraftingCommand; NO subclassing for
behaviour). Formats: TOML config, MessagePack documents, TOON AI handoff. Commits teach.
MANDATE: tool-first, run-ahead-autonomous, planner drives forks (memory edi-tool-first-mandate).

PHASE C: design the BLOCK / SYMBOL LIBRARY (the "flash sheet"): save a selection/group as a
named BLOCK definition; place/stamp INSTANCES (position, rotation, scale) of it with per-instance
overrides; a tag/set palette to browse + stamp. Slices C1 (definition), C2 (instances), C3 (palette).
First phase touching the widget layer.

THE SEAM (docs/dungeon-map-seams.md §M): no block system yet. Closest relatives:
- copy/paste: m_clipboard (DrawingCore.h) + planDraftingPaste (pure id-mint + offset) +
  CreateObjectsCommand — an instance is "paste with a transform".
- array-from-active: createArrayFromActiveObject + radialArrayDraftingObject (DraftingArray.h)
  mint N transformed copies atomically.
Block def = a saved named group; instance = transformed placement via paste/array +
createObjectsAndSelect. NEW part = persisting the definition + the palette UI. PRECEDENT for
document-level data: plugs/connections were added as document-level vectors on DraftingDocument +
ADDITIVE MessagePack (NO version bump, tolerant decode). Blocks should mirror that.

FORK to resolve (recommend + justify): instance = LIVE REFERENCE to the definition (AutoCAD;
updates if the def changes) vs FLATTEN-on-place (an independent transformed copy). edi is
tool-first + editable-geometry; lean FLATTEN for the MVP unless grounding argues otherwise.
`

const GROUND_SCHEMA = {
  type: 'object', additionalProperties: false,
  properties: {
    summary: { type: 'string' },
    reuse: { type: 'array', items: { type: 'string' }, description: 'exact existing code to reuse/extend (file:symbol)' },
    extensionPoints: { type: 'array', items: { type: 'object', additionalProperties: false, properties: { file: { type: 'string' }, what: { type: 'string' } }, required: ['file', 'what'] } },
    constraints: { type: 'array', items: { type: 'string' } },
  },
  required: ['summary', 'reuse', 'extensionPoints'],
}

phase('Ground')
const grounds = await parallel([
  () => agent(CONTEXT + `\nGROUND the PASTE/ARRAY machinery (minting transformed copies). Read in /Users/kogaryu/edi: src/core/DrawingCore.h (m_clipboard, planDraftingPaste, createArrayFromActiveObject, createTransformedActiveObject, createObjectsAndSelect), src/drafting/DraftingArray.h and .cpp, the paste planner (grep planDraftingPaste), tests/drafting_array_tests.cpp, tests/drafting_clipboard_tests.cpp. Report how a block INSTANCE (transformed placement) reuses this.`, { agentType: 'Explore', schema: GROUND_SCHEMA, phase: 'Ground', label: 'ground:paste-array' }),
  () => agent(CONTEXT + `\nGROUND the DOCUMENT + PERSISTENCE model for a block DEFINITION. Read: src/drafting/DraftingDocument.h (objects/layers/selection + the plugs/connections vectors as the precedent), src/drafting/DraftingSerialize.cpp (how plugs/connections serialize additively), src/drafting/DraftingTypes.h (ObjectMetadata, tags/exportGroup, ids). Report where a 'blocks' definition vector lives + persists (no JSON, additive MessagePack, no version bump), and how a named group of objects + a transform basis is captured.`, { agentType: 'Explore', schema: GROUND_SCHEMA, phase: 'Ground', label: 'ground:persist' }),
  () => agent(CONTEXT + `\nGROUND the WIDGET/UI layer for a block PALETTE + stamping. Read: src/widgets/ (EdiShellWindow, the belt/carousel, the DrawingCanvas family) and the pick-a-point placement path (PointCaptureIntent / resolvePointCapture in DrawingCore.h, DrawingDocumentController). Report how a tagged block palette panel mounts and how clicking a block stamps an instance at a picked point, reusing belt idioms.`, { agentType: 'Explore', schema: GROUND_SCHEMA, phase: 'Ground', label: 'ground:ui' }),
])
const g = grounds.map(r => r || { summary: '(agent failed)', reuse: [], extensionPoints: [] })
const groundingBlock = `GROUNDING:\n== paste/array ==\n${g[0].summary}\nreuse: ${g[0].reuse.join(' | ')}\n== persistence ==\n${g[1].summary}\nreuse: ${g[1].reuse.join(' | ')}\n== ui ==\n${g[2].summary}\nreuse: ${g[2].reuse.join(' | ')}`

const APPROACH_SCHEMA = {
  type: 'object', additionalProperties: false,
  properties: {
    name: { type: 'string' },
    model: { type: 'string', description: 'data model: where the definition lives, how instances reference it, the transform; DOD + no JSON + additive MessagePack' },
    slices: { type: 'array', items: { type: 'object', additionalProperties: false, properties: { id: { type: 'string' }, title: { type: 'string' }, scope: { type: 'string' }, accept: { type: 'string' } }, required: ['id', 'title', 'scope', 'accept'] } },
    pros: { type: 'array', items: { type: 'string' } },
    cons: { type: 'array', items: { type: 'string' } },
  },
  required: ['name', 'model', 'slices', 'pros', 'cons'],
}
phase('Design')
const APPROACHES = [
  'FLATTEN-on-place: a block definition is a saved named group; placing an instance pastes transformed COPIES (reuse planDraftingPaste / array). No live link.',
  'DEF+INSTANCE live reference (AutoCAD): a blocks vector holds definitions; an instance is a lightweight object referencing a definition id + a transform; the projection expands it at render.',
  'ARRAY-TEMPLATE: a block is a saved array source; placement is a 1-count array-from-source with a transform — reuses the array machinery most directly.',
]
const designs = await parallel(APPROACHES.map((a, i) => () => agent(CONTEXT + '\n' + groundingBlock + `\nDESIGN the block library for edi as APPROACH: ${a}\nGive the DOD data model (plain structs + free functions; no JSON; additive MessagePack), the C1/C2/C3 slices each with an acceptance test, and honest pros/cons for a tool-first MVP.`, { schema: APPROACH_SCHEMA, phase: 'Design', label: `design:${i}` })))
const live = designs.filter(Boolean)

const JUDGE_SCHEMA = { type: 'object', additionalProperties: false, properties: { ranking: { type: 'array', items: { type: 'object', additionalProperties: false, properties: { name: { type: 'string' }, score: { type: 'number' }, why: { type: 'string' } }, required: ['name', 'score', 'why'] } }, winner: { type: 'string' }, graft: { type: 'string' } }, required: ['ranking', 'winner'] }
const judges = await parallel(Array.from({ length: 3 }, (_unused, k) => () => agent(CONTEXT + `\nJUDGE these block-library approaches for a TOOL-FIRST edi MVP (ship the flash-sheet value with the least new architecture, DOD-clean, editable geometry). Approaches:\n${JSON.stringify(live, null, 1)}\nScore each 0-10, pick a winner, and name ideas worth grafting from the others.`, { schema: JUDGE_SCHEMA, phase: 'Design', label: `judge:${k}` })))

phase('Critique')
const CRITIQUE_SCHEMA = { type: 'object', additionalProperties: false, properties: { verdict: { type: 'string', enum: ['sound', 'needs-changes', 'flawed'] }, violations: { type: 'array', items: { type: 'object', additionalProperties: false, properties: { issue: { type: 'string' }, severity: { type: 'string', enum: ['blocker', 'major', 'minor'] }, fix: { type: 'string' } }, required: ['issue', 'severity', 'fix'] } }, missing: { type: 'array', items: { type: 'string' } } }, required: ['verdict', 'violations', 'missing'] }
const critique = await agent(CONTEXT + '\n' + groundingBlock + `\nJudges' rankings:\n${JSON.stringify(judges.filter(Boolean), null, 1)}\nApproaches:\n${JSON.stringify(live, null, 1)}\nAdversarially review the WINNING approach's slice plan for a tool-first edi: JSON leaks, subclassing-for-behaviour, undo atomicity, MessagePack additive-tolerance (no version bump unless an existing field is reinterpreted), per-instance overrides, and whether each slice is independently committable + green. Name what is missing.`, { schema: CRITIQUE_SCHEMA, phase: 'Critique', label: 'critique' })

return { grounding: g, approaches: live, judges: judges.filter(Boolean), critique }
```

## 6. Provisional acceptance criteria (the workflow refines these)

- **C1:** define a block from a selection → a named definition is stored on the document
  and round-trips through `.edidraw` (MessagePack, additive). *Test:* a focused ops/serialize test.
- **C2:** place an instance with a transform → the instance objects appear in one undo
  step. *Test:* controller test + a snapshot.
- **C3:** the palette lists blocks by tag and clicking one stamps an instance at a picked
  point. *Test:* `edi_shell_window_tests` / `drawing_canvas_widget_tests` (offscreen,
  driven by objectName / synthesized events).

## 7. After C → Phase D, then STOP

**Phase D — Seam B export:** a TOON projection of the neutral map graph
(rooms / plugs / connections / tags) via `src/formats/ToonExport.*`, plus an
`--export-map` action. Closes the author → export loop. **That is the tool-first
terminus** — stop after D (generation is out of scope).

## 8. The loop (every slice)

`cmake --build build && ctest --test-dir build --output-on-failure` fully green, plus
the no-JSON/no-qml scan. Commit per vein with a teaching body (`claude: <summary>`) +
the `Co-Authored-By` trailer. Update `docs/dungeon-map-tool-backlog.md` status as slices
land. Surface to the user at the C→D boundary.
