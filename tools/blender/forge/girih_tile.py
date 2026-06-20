"""THE ART FORGE — C0: a girih star-and-cross TILING generator + a PIL bake.

This is craftsman-SHAPED (a MANIFEST + a pure generator + an effector) but it
lives in `forge/`, NOT `craftsmen/`, on purpose: `load_craftsmen` scans only
`craftsmen/` (non-recursive), so this file is never pulled into the
`--list-craftsmen` 3D palette. A texture is not a placeable mesh — keeping it
out of the scan is correct.

Three parts, the proven dual-tier split:
  - MANIFEST: id `girih_tile`, named params (no hardcoded dims; every length is
    a cell-relative fraction) + a small named COLOR PALETTE (appearance, exempt
    from the no-hardcoded-dims rule — the realizer's GREY_MATERIALS pattern).
  - tile_polys(params) -> [(polygon_2d, color_key)]: PURE Python, no bpy, no PIL.
    The deterministic list of filled colored polygons composing ONE square cell
    of the 8/3 star-and-cross. Closed-form, no RNG.
  - bake_png(params, out_path): the PURE PIL rasterizer — the boundary-decided
    (b2) effector. It CONSUMES tile_polys directly; there is NO second copy of
    the motif math here (the parity rule). Byte-deterministic.

Why pure PIL over a Cycles ortho-render (the §9 pin): the REPRODUCIBLE gate
demands a byte-identical PNG, which a GPU render cannot reliably deliver (tile
order, BVH float accumulation, OCIO drift). Pillow over a closed-form polygon
list is a pure function of the params — byte-deterministic, GPU-free, CI-runnable.

The {n/k} compass math (`_resolve_k` + the tip/valley ring) is REUSED from
nfold_star — one owner for the coprime guard, never forked.

Seamlessness (C1's gate) is CONSTRUCTED-FOR here (ONE full star at the cell
CENTER + quarter-crosses at the four corners so the square tiles by pure
translation) but NOT yet asserted — C0 only proves the bake exists and the motif
is right.
"""

import importlib.util
import math
import os
import sys

# --- Import the {n/k} math from nfold_star without forking _resolve_k ---------
# `from nfold_star import ...` won't resolve cross-dir, so mirror load_craftsmen's
# spec_from_file_location to load craftsmen/nfold_star.py as a module. One owner
# for the coprime guard: we call _resolve_k, never re-derive it.
_NFOLD_PATH = os.path.join(
    os.path.dirname(os.path.abspath(__file__)), "..", "craftsmen", "nfold_star.py"
)
_spec = importlib.util.spec_from_file_location("edi_forge_nfold_star", _NFOLD_PATH)
_nfold = importlib.util.module_from_spec(_spec)
_spec.loader.exec_module(_nfold)
_resolve_k = _nfold._resolve_k  # the SINGLE coprime/clamp guard


# --- The named color palette (appearance — exempt from no-hardcoded-dims) -----
# The canonical girih palette: gold star, azure cross, indigo ground, black line.
# Like the realizer's GREY_MATERIALS — color keys map to RGB here, the params
# carry the keys. RGB tuples, 0..255.
PALETTE = {
    "gold": (212, 175, 55),
    "azure": (46, 116, 181),
    "indigo": (40, 44, 92),
    "black": (16, 16, 20),
}


MANIFEST = {
    "id": "girih_tile",
    "label": "Girih Star-and-Cross Tile",
    "params": [
        {"key": "points", "label": "Star Points (n)", "type": "integer", "default": 8},
        {"key": "skip", "label": "Star Skip (k)", "type": "integer", "default": 3},
        {"key": "cell", "label": "Cell Size", "type": "number", "default": 1.0},
        {"key": "starRadius", "label": "Star Radius (frac)", "type": "number", "default": 0.47},
        {"key": "lineWidth", "label": "Line Width (frac)", "type": "number", "default": 0.02},
        {"key": "pxPerCell", "label": "Pixels / Cell", "type": "integer", "default": 1024},
        {"key": "repeat", "label": "Bake Repeats (N)", "type": "integer", "default": 1},
        {"key": "starColor", "label": "Star Color", "type": "material", "default": "gold"},
        {"key": "crossColor", "label": "Cross Color", "type": "material", "default": "azure"},
        {"key": "groundColor", "label": "Ground Color", "type": "material", "default": "indigo"},
        {"key": "lineColor", "label": "Line Color", "type": "material", "default": "black"},
    ],
}


def _defaults() -> dict:
    """The MANIFEST defaults as a plain param bag — the proof's canonical input."""
    return {p["key"]: p["default"] for p in MANIFEST["params"]}


