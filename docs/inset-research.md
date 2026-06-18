# Robust polygon inset for non-convex loops — P6 research

**Status:** research deliverable for BATCH-2 P6 prep (2026-06-17)
**Author:** edi-blender-lab-researcher
**References current source:** `tools/blender/edi_craft.py:847` (`_inset_polygon`) and
`src/recipe/RecipeOpsValidate.cpp:256–276` (`prism_inset_too_large`).

---

## The problem in one paragraph

The v1 `_inset_polygon` (edi_craft.py:847–888) is a per-vertex **bisector-amplification**
offset: it sums the two edge normals at each corner, normalises to a bisector direction, then
scales by `inset / cos_half` so that the perpendicular distance from each offset edge to its
original edge equals `inset`. This is exact for convex corners. At a **reflex vertex** (interior
angle > π, i.e. a "dent" in the polygon) the two inward edge-normals point partly against each
other; their sum shrinks toward zero as the reflex deepens, driving `cos_half → 0` and
`d = inset / cos_half → ∞`. The `max(1e-6, cos_half)` clamp caps the denominator, which means
the vertex is flung an enormous distance — creating a self-intersecting or inverted loop. The
C++ validate guard (`prism_inset_too_large`, line 273–275) refuses `inset ≥ 0.5 × min_bbox`
as a blunt backstop, but this is too coarse: it passes many reflex loops that will still produce
bad geometry, and rejects some convex footprints unnecessarily.

---

## 1  The straight-skeleton approach

### What it is

The **straight skeleton** of a polygon is the trace left by each vertex as all edges move inward
at unit speed, each remaining parallel to its original direction. When two vertices collide (an
**edge event**), the connecting edge collapses and the two surviving vertices merge. When a
reflex vertex's bisector ray strikes a non-adjacent edge (a **split event**), that edge
bifurcates and the original polygon divides into two independent sub-polygons, each of which
continues shrinking on its own. The full skeleton is a planar graph of these traces; at any depth
`d`, reading the "wavefront at time d" from the skeleton gives the exact inset polygon (or
polygons) at that depth.

