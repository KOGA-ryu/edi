# Closeout — dungeon-map feature batch (DM-01..15, the usable-map polish ring)

> Freezes the boundaries + decisions of the post-stop-line polish ring so future
> work does not re-litigate them.

- **Boundary**: the usable-map polish features (frame-on-load, interior features,
  neutral plug flags, Seam-C edited round-trip, region fill, map-browser content,
  per-instance block rotation/scale) and the dungeon-map↔edi-ui chrome split.
- **Department**: edi-dungeon-map
- **Campaign**: dungeon-map-20260617-feature-batch
- **Date**: 2026-06-17
- **Final tip**: `8144777` (handed to edi-ui to merge onto master `cf9e383`+).

## What shipped (all 15 DM tasks, neutral + tool-first)

| Task | Shipped | Commit (rebased) |
| --- | --- | --- |
| DM-02/03 | interior features: `RoomSpec.features` (room-local feet) → neutral Point markers (`feature:<type>` tag) | merged `c6e98e3` |
| DM-04/05/06 | neutral plug `flags` (open `vector<string>`): parse → additive persist → TOON column | merged `c6e98e3` |
| DM-07/08 | Seam C edited-doc room round-trip verified intact + regression pinned | merged `c6e98e3` |
| DM-09/10 | region fill: pure `planRegionFill` (`DraftingRegionFill`) + `RegionFill` capture verb → neutral filled Polygon | merged `9ebfb7f` |
| DM-12/13 | block per-instance rotation/scale: `BlockPlacementMetadata` fields + additive persist + TOON `blocks[]` | merged `9ebfb7f` |
| DM-14/15 | place rotated/scaled block + transform a placed instance (consume `transformGeometry`) | `797470d`/`6e591a1` |
| DM-01 | `documentObjectsBounds(doc)→optional<Bounds2D>` + `computeDocumentBounds()` getter | `5644633` |
| DM-11 | shared no-Qt `DraftingMapQuery` (`deriveEdge`+`plugIsConnected`); `MapToonExport` switched to it | `fdf4bc7` |
| (hardening) | composed-scale `+inf` guard in `transformBlockInstance` | `8144777` |

## Decisions frozen (do NOT re-argue)

1. **Region fill is DISTINCT from drafting's DR-15.** DR-15 recolors a SELECTED
   object's fill; DM-09/10 mints a NEW Polygon from a seed CLICK. **Algorithm (a)
   room-footprint lookup** for v1 (find the `DraftingMapRoom` whose footprint
   contains the seed). **(b) general wall-loop trace is PARKED** behind a
   forward-compatible signature (a future `walls` param). **(c) closed-object
   containment is REJECTED** — it would collide with DR-15. v1 fills the AUTHORED
   footprint (ignores wall half-thickness) — the accepted fidelity cut.
2. **Interior features are stored in AUTHORED FEET, room-local** (offset from the
   NW `origin`), while the rest of `RoomSpec` is pre-scaled to canvas at parse. This
   asymmetry is deliberate + commented (a room-local offset is most naturally raw
   feet, resolved at the mint as `origin + offset×scale`). Numerically verified.
3. **`plug.flags` is an OPEN, NEUTRAL `vector<string>`.** edi RECORDS, never
   interprets — no passable/weight/direction. Same for the feature `type` tag and
   the room `material`. Rules live downstream of Seam B.
4. **Block per-instance transform:** identity placement is byte-identical via a
   SHORT-CIRCUIT (because `transformGeometry` is not bit-exact at identity).
   `rotationDeg`/`scale` are additive-codec, emit-when-non-default, no bump. The
   transform pivots are the placement center (DM-14) and the group union-bounds
   center frozen pre-mutation (DM-15); the group transforms in one undo step.
   NaN/scale guards live in the setters, the verb input, AND the composed result
   (`8144777`) — **the additive codec never persists a non-finite value**.
5. **`transformGeometry` is drafting-owned** (consumed, never modified). **Known
   LOSSY v1 limit:** a rotated non-circular Ellipse drops axis tilt, rotated Text
   drops baseline angle, Guide=identity (the contract). A block of those shapes
   won't preserve tilt under rotation — accepted, not a bug.
6. **The chrome split (DM-01/11/14/15 surfaces):** dungeon-map owns the
   controller verbs + pure plans + the no-Qt query/bounds helpers; **edi-ui owns
   the widgets/panels/canvas wiring** (the spins, `fillRegionButton`, the
   inspector "Block instance" section, `buildMapBrowserPanel` content,
   `computeFitView`) and the `map`-workspace golden re-bless. `DraftingMapQuery.h`
   is the SHARED include that keeps the browser and the TOON export from drifting.

## New wholly-ours files
`src/drafting/DraftingRegionFill.{h,cpp}` (DM-09), `src/drafting/DraftingMapQuery.{h,cpp}`
(DM-11). Both pure free functions over plain structs, registered in `CMakeLists.txt`
(additive, edi-ui-blessed).

## Carried notes (non-blocking, recorded for posterity)
- The additive codecs' **old-binary-reads-new-file** corner is guaranteed by the
  unknown-key-ignore contract, not an in-tree fixture (true for flags / block
  rotation/scale — consistent across batch-1/4/14-15 audits).
- DM-14/15 tests use **single-object blocks**; the multi-member shared-pivot path is
  correct by inspection but unexercised by a ≥2-object group (low-value coverage gap).
- 360° rotation is treated as non-identity (non-canonicalized angle; harmless).

## Out of scope / explicitly NOT allowed
- **No generation** (WFC/BSP/procedural) — the tool-first stop-line holds; edi is
  the authoring bench, rules + generation are downstream of Seam B.
- **`transformGeometry` is NOT ours to change** (drafting-owned shared primitive).
- **Region-fill algorithm (b)** (general wall-loop trace) stays parked.
- **The chrome** (widgets/panels/canvas/golden) is edi-ui's — dungeon-map hands
  controller/helper slivers, never edits the shell.

## Integration story (the process that worked)
Merge-often via edi-ui: small verified batches rebased onto master and handed to
edi-ui to merge (the 86-commit divergence scare became routine clean rebases; two
additive controller conflicts with DR-08/DR-09 resolved by keeping both). **LEDGER
policy:** the shared `docs/handoffs/LEDGER.md` is edi-ui-owned on master;
departments track in dept-local handoffs + bus-hub reports (this killed the
recurring rebase conflict).

## Pointers
- Handoff (full gate log + task board): `docs/handoffs/dungeon-map-20260617-feature-batch.md`
- Surface spec: `~/edi/docs/ui-surface/dungeon-map/DM-surfaces.md`
- Charter: `docs/departments/edi-dungeon-map.md` · Arch: `docs/architecture/edi-dungeon-map.md`
- Prior closeout (the H2 boundary this built on): `docs/closeouts/h2-src-drafting-map-boundary.md`
