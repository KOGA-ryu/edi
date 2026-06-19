#!/usr/bin/env python3
"""ctest smoke for the M0 realizer's Blender-free half (edi_realize.py).

Asserts the pure tier — TOON parse + greybox plan + OBJ proof — is
deterministic and matches the Seam-B contract: the crypt fixture resolves all
10 structural piece types plus the two props and exactly one light; the parser
tolerates the OLDER TOON shape (no `flags` plug column, no `blocks` section); a
doorway lands on exactly one wall segment; and quoted coordinate cells parse.
The bpy half (build + OptiX render) is exercised in Blender; everything testable
without a GPU is pinned here so a refactor cannot silently bend the plan.
"""

import os
import sys

sys.dont_write_bytecode = True  # no tools/blender/__pycache__ in the source tree

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
sys.path.insert(0, os.path.join(ROOT, "tools", "blender"))

import edi_realize  # noqa: E402

CRYPT = os.path.join(ROOT, "samples", "crypt_m0", "crypt.toon")
MAPS = os.path.join(ROOT, "tests", "data", "maps")
ZOO = os.path.join(ROOT, "samples", "zoo")
MANIFEST = os.path.join(ZOO, "tester_zoo.manifest.toon")
STAR6_DEMO = os.path.join(ZOO, "star6_demo.toon")


def fail(msg):
    print(f"edi_realize smoke: FAIL — {msg}", file=sys.stderr)
    sys.exit(1)


def counts(pieces):
    out = {}
    for p in pieces:
        out[p.kind] = out.get(p.kind, 0) + 1
    return out


# --- 1. The crypt fixture parses to the expected shape -----------------------
with open(CRYPT, encoding="utf-8") as fh:
    doc = edi_realize.parse_toon(fh.read())

if doc.title != "crypt":
    fail(f"title {doc.title!r}")
if len(doc.rooms) != 2:
    fail(f"rooms {len(doc.rooms)}")
if len(doc.connections) != 1:
    fail(f"connections {len(doc.connections)}")
if len(doc.blocks) != 3:
    fail(f"blocks {len(doc.blocks)}")
# the newer `flags` plug column parses into a list
if not any("crypt" in p.flags for p in doc.plugs):
    fail("plug flags not parsed")
# coordinate cells unquote to floats
ent = doc.room("entrance")
if ent is None or (ent.ox, ent.oy, ent.w, ent.h) != (0.0, 0.0, 15.0, 15.0):
    fail(f"entrance geometry {ent}")

# --- 2. Greybox resolves all 10 structural types + props + one light ---------
# NOTE on the "10 piece types" gate: the realizer's socket table (TEN_PIECE_TYPES)
# COVERS all 10, but how many a given map YIELDS depends on the layout —
# `corridor_l` needs an L-bend (offset plugs) and `stair` needs a `.stair` block
# row. The crypt fixture is PURPOSE-BUILT to exercise all 10; the stock maps
# yield only 8 (pinned in step 6). So this assertion is a property of crypt.toon,
# not of every map.
pieces = edi_realize.plan_greybox(doc)
c = counts(pieces)
present = [t for t in edi_realize.TEN_PIECE_TYPES if c.get(t, 0) > 0]
if len(present) != 10:
    fail(f"crypt fixture should yield all 10 structural types, got {len(present)}: {present}")
if sum(1 for p in pieces if p.is_light) != 1:
    fail("expected exactly one light (the brazier)")
if c.get(edi_realize.PIECE_SARCOPHAGUS, 0) != 1:
    fail("expected one sarcophagus")
if c.get(edi_realize.PIECE_BRAZIER, 0) < 1:
    fail("expected a brazier piece")
# a door maps to exactly one wall segment: 2 plugs -> 2 doorway frames
if c.get(edi_realize.PIECE_DOORWAY, 0) != 2:
    fail(f"expected 2 doorway frames, got {c.get(edi_realize.PIECE_DOORWAY)}")
# the offset plugs force an L corridor with exactly one bend
if c.get(edi_realize.PIECE_CORRIDOR_L, 0) != 1:
    fail(f"expected 1 corridor bend, got {c.get(edi_realize.PIECE_CORRIDOR_L)}")
# asset_refs carry the theme prefix
if not all("." in p.asset_ref for p in pieces):
    fail("an asset_ref lost its <theme>.<piece> prefix")

# --- 3. The OBJ proof is well-formed; lights emit no geometry ----------------
obj = edi_realize.pieces_to_obj(pieces)
n_obj = obj.count("\no ") + (1 if obj.startswith("o ") else 0)
n_solid = sum(1 for p in pieces if not p.is_light)
if n_obj != n_solid:
    fail(f"OBJ object count {n_obj} != solid pieces {n_solid} (light leaked geometry?)")
