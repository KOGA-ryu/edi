# Legacy version inventory (mined from git history)

The near-complete QML/JS/JSON version of this app lives in this repo's history,
last intact around `ce0b751` (before the 2026-06-08 removal cascade: `8afe9f0`,
`583db04`, `e2689b6`, `7b0a60d`). This is a distilled behavioral spec mined from
that snapshot — a reference for rebuilding, never code to port. Read old files
with `git show ce0b751:<path>`.

An older stale copy also exists at
`/Users/kogaryu/draft/draftsman_STALE_PARENT_DO_NOT_USE` (read-only reference at
most; never build there).

## What the old app was

A multi-workspace shell, not just a drafting tool. Frameless window, activity
rail on the left switching workspaces, three resizable/collapsible panels
(left/right/bottom) with layout presets (`full`, `focus`, `review`, `tiny`),
status bar. Workspaces: `drawing_tool`, `text_editor`, `review`/`ui_taxonomy`,
`map_editor` (CSV dungeon-map token editor), `blender_recipe_lab` (composed the
drawing + text workspaces), `settings`, `blank`. Driven by project profiles
(`data/project_profiles/*.json` — schema worth re-expressing in TOML).

## Drawing tool: lost vs. leapfrogged

The current C++ rebuild is NOT strictly behind the old version. Ground truth:

**Lost in the port (the old tool had these; current does not):**
- **Zoom**: Ctrl/Cmd+scroll, exponential (`pow(1.0015, delta)`), zooms at cursor.
- **Pan**: plain scroll or middle-mouse drag (current gesture state machine still
  carries an unused `Panning` mode from this).
- **Undo/redo**: `drawingCanUndoCommand`/`drawingCanRedoCommand` existed with
  Ctrl+Z / Ctrl+Y bindings.
- **Keyboard interaction**: Esc cancel pending shape/gesture; Del/Backspace
  delete; Ctrl+D duplicate; Ctrl+C/V copy/paste; arrow-key nudge in three step
  modes (grid / Alt=1px fine / Shift=4× large).
- **Arc tool**: circle/arc tool with start/end angle params (defaults 15°/120°).
- **Regular polygon tool**: sides (default 6) + rotation (default 30°);
  triangle/hex/free variants.
- **Tool variants**: line → straight/polyline/arrow; rectangle → box/rounded/frame.
- **Object metadata**: role (wall/floor/cutout/collider), material, export_group,
  tags, with a presets system.
- **Rulers**: top + left rulers with grid-step major/minor marks and labels.
- **Aspect-lock** toggle for rectangle-like objects.
- **Reference frames**: image reference frame and ASCII crop frame tools.

**Leapfrogged (current has these; the old version did not):**
guides (old had none — snap substituted), dimensions (old `measure_inspect`
was disabled), construction lines, offset/mirror/repeat (old `offset_trim`/
`mirror_array` were disabled), align/distribute, layers with pen/stroke plot
styles, the entire plot pipeline (ordering, travel stats, warnings, bounds
safety, calibration), and the test suite (51 targets).

**Never built in either version (old roadmap, still open):**
spline curves, hatch boundary fill, SVG import/fit, trace markup, plotter
device output (HPGL/G-code).

## Text editor (user's learning project — capability map)

Multi-document tabs (pinned ordering), roles (prompt/context/reference/scratch/
output), find & replace (case-insensitive, prev/next, replace one/all, Ctrl+F,
Esc), split pane with independent document choice, word wrap + line numbers
toggles, line/col + char/word count + read-time stats, per-document cursor and
selection persistence, save / save-all / export-bundle through a store.
Persisted as `data/text_editor/documents.json` (rebuild: MessagePack manifest).

## Settings (becomes the TOML schema)

- Theme: mode (dark/light/system), palette (base/surface/accent/text, #RRGGBB),
  UI + code font names and sizes (9–28px), live preview, reset.
- Panels: visibility per panel, sizes, layout presets, right-inspector section
  toggles (facts/selection/code_refs/notes/receipts/actions); persisted shell
  layout (was `data/shell_layout.json` → rebuild as TOML).
- Project profiles: default activity, panel defaults, data sources, write
  policy, custom actions.

## Rebuild order (evidence-based)

1. Save/open drawing documents — MessagePack via existing `src/formats/` codecs.
2. Undo/redo — command stack on the existing `DraftingCommand` variant.
3. Zoom/pan + keyboard map — restore the old semantics above; gesture state
   machine already models panning.
4. Arc + regular polygon tools — parameters and defaults above; current model
   already renders polygons.
5. Plotter output (HPGL or G-code) — the plot job already produces ordered
   stroke/travel segments.
6. Shell layout + theme settings in TOML.
