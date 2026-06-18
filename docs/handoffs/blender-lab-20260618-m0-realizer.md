# Campaign — blender-lab-20260618-m0-realizer (M0 crypt hardware gate)

**Owner:** edi-blender-lab (hub-driven; render-iterate loop needs live eyes on the PNG).
**Source brief:** `~/dept-bus/M0-CRYPT-SLICE.md`.

## The deliverable (this department's half of M0)
A standalone **bpy REALIZER** that reads the Seam-B TOON map, resolves
socket + asset_ref to **GREYBOX** placeholder meshes (one primitive per piece
type), snaps them on the 5 ft grid, sets the brazier as a light, and renders
**Cycles OptiX on the RTX 5090** at 1080p. Plus: **re-verify the edi_craft bpy
seam on Blender 4.5.9** (was authored vs 5.1.1 — fix API drift).

### GATE (PASS = all true)
1. Valid 1080p PNG of the assembled crypt: 2 rooms + 1 corridor + the 10 piece
   types + 2 props (sarcophagus, brazier) + 1 light.
2. Rendered on the 5090 via Cycles OptiX — GPU device CONFIRMED in the render
   log (NOT CPU fallback).
3. Under 2 minutes, within 32 GB VRAM.
4. Whole chain runs from ONE command: TOON -> realizer -> PNG.

## Environment (confirmed 2026-06-18)
- Blender: `~/.local/bin/blender` → **Blender 4.5.9 LTS** (build 2026-04-21).
- GPU: **NVIDIA GeForce RTX 5090, 32607 MiB, driver 595.80**.

## The Seam-B TOON grammar (what the realizer parses) — `src/io/MapToonExport.cpp`
```
kind: map
title: <str>
units: <str>

rooms[N]{name,origin,size,material}:
  <name>,"<x>,<y>","<w>,<h>",<material>

plugs[N]{room,name,edge,type,connected,flags}:        # flags column is NEWER; older maps omit it
  <room>,<name>,<N|E|S|W>,<type>,<true|false>,<flags>  # flags = ·-joined tags (U+00B7)

connections[N]{from,to,type}:
  <fromRoom.fromPlug>,<toRoom.toPlug>,<type>

blocks[N]{room,asset,origin,scale,rotation}:           # NEWER; older maps omit the whole section
  <room>,<asset_ref>,"<x>,<y>",<scale>,<rotation>
```
Quoting: a cell is quoted only if it holds `, " space tab newline`; coordinate
pairs `x,y` are therefore quoted. The realizer's parser must tolerate the
optional `flags` column and optional `blocks[]` section (the older sample maps
under `tests/data/maps/*.toon` have neither).

## M0 SOCKET CONTRACT (from the brief; sync final from dungeon-map reviewer gate)
- Grid module = 1 tile = 5 ft; min-corner origin. DOOR_PLUG sized to
  kCorridorWidth so plug -> doorway -> corridor line up. asset_ref =
  `<theme>.<piece>` (e.g. `crypt.floor_tile`).
- 10 piece types + sockets:
  FLOOR_tile (4x FLOOR_EDGE) · WALL_panel (WALL_MOUNT L/R + TOP + BASE) ·
  WALL_corner inner/outer · DOORWAY_frame (DOOR_PLUG) + END_CAP ·
  CORRIDOR straight + L (CORRIDOR_END x2) · CEILING_slab (CEILING_MOUNT) ·
  COLUMN (CORNER_MOUNT) · STAIR (STAIR_LEVEL, +1 band) ·
  sarcophagus (PROP_FLOOR) · brazier (PROP_FLOOR + light).

## Plan (gates)
- [x] G0 env recon — Blender 4.5.9 + 5090 confirmed.
- [x] G1 realizer scaffold — `tools/blender/edi_realize.py` pure tier (parse +
      greybox plan + OBJ proof) + `samples/crypt_m0/crypt.toon`. Resolves all 10
      structural piece types + 2 props + 1 light; tolerates old/new TOON shape.
- [x] G2 bpy build + OptiX render — crypt rendered 1080p, OptiX/5090 CONFIRMED,
      3.4 s, ~1.5 GB peak. Evidence: `samples/crypt_m0/crypt.png` + `render.log`.
- [x] G3 re-verify edi_craft bpy seam on 4.5.9 — `build()` runs clean ("built 9
      ops"), NO API drift. Cross-language checks (obj-out/list-craftsmen/smoke) green.
- [x] G4 reviewer gate — NO blockers; TOON parse byte-faithful to MapToonExport.cpp.
      3 SHOULD-FIX addressed: (1) doorway frame now placed at the plug anchor (was
      detaching by half a tile on even-tile-count edges); (2) stock-map 8/10 reality
      pinned + gate claim re-scoped (socket table covers 10, crypt fixture yields 10);
      (3) L-router greybox limit documented (real routing is dungeon-map's domain).
- [ ] G5 closeout + rebase LOCAL master + edi-gate (build/ctest/scan green).

## GATE EVIDENCE (PASS)
- PNG: `samples/crypt_m0/crypt.png` — 1920×1080, valid crypt: 2 rooms +
  L-corridor + 10 structural piece types + sarcophagus + brazier + brazier light.
- Log: `samples/crypt_m0/render.log` — "compute_device_type: OPTIX / cycles.device:
  GPU", "GPU CONFIRMED: rendering on OPTIX device(s): ['NVIDIA GeForce RTX 5090']",
  wall-time 3.4 s (<120 s), peak ~1.5 GB (<32 GB).
- One command: `blender --background --python tools/blender/edi_realize.py --
  <map.toon> --render=<png>`.

## Contract sync note
Built against the spec socket contract (dungeon-map had NOT yet published a
reviewer-gate contract — they were still at their prior closeout). The realizer's
`parse_toon` was audited byte-for-byte against the live writer `src/io/
MapToonExport.cpp`. When dungeon-map emits the real crypt TOON, drop it in place
of `samples/crypt_m0/crypt.toon`; the parser already tolerates the writer's exact
shape (quoting, `·`-joined flags U+00B7, optional flags column + blocks section).

## Status log
- 2026-06-18 — campaign opened; env confirmed (Blender 4.5.9 + RTX 5090).
- 2026-06-18 — G1–G3 built + verified; crypt rendered on OptiX/5090 in 3.4 s.
- 2026-06-18 — G4 reviewer gate: no blockers, 3 SHOULD-FIX applied; green gate
  105/105 + scan clean. Proceeding to closeout + master integration.
</content>
</invoke>