def _star_ring(cx: float, cy: float, n: int, k: int, outer: float):
    """The {n/k} star outline: a 2n-vertex alternating tip/valley ring centered
    at (cx, cy). REUSES nfold_star's math exactly — tips at `outer`, valleys at
    the {n/k}-derived inner radius (eff_inner = outer / (k-1), clamped). The
    angular step is pi/n; tip at even j, valley at odd j. This is the same
    construction as nfold_star._local_mesh's outline, regenerated in 2D (no
    extrude) so the texture is a flat fill, not a prism."""
    # eff_inner mirrors nfold_star._local_mesh: the valley radius derived from k,
    # clamped strictly between a tiny floor and just under the tips so the star
    # is never collapsed and never self-crossing.
    eff_inner = outer / (k - 1)
    eff_inner = max(1e-3 * outer, min(eff_inner, outer * 0.95))
    ring = []
    for j in range(2 * n):
        radius = outer if j % 2 == 0 else eff_inner
        angle = math.pi * j / n
        ring.append((cx + math.cos(angle) * radius, cy + math.sin(angle) * radius))
    return ring


def tile_polys(params: dict):
    """PURE: the deterministic list of (polygon_2d, color_key) composing ONE
    square cell [0,cell] x [0,cell] of the 8/k star-and-cross.

    Layout (the §2.2 construction — lattice-periodic, so C1's seam pass comes
    for free, but the SEAM is NOT asserted at C0):
      - GROUND fills the whole cell first (the indigo background everything
        overlays).
      - ONE FULL {n/k} STAR centered at the cell CENTER (cell/2, cell/2) — the
        4-fold node of the p4 cell. This is the DOMINANT gold figure and the
        whole point: a single, complete, unmistakable 8-pointed star per cell.
        Its tips reach `starRadius*cell`, so with the default ~0.47 they nearly
        touch the cell edges and the adjacent cells' stars interlock.
      - A QUARTER-CROSS at EACH of the FOUR CORNERS (the lattice nodes shared
        with the three neighbor cells). Each corner carries a diamond centered
        ON the corner; the four quarters from four cells meeting at a node
        complete into ONE whole azure cross under pure translation by `cell`.
        This is the interstitial that makes it "star-AND-cross", and it is what
        completes across the seam (C1) — the cross center is on the lattice node.

    Origin at the cell min-corner; all lengths are cell-relative fractions
    (no magic literal). Closed-form, no RNG → byte-identical (the §6 #5 gate)."""
    n = max(3, int(float(params.get("points", 8))))
    k = _resolve_k(n, int(float(params.get("skip", 3))))
    cell = float(params.get("cell", 1.0))
    star_frac = float(params.get("starRadius", 0.47))
    star_color = params.get("starColor", "gold")
    cross_color = params.get("crossColor", "azure")
    ground_color = params.get("groundColor", "indigo")

    outer = star_frac * cell

    polys = []

    # 1. GROUND — the full cell, painted first so everything else overlays it.
    polys.append(([(0.0, 0.0), (cell, 0.0), (cell, cell), (0.0, cell)], ground_color))

    # 2. The CROSS at each of the four CORNERS (the interstitial between four
    #    interlocking stars). A diamond (45°-rotated square) centered on the
    #    corner; its half-diagonal is the gap between the corner and where the
    #    centered star's corner-pointing tip reaches along the cell diagonal.
    #    Each corner shows one quarter of the diamond (the rest is clipped to the
    #    cell); the four quarters meeting at a lattice node complete into one
    #    whole azure cross. Cell-relative — no magic literal.
    #    The diagonal half-distance from the cell center to a corner is
    #    cell/sqrt(2); the star's tip reaches `outer` along that diagonal, so the
    #    leftover corner gap is (cell/sqrt(2) - outer), and the diamond spans that
    #    gap centered on the corner.
    diag_half = cell * 0.5 * math.sqrt(2.0)
    cross_half = max(1e-3 * cell, diag_half - outer)
    for (cxr, cyr) in ((0.0, 0.0), (cell, 0.0), (cell, cell), (0.0, cell)):
        polys.append((
            [(cxr, cyr - cross_half), (cxr + cross_half, cyr),
             (cxr, cyr + cross_half), (cxr - cross_half, cyr)],
            cross_color,
        ))

    # 3. ONE FULL {n/k} STAR centered at the cell CENTER — the dominant gold
    #    figure. A single complete 2n-vertex tip/valley ring; nothing clipped
    #    away, so it reads unmistakably as one 8-pointed star (the FIDELITY fix:
    #    the old build placed clipped corner fragments that merged into a plus).
    cx = cy = cell * 0.5
    polys.append((_star_ring(cx, cy, n, k, outer), star_color))

    return polys


def star_boundary_vertices(params: dict) -> int:
    """The star's boundary-vertex count = 2*points (the FIDELITY #2b proof).
    A helper so the smoke test asserts on the contract, not a magic number."""
    n = max(3, int(float(params.get("points", 8))))
    return 2 * n


