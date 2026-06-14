# Seams — what the dungeon-map roadmap plugs into

The companion to `docs/dungeon-map-roadmap.md`. That doc says *what* to build; this
one maps the **existing, already-in-use extension points** each phase reuses, so the
work is "add an arm / append a row / follow the exporter pattern," not new
architecture. Locations are current as of master `6398c2e`.

Almost everything below is **reuse**. The genuinely new seam the map work needs is a
block/instance system (§M) — and even that has a close existing relative.

---

## A. The new-primitive recipe — `[Wall M1.1, Room M2.1]`

A wall and an explicit room are new `DraftingGeometry` kinds. The full procedure is
already written up in **`docs/adding-a-drafting-tool.md`** (the ~22-site checklist)
and **`docs/tool-change-log.md`** (five worked examples incl. the spline). Don't
re-derive it — the geometry-visitor sites are compiler-guarded, so a miss is a named
build error. The variant + its visit sites:

- `DraftingGeometry` variant + `static_assert(variant_size == N)` — `src/drafting/DraftingTypes.h:269`
- `geometryKind` — `DraftingGeometry.cpp:306` · `validateGeometry` — `:471` ·
  `computeBounds` — `:536` · `translateGeometry` — `:608` · `handleAnchors` — `:825`
  (each ends in an `always_false_v` guard that names a missed arm)
- hit (`DraftingHitTest.cpp`), snap (`DraftingSnap.cpp`), numeric edit, plot, serialize,
  projection, painter, tool creation, belt — per the recipe doc.

**Wall wrinkle:** unlike past primitives, a wall has a *thickness* and renders as a
**band**, not a hairline — so its painter arm is new work (a thick stroked/filled
band with mitered joins), and `computeBounds`/`hitDistance` must account for the
half-thickness. Closest precedent for "shape that paints specially": the rectangle
(`drawRectangleShape`) and the ellipse polygon.

## B. Object metadata flags — `[Wall game flags M1.3, Door flags M2.2]`

Per-object semantic flags that are *not* geometry ride `ObjectMetadata`
(`DraftingTypes.h:160`). The exact precedent is **`LineVisualMetadata`**
(`:155`, the `startArrow`/`endArrow` double-arrow flags). A wall's
`blocksMovement / blocksSight / blocksLight / isDoor / isSecret` is the same move: a
small flag struct on the metadata, surfaced as projection keys → read by the painter
→ toggled by inspector controls. (Change-log "Double-arrow" entry is the worked
example: ~10 sites, only the tool-name switch compiler-guarded.)

## C. Geometry math primitives — `[Corner join M1.2, Room area M2.1, LoS export M4]`

Pure, reusable, already promoted to the shared home (`DraftingGeometry.h`):

- `segmentIntersection` / `lineIntersection` — `:67` (built for trim/fillet; the wall
  miter and any wall-graph face-finding reuse them)
- `area(geometry)` — `:39` (room auto-area, free)
- `sampleArc` / `sampleEllipse` / `sampleSpline`, `boundsFromPoints` — flatten curves
  to points for bounds/hit/paint/export.

## D. The snap engine — `[Walls share corners M1.2]`

Walls join at shared endpoints, which the snap engine already makes happen:
`snapCandidatesForObject` / `snapCandidatesForDocument` / `resolveSnap`
(`DraftingSnap.h:65-69`). **Intersection snap** (this session) and endpoint snap are
live. A wall arm in `snapCandidatesForObject` (its two ends) is the only add, and it
mirrors the line arm.

## E. Commands + atomic multi-object edits — `[Create wall, room+walls, doors]`

The mutation seam: `DraftingCommand` variant + `applyDraftingCommand`
(`DraftingCommands.h:146,183`), with `CreateObjectCommand` / `CreateObjectsCommand`
(atomic batch) / `UpdateGeometryCommand`. The controller wraps multi-object edits in
one undo step with `beginEdit()` … `commitEdit()` and `createObjectsAndSelect`
(`DrawingCore.h:329`) — exactly what the **fillet** verb used to trim two lines + add
an arc atomically. A door (open a wall + place a marker) and a generated room (walls +
floor) use this same bracket.

## F. Controller orchestration + gestures — `[Draw walls, place doors]`

The resolve→plan→apply→emit flow lives in `DrawingDocumentController`. A wall is a
two-click tool (reuse the `m_pendingCreation` two-click path in `clickCanvasNormalized`
+ `creationRequest` + `isTwoClickCreationTool`); a multi-segment wall run reuses the
**multi-click** path (polyline/spline). Placement (doors, generation centres) reuses
the **pick-a-point** capture: `PointCaptureIntent` (`DrawingCore.h:38`),
`resolvePointCapture` (`:346`) — add an intent, dispatch in the switch. Whole-geometry
edits ride `applyActiveObjectGeometryUpdate` / `applyCommandAndEmit`.

## G. Projection → painter pipeline — `[Render walls/doors/rooms]`

One-way: document → model projection → scene items → paint.

- `draftingObjectToCanvasProjection` — `DrawingDocumentProjection.cpp:445` (emits the
  `points` / segment keys + style); `numericFieldsForObject` — `:118`;
  `projectedObjectStyle` (stroke/fill).