if "v " not in obj or "f " not in obj:
    fail("OBJ missing vertices/faces")

# --- 4. The parser tolerates the back-compat TOON shape (no flags / no blocks) -
# These stock fixtures predate the `flags` plug column + the `blocks` section;
# the current C++ writer always emits `flags`, so this guards back-compat, not
# the shape dungeon-map emits today.
with open(os.path.join(MAPS, "hallway.toon"), encoding="utf-8") as fh:
    old = edi_realize.parse_toon(fh.read())
if len(old.rooms) != 4 or len(old.connections) != 3:
    fail(f"back-compat parse: rooms={len(old.rooms)} conns={len(old.connections)}")
if old.blocks:
    fail("back-compat format has no blocks section; blocks must be empty")
if any(p.flags for p in old.plugs):
    fail("back-compat format has no flags column; flags must be empty")
# and still plans without crashing
edi_realize.plan_greybox(old)

# --- 5. Quote-aware cell splitting -------------------------------------------
cells = edi_realize._split_cells('  entrance,"0,0","15,15",stone')
if cells != ["entrance", "0,0", "15,15", "stone"]:
    fail(f"_split_cells {cells}")

# --- 6. Pin the stock maps' real piece coverage (8/10) -----------------------
# Documents reality (reviewer SHOULD-FIX 2): a straight-corridor, no-stair map —
# the common case — proves only 8 of the 10 types; `corridor_l` + `stair` are
# layout-specific. If this ever changes, the gate claim must be re-examined.
for name in ("hallway", "hub", "loop"):
    with open(os.path.join(MAPS, f"{name}.toon"), encoding="utf-8") as fh:
        d = edi_realize.parse_toon(fh.read())
    cc = counts(edi_realize.plan_greybox(d))
    got = {t for t in edi_realize.TEN_PIECE_TYPES if cc.get(t, 0) > 0}
    expect_absent = {edi_realize.PIECE_CORRIDOR_L, edi_realize.PIECE_STAIR}
    if got & expect_absent:
        fail(f"{name}: unexpectedly yielded {got & expect_absent}")
    if len(got) != 8:
        fail(f"{name}: expected 8/10 stock coverage, got {len(got)}: {sorted(got)}")

# --- 7. The single SCALE knob (brief 044) -----------------------------------
D = edi_realize.DEFAULT_DIMS
if D.scaled(1.0) is not D:
    fail("scaled(1.0) should be identity (same object)")
d2 = D.scaled(2.0)
# every ENVELOPE LENGTH field doubles
for fld in ("wall_h", "wall_t", "floor_t", "corridor_w", "door_w",
            "column_w", "prop_base_z", "brazier_w", "brazier_h",
            "brazier_light_z", "stair_h", "stair_z", "brazier_softness"):
    if abs(getattr(d2, fld) - 2.0 * getattr(D, fld)) > 1e-9:
        fail(f"scaled(2).{fld} should double")
if d2.sarcophagus != tuple(2.0 * v for v in D.sarcophagus):
    fail("scaled(2).sarcophagus tuple should double")
# the grid MODULE (tile, contract §1), the length tolerance, and all
# dimensionless / intensity / output fields are scale-INVARIANT
for fld in ("tile", "min_leg", "corner_factor", "endcap_factor",
            "cam_off_x", "cam_off_y", "cam_off_z", "res_x", "res_y", "samples",
            "light_per_sqft", "light_min", "ambient_strength"):
    if getattr(d2, fld) != getattr(D, fld):
        fail(f"scaled(2).{fld} must stay scale-invariant")
# corridor pieces actually widen under the scaled table
w1 = [p for p in edi_realize.plan_greybox(doc, D) if p.kind == edi_realize.PIECE_CORRIDOR]
w2 = [p for p in edi_realize.plan_greybox(doc, d2) if p.kind == edi_realize.PIECE_CORRIDOR]
if not (w1 and w2) or not (max(p.sy for p in w2) > max(p.sy for p in w1)):
    fail("scaled table should widen corridor pieces")

