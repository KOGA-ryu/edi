#!/usr/bin/env python3
"""edi_realize.py — the M0 REALIZER (Seam A's half of the crypt hardware gate).

Reads a **Seam-B TOON map** (the neutral dungeon export produced by edi /
dungeon-map: `rooms` + `plugs` + `connections` + `blocks`), resolves each
socket + asset_ref to a **GREYBOX placeholder mesh** (one simple primitive per
piece type — we are proving the pipeline, not the art), snaps everything on the
5 ft grid, sets the brazier block as a light, and renders **Cycles OptiX on the
GPU**.

Two tiers, mirroring tools/blender/edi_craft.py so the logic is testable without
a Blender install:

  * PURE (no bpy) — `parse_toon` + `plan_greybox` turn the TOON into a flat list
    of `Piece` records (kind, asset_ref, centre, size, rotation, light?). The
    `--obj-out=<path>` proof writes that plan as a Wavefront OBJ. This is the
    deterministic half the cross-language smoke test exercises.

  * BLENDER — run inside Blender:
        blender --background --python tools/blender/edi_realize.py -- \
            <map.toon> --render=<out.png> [--samples=N]
    It builds the pieces as primitives, configures Cycles + OptiX + the GPU
    (refusing to fall back to CPU silently — the device choice is logged), frames
    a camera, and renders 1080p.

Why a pure tier at all: a one-shot reviewer (and CI without a GPU) must be able
to assert "the realizer resolves all 10 piece types from this map and snaps them
to these coordinates" deterministically. bpy is only the final effector.
"""

from __future__ import annotations

import math
import sys
from dataclasses import dataclass, field
from typing import Optional

# --- Grid + greybox constants (the M0 socket contract, in FEET) --------------
# Grid module = 1 tile = 5 ft, min-corner origin (matches the brief + the
# dungeon-map MapSpec). One Blender unit = one foot here; the brazier light
# energy below is tuned for that scale.
TILE_FT = 5.0
WALL_H = 12.0           # interior wall / ceiling height
WALL_T = 0.6           # wall panel thickness
FLOOR_T = 0.4          # floor / ceiling slab thickness
# Corridor width is REALIZER-AUTHORITATIVE (frozen socket contract §5 — it is not
# on the wire; the generator doubles ROOMS in the TOON, the realizer doubles the
# HALLWAY here). Doubled to 2 modules (10 ft) so a doubled crypt's corridor reads
# at the same proportion as its doubled rooms. DOOR_W (< CORRIDOR_W) is the clear
# opening, leaving a frame reveal; the wall opening rounds up to whole 5 ft
# segments covering CORRIDOR_W (see has_door).
CORRIDOR_W = 2 * TILE_FT  # 10 ft = 2 tiles (DOOR_PLUG spans 2 modules)
DOOR_W = 8.0              # doorway clear opening (< CORRIDOR_W, leaves a frame)


# --- The piece vocabulary (the 10 types + 2 props) ---------------------------
# asset_ref = "<theme>.<piece>". The realizer's socket table is exactly these
# keys; a real artist mesh later swaps in by asset_ref without touching the plan.
PIECE_FLOOR = "floor_tile"
PIECE_WALL = "wall_panel"
PIECE_CORNER = "wall_corner"
PIECE_DOORWAY = "doorway_frame"
PIECE_ENDCAP = "end_cap"
PIECE_CORRIDOR = "corridor_straight"
PIECE_CORRIDOR_L = "corridor_l"
PIECE_CEILING = "ceiling_slab"
PIECE_COLUMN = "column"
PIECE_STAIR = "stair"
PIECE_SARCOPHAGUS = "sarcophagus"
PIECE_BRAZIER = "brazier"

# The canonical 10 structural piece types (props are placed via the blocks
# section, so they are counted separately for the gate's "10 piece types").
TEN_PIECE_TYPES = (
    PIECE_FLOOR, PIECE_WALL, PIECE_CORNER, PIECE_DOORWAY, PIECE_ENDCAP,
    PIECE_CORRIDOR, PIECE_CORRIDOR_L, PIECE_CEILING, PIECE_COLUMN, PIECE_STAIR,
)


# =============================================================================
# Pure data model
# =============================================================================
@dataclass
class Room:
    name: str
    ox: float
    oy: float
    w: float
    h: float
    material: str = "stone"


