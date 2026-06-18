# M0 SOCKET CONTRACT — the generator ↔ realizer interface (SETTLED v1)

**Status:** SETTLED — dungeon-map reviewer gate (reply 039) folded; reconciled
against the ALREADY-MERGED realizer (`tools/blender/edi_realize.py`, master ac3bf96)
and the live exporter (`src/io/MapToonExport.cpp`). Frozen and bus-hub'd to
blender-lab + the hub. This is the interface the M0 generator, the M0 bpy realizer,
and the user's future real meshes all agree on.

**Owner:** edi-dungeon-map. Layered law holds: **edi RECORDS geometry + neutral
tags; it assigns no game rules.** The realizer is a *renderer* — it deterministically
expands the neutral map into greybox meshes; it does not invent rules.

**Binding split (the reviewer's ownership ruling):** edi BINDS only **§0 (the wire
arrays) + §1 (units / grid / frame / plug-position rule)**. **§4 piece types, §5
greybox constants, and the socket snap rules are REALIZER-AUTHORITATIVE** — they are
deliberately NOT in the TOON and edi neither stores nor verifies them (edi does not
enforce WALL_H). Their values here are reference, recorded from the merged realizer.

---

## 0. What crosses the seam (the wire) — BINDING

The generator emits ONE neutral TOON map — the existing
`edi::io::exportMapToToon(const DraftingDocument&, …)` output (the document
projection that carries placed blocks). The realizer reads these arrays:

```
rooms[N]{name,origin,size,material}              # footprint, AUTHORED FEET
plugs[M]{room,name,edge,type,connected,flags}    # 6 columns — see below
connections[K]{from,to,type}                     # adjacency; from/to = "room.plug"
blocks[B]{room,asset,origin,scale,rotation}      # placed instances, asset_ref carried
```

- `origin` = (x,y) feet. For a **room** = NW / min-corner. For a **block** = the
  placement CENTRE. `size` = (width,height) feet.
- **`edge`** is a BARE single letter ∈ **{N, E, S, W, ?}** (`?` = no edge derivable).
  NOT a word. Emitted by `edgeName`/`deriveEdge` (`MapToonExport.cpp:30`).
- **`flags`** (the 6th plug column) = `·`-joined neutral open-vocabulary tags from
  `DraftingPlug.flags`; missing/empty ⇒ empty cell. edi RECORDS them, assigns no
  rule meaning. (This is an existing neutral-tag channel on the wire.)
- `connected` ∈ {true,false} — derived (a plug is connected iff a connection names it).
- `scale`/`rotation` per-instance (1 / 0 for M0 — translate-only).
- `asset` = the block's `assetRef`, verbatim (§2). The realizer tolerates empty cells
  and the OLDER format (no `flags`, no `blocks`).

**No new columns for M0.** The blocks export is NOT extended with a tags column
(§8). asset_ref + plug `type`/`flags` + connection `type` carry every neutral tag the
realizer needs.

---

## 1. Coordinate frame, grid, & the plug-position rule — BINDING

- **Unit = FEET.** `units=feet`; numbers are authored feet (export at
  `canvasPerAuthoredUnit = 1.0` so footprints stay in feet).
- **Grid module = 1 tile = 5 ft.** GENERATOR DISCIPLINE: the generator authors every
  room origin and size as an integer multiple of 5 ft, axis-aligned. Neither the
  parser nor createMapFromSpec ENFORCES this — it is a contract obligation on the
  generator. The export reproduces whatever multiples the generator authors.