# --- 8. Brazier light is area-driven, and table-scale-INVARIANT --------------
# Energy = density × the containing room's AREA (so a big room is lit edge-to-
# edge). Crucially it depends on the WIRE room size, NOT the table scale: the
# rooms arrive already-scaled, so scaling the table must NOT also scale energy
# (that would over-light). Pin both.
crypt_room = doc.room("crypt")
expect_energy = D.light_per_sqft * (crypt_room.w * crypt_room.h)
light1 = [p for p in edi_realize.plan_greybox(doc, D) if p.is_light][0]
if abs(light1.light_energy - expect_energy) > 1e-6:
    fail(f"brazier energy {light1.light_energy} != density×area {expect_energy}")
light2 = [p for p in edi_realize.plan_greybox(doc, d2) if p.is_light][0]
if abs(light2.light_energy - light1.light_energy) > 1e-6:
    fail("scaling the TABLE must not change the light (it rides wire room area)")

# --- 9. The toggleable scale-reference overlay (figure + floor checker) ------
# Default OFF: no figure pieces, plain floor material (back-compat — earlier
# steps used reference=False implicitly).
base = edi_realize.plan_greybox(doc, D)
if any(p.kind == "ref_figure" for p in base):
    fail("reference overlay must be OFF by default")
if any(p.material in ("ref_a", "ref_b") for p in base):
    fail("floor checker must be OFF by default")
# ON: a figure appears and the floor is checkered.
refp = edi_realize.plan_greybox(doc, D, reference=True)
fig = [p for p in refp if p.kind == "ref_figure"]
if len(fig) != 2:
    fail(f"reference overlay should add a 2-piece figure, got {len(fig)}")
if not any(p.material == "ref_a" for p in refp) or not any(p.material == "ref_b" for p in refp):
    fail("reference overlay should checker the floor (ref_a + ref_b)")


def figure_top(pieces):
    f = [p for p in pieces if p.kind == "ref_figure"]
    return max(p.z + p.sz / 2.0 for p in f)


# THE POINT: the figure is FIXED at figure_h regardless of S (it does NOT scale).
top1 = figure_top(edi_realize.plan_greybox(doc, D.scaled(1.0), reference=True))
top4 = figure_top(edi_realize.plan_greybox(doc, D.scaled(4.0), reference=True))
# top = floor_t (scales) + figure_h (fixed); subtract floor_t to compare the
# figure's own height contribution — it must be identical at S=1 and S=4.
h1 = top1 - D.scaled(1.0).floor_t
h4 = top4 - D.scaled(4.0).floor_t
if abs(h1 - D.figure_h) > 1e-9 or abs(h4 - D.figure_h) > 1e-9:
    fail(f"figure height must equal figure_h at every S (got {h1}, {h4})")
if abs(h1 - h4) > 1e-9:
    fail("figure must NOT scale with S (that is what makes scale visible)")

# --- 10. Phase-2 inverted model: golden parse of the 074-FINAL sample --------
# The ratified maxed-out wire sample. Pins parse_toon against EVERY conditional
# column/section so a refactor can't silently drop one. (dungeon-map brief 074.)
with open(os.path.join(MAPS, "inverted_sample.toon"), encoding="utf-8") as fh:
    inv = edi_realize.parse_toon(fh.read())
if inv.feet_per_band != 12.0:
    fail(f"feet_per_band meta {inv.feet_per_band}")
if inv.levels != {0: 0.0, 1: 12.0}:
    fail(f"levels manifest {inv.levels}")
gh = inv.room("great_hall")
if gh is None or gh.derivation != "span_derived" or gh.bounded_by != ["junction_a", "junction_b"]:
    fail(f"span room fields {gh}")
if gh.walls != "NESW" or gh.kind != "enclosed" or gh.level != 0:
    fail(f"great_hall mask/kind/level {gh.walls}/{gh.kind}/{gh.level}")
sc = inv.room("sky_court")
if sc is None or sc.level != 1 or sc.kind != "open" or sc.walls != "N-S-" or sc.ceiling is not False:
    fail(f"sky_court fields {sc}")
if len(inv.nodes) != 2 or inv.nodes[0].radius != 2.5 or inv.nodes[0].name != "junction_a":
    fail(f"nodes {inv.nodes}")
if [p.level for p in inv.plugs] != [0, 1]:
    fail(f"plug levels {[p.level for p in inv.plugs]}")
# the inverted plan: nodes render small + a span-room + an open room; cross-level
# connection lays NO flat corridor (it's a vertical transition).
ipieces = edi_realize.plan_greybox(inv, edi_realize.DEFAULT_DIMS, scale=1.0)
if not any(p.kind == "node" for p in ipieces):
    fail("inverted plan should render connector nodes")
icnt = counts(ipieces)
if icnt.get(edi_realize.PIECE_CORRIDOR, 0) != 0:
    fail("a cross-level connection must not lay a flat corridor")