@dataclass
class Plug:
    room: str
    name: str
    edge: str            # N | E | S | W | ?
    type: str            # door | ...
    connected: bool
    flags: list[str] = field(default_factory=list)


@dataclass
class Connection:
    src: str             # "room.plug"
    dst: str             # "room.plug"
    type: str            # corridor | ...


@dataclass
class Block:
    room: str
    asset: str           # "<theme>.<piece>"
    x: float
    y: float
    scale: float = 1.0
    rotation: float = 0.0


@dataclass
class MapDoc:
    title: str = ""
    units: str = "feet"
    rooms: list[Room] = field(default_factory=list)
    plugs: list[Plug] = field(default_factory=list)
    connections: list[Connection] = field(default_factory=list)
    blocks: list[Block] = field(default_factory=list)

    def room(self, name: str) -> Optional[Room]:
        for r in self.rooms:
            if r.name == name:
                return r
        return None


@dataclass
class Piece:
    """One resolved greybox object: an asset_ref snapped to a world transform.

    The transform is a centre + half-agnostic box size (we only build boxes /
    cylinders for greybox), an optional Z-rotation, and the light flag. A real
    mesh swaps in by `asset_ref`; the transform is the socket snap."""
    kind: str            # one of the PIECE_* names
    asset_ref: str       # "<theme>.<piece>"
    x: float
    y: float
    z: float
    sx: float
    sy: float
    sz: float
    rot_deg: float = 0.0
    is_light: bool = False
    light_energy: float = 0.0
    material: str = "stone"
    shape: str = "box"   # box | cylinder


# =============================================================================
# TOON parsing (the Seam-B grammar — see src/io/MapToonExport.cpp)
# =============================================================================
def _split_cells(line: str) -> list[str]:
    """Split one TOON row on commas that are NOT inside double quotes, then
    unquote. Mirrors the writer's `cell()`: a quoted cell may contain a comma
    (e.g. a coordinate pair "21,47.5") and an escaped quote \"."""
    cells: list[str] = []
    buf: list[str] = []
    in_quote = False
    i = 0
    while i < len(line):
        c = line[i]
        if in_quote:
            if c == "\\" and i + 1 < len(line):
                buf.append(line[i + 1])
                i += 2
                continue
            if c == '"':
                in_quote = False
                i += 1
                continue
            buf.append(c)
        else:
            if c == '"':
                in_quote = True
            elif c == ",":
                cells.append("".join(buf))
                buf = []
            else:
                buf.append(c)
        i += 1
    cells.append("".join(buf))
    return [c.strip() for c in cells]


def _xy(cell: str) -> tuple[float, float]:
    parts = _split_cells(cell) if "," in cell else cell.split(",")
    a, b = parts[0], parts[1]
    return float(a), float(b)


def parse_toon(text: str) -> MapDoc:
    """Parse a Seam-B TOON map. Tolerates the OLDER format (no `flags` plug
    column, no `blocks` section) and the NEWER one. Section header form:
        name[count]{col,col,...}:
    followed by indented rows until a blank line."""
    doc = MapDoc()
    lines = text.splitlines()
    i = 0
    n = len(lines)
    section = None        # current section name
    cols: list[str] = []  # current section columns

    def header_value(raw: str) -> str:
        v = raw.strip()
        if v.startswith('"') and v.endswith('"') and len(v) >= 2:
            return v[1:-1].replace('\\"', '"')
        return v

    while i < n:
        line = lines[i]
        stripped = line.strip()
        i += 1
        if not stripped:
            section = None
            continue

        # Section header?  name[count]{cols}:
        if "[" in stripped and "]{" in stripped and stripped.endswith(":"):
            name = stripped[: stripped.index("[")]
            colpart = stripped[stripped.index("{") + 1 : stripped.index("}")]
            section = name
            cols = [c.strip() for c in colpart.split(",")]
            continue

        # Top-level key: value (only before/between sections)
        if section is None and ":" in stripped and not stripped.startswith('"'):
            key, _, val = stripped.partition(":")
            key = key.strip()
            if key == "kind":
                continue
            if key == "title":
                doc.title = header_value(val)
            elif key == "units":
                doc.units = header_value(val)
            continue

        # A data row of the current section.
        cells = _split_cells(stripped)

        def col(name: str, default: str = "") -> str:
            if name in cols:
                idx = cols.index(name)
                if idx < len(cells):
                    return cells[idx]
            return default

        if section == "rooms":
            ox, oy = _xy(col("origin"))
            w, h = _xy(col("size"))
            doc.rooms.append(Room(col("name"), ox, oy, w, h, col("material", "stone")))
        elif section == "plugs":
            flags_raw = col("flags")
            flags = [f for f in flags_raw.split("·") if f] if flags_raw else []
            doc.plugs.append(Plug(
                room=col("room"), name=col("name"), edge=col("edge", "?"),
                type=col("type", "door"),
                connected=col("connected", "false").lower() == "true",
                flags=flags))
        elif section == "connections":
            doc.connections.append(Connection(col("from"), col("to"), col("type", "corridor")))
        elif section == "blocks":
            bx, by = _xy(col("origin"))
            doc.blocks.append(Block(
                room=col("room"), asset=col("asset"), x=bx, y=by,
                scale=float(col("scale", "1") or "1"),
                rotation=float(col("rotation", "0") or "0")))
    return doc