# -----------------------------------------------------------------------------
# bake_png — the PURE PIL rasterizer (the (b2) effector). CONSUMES tile_polys;
# NO second copy of the motif math lives here.
# -----------------------------------------------------------------------------

def _color_rgb(key, fallback="indigo"):
    """Resolve a palette key (or a literal RGB tuple) to an RGB triple. A bare
    name not in the palette degrades to the fallback — never crashes (mirrors
    the realizer's flat-color fallback discipline)."""
    if isinstance(key, (tuple, list)) and len(key) == 3:
        return tuple(int(c) for c in key)
    return PALETTE.get(key, PALETTE[fallback])


def bake_png(params: dict, out_path: str):
    """Rasterize ONE cell (x `repeat`) to a byte-deterministic RGB PNG via PIL.

    Each (polygon, color_key) from tile_polys is scaled cell->pixels and filled
    with ImageDraw.polygon; then the region boundaries are stroked in `lineColor`
    at width round(lineWidth*pxPerCell) (>= 3 px, the CRISPNESS/READS-AT-SCALE
    gate). No RNG, fixed draw order → identical bytes every run.

    Crispness note: we draw directly at the target resolution with NO anti-
    aliasing (PIL's polygon fill is hard-edged). That keeps the boundary columns
    exact so C1's seam test can claim ε = 0 (the reviewer's pin). No supersample,
    because AA on the boundary would break the ε=0 promise."""
    from PIL import Image, ImageDraw  # imported here so the pure tier needs no PIL

    cell = float(params.get("cell", 1.0))
    px_per_cell = int(float(params.get("pxPerCell", 1024)))
    repeat = max(1, int(float(params.get("repeat", 1))))
    line_w_frac = float(params.get("lineWidth", 0.02))
    line_rgb = _color_rgb(params.get("lineColor", "black"))
    ground_rgb = _color_rgb(params.get("groundColor", "indigo"))

    size = px_per_cell * repeat
    img = Image.new("RGB", (size, size), ground_rgb)
    draw = ImageDraw.Draw(img)

    # cell-space -> pixel-space. Y is flipped so +y is UP in cell space but the
    # PNG's row 0 is the top — keeps the math in math-convention, the image in
    # image-convention. Deterministic linear map (ortho, no foreshortening).
    scale = px_per_cell / cell

    def to_px(pt, ox, oy):
        x = (pt[0] + ox) * scale
        y = size - (pt[1] + oy) * scale  # flip Y
        return (x, y)

    line_w = max(3, round(line_w_frac * px_per_cell))  # >= 3 px (the gate)

    polys = tile_polys(params)  # the SINGLE source — consumed, not re-derived

    # Lay every repeat by pure translation (the lattice vectors). For C0 repeat=1;
    # the loop keeps repeat>1 a pre-tiled sheet for free.
    for ry in range(repeat):
        for rx in range(repeat):
            ox = rx * cell
            oy = ry * cell
            for polygon, color_key in polys:
                pts = [to_px(p, ox, oy) for p in polygon]
                draw.polygon(pts, fill=_color_rgb(color_key))
            # Strokes overlay the fills (the "drawn" girih outline). Drawn after
            # all fills in this repeat so lines are never painted over.
            for polygon, _color_key in polys:
                pts = [to_px(p, ox, oy) for p in polygon]
                draw.line(pts + [pts[0]], fill=line_rgb, width=line_w, joint="curve")

    img.save(out_path, "PNG")
    return out_path


