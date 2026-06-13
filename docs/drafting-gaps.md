# Drafting & drawing tools — missing pieces (audit 2026-06-13)

Grounded in the tree at `115fc42` (master). Evidence is `file:line` or a commit
hash, so every claim is checkable. Ordered by leverage, not by area.

The surface is **broad and load-bearing** already — this note records the *edges*,
not a verdict on the whole. (What's solid is listed at the bottom.) `direction.md`
calls the drafting surface "done and earning"; that means the foundation holds, not
that it's exhaustive. These are the gaps it glosses.

---

## 0. Fill is a dormant data model — closed shapes can't be filled (highest leverage)

- **Exists:** `FillStyle { double opacity = 0.0; std::string color = "#ffffff"; }`
  — `src/drafting/DraftingTypes.h:113`, carried on the object and serialized.
- **Missing, all three layers:**
  - no controller setter (the setter family is `setSelectedObjectStroke*` /
    `…Locked` / `…Visible` / `…Role…`; there is **no** `setSelectedObjectFill*`),
  - no inspector UI in the Style group,
  - **not rendered** — the painter fills only the *point marker*
    (`DrawingCanvasObjectPainter.cpp:333`), and SVG export writes `fill="none"`
    explicitly (`DraftingSvgOut.cpp:79`).
- **Impact:** rectangle / circle / polygon / arc are **outline-only**. Solid and
  region fills are foundational for the art-tool half, and the data + serialization
  already exist — only render → setter → inspector are missing.

## 1. Primitives / geometry that don't exist

(The geometry variant is Point, Line, Rectangle, Circle, Arc, Polygon, Polyline,
Guide, ConstructionLine, Dimension — `DraftingTypes.h:229`.)

- **Ellipse / oval** — no kind. Common precision primitive.
- **Spline / Bézier curve** — no kind. Freeform curves; load-bearing for art.
- **Text / annotation object** — no `Text` geometry. The only text on a drawing is
  dimension labels and guide labels; you cannot place a free note, callout, or title.
- **Hatch / pattern fill** — no region-fill patterns (depends on #0 first).

## 2. Modify / editing verbs that don't exist

(Have: offset, mirror, array repeat/grid/radial, align, nudge, rotate/scale via
handles, clipboard.)

- **Trim / Extend** — trim a line to a boundary; extend to meet one. (Essentially
  absent — only incidental string matches in `src/drafting`.)
- **Fillet / Chamfer** — round or bevel the corner between two lines. (0 hits.)
- **Break / Split / Join / Explode** — split a line at a point; join collinear
  segments; explode a polyline into segments and back.

## 3. Styling exposure & line types

- **Line styles are `solid | dash | dot` only** (`StrokeStyle.lineStyle`,
  `DraftingTypes.h:100`). No dash-dot, center, phantom, or custom dash patterns —
  the "line types" requirement is at its minimum form.
- **Color is free-hex text only** (`styleColorField`,
  `DraftingFeaturePanels.cpp:708`) — no swatch / picker. The code itself flags this
  as "the art-tool door" (`:698`). Fine for precision; an art tool wants a picker.
- Stroke width is a free number (good); no named weights (acceptable).

## 4. Interaction modes

- **No "pick a point" capture mode.** The radial array core takes a `center`
  parameter (`radialArrayDraftingObject`, `DraftingArray.cpp:171`), but the UI can
  only pass the *drawable centre* — there is no click-capture to let the user choose
  a custom center. One general pick-a-point mode would unlock user-picked array
  centers, rotation pivots, snap-from references, etc.

## 5. Snapping coverage

- Snap sources are **Endpoint, Midpoint, Center, Grid** (`DraftingSnap.h:18`).
- **Missing the rest of the CAD set:** Intersection, Perpendicular, Tangent,
  Nearest / on-edge, Quadrant, Node / vertex. Precision drafting leans on these.

## 6. Dimensions

- Have: Distance, Width, Height, Radius, Diameter (`DraftingToolCreation.h:25-29`).
- **Missing:** Angular, Arc-length, Leader / callout, and ordinate / baseline /
  continued dimension chains. No per-dimension text override (only a `showLabel`
  bool, `DimensionVisualMetadata`, `DraftingTypes.h:138`).

## 7. Organization

- **No grouping / blocks / symbols.** An object carries an `exportGroup` *tag*
  (`ObjectMetadata.exportGroup`), but objects can't be grouped into a movable unit
  or reused as a block/symbol. (Lua is the planned parametric/library voice for a
  later phase; nothing today gives drawn objects reusable identity.)

---

## What's already solid (not gaps)

Zoom, pan, rulers; layers; the ten primitives plus guides and construction lines;
arrays (repeat / grid / radial); offset / mirror / align / nudge; per-object stroke
styling (color / width / opacity / line style); measurement + calibration; five
dimension kinds; clipboard / undo-redo (atomic, coalesced drags); locked / visible;
role / material / tags; SVG / HPGL / G-code export; the modular-panel inspector.

The drafting surface is genuinely strong. The gaps above are the difference between
"a capable exact-measurement canvas" and "a full draft *and* art toolset" — with
**fill (#0)** the one place a feature is already half-built in the data model and
just needs wiring out.