- **Min-corner origin.** A room `origin` is its NW (min-x, min-y) corner.
- **2D wire frame:** +x = east, +y = south (edi's authored 2D frame). This is the
  frame of every number on the wire.
- **2D→3D (realizer):** map-x → world X, map-y → world Y, **+Z up** (Blender-native).
  NORMATIVE — the realizer CONFIRMS on the bus, it does not renegotiate. (Right-handed,
  consistent with the merged realizer.)
- **PLUG POSITION RULE (resolves reviewer Blocker B):** plug position is NOT on the
  wire — the plugs array carries only `edge`. The realizer derives a plug's doorway
  centre as the **MIDPOINT of its named room edge** (`_plug_anchor`, edi_realize.py).
  The generator authors each M0 plug at its edge midpoint to match. A connection
  routes a **straight** corridor when the two plug midpoints are colinear, else **one
  L bend** (the realizer handles both: `corridor_straight` / `corridor_l`). So the
  realizer reproduces every doorway/corridor centerline from room footprint + plug
  edge alone — no coordinate is transmitted, and none needs to be for M0.
  *(Post-M0, if multi-plug edges need more than a midpoint, add an additive `at`
  offset column — deferred, §Defer.)*

---

## 2. asset_ref naming — BINDING (the string), realizer owns the mesh table

- Form: **`<theme>.<piece>`**, lowercase snake. M0 theme = `crypt`. The realizer reads
  the theme from the prefix (`crypt.`), the piece from the suffix; it owns the
  asset_ref → mesh table. A real artist mesh later swaps in by asset_ref.
- **PROPS and STAIR ride as block asset_refs** in `blocks{asset}`. The TOON carries no
  elevation/stair data to expand structurally, so a stair is delivered as a placed
  block `crypt.stair` (the merged sample does exactly this), alongside
  `crypt.sarcophagus` / `crypt.brazier`.
- **Structural pieces are EXPANDED from the graph, NOT asset_refs, NOT on the wire:**
  the realizer derives floor / wall / corner / doorway / endcap / corridor / ceiling /
  column from the room footprints + plugs + connections. This is the split: **structure
  is expanded from the graph; props + stairs arrive as blocks.**

---

## 3. The M0 crypt the generator hardcodes (the slice)

- **2 rooms**, 5 ft-aligned: entrance chamber + crypt chamber. RECOMMENDED:
  entrance 15×15 @ (0,5), crypt 25×25 @ (35,0) — facing-edge midpoints colinear at
  world y=12.5 ⇒ a STRAIGHT corridor (see §1 plug-position rule).
- **1 plug per room** at the facing-edge midpoint (entrance East, crypt West) +
  **1 connection** (type "corridor") joining them.
- **2 block instances** in the crypt chamber: `crypt.sarcophagus` (a box prop) and
  `crypt.brazier` (a prop + the scene light). Identity transform (scale 1, rot 0).
- Hand-authored, not generated — proves the seam, not an algorithm.

---

## 4. The piece types + sockets (REALIZER-AUTHORITATIVE vocabulary)

Recorded from the merged realizer; edi does NOT store or check these. Triggered by:

| Piece | Triggered by | Delivery |
| --- | --- | --- |
| floor_tile | each 5 ft cell of a room footprint | graph-expanded |
| wall_panel | each 5 ft perimeter segment, non-doorway | graph-expanded |
| wall_corner | each room corner | graph-expanded |
| doorway_frame | each connected plug (edge midpoint) | graph-expanded |
| endcap | each UNconnected plug | graph-expanded |
| corridor_straight / corridor_l | each connection (straight if colinear, else L) | graph-expanded |
| ceiling_slab | each floor cell | graph-expanded |
| column | room corners (realizer's call) | graph-expanded |
| stair | a `<theme>.stair` block | **block asset_ref** |
| sarcophagus / brazier (PROP + light) | `<theme>.sarcophagus` / `.brazier` blocks | **block asset_ref** |

A `connected=true` plug ⇒ an OPEN doorway_frame; `connected=false` ⇒ a sealed endcap.
The brazier is the only light in M0.

---

## 5. Greybox constants (REALIZER-AUTHORITATIVE reference — edi does NOT enforce)

Recorded from `tools/blender/edi_realize.py` (master). These are the realizer's
greybox envelope; the user's future real meshes fit the same socket envelope so
greybox and art are swap-compatible.

| Const | Value (ft) | Meaning |
| --- | --- | --- |
| TILE / MODULE | 5 | grid tile = floor cell = wall-panel length |
| WALL_H | 12 | interior wall / ceiling height |
| CORRIDOR_W | 5 | corridor width = one tile (DOOR_PLUG sized to a tile) |
| DOOR_W | 4 | doorway clear opening (< CORRIDOR_W, leaves a frame) |

(edi's 2D-draw `kCorridorWidth = 0.045` is a CANVAS constant, never exported and
unrelated to these — see §6.)

---

## 6. The DOOR_PLUG alignment invariant — BINDING part is the §1 rule only

The only WIRE invariant is the §1 plug-position rule: plug = edge midpoint; the
realizer derives doorway + corridor centerlines from it; the generator authors plugs
at edge midpoints to match. `CORRIDOR_W`/`DOOR_W` are the realizer's expansion widths
(§5), independent of edi's 2D-draw `kCorridorWidth`. **There is NO unit equivalence
between DOOR_W and `kCorridorWidth`** — `kCorridorWidth = 0.045` is a canvas constant
that does not scale and is never exported (it would read 0.045 ft at export scale, 2.25
ft at the 0.02 render scale — neither is a tile). Disregard the earlier "DOOR_PLUG
sized to kCorridorWidth" framing; the realizer sizes DOOR_PLUG to one TILE on its side.

---

## 7. Determinism — BINDING

Given the same TOON, the realizer produces the same mesh layout. No randomness, no
rule inference. Tiling order, wall winding, corner choice, and corridor straight-vs-L
are functions of footprint + grid + the §1 midpoint rule only.

---

## 8. Neutral-tags scope — RATIFIED (no blocks tags column for M0)

The wire already carries neutral tags via plug `flags` (§0), plug/connection `type`,
and the block `asset` string (`crypt.sarcophagus`). No `tags` column is added to
`blocks[]` for M0. A future blocks tags column is additive/tolerant (missing ⇒
default, no version bump) and lands only if a realizer need appears. RATIFIED.

---

## Confirmation asks for blender-lab (bus, not renegotiation)
1. Confirm the 2D→3D mapping (§1: map-x→X, map-y→Y, +Z up) matches the merged
   `edi_realize.py`.
2. Confirm `crypt.stair` as a block asset_ref (§2/§4) is the realizer's stair path.
3. Confirm the generator's straight-corridor crypt (§3) renders — the merged sample
   used an L; the generator's first proof is straight.

## Deferred (additive, post-M0)
- A blocks `tags` column (only on a future realizer need).
- A plug `at`/offset column (only if the midpoint rule proves too rigid for multi-plug
  edges in M1+).
