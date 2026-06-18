# Closeout — blender-lab-20260617-batch2-polish

> Freezes the recipe-lab POLISH batch (the 8 deferred items from
> `blender-lab-feature-batch.md` + the roadmap-depth gaps). Complete 2026-06-17 on
> `dept/blender-lab`. Handoff: `docs/handoffs/blender-lab-20260617-batch2-polish.md`.
> Run AUTONOMOUSLY (user call): the planner ran the queue ahead, gated through the
> reviewer at checkpoints, dept-cycled workers at ticks, and bussed each green tip to
> edi-ui (the merge owner). Rebased only onto LOCAL master throughout.

## What shipped (all verified; the 8 deferred + 3 roadmap-depth + 2 surfaced follow-ups)

| Item | What | Gate |
|---|---|---|
| P1 | sweep×screw silent-override validate-WARNING + edge-incidence `assert_manifold` in the smoke | spot-check |
| P2 | boolean proof: suppress consumed-operand standalone OBJ emission (mirror bpy) | reviewer-rec, spot-check |
| P3 | **watertight helix** — closed the open thread ribbon into a 2-manifold solid | Opus audit (12-config manifold) |
| P3b | orient BOTH axis-spine closures (partial-revolve + helix) + `assert_oriented` | spot-check |
| P4 | non-linear taper `taperCurve` (`t**curve` remap) | spot-check + checkpoint audit |
| P4b | per-axis taper `taperEndY` (0-sentinel = follow) | spot-check + checkpoint audit |
| RD1 | **ScriptOp ASCII bbox** placeholder (roadmap M1) — Script no longer invisible in ASCII | spot-check |
| RD2 | **recipe semantic-diff → TOON** (roadmap M6) `exportRecipeStreamDiffToToon` | spot-check + checkpoint audit |
| RD3 | **`docs/craftsmen-authoring.md`** teaching doc (roadmap M4) | researcher, docs |
| P6 | **robust non-convex inset** — edge-offset-then-intersect + refuse/fallback + `prism_inset_reflex_pinch` | Opus audit (formula re-derived) |
| P7 | `bounds_of` helix/sweep tightness (scan actual mesh verts) | spot-check |
| P5 | **sweep miter joints** — bisector frame + `2/|b|` width comp + `prism_sweep_corner_too_sharp` | Opus audit (miter math + golden) |

Research: `docs/inset-research.md` (P6), `docs/miter-research.md` (P5), `docs/craftsmen-authoring.md` (RD3).

## Frozen decisions / discipline

- **Every default byte-preserving:** new taper/inset/miter params default to identity; the
  4 OBJ goldens (doric, extruded_figure, boolean_op) stayed **byte-identical** the whole batch.
  The ONE deliberate golden change was `swept_profile.obj` (P5 miter at its 90° corner —
  flagged by the miter research ahead of time, reviewer-confirmed faithful: only the 4 corner
  verts moved, byte-reproduced).
- **Geometry is now hardened + asserted:** the partial-revolve + helix closures are watertight
  AND oriented, locked by `assert_manifold` + `assert_oriented` in the smoke. The non-convex
  inset refuses-or-falls-back (never emits a self-intersecting mesh). The sweep miters corners
  and refuses near-hairpins (SVG miterlimit=4).
- **AI handoff completed:** BL-15's TOON export now has a sibling semantic DIFF (RD2), both
  reusing the ONE `recipeOpsToConfig` flat-key source (TOML truth = TOON handoff = diff keys,
  no drift). Never JSON.
- **The fast-track pace was validated:** a retroactive Opus CHECKPOINT audit of the
  spot-checked-then-merged slices (P4/P4b/RD2) found them all clean — the green-gate +
  spot-check pace missed no latent bug.
- No new C++ files (CMakeLists is edi-ui's); no `LEDGER.md`/shell edits; no JSON; no
  `.js`/`.qml`/QtQml. All in existing `src/recipe` + `tools/blender` + `tests` + `docs`.

## Tracked follow-ups (NOT blocking — a future batch if wanted)

1. **Swept solid 2-manifold check** — P5's miter removes the corner overlap, so for non-hairpin
   paths the swept solid may now be 2-manifold; run an edge-incidence check on a gentle-corner
   sweep and, if it passes, un-exclude it from `assert_manifold`/`assert_oriented` (P5 audit rec).
2. **Partial-AND-helical sweep** — `screw_rise != 0` still ignores `sweep_degrees < 360` in v1
   (the P1 warning surfaces it; the combination itself is unbuilt).
3. **Per-axis taper CURVE** (taperCurveY) and **straight-skeleton inset** (the "correct" tool
   the P6 edge-offset v2 approximates) — both deliberately deferred.
4. The P6 `prism_inset_reflex_pinch` is a conservative O(n) guard, not a full self-intersection
   solver (the Python area-flip/edge-crossing is the runtime backstop).

## edi-ui dependencies (chrome — flagged, not ours)

The `--recipe-diff` CLI verb (RD2) joins the earlier `--recipe-toon` (BL-15) +
"Append/Export Ops Recipe" File-menu items + the profile/path/operand canvas-pickers — all
edi-ui's, wired from `docs/ui-surface/blender-lab/`. The new depth params surface
automatically in the Steps field editor (generic opField + bind).

## Not re-opened
No generation. CSG stays execution-only. The recipe lab's depth seam + craftsmen + composer
+ AI handoff are now feature-complete through the roadmap (M1–M6) with the geometry hardened.
