# Closeout — drafting feature bucket DR-01..15 (the construction-kit verbs)

> Freezes the boundaries the bucket settled so future work does not re-litigate them.

- **Boundary**: the drafting construction-kit ops/verbs added in batch DR-01..15 — their
  representations, the contracts other departments consume, and what is deliberately OUT.
- **Department**: edi-drafting
- **Campaign**: drafting-20260617-feature-bucket
- **Date**: 2026-06-17

## What shipped (all green via `edi-gate`, behavior-preserving where not net-new)
- **DR-01 `transformGeometry`** (the keystone): rotate(deg, CCW) + uniform scale about a pivot
  over all 14 kinds; `mapPoint` centralizes trig. Rectangle is **center-anchored** (ruling A).
  Ellipse drops axis tilt, TextAnnotation drops baseline angle, Guide is identity — documented
  v1 limitations. dungeon-map CONSUMES it.
- **DR-02/03** snaps: `Quadrant`/`OnCurve` sources (OnCurve is a fallback tier — never outranks
  a keypoint); `Tangent`/`Perpendicular` relative snaps (`relativeSnapCandidatesForDocument`;
  perpendicular emits only when the foot is ON the segment).
- **DR-04** `pointAlongEntity`; **DR-05** `circleThroughThreePoints`/`TwoPoints`; **DR-06**
  `divideCirclePoints` + inscribe N-gon / `{n/k}` star (gcd(n,step)==1 ⇒ one closed path).
- **DR-07** chamfer, **DR-08** extend-to-boundary, **DR-09** break — controller verbs mirroring
  the fillet/trim atomic pattern (capture-ids-before-mutate → one beginEdit/commitEdit →
  finishEdit on rejection).
- **DR-10** rotate-copies rosette (first `transformGeometry` consumer; sibling intent) +
  **DR-10-fix** (setter stores faithfully, op rejects a degenerate near-zero total angle).
- **DR-11** kaleidoscope multi-axis mirror (compose `R(+θ)∘Mx∘R(−θ)`) + the canonical-mirror
  **rectangle `rotationDeg` reflection fix** (a pre-existing latent bug DR-11 surfaced).
- **DR-12** `arrayAlongCurve`; **DR-13** Angular `DimensionKind`; **DR-14** arc/radius/diameter/
  sweep dimension planners; **DR-15** fill-authoring fillable-kind gate.

## The settled boundaries (do NOT re-argue)
- **transformGeometry contract** — frozen in the hub contract + arch doc §7; Rectangle
  center-anchored; the Ellipse/Text lossy caveats ARE the contract (a future additive
  `rotationDeg` on those is the deferred lossless slice).
- **Canonical mirror reflects rectangle `rotationDeg → −r`** (`db71c1b`) — corrected; shared
  with dungeon-map (orientation-only; position unaffected).
- **Angular dimension = Candidate A**: `DimensionGeometry{a=vertex, b=ray1 tip (|b−a|=arc
  radius), offset=signed included angle°}` — NO new field; `offset` repurposed for the Angular
  kind only. Old readers degrade `angular→Distance` (additive tolerance; no version bump).
- **Fill authoring**: the fillable set is **Rectangle/Circle/Ellipse/Polygon** (frozen in
  `drafting-fill-side-channel.md`); `draftingShapeIsFillable` gates the fill setters in lockstep
  with `closedFillRing` + the painter.
- **Selection-after-edit is per-verb**: break keeps the original; chamfer/fillet select the new
  object. Rotate-copies allows center==source-centre (valid rosette).

## Out of scope / explicitly parked
- **True bucket-fill** (trace an empty region into a new closed path) — needs a boundary tracer.
- **Boolean path ops** (union/difference/intersection) — own batch, settled boundary first.
- **Apollonius solver, spline rebuild/fair, gumball/SmartTrack, tiled-clone P-group** — each its
  own slice.
- Angle **measurement-text** formatting — a follow-up (the angle VALUE is in `offset`).

## edi-ui SEAMS (drafting behavior in edi-ui's files / chrome — they land these)
- The **Angular-dimension canvas ARC painter** + a guard so the LINEAR-dim painter doesn't
  mis-draw `offset`-as-angle; optionally extend `isAngleField("offset")` so the inspector labels
  the Angular field as an angle. Must ride with the Angular tool wiring.
- **Tool/belt/tool-options chrome** for the new verbs (chamfer setback, rotate-copies/kaleidoscope
  angle+count, array-along-curve path-pick, the dimension tools, the fill colour/opacity picker).
  The ops + controller verbs are done; the surfaces are edi-ui's per `DR-surfaces.md`.

## Pointers
- Code: `src/drafting/Drafting{Geometry,Snap,Derived,Modify,Array,Mirror,DimensionOps,PlotPlan,
  Types}.*`, `src/core/DrawingDocumentController.cpp`, `DrawingDocumentProjection.cpp`.
- Handoff: `docs/handoffs/drafting-20260617-feature-bucket.md`. Map: `docs/architecture/edi-drafting.md`.
- Related closeouts: `drafting-fill-side-channel.md`, `drafting-cartography.md`; ruling
  `~/dept-bus/RULING-H2-src-drafting-boundary.md`; contract `~/dept-bus/edi-drafting/briefs/002-transformGeometry-contract.md`.
