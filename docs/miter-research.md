# Miter joints for the polyline sweep — P5 research

**Status:** research deliverable for BATCH-2 P5 prep (2026-06-17)
**Author:** edi-blender-lab-researcher
**Current source:** `tools/blender/edi_craft.py:959–1046` (`_swept_prism_world`, BL-08 v1).

---

## ⚠ Golden flag — read first

The committed `samples/swept_profile/swept_profile_ops_compiled.toml` has a path with a
**90° corner**: `(0,0) → (4,0) → (4,3)`. The corner vertex at `(4,0)` is an interior path
point. P5's miter implementation WILL change the 3D vertex positions at that corner. **The
builder MUST regenerate `samples/swept_profile/swept_profile.obj` as part of P5.** The start
and end loops are unaffected (single-segment frames, see §3). No other sample has a swept
profile path with corners.

---

## The problem with BL-08 v1

`_swept_prism_world` (edi_craft.py:1019–1034) assigns each path point a **tangent = the
direction of the outgoing segment** (or the incoming segment at the last point). At an interior
corner the tangent jumps discontinuously from the incoming segment direction to the outgoing
segment direction — the footprint loop placed at the corner is oriented for the outgoing segment
only, so the edge quads connecting the two adjacent loops diverge/overlap on the "turn" side of
the tube. The BL-08 audit noted this as deferred item #6: *"Sweep miter at path corners — v1 is
straight-segment (sharp corners self-intersect)"*
(`docs/closeouts/blender-lab-feature-batch.md`, item 6).

The fix: at each **interior** path point, use the **bisector frame** (the average direction of
the two adjacent segments) and **scale the cross-section** in the in-plane direction to
compensate for the oblique cut. That is the miter joint.

---

## 1  Miter geometry at an interior corner

### Setup

At path vertex `k` (where `0 < k < m−1`):

```
t_in  = normalize(path[k]   − path[k−1])   # unit vector along incoming segment
t_out = normalize(path[k+1] − path[k]  )   # unit vector along outgoing segment
```

### The bisector (miter) tangent

The miter plane bisects the angle between the two adjacent segments. Its normal in the 2D
path plane is the **normalized sum** of the two unit segment directions:

```
b     = t_in + t_out           # (NOT yet normalised)
bl    = |b|  = hypot(b.x, b.y)
t_miter = b / bl               # the "corner tangent" that defines the loop's orientation
```

Why the sum? Each unit tangent points "forward" along its segment. Their sum bisects the angle
between them — the same way that the angle bisector in a triangle bisects the angle between two
sides. The resulting direction is perpendicular to the miter plane in the XY path plane.

### The loop's local frame at the corner

BL-08's convention (edi_craft.py:1026–1027): the footprint's local x maps to the
**in-plane normal** to the path direction (perpendicular to the tangent, in the XY plane) and
local y maps to **world Z** (up, height of the cross-section).

Applying the same convention to `t_miter`:

```
nx, ny = −t_miter.y, t_miter.x   # miter in-plane normal (= "sideways" at the corner)
```

World Z stays world Z. The footprint vertex `(fx, fy)` (where `fx` = sideways offset in the
profile and `fy` = profile height above base) is then placed at:

```
world = (ox + px + fx·nx, oy + py + fx·ny, base_z + fy)
```

with the miter scale applied to `fx` (see §2).

---

## 2  Width compensation — the miter scale factor

### Why scaling is needed

The miter plane is tilted with respect to both adjacent segment directions. A loop of width `W`
placed in the tilted miter plane, projected onto a plane perpendicular to the incoming segment,
appears **narrower** by `cos(α/2)` where `α` is the turn angle between t_in and t_out. To
preserve the correct cross-section width of the swept tube, the footprint's in-plane (fx)
component must be **pre-scaled** by the inverse:

```
miter_scale = 1 / cos(α/2)
```

### Derivation of the formula

The turn angle α is the angle between t_in and t_out: `cos(α) = dot(t_in, t_out)`.  
From the half-angle identity: `cos²(α/2) = (1 + cos(α)) / 2`.

Therefore: `cos(α/2) = |t_in + t_out| / 2 = bl / 2`

