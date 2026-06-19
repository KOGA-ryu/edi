# Campaign: drafting-20260618-phase2-ops (spatial-model inversion — Phase 2)

**My role escalates: REVIEW/CONSULT (Phase 1) → BUILD (Phase 2).** I OWN + build the drafting-core ops
slices; dungeon-map LEADS sequencing (requests each slice with its data-model context); edi-ui is the SOLE
integrator (merges to master); the HUB relays dept-commit → edi-ui. I build on `dept/drafting`, green-gate,
and the slice goes to edi-ui via the hub relay. Source: `~/dept-bus/SPATIAL-MODEL-BACKLOG.md` PHASE 2 —
EXECUTION. Phase 1 data-spine COMPLETE on master (dec48db); my worktree rebased onto it (tip b90fd78).

## My Phase-2 BUILD targets (dungeon-map sequences them)
1. **derive-span-footprint** — P2-B, CRITICAL PATH, dungeon-map requests FIRST (gates the generator).
   STANDBY for the request (it brings the exact span-room struct + sizing contract).
2. **planDraftingRoom honors per-edge wall presence** — skip absent edges (no WallGeometry + skip corner-join),
   per-edge thickness/material fallback, zero-wall room legal. All-walls rooms byte-identical.
3. **empty-batch wall-less-room fix** — `createObjectsAndSelect` (src/core) must accept a zero-object room
   (paired with #2). Keep the genuine-empty no-op guard.
4. **overlap consumers** — `findOverlappingRoomPairs` (reuse footprintsOverlap + overlapRect, per-level,
   deterministic) + `mergeFootprints` MERGE polygon geometry (decision 4: polygon real / bbox-union first).

## derive-span-footprint — design sketch (settle exact API with dungeon-map on request)
- PURE op in `src/drafting` (new `DraftingSpan.{h,cpp}` or beside DraftingRoom), Qt-free, free function.
- "2-node widened-edge FIRST" (decision 2): the span between node A.anchor and node B.anchor is realized as a
  RECTANGLE — the A→B segment widened perpendicular by a band half-width → a big+wide room footprint.
- INPUTS: `Point2D a, Point2D b` (the two connected DraftingNode anchors) + sizing DATA — at minimum a band
  WIDTH (the "wide"), and likely an end-extension/pad (the "big") past each node. EVERY dimension is DATA
  (no-hardcoded-dims rule): the width/pad come from a spec field or a NAMED default constant, NOT a literal.
- OUTPUT: a footprint. For an axis-aligned A→B the natural output is an AABB (origin NW + width + height) so it
  drops into the existing DraftingMapRoom rectangle fields; a diagonal span needs either a rotated rect or the
  optional polygon ring (coordinate with dungeon-map — 2-node axis-aligned is the FIRST cut; polygon is the
  fast-follow). Likely signature: `Rect deriveSpanFootprint(Point2D a, Point2D b, double bandWidth, double endPad)`
  or returning the 4 corners; CONFIRM shape with dungeon-map (do they want AABB-only first, or polygon-ready?).
- TESTS: horizontal span, vertical span, A/B order independence, band width honored, end-pad honored,
  degenerate (a==b) handling, and (if polygon) corner winding. Determinism.
- OPEN Qs for dungeon-map's request: (a) AABB-first or polygon-from-the-start? (b) where does bandWidth/endPad
  come from — a new RoomSpec/span field, or a named default I own? (c) does the op take node ids + the doc, or
  just the two anchors (I prefer just anchors — keeps it a pure geometry primitive, the caller resolves ids)?

## Status
- Phase 1 review COMPLETE: 5 slices all CONSULT CLEAN (syncGraph, level, RoomDerivation, DraftingNode,
  footprintsOverlap+OverlapPolicy). See drafting-20260618-phase1-review.md.
- Phase 2: STANDING BY for dungeon-map's derive-span-footprint request. Will settle the API (open Qs above)
  then brief edi-drafting-builder. No autonomous start before the request (dungeon-map sequences).
- Carryover: 050/051 (kCanvasBoardExtent) merged to master as part of Phase-1 integration.