# =============================================================================
# Greybox planning (the socket resolve + grid snap) — PURE
# =============================================================================
def _theme_of(doc: MapDoc) -> str:
    """The asset theme prefix. Derived from any block asset_ref `<theme>.<piece>`
    in the map, else from the title's first token, else 'crypt'."""
    for b in doc.blocks:
        if "." in b.asset:
            return b.asset.split(".", 1)[0]
    if doc.title:
        return doc.title.split()[0].lower()
    return "crypt"


def _plug_anchor(room: Room, plug: Plug) -> tuple[float, float]:
    """World (x,y) of a plug's doorway centre, on the named room edge midpoint."""
    cx = room.ox + room.w / 2.0
    cy = room.oy + room.h / 2.0
    if plug.edge == "N":
        return cx, room.oy + room.h
    if plug.edge == "S":
        return cx, room.oy
    if plug.edge == "E":
        return room.ox + room.w, cy
    if plug.edge == "W":
        return room.ox, cy
    return cx, cy


def _find_plug(doc: MapDoc, handle: str) -> Optional[tuple[Room, Plug]]:
    room_name, _, plug_name = handle.partition(".")
    room = doc.room(room_name)
    if room is None:
        return None
    for p in doc.plugs:
        if p.room == room_name and p.name == plug_name:
            return room, p
    return None