# floor stacking: the level-1 sky_court floor sits at elevation 12 (manifest).
court_floor = [p for p in ipieces if p.kind == edi_realize.PIECE_FLOOR and p.z > 12.0]
if not court_floor:
    fail("level-1 room floor should be stacked at the manifest elevation (z>12)")
# the unscaled node footprint = 2*radius (5.0) when the wire supplies radius.
node_pieces = [p for p in ipieces if p.kind == "node"]
if abs(node_pieces[0].sx - 5.0) > 1e-9:
    fail(f"node footprint should be 2*radius=5.0, got {node_pieces[0].sx}")
# back-compat: the OLD crypt (no nodes/levels) yields NO node pieces + level-0 only.
if any(p.kind == "node" for p in edi_realize.plan_greybox(doc)):
    fail("node-less map must produce no node pieces")

# --- 11. R2a: zoo-manifest parse round-trip (the cross-language contract loop) -
# Closes the loop owed to R1a's reviewer: parse the SAME manifest fixture and
# assert star6's fields decode back EXACTLY. If any column shifts, this fails.
manifest = edi_realize.load_manifest(MANIFEST)
if list(manifest.keys()) != ["asset_0001"]:
    fail(f"manifest ids {list(manifest.keys())}")
star6 = manifest["asset_0001"]
if star6.id != "asset_0001":
    fail(f"asset id {star6.id!r}")
if star6.name != "star6":
    fail(f"asset name {star6.name!r}")
if star6.category != "ornament":
    fail(f"asset category {star6.category!r}")
if star6.meshRef != "meshes/star6.blend":
    fail(f"asset meshRef {star6.meshRef!r}")
if star6.proxyRef != "":
    fail(f"asset proxyRef should be empty, got {star6.proxyRef!r}")
if star6.textures != []:
    fail(f"asset textures should be empty, got {star6.textures!r}")

# --- 12. R2a: a placement resolves to a build-once INSTANCE piece -------------
with open(STAR6_DEMO, encoding="utf-8") as fh:
    demo = edi_realize.parse_toon(fh.read())
n_blocks = len(demo.blocks)
if n_blocks < 1:
    fail("star6 demo should place at least one block")
# WITH the manifest: every star6 placement becomes an instance carrying meshRef.
inst_pieces = [p for p in edi_realize.plan_greybox(demo, assets=manifest)
               if p.kind == "instance"]
if len(inst_pieces) != n_blocks:
    fail(f"expected {n_blocks} instance pieces, got {len(inst_pieces)}")
if not all(p.mesh_ref == "meshes/star6.blend" for p in inst_pieces):
    fail("an instance piece lost its resolved meshRef")
if not all(p.asset_ref == "asset_0001" for p in inst_pieces):
    fail("an instance piece lost its asset id")
# the per-instance scale rides on sx/sy/sz (for obj.scale in R2b)
demo_scales = sorted(b.scale for b in demo.blocks)
if sorted(p.sx for p in inst_pieces) != demo_scales:
    fail("instance per-placement scale should ride on sx/sy/sz")

# --- 13. R2a fallback law: NO manifest ⇒ ZERO instance pieces -----------------
# A star6 placement with no manifest falls back to the greybox path (unknown
# prop), proving the no-manifest path is unchanged.
nofb = edi_realize.plan_greybox(demo, assets=None)
if any(p.kind == "instance" for p in nofb):
    fail("no-manifest plan must produce zero instance pieces (fallback law)")
if not any(p.kind == "prop" for p in nofb):
    fail("unresolved star6 placement should fall back to a greybox prop")

# --- 14. R2a regression: the crypt sample plans byte-identically (assets=None) -
# The greybox FALLBACK LAW: passing assets=None must equal the default-arg plan.
crypt_default = edi_realize.plan_greybox(doc)
crypt_none = edi_realize.plan_greybox(doc, assets=None)
if crypt_default != crypt_none:
    fail("crypt plan must be byte-identical with assets=None vs default")
if any(p.kind == "instance" for p in crypt_default):
    fail("the crypt sample has no manifest and must yield no instance pieces")

# the OBJ proof emits a labelled placeholder marker per instance piece
demo_obj = edi_realize.pieces_to_obj(edi_realize.plan_greybox(demo, assets=manifest))
if demo_obj.count("o instance_") != n_blocks:
    fail(f"OBJ proof should mark {n_blocks} instances, got {demo_obj.count('o instance_')}")
if "star6_blend" not in demo_obj:
    fail("instance OBJ marker should be labelled with its meshRef")

print("edi_realize smoke: ok")
