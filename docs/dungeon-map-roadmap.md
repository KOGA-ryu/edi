# Roadmap — edi as a dungeon / encounter-map authoring tool

The build plan for turning edi's drafting core into a CAD-grade authoring tool for
tabletop (D&D-type) **dungeon and encounter maps**. Grounded in the verified
2026-06-13 research (`/deep-research`, 24/25 claims confirmed): pro-CAD and the VTT
world converge on one model — **a map is a graph of wall segments carrying
per-segment data, plus rooms derived from those walls.** The wall is the keystone;
everything hangs off it.

Sources behind this plan: Vectorworks (Spaces, automatic wall joins), IFC
`IfcRelSpaceBoundary`, ArchiCAD GDL doors/windows, AutoCAD blocks, the Dungeondraft
custom-assets wiki, Foundry VTT walls, the Universal VTT export format, and
mxgmn/WaveFunctionCollapse.

**Reading the tags:** `[REUSE]` = stands on something already shipped ·
`[NEW]` = net-new code · `[DECISION]` = a fork that's the user's call ·
each milestone is **independently usable** — you can stop after any phase and have
gained something.

---

## Phase 0 — Foundation (already shipped)

A dungeon map *is* a drafting drawing, so most of the substrate exists. This phase
is "nothing to build — here's what we stand on":

- Grid + rulers + zoom; layers; save/open (`.edidraw`); SVG/HPGL export.
- Snapping — endpoint, **intersection**, midpoint, center, vertex, grid, guide.
- Arrays (linear / grid / radial), mirror, offset, align, distribute, nudge.
- Primitives — line, polyline, spline, rectangle, circle, arc, polygon, text.
- Modify verbs — trim, fillet; the pick-a-point interaction; fill (color+opacity).

The gap the research names is small: **a wall+room model and a symbol library.**

---

## Phase 1 — The Wall (the keystone) ★ start here

The one missing CAD primitive every map feature stands on, and the thing that makes
a map *game-ready* rather than just a drawing. In edi it is two patterns already
proven twice: a new primitive (the `docs/adding-a-drafting-tool.md` Shape-A recipe)
plus a metadata-flag set (the double-arrow move).

- **M1.1 — `WallGeometry` primitive** `[NEW]` `[medium]` — a centerline segment +
  thickness, through the full new-primitive recipe (variant arm, validate, bounds,
  hit, snap, translate, handles, serialize, projection, painter, tool, belt row).
  Rendered as a thickened band, not a hairline. *This is the big slice; the rest
  of Phase 1 is small.*
- **M1.2 — Corner auto-join / miter** `[NEW]` `[small]` — walls sharing an endpoint
  render as one clean joined band (miter at the corner, square at a free end).
  Endpoint + intersection snap (shipped) already make segments share corners; this
  is the render/projection side.
- **M1.3 — Wall game-semantic flags** `[NEW]` `[small]` — per-wall metadata:
  `blocksMovement`, `blocksSight`, `blocksLight`, `isDoor`, `isSecret`. The
  double-arrow `LineVisualMetadata` pattern + inspector toggles. This is the data
  that separates a *map* from a *drawing*. (Research note: keep this a free set of
  flags — the specific "six named Foundry wall types" claim was *refuted*; don't
  hardcode an enum of types.)

**After Phase 1:** edi draws real floor-plan walls that join cleanly and carry
blocking semantics. It already feels like a dungeon tool.

---

## Phase 2 — Rooms & Openings

- **M2.1 — Room / space object (explicit v1)** `[REUSE+NEW]` `[small]` — a room is a
  closed polygon (have it) + auto-computed **area** (edi already computes area) + a
  **label**. `[DECISION]` Start *explicit* (you draw the room boundary). The pro
  model *derives* the room from enclosing walls via planar face-finding — a real,
  later, deeper slice (M2.3). Explicit first.
- **M2.2 — Doors / windows as wall-hosted openings** `[NEW]` `[medium]` — an opening
  is *hosted in* a wall (a gap in the band + a door marker), per the research, not a
  free-floating object. Depends on M1. Doors also feed the export's "portals".
