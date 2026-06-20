#!/usr/bin/env python3
"""ctest smoke for the ART FORGE C0 — girih_tile (pure tier + PIL bake).

Asserts, OFFLINE:
  - tile_polys(defaults) returns the star with exactly 2*points boundary verts;
    _resolve_k(8,3) is coprime + in [2, points-1]; >= 3 distinct color groups.
  - REPRODUCIBLE (§6 #5): two tile_polys calls byte-identical; two bake_png
    calls produce byte-identical PNG bytes.
  - CRISPNESS / line width (§9 #4): lineWidth*pxPerCell >= 3.
  - The baked PNG is pxPerCell x pxPerCell with > 1 distinct color.

PIL is guarded: if Pillow is somehow absent, the pure tile_polys asserts still
run and the PNG ones SKIP with a clear message (so ctest stays green on a
PIL-less box). Pillow IS present here, so the PNG asserts run.

C1 adds the SEAMLESS gate (rubric #1, the top priority): the baked cell is a
true translational tile — column x=0 equals column x=W-1 and row y=0 equals row
y=H-1 EXACTLY (ε=0, because PIL's polygon fill is hard-edged / no AA on the
boundary). This is the load-bearing offline check; the 3x3 lay-up is the visual
proof the reviewer eyeballs.
"""

import math
import os
import sys
import tempfile

# Keep ctest from littering __pycache__ into the source tree.
sys.dont_write_bytecode = True

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
sys.path.insert(0, os.path.join(ROOT, "tools", "blender", "forge"))

import girih_tile  # noqa: E402


def main() -> int:
    defaults = {p["key"]: p["default"] for p in girih_tile.MANIFEST["params"]}
    assert defaults["points"] == 8 and defaults["skip"] == 3, "C0 default motif is 8/3"

    # --- {n/k} coprime guard (one owner: nfold_star._resolve_k) ----------------
    n, raw_k = defaults["points"], defaults["skip"]
    k = girih_tile._resolve_k(n, raw_k)
    assert k == 3, f"_resolve_k(8,3) should be 3, got {k}"
    assert math.gcd(k, n) == 1, f"k must be coprime with n: gcd({k},{n})={math.gcd(k,n)}"
    assert 2 <= k < n, f"k must be in [2, n-1], got {k}"

    # --- the star has exactly 2*points boundary verts (FIDELITY #2b) -----------
    polys = girih_tile.tile_polys(defaults)
    assert girih_tile.star_boundary_vertices(defaults) == 2 * n
    # There is exactly ONE full star (the §2.2 fix: a single complete star
    # CENTERED at the cell center, not clipped corner fragments), and it carries
    # the full 2n-vertex tip/valley ring.
    star_polys = [p for p, key in polys if len(p) == 2 * n]
    assert len(star_polys) == 1, \
        f"expected exactly ONE full star (centered), got {len(star_polys)}"
    ring = star_polys[0]
    assert len(ring) == 2 * n, f"star ring must have 2*points={2*n} verts, got {len(ring)}"

    # --- the star is CENTERED at the cell center (the FIDELITY fix) -------------
    cell = float(defaults["cell"])
    cx = sum(p[0] for p in ring) / len(ring)
    cy = sum(p[1] for p in ring) / len(ring)
    assert abs(cx - cell * 0.5) < 1e-9 and abs(cy - cell * 0.5) < 1e-9, \
        f"star centroid must be the cell center ({cell*0.5}), got ({cx},{cy})"

    # --- >= 3 distinct color groups (star / cross / ground) --------------------
    color_keys = {key for _, key in polys}
    assert len(color_keys) >= 3, f"need >= 3 color groups (star/cross/ground), got {color_keys}"

    # --- REPRODUCIBLE: two tile_polys calls byte-identical (§6 #5) -------------
    assert girih_tile.tile_polys(defaults) == girih_tile.tile_polys(defaults), \
        "tile_polys must be byte-identical across calls (no RNG)"

    # --- CRISPNESS / line width: lineWidth*pxPerCell >= 3 (§9 #4) --------------
    line_px = defaults["lineWidth"] * defaults["pxPerCell"]
    assert line_px >= 3, f"lineWidth*pxPerCell must be >= 3 px, got {line_px}"

    # --- PIL tier (guarded) ----------------------------------------------------
    try:
        from PIL import Image  # noqa: F401
    except ImportError:
        print("girih_tile_smoke: Pillow absent — pure tier passed, PNG asserts SKIPPED")
        return 0

    tmp_a = tempfile.mktemp(suffix=".png")
    tmp_b = tempfile.mktemp(suffix=".png")
    try:
        girih_tile.bake_png(defaults, tmp_a)
        girih_tile.bake_png(defaults, tmp_b)

        # Byte-identical PNGs (§6 #5 — the bake is a pure function of params).
        with open(tmp_a, "rb") as f:
            bytes_a = f.read()
        with open(tmp_b, "rb") as f:
            bytes_b = f.read()
        assert bytes_a == bytes_b, "two bake_png calls must produce byte-identical PNG bytes"

        # The PNG is pxPerCell x pxPerCell and has the motif (> 1 distinct color).
        im = Image.open(tmp_a).convert("RGB")
        px = defaults["pxPerCell"]
        assert im.size == (px, px), f"baked PNG must be {px}x{px}, got {im.size}"
        colors = im.getcolors(maxcolors=1 << 24)
        assert colors is not None and len(colors) > 1, \
            f"baked PNG must render the motif (> 1 distinct color), got {colors}"

        # --- SEAMLESS edge-match (C1 gate, rubric #1) — ε = 0 EXACT ------------
        # The cell is the square translational unit; framing [0,cell)x[0,cell)
        # (half-open: a point at x=cell lands on pixel pxPerCell, OFF the
        # pxPerCell-wide image, so no doubled seam column) makes opposite edges
        # pixel-equal by lattice-translation invariance. PIL fills are hard-edged
        # (no AA), so we claim 0, not a few/255.
        px = im.load()
        W, H = im.size
        assert (W, H) == (px_size := defaults["pxPerCell"], px_size), \
            f"seam check expects a square cell, got {im.size}"
        col_diff = max(
            max(abs(a - b) for a, b in zip(px[0, y], px[W - 1, y]))
            for y in range(H)
        )
        row_diff = max(
            max(abs(a - b) for a, b in zip(px[x, 0], px[x, H - 1]))
            for x in range(W)
        )
        assert col_diff == 0, f"SEAMLESS: col(x=0) must equal col(x=W-1) exactly, max|Δ|={col_diff}"
        assert row_diff == 0, f"SEAMLESS: row(y=0) must equal row(y=H-1) exactly, max|Δ|={row_diff}"

        # The edge_match helper (used by the re-bake CLI) must agree with the
        # direct pixel read — one owner for the seam math.
        em_col, em_row = girih_tile.edge_match(defaults)
        assert em_col == 0 and em_row == 0, \
            f"edge_match must report ε=0, got col={em_col} row={em_row}"
        assert (em_col, em_row) == (col_diff, row_diff), \
            "edge_match must agree with the direct PNG edge read"
    finally:
        for p in (tmp_a, tmp_b):
            if os.path.exists(p):
                os.remove(p)

    print("girih_tile_smoke: OK (pure tier + PIL bake)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