def plan_greybox(doc: MapDoc) -> list[Piece]:
    """Resolve the whole map to greybox pieces. Deterministic + pure."""
    theme = _theme_of(doc)
    pieces: list[Piece] = []

    def ref(piece: str) -> str:
        return f"{theme}.{piece}"

    # Which (room, edge-segment) cells host a doorway — so we leave a gap in the
    # wall and drop a doorway_frame there instead of a solid panel.
    doorways: dict[str, list[tuple[float, float, str]]] = {}
    for p in doc.plugs:
        room = doc.room(p.room)
        if room is None:
            continue
        ax, ay = _plug_anchor(room, p)
        doorways.setdefault(p.room, []).append((ax, ay, p.edge))

    # ---- Rooms: floor + ceiling tiles, perimeter walls, corners, columns ----
    for room in doc.rooms:
        nx = max(1, round(room.w / TILE_FT))
        ny = max(1, round(room.h / TILE_FT))
        tw = room.w / nx
        th = room.h / ny
        for ix in range(nx):
            for iy in range(ny):
                cx = room.ox + (ix + 0.5) * tw
                cy = room.oy + (iy + 0.5) * th
                pieces.append(Piece(PIECE_FLOOR, ref(PIECE_FLOOR), cx, cy, FLOOR_T / 2.0,
                                    tw, th, FLOOR_T, material=room.material))
                pieces.append(Piece(PIECE_CEILING, ref(PIECE_CEILING), cx, cy, WALL_H - FLOOR_T / 2.0,
                                    tw, th, FLOOR_T, material=room.material))

        room_doors = doorways.get(room.name, [])

        def has_door(edge: str, center: float) -> bool:
            # A wall segment is left OPEN (returns True) iff its span overlaps the
            # corridor CLEARANCE — the CORRIDOR_W-wide band centred on the plug.
            # This makes the opening track the corridor width: a 5 ft corridor
            # opens one 5 ft segment, a 10 ft corridor opens two, so the doorway
            # never necks the corridor down. Strict `<`/`>` so a segment that only
            # touches the clearance boundary stays solid.
            half = (tw if edge in ("N", "S") else th) / 2.0
            clear = CORRIDOR_W / 2.0
            for (dx, dy, dedge) in room_doors:
                if dedge != edge:
                    continue
                pos = dx if edge in ("N", "S") else dy
                if (center + half) > (pos - clear) and (center - half) < (pos + clear):
                    return True
            return False

        # Perimeter wall panels: one per tile edge segment, but the segment whose
        # span contains a door is left OPEN (skipped). The doorway frame itself is
        # NOT placed here — it is placed at the true plug ANCHOR below, so it
        # coincides with the corridor mouth. (Placing it at the segment center
        # detached it from the corridor by up to half a tile on even-tile-count
        # edges — 10/20/40 ft rooms — leaving a visible hole; reviewer SHOULD-FIX 1.)
        wz = WALL_H / 2.0
        for ix in range(nx):
            cx = room.ox + (ix + 0.5) * tw
            for edge, ey in (("S", room.oy), ("N", room.oy + room.h)):
                if not has_door(edge, cx):
                    pieces.append(Piece(PIECE_WALL, ref(PIECE_WALL), cx, ey, wz,
                                        tw, WALL_T, WALL_H, material=room.material))
        for iy in range(ny):
            cy = room.oy + (iy + 0.5) * th
            for edge, ex in (("W", room.ox), ("E", room.ox + room.w)):
                if not has_door(edge, cy):
                    pieces.append(Piece(PIECE_WALL, ref(PIECE_WALL), ex, cy, wz,
                                        WALL_T, th, WALL_H, material=room.material))

        # Doorway frames: one per plug, AT the plug anchor (= the corridor mouth),
        # oriented by edge. Width DOOR_W < tile, so a frame reveal remains either
        # side of the opening left by the skipped wall segment.
        for (dx, dy, dedge) in room_doors:
            if dedge in ("N", "S"):
                pieces.append(Piece(PIECE_DOORWAY, ref(PIECE_DOORWAY), dx, dy, wz,
                                    DOOR_W, WALL_T, WALL_H, material=room.material))
            else:
                pieces.append(Piece(PIECE_DOORWAY, ref(PIECE_DOORWAY), dx, dy, wz,
                                    WALL_T, DOOR_W, WALL_H, material=room.material))

        # Four wall corners.
        for (cxr, cyr) in ((room.ox, room.oy), (room.ox + room.w, room.oy),
                           (room.ox, room.oy + room.h), (room.ox + room.w, room.oy + room.h)):
            pieces.append(Piece(PIECE_CORNER, ref(PIECE_CORNER), cxr, cyr, wz,
                                WALL_T * 1.6, WALL_T * 1.6, WALL_H, material=room.material))

        # One free-standing column, inset from a corner (CORNER_MOUNT greybox).
        pieces.append(Piece(PIECE_COLUMN, ref(PIECE_COLUMN),
                            room.ox + tw, room.oy + th, WALL_H / 2.0,
                            1.4, 1.4, WALL_H, material=room.material, shape="cylinder"))

    # ---- Connections: route an axis-aligned L corridor between the plugs ----
    for conn in doc.connections:
        a = _find_plug(doc, conn.src)
        b = _find_plug(doc, conn.dst)
        if a is None or b is None:
            continue
        (ra, pa), (rb, pb) = a, b
        ax, ay = _plug_anchor(ra, pa)
        bx, by = _plug_anchor(rb, pb)
        pieces.extend(_route_corridor(ref, ax, ay, bx, by))

    # ---- Blocks: props (sarcophagus / brazier / stair / ...) ----------------
    for blk in doc.blocks:
        piece = blk.asset.split(".", 1)[1] if "." in blk.asset else blk.asset
        s = max(0.1, blk.scale)
        if piece == PIECE_BRAZIER:
            # Brazier: a short pedestal cylinder PLUS the room's only light.
            pieces.append(Piece(PIECE_BRAZIER, blk.asset, blk.x, blk.y, 2.0 * s,
                                2.0 * s, 2.0 * s, 4.0 * s, rot_deg=blk.rotation,
                                material="bronze", shape="cylinder"))
            pieces.append(Piece(PIECE_BRAZIER, blk.asset, blk.x, blk.y, 5.0 * s,
                                0.0, 0.0, 0.0, rot_deg=blk.rotation,
                                is_light=True, light_energy=18000.0 * s))
        elif piece == PIECE_SARCOPHAGUS:
            pieces.append(Piece(PIECE_SARCOPHAGUS, blk.asset, blk.x, blk.y, 2.0 * s,
                                7.0 * s, 3.0 * s, 4.0 * s, rot_deg=blk.rotation,
                                material="marble"))
        elif piece == PIECE_STAIR:
            # STAIR (+1 band): a stepped block placed at a threshold.
            pieces.append(Piece(PIECE_STAIR, blk.asset, blk.x, blk.y, 1.5 * s,
                                CORRIDOR_W, CORRIDOR_W, 3.0 * s, rot_deg=blk.rotation,
                                material="stone"))
        else:
            # Unknown prop: a unit greybox cube so it still appears (and logs).
            pieces.append(Piece("prop", blk.asset, blk.x, blk.y, 2.0 * s,
                                3.0 * s, 3.0 * s, 4.0 * s, rot_deg=blk.rotation,
                                material="stone"))

    return pieces


