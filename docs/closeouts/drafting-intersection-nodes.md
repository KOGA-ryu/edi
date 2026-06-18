# Closeout — materialize intersections + Node snap (M2)

> Freezes the M2 boundary.

- **Boundary**: turning the intersection SNAP into materialized, snappable Point nodes.
- **Department**: edi-drafting
- **Campaign**: drafting-20260617-batch2 (M2)
- **Date**: 2026-06-17

## The decision
- **Op** (`src/drafting/DraftingIntersectionOps.{h,cpp}`): `documentIntersectionPoints` (crossings
  of the document's VISIBLE straight `Line`/`ConstructionLine` segments via `segmentIntersection`,
  deduped by a relative epsilon), `subsetIntersectionPoints` (selection-scoped), and
  `filterExistingIntersections` (drops crossings that already have a Point within tolerance).
- **Verb** `dropIntersectionPoints()`: materializes a `Point` object at each (filtered) crossing —
  over the selection if any, else the whole visible document — in ONE `CreateObjectsCommand` (via
  the existing `createObjectsAndSelect`), auto-selecting the new points, one undo. **Idempotent**:
  re-running adds nothing (existing points within tolerance are filtered).
- **Node snap source**: `DraftingSnapSourceKind::Node` + `nodeEnabled` (default true) makes
  standalone `Point` objects (incl. the dropped intersection nodes) snappable; controller
  getter/setter via the `setSnapFlag` member-pointer helper; projection key `node_enabled`.

## Why (not to re-argue)
- **Node precedence:** Node is emitted BEFORE Endpoint in the PointGeometry arm so Endpoint
  shadows it under the `nearestObjectSnap` `<=` last-equal-distance-wins rule — Node surfaces only
  when `endpointEnabled` is off. (Same precedence discipline as DR-02's OnCurve fallback tier; do
  NOT "fix" it into Node-competes-on-distance without a deliberate decision.)
- **Idempotent by filtering existing points**, not by suppressing the command — so a real second
  set of crossings (after new geometry) still drops.
- Scope = `Line`/`ConstructionLine` straight segments; polyline-edge / wall-centerline crossings
  are a documented FUTURE extension.

## Out of scope / deferred
- **Snap-settings TOML persistence** — confirmed NONE of the per-source snap flags persist to
  `edi.toml` today (endpoint/midpoint/intersection/quadrant/…/node). Adding `node` alone would be
  inconsistent; snap-flag persistence is its own slice. Deferred.
- The **Node snap inspector checkbox** — edi-ui dependency.
- Polyline/wall-segment intersections — future extension.

## Pointers
- Code: `src/drafting/DraftingIntersectionOps.{h,cpp}`, `DraftingSnap.{h,cpp}` (`Node`),
  `src/core/DrawingDocumentController.cpp` (`dropIntersectionPoints`, `setNodeSnapEnabled`),
  `DrawingDocumentProjection.cpp` (`node_enabled`).
- Tests: `tests/drafting_intersection_points_tests.cpp`, Node block in `drafting_snap_tests.cpp`,
  controller block in `drawing_document_controller_tests.cpp`.
- Handoff: `docs/handoffs/drafting-20260617-batch2.md`.