So: `miter_scale = 1 / cos(α/2) = 2 / bl`

This is the elegant form — no trig, no arccos. Just divide 2 by the magnitude of the
non-normalised bisector:

```python
miter_scale = 2.0 / bl    # at interior path point k
```

### Which axis the scale applies to

The scale applies to **`fx` only** (the footprint's horizontal component, i.e., the dimension
that is in the XY path plane, perpendicular to the path). The **`fy`** component (the profile's
height in world Z) is NOT affected by a horizontal path corner — the tube's Z-height is always
vertical regardless of the XY turn.

Practical implementation: scale the `(nx, ny)` normal by `miter_scale` before placing the
loop vertex:

```python
nx_eff = nx * miter_scale   # only at interior points; endpoints: miter_scale = 1.0
ny_eff = ny * miter_scale
# then the existing vertex formula is unchanged:
loops.append((ox + px + fx * nx_eff, oy + py + fx * ny_eff, base_z + fy))
```

If tapering is active, the tapered `fx` is used (as today); the miter scale is applied ON TOP
of the taper scale, which is correct — the miter is about the cross-section's XY width after
whatever taper has been applied, not before.

### Concrete values at common angles

| Turn angle α | Interior SVG angle θ = π−α | `miter_scale` | Comment |
|---|---|---|---|
| 0° (straight) | 180° | 1.000 | Identity — byte-identical to v1 |
| 45° | 135° | 1.082 | Gentle curve, negligible |
| 90° | 90° | 1.414 (= √2) | Typical right-angle cornice corner |
| 120° | 60° | 2.000 | Getting sharp |
| 150° | 30° | 3.864 | Very sharp |
| 151° | 29° | **4.000** | SVG `stroke-miterlimit` standard threshold |
| 180° (hairpin) | 0° | ∞ | Undefined — bl = 0, refuse in validate |

### The failure mode as α → 180°

At a near-hairpin corner (`t_out ≈ −t_in`), `bl = |t_in + t_out| → 0` and
`miter_scale → ∞`. The loop expands to infinite width in the XY plane. This is the same
singularity that SVG's `stroke-miterlimit` addresses — the miter "spike" grows without bound at
very acute angles. See §4 for the guard.