def _route_corridor(ref, ax: float, ay: float, bx: float, by: float) -> list[Piece]:
    """Lay a one-tile-wide L corridor: a horizontal leg then a vertical leg, with
    a corridor_l piece at the single bend and a corridor end_cap at each plug.
    A degenerate (already-colinear) route lays only straight pieces. Floor +
    flanking low walls per leg tile.

    GREYBOX LIMIT (reviewer SHOULD-FIX 3): this is a naive single-bend L between
    two edge-midpoint anchors. It assumes the plugs roughly FACE each other; for
    plugs that face away (or rooms that overlap the route) the tiles can run
    through a room interior. Real obstacle-aware corridor ROUTING is dungeon-map's
    domain (Seam B/C) — the realizer only greyboxes whatever route it is handed.
    For M0 the crypt's plugs face across the gap, so the L is clean."""
    out: list[Piece] = []
    bend_x, bend_y = bx, ay  # turn at (end.x, start.y)

    def lay_leg(x0, y0, x1, y1):
        horiz = abs(x1 - x0) >= abs(y1 - y0)
        length = abs(x1 - x0) if horiz else abs(y1 - y0)
        if length < 0.5:
            return
        steps = max(1, round(length / TILE_FT))
        for s in range(steps):
            t0 = (s + 0.5) / steps
            cx = x0 + (x1 - x0) * t0
            cy = y0 + (y1 - y0) * t0
            seg = TILE_FT
            sx, sy = (seg, CORRIDOR_W) if horiz else (CORRIDOR_W, seg)
            out.append(Piece(PIECE_CORRIDOR, ref(PIECE_CORRIDOR), cx, cy, FLOOR_T / 2.0,
                             sx, sy, FLOOR_T, material="stone"))
            # flanking low walls
            if horiz:
                for off in (-CORRIDOR_W / 2.0, CORRIDOR_W / 2.0):
                    out.append(Piece(PIECE_CORRIDOR, ref(PIECE_CORRIDOR), cx, cy + off, WALL_H / 2.0,
                                     seg, WALL_T, WALL_H, material="stone"))
            else:
                for off in (-CORRIDOR_W / 2.0, CORRIDOR_W / 2.0):
                    out.append(Piece(PIECE_CORRIDOR, ref(PIECE_CORRIDOR), cx + off, cy, WALL_H / 2.0,
                                     WALL_T, seg, WALL_H, material="stone"))

    lay_leg(ax, ay, bend_x, bend_y)
    lay_leg(bend_x, bend_y, bx, by)

    # bend piece (only if the route actually turns)
    if abs(bx - ax) > 0.5 and abs(by - ay) > 0.5:
        out.append(Piece(PIECE_CORRIDOR_L, ref(PIECE_CORRIDOR_L), bend_x, bend_y, FLOOR_T / 2.0,
                         CORRIDOR_W, CORRIDOR_W, FLOOR_T, material="stone"))

    # end caps at both plug mouths
    for (ex, ey) in ((ax, ay), (bx, by)):
        out.append(Piece(PIECE_ENDCAP, ref(PIECE_ENDCAP), ex, ey, WALL_H / 2.0,
                         CORRIDOR_W * 0.4, CORRIDOR_W * 0.4, WALL_H, material="stone"))
    return out