- **M2.3 — Auto-derive rooms from the wall graph** `[NEW]` `[large]` *(later)* —
  planar face-finding over the wall segment graph → rooms appear automatically with
  area + label, IFC-style. Defer until the explicit path proves the workflow.

---

## Phase 3 — The Symbol / Block library (your "flash sheet")

The research's block model is exactly the reusable-motif library you described
(furniture, doors, monster tokens — and your sacred-geometry tiles).

- **M3.1 — Block definition** `[NEW]` `[medium]` — save a selection/group as a named
  block. (This is the `docs/drafting-gaps.md` "grouping/blocks/symbols" gap.)
- **M3.2 — Block instances** `[NEW]` `[medium]` — place/stamp instances (position,
  rotation, scale) referencing one definition, with per-instance overrides
  (AutoCAD block def+reference model).
- **M3.3 — Library palette + Tag/Set taxonomy** `[NEW]` `[medium]` — a browsable
  palette of blocks categorized by tag/set (doors · furniture · monsters · tiles).
  This is the "throw down patterns fast like a flash sheet" payoff.

---

## Phase 4 — Game-ready export (the interop payoff)

What makes a map *usable* at the table: export to a virtual tabletop.

- **M4.1 — Map semantic layer** `[NEW]` `[small]` — grid scale (5 ft / square),
  walls → line-of-sight, doors → portals, optional lights. Mostly reading the flags
  M1.3/M2.2 already carry.
- **M4.2 — Universal VTT exporter (`.uvtt` / `.dd2vtt`)** `[NEW]` `[medium]`
  `[DECISION]` — a rendered PNG + walls/portals/lights as data, the format Foundry
  & Roll20 import. **The catch: UVTT is JSON, and edi forbids JSON.** Clean
  resolution: it's an *output artifact* like the existing SVG/HPGL exporters — JSON
  lives only in this exporter, never the internal model. But it *is* writing JSON,
  so it's a conscious call, not to be slipped in. This is the one place the no-JSON
  rule meets an external requirement.

**After Phase 4:** you author a dungeon in edi and open it in your VTT with working
walls and doors. That's the whole loop.

---

## Phase 5 — Generation (on top, later)

The unique value vs every other dungeon generator: edi's generator **emits editable
CAD geometry** (walls + rooms you can then trim/fillet/fill), not a baked tilemap.

- **M5.1 — Tile model** `[NEW]` `[medium]` — author tiles + adjacency rules; the
  vocabulary a generator composes (ties the sacred-geometry tile work to maps).
- **M5.2 — A generator that emits drafting geometry** `[NEW]` `[medium]` — start with
  the classic **BSP rooms-and-corridors** dungeon generator; output is walls + rooms
  you edit by hand. Simplest path that proves "generate → edit as one data."
- **M5.3 — WFC / tile-based generation** `[NEW]` `[large]` `[DECISION]` — Wave
  Function Collapse over the M5.1 tiles. *Needs its own focused research pass first*
  — the deep-research verifiers could not confirm WFC's discrete claims (it's
  algorithmic), though the sources (Gumin, model synthesis, "editable WFC") are
  solid.

---

## Cross-cutting polish (fold in opportunistically)

From the use→polish backlog (`edi-pattern-authoring-polish`), all of which also
serve map authoring:

- **Region / bucket fill** — colour an enclosed area without tracing a polygon
  (today fill needs a closed shape). High value once walls make enclosed regions.
- **Delete-all-construction-lines** — bulk-clear scaffolding (mirrors delete-all-guides).
- **Layer presets** — a "construction" layer vs a "final" layer, hide/show in one move.

---

## Sequencing & scope discipline

The dependency spine is **Wall → Rooms/Doors → Blocks → Export → Generation.** Build
it in that order; each phase is a usable milestone, not a prerequisite to value, so
the "bloat" risk is managed by *stopping when you have what you need* rather than
chasing every phase. The whole track reuses the proven drafting core — it adds
layers on the trunk, it does not start a new tree.

**Recommended first move: M1.1, the `WallGeometry` primitive.** It is a known-shape
build (the documented recipe), and it is the foundation literally everything else
stands on.