This is the mathematically correct inset for arbitrary simple polygons: every edge of the output
is at perpendicular distance exactly `inset` from its source edge, corners are mitered (not
rounded), and reflex corners are handled by topology change rather than vertex blowup.
([Wikipedia — Straight skeleton](https://en.wikipedia.org/wiki/Straight_skeleton);
[Felkel & Obdržálek 1998 conference paper via Semantic Scholar](https://www.semanticscholar.org/paper/Straight-Skeleton-Implementation-Felkel-Alek/52a87233694137341e1d553644415898a298b069))

### Implementation complexity

The classic Felkel–Obdržálek O(n r) algorithm (where r = number of reflex vertices) is the
standard teaching reference, but:

- It is **provably incorrect** for some degenerate inputs (three events at the same time, or
  certain coincident bisectors). [polyskel](https://github.com/Botffy/polyskel), the main pure-
  Python implementation, documents this in its own README: *"the algorithm is fairly dated, and
  shown to be incorrect for certain input polygons … This implementation is a bit crap, and does
  not really attempt to fix the algorithm."* The authors point to Stefan Huber's doctoral thesis
  for the modern treatment.
- Correct algorithms use a **priority queue** of events; detecting split events requires
  intersecting each reflex bisector ray against all non-adjacent edges — O(n) per reflex vertex,
  O(nr) total. With multiple simultaneous events the code grows to hundreds of careful lines.
- CGAL ships a robust C++ straight-skeleton package; there is no equivalent dependency-free Python
  package that is known-correct.
  ([CGAL Straight Skeleton 2D manual](https://doc.cgal.org/latest/Straight_skeleton_2/index.html))

### Verdict for P6

**Do not build a from-scratch straight skeleton for the proof tier.** The implementation is
large, the correctness traps are real (polyskel demonstrates this), and the proof tier's bar is
lower than a production skeleton: we need a non-degenerate, non-self-intersecting footprint for
*reasonable* non-convex inputs, not topological decomposition into sub-polygons at large inset
depths. The skeleton is the right long-term tool if the lab ever needs deep insets that split the
footprint; for P6, a simpler approach with an honest refusal rule is the right call.

---

## 2  Simpler robust alternatives

### 2a  Edge-offset-then-intersect (recommended core)

**Algorithm:**

1. For each directed edge `A → B`, compute the unit inward normal `n` (for a CCW polygon,
   `n = (-(By−Ay)/|AB|, (Bx−Ax)/|AB|)`).
2. Build the **offset line** for that edge: the line through `A + inset·n` with direction
   `(B−A)/|AB|`. This is the original edge translated perpendicularly inward by exactly `inset`.
3. For each vertex `i`, find the intersection of `offset_line[i-1]` and `offset_line[i]`. That
   intersection is the new vertex `V'[i]`. (This is a 2×2 linear system; if the two lines are
   parallel, the two edges are collinear and the intermediate vertex is degenerate — collapse it.)

**How it handles a reflex corner:** At a reflex vertex with interior angle θ > π, the two
adjacent offset lines still intersect; the intersection is geometrically on the interior side of
the original polygon. There is no `1/cos_half` amplification, so the formula is numerically
stable for all angles. A 350° reflex vertex produces an offset vertex far into the polygon
interior — correct in the sense that the perpendicular distances are satisfied — but the resulting
polygon may then self-intersect with a non-adjacent edge if `inset` is large relative to the
local feature size. That is caught by the post-hoc check (§2b).

**What it gets wrong:** It cannot *split* the polygon at a deep reflex vertex (only the straight
skeleton does that). If the inset is large enough that the reflex vertex's new position would
cross another edge, the post-hoc check refuses rather than producing two sub-polygons. For the
proof tier this is acceptable.

**Cost:** O(n) to compute offset lines and intersections; the post-hoc self-intersection check
is O(n²) in n polygon edges, but recipe footprints are small (typically 4–16 vertices) so this
is negligible.

**Prior art:** This is exactly Phase 1 of Clipper2's offset engine: *"creates parallel edges of
every edge in the input … computes unit normal vectors perpendicular to path segments and applies
join logic at vertices."* Clipper2 then runs a union operation (Phase 2) to resolve the
self-intersections it creates for concave joins. We skip Phase 2 and refuse instead.
([Clipper2 — Offsetting Operations overview](https://deepwiki.com/AngusJohnson/Clipper2/5-offsetting-operations);
[Clipper2 GitHub](https://github.com/AngusJohnson/Clipper2))

### 2b  Post-hoc winding/area sanity pass

After computing `V'`, check the signed area (shoelace formula) of the offset polygon:

- If `sign(area(V')) ≠ sign(area(V))`, the polygon **inverted** (wound the other way) — the
  inset collapsed the loop. Return None.
- If `|area(V')| < ε`, the polygon degenerated to a point or line. Return None.

The shoelace signed area is O(n) and zero-dependency.
([Shoelace formula — Wikipedia](https://en.wikipedia.org/wiki/Shoelace_formula))

### 2c  O(n²) edge-crossing check on the offset polygon

After computing V', test every non-adjacent edge pair of the offset polygon for proper
intersection (a standard 2D segment test using cross products). If any pair crosses, the polygon
is self-intersecting — the inset was too deep for this footprint. Return None.

For n ≤ 20, O(n²) is ≈ 200 tests, each 5–10 floating-point ops. Unnoticeable at runtime.

Clipper2's analogous approach (for full resolution, not just detection) is the
"Polygon Offsetting by Computing Winding Numbers" strategy (Chen & McMains, ASME DETC2005-85513;
cited in [Clipper2 overview](https://www.angusj.com/clipper2/Docs/Overview.htm)).

### 2d  Per-vertex feature-size cap (NOT recommended for P6)

One could pre-compute the maximum safe inset at each vertex — the bisector amplification factor
— and cap `inset` to the minimum. For convex vertices this is unlimited; for a reflex vertex
with interior angle θ, the amplification `k = 1/|sin((2π−θ)/2)|` grows as θ→2π. Capping by
`k` per vertex avoids blowup at the cost of applying a *different* inset depth at each corner,
which is conceptually wrong (the output edges are no longer uniformly offset). This distorts the
footprint without producing a geometrically meaningful result. Reject this option.

---

## 3  What "good enough for the proof tier" means here

The OBJ proof exists to show **shape** — that the recipe produces a plausible 3D object at the
right proportions. The final solid is built by bpy, which has its own robust geometry kernel.
Therefore:

- A v2 `_inset_polygon` that produces a geometrically clean (non-self-intersecting,
  non-inverted) inset polygon for *reasonable* non-convex footprints is the correct target.
- For inputs where the inset would self-intersect or invert, the right response is **refusal by
  name in C++ validate** (before the recipe reaches Python) plus **None-return + warning** in
  Python (as a defense in depth if the C++ guard is too conservative).
- We do NOT need to produce sub-polygons, handle split events, or resolve self-intersections
  (straight-skeleton territory). The proof tier's job is "good mesh or clear failure."
- The C++ validate refusal fires before Blender is involved; the Python check is a safety net for
  inputs that pass C++ but turn out to be marginal at the actual inset depth.

This matches the existing architecture: `prism_inset_too_large` is a C++ pre-check, and
`_inset_polygon` is Python execution. We improve both layers.

---

## 4  Concrete recommendation for P6

### Python: replace `_inset_polygon` (edi_craft.py:847)

Implement **edge-offset-then-intersect + area-flip check + edge-crossing check** as a drop-in
replacement. Return `None` on failure; callers (`proof_prism` at line ≈940 and the sweep
cross-section at line ≈977) fall back to the un-inset polygon and emit a warning comment in
the OBJ (`# WARNING: inset rejected — self-intersection or inversion`).

**Builder steps:**

1. **Signed area / winding** (identical to v1 lines 856–861; reuse).
2. **Build offset lines:** for each edge `i`, point `P_i = pts[i] + inset * n_i`, direction
   `d_i = normalize(pts[(i+1)%n] - pts[i])`. The inward normal `n_i = (-d_i.y, d_i.x)` for
   CCW, `(d_i.y, -d_i.x)` for CW. Store as `(Px, Py, dx, dy)`.
3. **Intersect adjacent offset lines** for each vertex: solve the 2×2 system
   `P_prev + t * d_prev = P_curr + s * d_curr`. Cross both sides with `d_curr` (which kills
   the `s` term) → the determinant is the 2D cross product
   `denom = d_prev × d_curr = d_prev.x * d_curr.y - d_prev.y * d_curr.x`.
   *(Corrected 2026-06-17, P6 audit: an earlier draft wrote `d_prev.x*(-d_curr.y) -
   d_prev.y*(-d_curr.x)` = `-(d_prev × d_curr)` — a sign flip that places the vertex on the
   OUTWARD side. The form above is the verified-correct one the P6 implementation uses.)*  
   - If `|denom| < 1e-9`: edges are parallel (collinear input vertices); use the midpoint of the
     two line anchor points as a fallback vertex (edge effectively collapses here).
   - Otherwise: `t = ((P_curr - P_prev) × d_curr) / denom`; new vertex =
     `P_prev + t * d_prev`.
4. **Area flip check:** compute shoelace signed area of offset result. If sign flipped or
   `|new_area| < 1e-9`, return `None`.
5. **Edge-crossing check:** for all pairs `(i, j)` with `j ≥ i+2` and `(i,j) ≠ (0, n-1)`,
   test whether segments `V'[i]→V'[i+1]` and `V'[j]→V'[j+1]` properly intersect (orient test
   using cross products). Return `None` on first hit.
6. Return offset result.

This is ~60 lines of dependency-free Python, well within a Sonnet builder's scope.

### C++ validate: add `prism_inset_reflex_pinch` (RecipeOpsValidate.cpp:258–277)

Keep the existing `prism_inset_too_large` bbox guard (it catches global collapse). Add a
**per-reflex-vertex pinch guard** as a second, tighter check:

1. Compute signed area of footprint (same shoelace loop; O(n)).
   Determine winding: `ccw = (area > 0)`.
2. For each vertex `i`, compute the cross product of the two adjacent edge vectors:
   `cross = (pts[i] - pts[i-1]) × (pts[i+1] - pts[i])`  (z-component only).
   A vertex is **reflex** when `(ccw && cross < 0) || (!ccw && cross > 0)`.
3. For each reflex vertex, compute the adjacent edge lengths `l1` and `l2`.
   The **pinch limit** (conservative): at a reflex vertex, the edge-offset intersection
   travels farther into the polygon interior than `inset`. The minimum safe bound is:
   `inset < 0.5 * min(l1, l2)`.
   (If this is violated, the offset vertex can cross through the far endpoint of the shorter
   adjacent edge, guaranteeing a self-intersection in the output.)
4. If any reflex vertex violates this, emit:
   ```
   add(findings, Severity::Error, "prism_inset_reflex_pinch",
       op.name + " inset is too large for a reflex vertex in its footprint.");
   ```

**Refusal rule summary:**

| Check | Key | Condition |
|---|---|---|
| Global collapse guard (existing) | `prism_inset_too_large` | `inset ≥ 0.5 × min(bbox_w, bbox_h)` |
| Reflex pinch guard (new P6) | `prism_inset_reflex_pinch` | for any reflex vertex: `inset ≥ 0.5 × min(l1, l2)` |

Both checks are O(n) in the number of footprint vertices.

The Python `_inset_polygon_v2` forms the **defense-in-depth** tier: inputs that pass both C++
checks are still computed correctly (edge-offset-then-intersect has no `1/cos_half` blowup) and
the area-flip + edge-crossing checks catch any remaining marginal cases at runtime, returning
`None` so the proof falls back gracefully rather than silently producing bad geometry.

---

## 5  Source references

**Current code being replaced:**

- `tools/blender/edi_craft.py:847–888` — `_inset_polygon` v1 (bisector-amplification; the
  `cos_half` instability is at line 885–887).
- `tools/blender/edi_craft.py:940` — first call site (prism footprint).
- `tools/blender/edi_craft.py:977` — second call site (sweep cross-section).
- `src/recipe/RecipeOpsValidate.cpp:256–277` — the `prism_inset_too_large` guard (bbox-based).
- `src/recipe/RecipeOpsValidate.cpp:259–263` — the comment already names "a real straight-
  skeleton inset" as the future fix; P6 delivers the next-best thing.

**External citations:**

- Straight skeleton definition, edge/split events, time complexity:
  [Wikipedia — Straight skeleton](https://en.wikipedia.org/wiki/Straight_skeleton)
- Felkel & Obdržálek algorithm + known incorrectness:
  [Semantic Scholar — Straight Skeleton Implementation](https://www.semanticscholar.org/paper/Straight-Skeleton-Implementation-Felkel-Alek/52a87233694137341e1d553644415898a298b069)
- polyskel (the main Python implementation; candidly crap for edge cases):
  [github.com/Botffy/polyskel](https://github.com/Botffy/polyskel)
- CGAL robust straight skeleton (reference, not a dependency):
  [CGAL Straight Skeleton 2D User Manual](https://doc.cgal.org/latest/Straight_skeleton_2/index.html)
- Clipper2 offset strategy (edge-offset + union; confirms the edge-offset approach):
  [Clipper2 — Offsetting Operations (DeepWiki)](https://deepwiki.com/AngusJohnson/Clipper2/5-offsetting-operations)
  [Clipper2 GitHub](https://github.com/AngusJohnson/Clipper2)
- Winding-number / signed-area approach to offset polygon cleanup:
  Chen & McMains, "Polygon Offsetting by Computing Winding Numbers," ASME DETC2005-85513
  (cited in [Clipper2 overview](https://www.angusj.com/clipper2/Docs/Overview.htm))
- Self-intersection removal post-offset:
  [quadst.rip — Polygon Self-Intersection Removal](https://quadst.rip/poly-isect)
- Shoelace signed-area formula (area-flip check):
  [Wikipedia — Shoelace formula](https://en.wikipedia.org/wiki/Shoelace_formula)
- Survey of polygon offsetting strategies (edge-shift + self-intersection taxonomy):
  [fcacciola.50webs.com — A Survey of Polygon Offsetting Strategies](http://fcacciola.50webs.com/Offseting%20Methods.htm)