# =============================================================================
# OBJ proof tier (no bpy) — deterministic, what CI / the reviewer asserts
# =============================================================================
def pieces_to_obj(pieces: list[Piece]) -> str:
    """Emit the greybox as a Wavefront OBJ: one axis-aligned box per piece
    (cylinders are approximated by their bounding box — the proof is placement,
    not roundness). Lights are skipped (no geometry)."""
    lines = ["# edi_realize greybox proof", "# one box per resolved piece"]
    vbase = 1
    for idx, p in enumerate(pieces):
        if p.is_light or (p.sx == 0 and p.sy == 0 and p.sz == 0):
            continue
        hx, hy, hz = p.sx / 2.0, p.sy / 2.0, p.sz / 2.0
        corners = [
            (p.x - hx, p.y - hy, p.z - hz), (p.x + hx, p.y - hy, p.z - hz),
            (p.x + hx, p.y + hy, p.z - hz), (p.x - hx, p.y + hy, p.z - hz),
            (p.x - hx, p.y - hy, p.z + hz), (p.x + hx, p.y - hy, p.z + hz),
            (p.x + hx, p.y + hy, p.z + hz), (p.x - hx, p.y + hy, p.z + hz),
        ]
        lines.append(f"o {p.kind}_{idx}_{p.asset_ref.replace('.', '_')}")
        for (vx, vy, vz) in corners:
            lines.append(f"v {vx:.4f} {vy:.4f} {vz:.4f}")
        faces = [(1, 2, 3, 4), (5, 6, 7, 8), (1, 2, 6, 5),
                 (2, 3, 7, 6), (3, 4, 8, 7), (4, 1, 5, 8)]
        for (a, b, c, d) in faces:
            lines.append(f"f {vbase+a-1} {vbase+b-1} {vbase+c-1} {vbase+d-1}")
        vbase += 8
    return "\n".join(lines) + "\n"


def plan_summary(doc: MapDoc, pieces: list[Piece]) -> str:
    """A human/log summary: piece-type counts + the gate checklist."""
    counts: dict[str, int] = {}
    for p in pieces:
        counts[p.kind] = counts.get(p.kind, 0) + 1
    lights = sum(1 for p in pieces if p.is_light)
    present = [t for t in TEN_PIECE_TYPES if counts.get(t, 0) > 0]
    out = [
        f"map: {doc.title!r}  units={doc.units}",
        f"rooms={len(doc.rooms)} connections={len(doc.connections)} blocks={len(doc.blocks)}",
        f"pieces={len(pieces)} lights={lights}",
        "piece-type counts:",
    ]
    for k in sorted(counts):
        out.append(f"  {k:18s} {counts[k]}")
    out.append(f"structural piece types present: {len(present)}/10  -> {present}")
    missing = [t for t in TEN_PIECE_TYPES if t not in present]
    if missing:
        out.append(f"MISSING piece types: {missing}")
    return "\n".join(out)


# =============================================================================
# Blender tier (bpy) — build + Cycles OptiX render
# =============================================================================
GREY_MATERIALS = {
    # name: (r,g,b,a) base color
    "stone": (0.46, 0.45, 0.42, 1.0),
    "marble": (0.80, 0.78, 0.74, 1.0),
    "bronze": (0.45, 0.30, 0.12, 1.0),
}


def _bpy_material(bpy, key: str):
    name = f"grey_{key}"
    mat = bpy.data.materials.get(name)
    if mat:
        return mat
    mat = bpy.data.materials.new(name)
    mat.use_nodes = True
    color = GREY_MATERIALS.get(key, GREY_MATERIALS["stone"])
    bsdf = mat.node_tree.nodes.get("Principled BSDF")
    if bsdf:
        bsdf.inputs["Base Color"].default_value = color
        bsdf.inputs["Roughness"].default_value = 0.8
    mat.diffuse_color = color
    return mat