**Established prior art — SVG `stroke-miterlimit`:** The formula `miterLength / stroke-width =
1 / sin(θ/2)` (equivalently `1/cos(α/2)` in our notation, since `sin(θ/2) = cos(α/2)` for
θ = π−α) is the W3C SVG specification for miter join geometry. The default limit is 4.0;
a value of 4 switches to bevel at θ < 29° (α > 151°). The formula is referenced in
[MDN: stroke-miterlimit](https://developer.mozilla.org/en-US/docs/Web/SVG/Reference/Attribute/stroke-miterlimit)
and the [SVG strokes specification](https://svgwg.org/specs/strokes/).

---

## 3  Endpoints — unchanged

At the first path point (`k = 0`) and the last (`k = m−1`), only **one** adjacent segment
exists. There is no "second segment" from which to form a bisector. These points use the
**single-segment perpendicular frame** — exactly as v1 does today:

- `k = 0`: tangent = outgoing direction `t_out = normalize(path[1] − path[0])`; `miter_scale = 1.0`
- `k = m−1`: tangent = incoming direction `t_in = normalize(path[m−1] − path[m−2])`; `miter_scale = 1.0`

These are the start cap and end cap rings. No change from v1.

**Byte-identical invariant for straight paths:** On a straight path (all collinear points),
every interior point has `t_in = t_out`, so `b = 2·t_in`, `bl = 2`, `t_miter = t_in`,
`miter_scale = 1.0`, and `(nx_eff, ny_eff) = (nx, ny)`. The v2 output is bit-for-bit identical
to v1 for any straight path. Existing golden files with straight paths are untouched.

---

## 4  Degenerate / sharp corners

### The guard recommendation: REFUSE in validate, clamp in Python

**Recommendation: refuse in C++ validate** (not clamp). Here is why:

- A miter scale of 4.0 or more means the cross-section expands by ≥4× in the XY plane through
  the corner — clearly an undesirable distortion, not a "close enough" shape.
- A clamped output would be silently wrong: the tube's width at the clamped corner would not
  match the specified cross-section and the surfaces on either side of the corner would no longer
  meet flush (the fix creates a different, visible artifact).
- Refusing in validate gives the user a named error they can act on (widen the corner or increase
  the path radius), consistent with the lab's approach to `prism_inset_too_large` and
  `prism_inset_reflex_pinch`.

**Python defense-in-depth:** Add a Python clamp `miter_scale = min(miter_scale, 10.0)` for the
case where `_swept_prism_world` is called without validate (direct Python invocation, tests).
This prevents infinite coordinates while keeping the behavior obviously wrong (the author sees
the artifact and fixes the path).

### Threshold

Align with the SVG/CSS `stroke-miterlimit` standard: refuse when **`miter_scale > 4.0`**,
i.e., when `bl < 0.5` (since `miter_scale = 2.0/bl`).

This corresponds to a turn angle of **α > 151°** between consecutive path segment tangents — a
very tight corner that is architecturally implausible for a moulding run.

**C++ validate implementation** (in the `AddPrismOp` arm, guarded by `!op.path.empty()`):

```cpp
// P5: miter-joint validate — refuse corners too sharp for a clean miter.
// SVG stroke-miterlimit standard: refuse when miter_scale = 2/|t_in+t_out| > 4
// (turn angle > 151°). Condition: |t_in+t_out| < 0.5.
for (size_t k = 1; k + 1 < op.path.size(); ++k) {
    double inx = op.path[k].x - op.path[k-1].x;
    double iny = op.path[k].y - op.path[k-1].y;
    double outx = op.path[k+1].x - op.path[k].x;
    double outy = op.path[k+1].y - op.path[k].y;
    double lin = std::hypot(inx, iny), lout = std::hypot(outx, outy);
    if (lin < 1e-9 || lout < 1e-9) continue;  // degenerate/duplicate point; dup filter handles
    inx /= lin; iny /= lin; outx /= lout; outy /= lout;
    double bl = std::hypot(inx + outx, iny + outy);
    if (bl < 0.5) {  // miter_scale > 4.0
        add(findings, Severity::Error, "prism_sweep_corner_too_sharp",
            op.name + " sweep path has a corner sharper than ~151° (miter factor >4).");
        break;  // one error per op is sufficient
    }
}
```

Error key: **`prism_sweep_corner_too_sharp`**.

---

## 5  Concrete recommendation for P5

### Replace the per-point tangent block in `_swept_prism_world` (edi_craft.py:1021–1027)

**Builder steps (number-by-number, modify only the framing block):**

**Step 1.** Before the loop at line 1018, pre-compute **unit segment tangents** for all
m−1 segments:

```python
segs = []
for k in range(m - 1):
    dx, dy = path[k+1][0] - path[k][0], path[k+1][1] - path[k][1]
    L = math.hypot(dx, dy) or 1.0
    segs.append((dx/L, dy/L))
```

**Step 2.** Inside the `for k in range(m):` loop (currently lines 1020–1027), replace the
tangent-computation block with:

```python
# BL-08 v1: outgoing/incoming segment tangent (no miter)
# P5 v2: bisector miter frame at interior points; endpoints unchanged.
if k == 0:
    tx, ty = segs[0]
    miter_scale = 1.0
elif k == m - 1:
    tx, ty = segs[-1]
    miter_scale = 1.0
else:
    tin_x,  tin_y  = segs[k-1]
    tout_x, tout_y = segs[k]
    bx, by = tin_x + tout_x, tin_y + tout_y
    bl = math.hypot(bx, by)
    if bl < 1e-6:           # hairpin guard (validate should have refused first)
        tx, ty = tout_x, tout_y
        miter_scale = 1.0
    else:
        tx, ty = bx / bl, by / bl
        miter_scale = min(2.0 / bl, 10.0)   # Python safety clamp; validate is the real guard
nx_eff = (-ty) * miter_scale   # in-plane normal, pre-scaled
ny_eff =   tx  * miter_scale
```

**Step 3.** In the vertex placement (currently line 1034), replace `fx * nx, fx * ny` with
`fx * nx_eff, fx * ny_eff`:

```python
loops.append((ox + px + fx * nx_eff, oy + py + fx * ny_eff, base_z + fy))
```

*(The taper block lines 1031–1033 are untouched; taper modifies `fx, fy` in place and the miter
scale is applied via `nx_eff` / `ny_eff` transparently.)*

**Step 4.** In `RecipeOpsValidate.cpp`, in the `AddPrismOp` arm, add the
`prism_sweep_corner_too_sharp` check (C++ code in §4 above) after the existing inset guards.

**Step 5.** Regenerate the golden: run
```
python3 tools/blender/edi_craft.py --obj-out=samples/swept_profile/swept_profile.obj \
    samples/swept_profile/swept_profile_ops_compiled.toml
```
and update `samples/swept_profile/swept_profile.obj` with the new output (the 90° corner
vertex loop will have `miter_scale = √2`, so its XY ring will be ~41% wider at the corner,
which is geometrically correct for a clean 90° mitered tube).

**Step 6.** The cross-language smoke test (`tests/edi_craft_smoke.py`) covers the swept_profile
golden — it will now compare against the new golden, so the test stays green once the golden is
updated.

### What stays byte-identical

- All paths with **no corners** (single-segment or all-collinear multi-segment paths) → every
  interior point has `t_in = t_out`, `bl = 2`, `miter_scale = 1.0` → identical to v1.
- The **taper logic** (`tapering`, `sx`, `sy`, `taper_curve`) is unchanged and orthogonal.
- The **inset** (`_inset_polygon`) is unchanged and orthogonal.
- The **`normal_offset`** post-pass (`_offset_along_normals`) is unchanged.
- The **doric_column** sample uses a straight path → untouched.

---

## Sources

- [MDN: stroke-miterlimit](https://developer.mozilla.org/en-US/docs/Web/SVG/Reference/Attribute/stroke-miterlimit)
  — the canonical formula `miterLength/strokeWidth = 1/sin(θ/2)` and the miter-limit
  standard (default 4.0 = bevel at < ~29° interior angle / > ~151° turning angle).
- [SVG Strokes specification (W3C)](https://svgwg.org/specs/strokes/)
  — the normative reference for miter join geometry.
- [CSS-Tricks: Mastering SVG's stroke-miterlimit](https://css-tricks.com/mastering-svgs-stroke-miterlimit-attribute/)
  — the table of limit → angle correspondences (1.414 ↔ 90°; 4.0 ↔ ~29°).
- [Clipper2 ClipperOffset — MiterLimit](http://www.angusj.com/delphi/clipper/documentation/Docs/Units/ClipperLib/Classes/ClipperOffset/Properties/MiterLimit.htm)
  — Clipper2's polygon-offset miter limit; same mathematical concept applied to 2D polygon
  offsetting (the threshold is `temp_lim_ = 2/miterLimit²`; a miter limit of 2 is the
  minimum = sqrt(2) scale factor = 90° corner).
- [Houdini Sweep SOP docs](https://www.sidefx.com/docs/houdini/nodes/sop/sweep.html)
  — production sweep node; illustrates that frame construction at path points is the core
  design decision (tangent frame vs. rotation-minimizing frame vs. miter frame).
- [RMF-engine proposal (GregStanton/GitHub)](https://github.com/GregStanton/proposal-rmf-engine)
  — discusses the miter-bisector approach in the context of composable sweep geometries
  ("a mitered join scales the profile in the bisector plane").
- **Current source files:**
  - `tools/blender/edi_craft.py:959–1046` — `_swept_prism_world` v1 (the function being
    modified; the tangent-assignment block at lines 1021–1027 is the exact patch site).
  - `tools/blender/edi_craft.py:981–986` — path dedup (the consecutive-duplicate filter
    that precedes the loop; ensures `segs[k]` never has zero length).
  - `samples/swept_profile/swept_profile_ops_compiled.toml:15–22` — the path `(0,0)→(4,0)→(4,3)`
    confirming the golden has a 90° corner.
  - `docs/closeouts/blender-lab-feature-batch.md` item 6 — the deferred BL-08 miter note.