- `buildCanvasSceneItem` — `DrawingCanvasObjectPainter.cpp:162` (extract) ·
  `drawSceneItem` — `:204` (draw). The wall band + door symbol are new arms on **both**
  sides (the recurring "extract and draw are two halves" trap — change-log friction #3).

## H. Layers — `[Construction-vs-final polish, asset layering]`

`DraftingLayer` (`DraftingDocument.h:27`) already carries visibility/lock/order;
`applyLayerFlagsUpdate` (`DrawingCore.h:317`) is the kind-and-callable mutator. A
"construction" vs "final" layer, or VTT-style asset/wall/light layers, is layer data
+ existing layer ops — no new mechanism.

## I. Fill / region — `[Colour rooms]`

`FillStyle` + `UpdateFillStyleCommand` + the fill arm in `projectedObjectStyle` + the
painter's gated brush (this session's "fill the #0 gap"). A room polygon fills today.
*Gap:* bucket-fill an enclosed region without tracing it (roadmap cross-cutting polish).

## J. Grid + physical units — `[5 ft / square scale M4.1]`

`projectDraftingGrid` (`DraftingGrid.h:70`) maps canvas units → a physical board;
`physicalGeometryForObject` (`DrawingDocumentProjection.cpp:295`) and
`MeasurementMetadata` already express real measurements. "5 ft per square" is a grid
+ measurement config, not new code — the dimension tools already read it.

## K. Plot / flatten pipeline — `[Geometry → segments for export & LoS M4]`

`buildDraftingPlotPlan` + `appendVertexSegments` (`DraftingPlotPlan.h:113`) flatten
*any* geometry into ordered segments (built for the plotter). A UVTT exporter needs
exactly this: walls (and any curved boundary) reduced to line segments for
line-of-sight. Reuse the flattener; don't re-walk geometry.

## L. The export seam — `[UVTT export M4.2]`

Exporters are an established pattern: `exportSvgDocument` / `exportHpglDocument`
(`DrawingCore.h:58-59`) build text from the document (`DrawingSvgExport.cpp`) and write
it through `DrawingDocumentStore::exportText` (`DrawingDocumentStore.h:18`). A **UVTT
exporter is a new exporter on this exact pattern** — build the `.uvtt` payload, write
it through the store. `[DECISION]` UVTT is JSON; per the no-JSON rule, the JSON-writing
lives **only here**, isolated like SVG, never in the internal model.

## M. Block / instance — `[Block library M3]` — the one genuinely-new seam

There is no block/symbol system today. The **closest existing relatives** to build it
on:

- **Copy/paste:** `m_clipboard` (`DrawingCore.h:373`) + `planDraftingPaste` (pure id-mint
  + offset) + `CreateObjectsCommand` — a block "instance" is paste-with-a-transform.
- **Array-from-active:** `createArrayFromActiveObject` + `radialArrayDraftingObject`
  (`DraftingArray.h:60`) — already mints N transformed copies of a source object
  atomically; a block is the same "one source → placed copies," named and saved.

So a block definition = a saved named group; an instance = a transformed placement via
the paste/array machinery + `createObjectsAndSelect`. The new part is *persisting the
definition* and *the library/palette UI* — the placement math already exists.

## N. Save / load — `[Everything persists free]`

New geometry kinds + metadata flags ride the existing MessagePack serializer
(`DraftingSerialize` + `DrawingDocumentStore` save/open) — adding the wall's serialize
arm is one of the recipe sites in §A, and the `.edidraw` round-trip then just works.

---

## Phase → seam map

| Roadmap milestone | Seams it plugs into |
|---|---|
| **M1.1 Wall primitive** | A (recipe, +band painter) · C (math) · G (projection/painter) · N (serialize) |
| **M1.2 Corner auto-join** | C (`segmentIntersection`) · D (endpoint snap) · G (painter miter) |
| **M1.3 Wall game flags** | B (metadata flags) · G (projection keys) + inspector |
| **M2.1 Room object** | A (recipe or polygon reuse) · C (`area`) · I (fill) · B (label) |
| **M2.2 Doors as openings** | B (door flags) · E (atomic open-wall+marker) · F (pick-a-point place) · G |
| **M2.3 Auto-rooms (later)** | C (face-finding on the wall graph) · E |
| **M3 Block library** | **M (new)**, built on copy/paste + array + `createObjectsAndSelect` · N |
| **M4.1 Map semantic layer** | B (read flags) · J (5 ft grid) · H (layers) |
| **M4.2 UVTT export** | L (exporter pattern) · K (flatten to segments) · `[DECISION: JSON here only]` |
| **M5 Generation** | E (emit objects atomically) · the Wall/Room/Block kinds as the output vocabulary |
| Polish: bucket fill | I · C |
| Polish: delete-all-construction-lines | H + a `deleteAll*` verb (mirror `deleteAllGuides`) |

**Bottom line:** the dependency spine (Wall → Rooms/Doors → Blocks → Export →
Generation) lands on seams that already exist and are exercised daily by the drafting
tools. The wall is a recipe build with a new band painter; the rest is arms, flags,
an exporter, and one new (but well-precedented) block system.