def build_scene(bpy, pieces: list[Piece]):  # pragma: no cover — needs Blender
    """Instantiate every piece as a primitive. Returns the world-space bounds
    (minx,miny,minz,maxx,maxy,maxz) of the non-light geometry for camera framing."""
    # Clear the default scene.
    if bpy.context.object and bpy.context.object.mode != "OBJECT":
        bpy.ops.object.mode_set(mode="OBJECT")
    bpy.ops.object.select_all(action="SELECT")
    bpy.ops.object.delete()

    bb = [math.inf, math.inf, math.inf, -math.inf, -math.inf, -math.inf]

    def grow(x, y, z, sx, sy, sz):
        bb[0] = min(bb[0], x - sx / 2.0); bb[1] = min(bb[1], y - sy / 2.0); bb[2] = min(bb[2], z - sz / 2.0)
        bb[3] = max(bb[3], x + sx / 2.0); bb[4] = max(bb[4], y + sy / 2.0); bb[5] = max(bb[5], z + sz / 2.0)

    for p in pieces:
        if p.is_light:
            light_data = bpy.data.lights.new(name=f"{p.kind}_light", type="POINT")
            light_data.energy = p.light_energy
            light_data.color = (1.0, 0.7, 0.4)   # warm brazier glow
            light_data.shadow_soft_size = 0.6
            light_obj = bpy.data.objects.new(f"{p.kind}_light", light_data)
            light_obj.location = (p.x, p.y, p.z)
            bpy.context.collection.objects.link(light_obj)
            continue

        if p.shape == "cylinder":
            bpy.ops.mesh.primitive_cylinder_add(vertices=24, radius=0.5, depth=1.0,
                                                 location=(p.x, p.y, p.z))
        else:
            bpy.ops.mesh.primitive_cube_add(size=1.0, location=(p.x, p.y, p.z))
        obj = bpy.context.object
        obj.name = f"{p.kind}_{p.asset_ref}"
        obj.dimensions = (max(p.sx, 0.01), max(p.sy, 0.01), max(p.sz, 0.01))
        bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
        if p.rot_deg:
            obj.rotation_euler = (0.0, 0.0, math.radians(p.rot_deg))
        obj.data.materials.append(_bpy_material(bpy, p.material))
        # Dollhouse cutaway: the ceiling is fully RESOLVED + instantiated (the
        # seam proves all 10 piece types) but hidden from the render camera, so a
        # high 3/4 shot looks INTO the lit rooms instead of onto the roof. It
        # still occludes nothing it shouldn't — it just isn't traced.
        if p.kind == PIECE_CEILING:
            obj.hide_render = True
        grow(p.x, p.y, p.z, p.sx, p.sy, p.sz)

    # A faint neutral world ambient so geometry outside the brazier's throw still
    # reads (fill only — the brazier remains the sole light OBJECT, per the gate).
    world = bpy.context.scene.world or bpy.data.worlds.new("World")
    bpy.context.scene.world = world
    world.use_nodes = True
    bg = world.node_tree.nodes.get("Background")
    if bg:
        bg.inputs["Color"].default_value = (0.05, 0.06, 0.09, 1.0)  # cool moonlight
        bg.inputs["Strength"].default_value = 0.5

    return tuple(bb)


def setup_camera(bpy, bb):  # pragma: no cover — needs Blender
    """A 3/4 perspective looking down into the crypt, framing the whole bounds."""
    cx = (bb[0] + bb[3]) / 2.0
    cy = (bb[1] + bb[4]) / 2.0
    cz = (bb[2] + bb[5]) / 2.0
    span = max(bb[3] - bb[0], bb[4] - bb[1], bb[5] - bb[2], 1.0)

    cam_data = bpy.data.cameras.new("realize_cam")
    cam = bpy.data.objects.new("realize_cam", cam_data)
    bpy.context.collection.objects.link(cam)
    # Steep 3/4 dollhouse: high above and pulled back along -Y, aimed at centre.
    cam.location = (cx + span * 0.35, cy - span * 0.9, cz + span * 1.05)
    direction = (cx - cam.location[0], cy - cam.location[1], cz - cam.location[2])
    # Point the camera: use a track-to via rotation from the direction vector.
    import mathutils
    rot = mathutils.Vector(direction).to_track_quat("-Z", "Y").to_euler()
    cam.rotation_euler = rot
    bpy.context.scene.camera = cam
    return cam


def setup_optix(bpy, samples: int):  # pragma: no cover — needs Blender
    """Configure Cycles + OptiX + the GPU. RAISES if no OptiX/CUDA GPU device is
    available (we refuse the silent CPU fallback the gate forbids). Returns a
    list of (name, type, enabled) tuples for the render log."""
    scene = bpy.context.scene
    scene.render.engine = "CYCLES"

    prefs = bpy.context.preferences.addons["cycles"].preferences
    # Prefer OptiX; fall back to CUDA only if OptiX yields no devices.
    chosen_type = None
    for cand in ("OPTIX", "CUDA"):
        prefs.compute_device_type = cand
        try:
            prefs.get_devices()
        except Exception:
            pass
        gpu = [d for d in prefs.devices if d.type == cand]
        if gpu:
            chosen_type = cand
            break
    if chosen_type is None:
        raise RuntimeError("no OptiX/CUDA GPU device found — refusing CPU fallback")

    prefs.compute_device_type = chosen_type
    scene.cycles.device = "GPU"

    report = []
    any_gpu_enabled = False
    for d in prefs.devices:
        enable = (d.type == chosen_type)
        d.use = enable
        any_gpu_enabled = any_gpu_enabled or enable
        report.append((d.name, d.type, enable))
    if not any_gpu_enabled:
        raise RuntimeError("GPU device present but none enabled — refusing CPU fallback")

    scene.cycles.samples = samples
    scene.cycles.use_denoising = True
    scene.render.resolution_x = 1920
    scene.render.resolution_y = 1080
    scene.render.resolution_percentage = 100
    return chosen_type, report