def edge_match(params: dict):
    """OFFLINE SEAMLESS gate (rubric #1). Bake ONE cell IN MEMORY and return
    (max|col(x=0) - col(x=W-1)|, max|row(y=0) - row(y=H-1)|) over all channels.

    WHY this proves seamlessness: a wallpaper pattern is invariant under its
    lattice translation vectors — translating by `cell` maps the pattern onto
    itself. We FRAME exactly the square translational cell [0,cell)x[0,cell)
    (the half-open convention: a point at x=cell maps to pixel pxPerCell, which is
    OFF the pxPerCell-wide image and belongs to the NEXT tile — no doubled seam
    column). Because the construction is lattice-periodic AND the boundary region
    is symmetric ground, the first and last in-cell columns/rows are pixel-equal.

    ε = 0 (EXACT): PIL's polygon fill is hard-edged (no anti-aliasing), so the
    boundary pixels are exact — we claim 0, not a few/255 (the reviewer's §9 pin).
    Returns a pair of ints (both 0 when seamless). Pure of disk (uses an
    in-memory Image); needs PIL like bake_png."""
    from PIL import Image, ImageDraw  # PIL-tier, same guard as bake_png

    cell = float(params.get("cell", 1.0))
    px_per_cell = int(float(params.get("pxPerCell", 1024)))
    line_w_frac = float(params.get("lineWidth", 0.02))
    line_rgb = _color_rgb(params.get("lineColor", "black"))
    ground_rgb = _color_rgb(params.get("groundColor", "indigo"))

    size = px_per_cell  # the seam gate measures ONE cell (repeat is irrelevant)
    img = Image.new("RGB", (size, size), ground_rgb)
    draw = ImageDraw.Draw(img)
    scale = px_per_cell / cell

    def to_px(pt):
        return (pt[0] * scale, size - pt[1] * scale)

    line_w = max(3, round(line_w_frac * px_per_cell))
    polys = tile_polys(params)
    for polygon, color_key in polys:
        draw.polygon([to_px(p) for p in polygon], fill=_color_rgb(color_key))
    for polygon, _color_key in polys:
        pts = [to_px(p) for p in polygon]
        draw.line(pts + [pts[0]], fill=line_rgb, width=line_w, joint="curve")

    px = img.load()
    w, h = img.size

    def chan_max(a, b):
        m = 0
        for pa, pb in zip(a, b):
            for ca, cb in zip(pa, pb):
                d = abs(ca - cb)
                if d > m:
                    m = d
        return m

    col0 = [px[0, y] for y in range(h)]
    colw = [px[w - 1, y] for y in range(h)]
    row0 = [px[x, 0] for x in range(w)]
    rowh = [px[x, h - 1] for x in range(w)]
    return (chan_max(col0, colw), chan_max(row0, rowh))


def bake_layup_png(params: dict, out_path: str, n: int = 3):
    """VISUAL SEAMLESS proof: bake ONE cell, then PIL-paste it n x n by pure
    translation into a single PNG (default 3x3) — the lattice lay-up the
    reviewer eyeballs. NO new motif math: it reuses bake_png's single-cell
    output, so the proof and the gate share the one generator.

    The proof reads: no seam line at the joins, no doubled/missing motif, and
    the azure QUARTER-crosses at the cell corners COMPLETE into full crosses
    where four tiles meet (the corner construction wraps under translation by
    `cell`). Byte-deterministic (bake_png is)."""
    from PIL import Image  # PIL-tier guard

    tile_path = out_path + ".__tile.png"
    bake_png(params, tile_path)
    try:
        tile = Image.open(tile_path).convert("RGB")
        w, h = tile.size
        big = Image.new("RGB", (w * n, h * n))
        for ry in range(n):
            for rx in range(n):
                big.paste(tile, (rx * w, ry * h))  # pure translation = the lattice
        big.save(out_path, "PNG")
    finally:
        if os.path.exists(tile_path):
            os.remove(tile_path)
    return out_path


# -----------------------------------------------------------------------------
# CLI — mirrors edi_craft.py's --key=value arg style.
# -----------------------------------------------------------------------------

def _parse_args(argv: list[str]) -> dict:
    """--out=<png> plus optional --<param>=<value> overrides of the defaults.
    --layup=<png> bakes the N x N seam proof (--layupN=<int>, default 3)."""
    params = _defaults()
    out = None
    layup = None
    layup_n = 3
    int_keys = {p["key"] for p in MANIFEST["params"] if p["type"] == "integer"}
    num_keys = {p["key"] for p in MANIFEST["params"] if p["type"] == "number"}
    for arg in argv:
        if not arg.startswith("--") or "=" not in arg:
            continue
        key, val = arg[2:].split("=", 1)
        if key == "out":
            out = val
        elif key == "layup":
            layup = val
        elif key == "layupN":
            layup_n = max(1, int(float(val)))
        elif key in int_keys:
            params[key] = int(float(val))
        elif key in num_keys:
            params[key] = float(val)
        elif key in params:
            params[key] = val
    return {"out": out, "layup": layup, "layupN": layup_n, "params": params}


def main(argv: list[str]) -> int:
    parsed = _parse_args(argv)
    out = parsed["out"]
    layup = parsed["layup"]
    if not out and not layup:
        print("usage: girih_tile.py --out=<png> [--layup=<png> [--layupN=3]] "
              "[--points=.. --skip=.. --cell=.. --starRadius=.. --pxPerCell=..]",
              file=sys.stderr)
        return 2
    if out:
        os.makedirs(os.path.dirname(os.path.abspath(out)), exist_ok=True)
        bake_png(parsed["params"], out)
        print(f"baked {out}")
    if layup:
        os.makedirs(os.path.dirname(os.path.abspath(layup)), exist_ok=True)
        bake_layup_png(parsed["params"], layup, parsed["layupN"])
        print(f"baked {layup} ({parsed['layupN']}x{parsed['layupN']} lay-up)")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