def render(bpy, out_png: str):  # pragma: no cover — needs Blender
    scene = bpy.context.scene
    scene.render.image_settings.file_format = "PNG"
    scene.render.filepath = out_png
    bpy.ops.render.render(write_still=True)


def run_in_blender(argv: list[str]) -> int:  # pragma: no cover — needs Blender
    import time
    import bpy

    map_path = next((a for a in argv if not a.startswith("--")), None)
    out_png = next((a.split("=", 1)[1] for a in argv if a.startswith("--render=")), None)
    samples = int(next((a.split("=", 1)[1] for a in argv if a.startswith("--samples=")), "64"))
    if not map_path or not out_png:
        sys.stderr.write("usage: blender -b -P edi_realize.py -- <map.toon> --render=<out.png> [--samples=N]\n")
        return 2

    with open(map_path, "r", encoding="utf-8") as fh:
        doc = parse_toon(fh.read())
    pieces = plan_greybox(doc)

    print("=" * 64)
    print("edi_realize — M0 crypt realizer")
    print(f"blender version: {bpy.app.version_string}")
    print(plan_summary(doc, pieces))
    print("=" * 64)

    bb = build_scene(bpy, pieces)
    setup_camera(bpy, bb)
    chosen, devreport = setup_optix(bpy, samples)
    print(f"render engine: CYCLES   compute_device_type: {chosen}   cycles.device: GPU")
    print(f"samples: {samples}   resolution: 1920x1080")
    print("cycles devices:")
    for (name, dtype, enabled) in devreport:
        print(f"  [{'X' if enabled else ' '}] {dtype:6s} {name}")
    gpu_enabled = [n for (n, t, e) in devreport if e and t in ("OPTIX", "CUDA")]
    if not gpu_enabled:
        sys.stderr.write("FATAL: no GPU device enabled — would fall back to CPU\n")
        return 3
    print(f"GPU CONFIRMED: rendering on {chosen} device(s): {gpu_enabled}")

    t0 = time.time()
    render(bpy, out_png)
    dt = time.time() - t0
    print(f"render complete: {out_png}   wall-time: {dt:.1f}s")
    print(f"GATE timing: {'PASS' if dt < 120 else 'FAIL'} (<120s)  actual {dt:.1f}s")
    print("=" * 64)
    return 0


# =============================================================================
# Entry point
# =============================================================================
def main(argv: list[str]) -> int:
    # Inside Blender, script args follow "--"; standalone they are argv[1:].
    in_blender = False
    try:
        import bpy  # noqa: F401
        in_blender = True
    except Exception:
        in_blender = False

    if "--" in argv:
        argv = argv[argv.index("--") + 1:]
    else:
        argv = argv[1:]

    if in_blender and any(a.startswith("--render=") for a in argv):
        return run_in_blender(argv)

    # Pure tier (no Blender): parse + plan + (--obj-out / summary).
    obj_out = next((a.split("=", 1)[1] for a in argv if a.startswith("--obj-out=")), None)
    paths = [a for a in argv if not a.startswith("--")]
    if not paths:
        sys.stderr.write(
            "usage:\n"
            "  edi_realize.py <map.toon> [--obj-out=out.obj]        # pure proof\n"
            "  blender -b -P edi_realize.py -- <map.toon> --render=out.png [--samples=N]\n")
        return 2
    with open(paths[0], "r", encoding="utf-8") as fh:
        doc = parse_toon(fh.read())
    pieces = plan_greybox(doc)
    print(plan_summary(doc, pieces))
    if obj_out:
        with open(obj_out, "w", encoding="utf-8") as fh:
            fh.write(pieces_to_obj(pieces))
        print(f"wrote OBJ proof: {obj_out}  ({len(pieces)} pieces)")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
