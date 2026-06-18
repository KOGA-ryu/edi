# M0 SOCKET CONTRACT — the generator ↔ realizer interface (PROPOSAL v0)

**Status:** PROPOSAL — pending the dungeon-map reviewer gate, then bus-hub'd to
blender-lab (the realizer) + the hub. Once settled, this file is the frozen
interface the M0 generator, the M0 bpy realizer, AND the user's future real
meshes all agree on.

**Owner:** edi-dungeon-map (this department owns the contract; blender-lab depends
on it). Layered law holds: **edi RECORDS geometry + neutral tags; it assigns no
game rules.** The realizer is a *renderer*, not a rule engine — it deterministically
expands the neutral map into greybox meshes; it does not invent passability/AI/etc.

Companion: `~/dept-bus/M0-CRYPT-SLICE.md` (the milestone), the M0 SOCKET CONTRACT
section therein (the seed this hardens).

---

## 0. What crosses the seam (the wire)

The generator emits ONE neutral TOON map document — byte-for-byte the existing
`edi::io::exportMapToToon(const DraftingDocument&, …)` output (Seam C, the
document projection that carries placed blocks). The realizer reads exactly these
four flat tabular arrays, no more:

```
rooms[N]{name,origin,size,material}        # footprint, AUTHORED FEET
plugs[M]{room,name,edge,type,connected}    # door/portal sockets; connected derived
connections[K]{from,to,type}               # adjacency graph; from/to = "room.plug"
blocks[B]{room,asset,origin,scale,rotation}# placed PROP instances, asset_ref carried
```

- `origin` = (x,y) feet; for a room it is the **NW / min-corner**; for a block it
  is the placement **centre**. `size` = (width,height) feet.
- `edge` ∈ {north,east,south,west}. `connected` ∈ {true,false}.
- `scale`/`rotation` are per-instance (1 / 0 for M0 — placement is translate-only).
- `asset` = the block's `assetRef`, verbatim (see §2). Empty cells tolerated.

**No new columns for M0.** The blocks export is NOT extended with a tags column
(see §8) — the `asset` string + the plug/connection `type` already carry every
neutral tag the realizer needs. This keeps the wire byte-stable.

---

## 1. Coordinate frame & grid (the hard invariants)

- **Unit = FEET.** TOON `units=feet`; numbers are authored feet (parsed/exported at
  `canvasPerAuthoredUnit = 1.0`).
- **Grid module = 1 tile = 5 ft.** Every room origin and size is an integer
  multiple of 5 ft; rooms are axis-aligned. The generator MUST honor this so the
  realizer can tile cleanly.
- **Min-corner origin.** A room's `origin` is its NW (min-x, min-y) corner.
- **2D axes:** edi authored 2D is **X = east (+x right), Y = south (+y down)** — the
  drafting screen frame.
- **2D→3D mapping (realizer):** floor lies in the world ground plane, **+Z up**.
  Map `x → world X`; map `y → world −Y` (so the dungeon reads upright, north = +Y
  in Blender). Heights grow along +Z. *(This handedness choice is a contract point —
  realizer must confirm it matches its bpy build.)*

---

## 2. asset_ref naming

- Form: **`<theme>.<piece>`**, lowercase snake-case. M0 theme = `crypt`.
- **Only PROPS ride as block asset_refs** in `blocks{asset}`:
  `crypt.sarcophagus`, `crypt.brazier`.
- The 10 STRUCTURAL piece types (§4) are NOT asset_refs and do NOT appear in the
  TOON. They are the realizer's **expansion vocabulary**: the realizer derives
  floor/wall/corner/ceiling/doorway/corridor greybox from the room footprints,
  plugs, and connections. This is the key split: **structure is expanded from the
  graph; props arrive as blocks.**
- The realizer reads the theme from the asset_ref prefix (`crypt.`) and the piece
  from the suffix; it owns the asset_ref → mesh table.

---

## 3. The M0 crypt the generator hardcodes (the slice)

- **2 rooms**, 5 ft-aligned: an entrance chamber + a crypt chamber.
- **1 plug per room** on facing edges + **1 connection** joining them → 1 corridor.
- **2 block instances** placed in the crypt chamber: `crypt.sarcophagus` (PROP_FLOOR)
  and `crypt.brazier` (PROP_FLOOR + light).
- Hand-authored, not generated — proves the seam, not an algorithm.

---

## 4. The 10 structural piece types + sockets (realizer greybox vocabulary)

Each: the TOON element that triggers it → greybox primitive → socket(s) → snap.

| Piece | Triggered by | Greybox (M0) | Sockets | Snap |
| --- | --- | --- | --- | --- |
| **FLOOR_tile** | each 5 ft cell of a room footprint | 5×5×`FLOOR_T` slab on ground | 4× FLOOR_EDGE (N/E/S/W) | min-corner to grid |
| **WALL_panel** | each 5 ft perimeter segment that is NOT a doorway | 5×`WALL_H`×`WALL_T`, inward normal | WALL_MOUNT L/R + TOP + BASE | edge line, grid |
| **WALL_corner** inner/outer | each room corner | `WALL_T` square post, `WALL_H` tall | two WALL_MOUNT | corner cell |
| **DOORWAY_frame** (+END_CAP) | each plug | a `DOOR_W`-wide gap framed in the wall; END_CAP seals it | DOOR_PLUG | plug position on grid |
| **CORRIDOR** straight + L | each connection | `DOOR_W`-wide floor+walls between the two plugs; straight if colinear else one L | CORRIDOR_END ×2 | plug↔plug centerline |
| **CEILING_slab** | each floor cell | 5×5×`FLOOR_T` at z=`WALL_H` | CEILING_MOUNT | above its tile |
| **COLUMN** | room corners (optional M0) | `WALL_T` post | CORNER_MOUNT | corner cell |
| **STAIR** | elevation change | step band, +1 level | STAIR_LEVEL | — (unused in single-level M0) |
| **sarcophagus** | block `crypt.sarcophagus` | box prop on floor | PROP_FLOOR | block origin (centre) |
| **brazier** | block `crypt.brazier` | small prop + **point light** | PROP_FLOOR + light | block origin (centre) |

A plug with `connected=true` realises an OPEN DOORWAY_frame; `connected=false`
realises a sealed END_CAP. The brazier is the only light source in M0.

---

## 5. Shared greybox constants (meshes must agree on the envelope)

| Const | Value (ft) | Meaning |
| --- | --- | --- |
| `MODULE` | 5 | grid tile = floor cell = wall-panel length |
| `WALL_H` | 10 | wall / doorway / corridor height (2 modules) |
| `WALL_T` | 0.5 | wall + column thickness |
| `FLOOR_T` | 0.5 | floor + ceiling slab thickness |
| `DOOR_W` | 5 | doorway width == corridor width == 1 MODULE |

These are the GREYBOX envelope. The user's future real crypt meshes must fit the
same socket envelope (a real WALL_panel still spans 5 ft and mounts at the same
WALL_MOUNT planes) so greybox and art are swap-compatible.

---

## 6. The DOOR_PLUG alignment invariant (why plug→doorway→corridor line up)

`DOOR_W` == corridor width == 1 `MODULE`. A doorway occupies exactly ONE grid CELL
of a wall (its two edges land on 5 ft grid lines; its centerline is the cell centre,
mid-tile). The corridor is one cell wide and its cells align edge-to-edge with the
doorway cell. The generator places the two joined plugs **colinear** (same world y
for an east↔west pair, or same world x for a north↔south pair) so the corridor runs
straight, tile-aligned, with no seam gaps. (This is the kickoff's "DOOR_PLUG sized
to kCorridorWidth" — cell-aligned, not centerline-on-a-line, since a module-wide gap
centred on a grid line would straddle two cells.)

---

## 7. Determinism

Given the same TOON, the realizer MUST produce the same mesh layout. No random
placement, no rule inference. Floor tiling order, wall winding, and corner choice
are functions of the footprint + grid only.

---

## 8. Neutral-tags scope decision (RATIFY at the reviewer gate)

**Proposal: M0 adds NO `tags` column to the TOON blocks[] export.** Rationale:
- `asset` (`crypt.sarcophagus`) already encodes theme + piece.
- plug `type` and connection `type` already carry the neutral socket roles.
- A tags column is an additive, tolerant future extension (missing key ⇒ default,
  no version bump — the established discipline) and can land later without
  breaking M0.

If the reviewer/realizer judge a `tags` column NEEDED for M0, it is purely additive
and I will brief it as a follow-up slice — but the default is: **ship M0 on the
existing four arrays.**

---

## Open contract points for the reviewer/realizer to confirm
1. The 2D→3D handedness (§1: `y → −Y`, +Z up) — does the bpy realizer agree?
2. `WALL_H=10`, `WALL_T=0.5`, `DOOR_W=5` — acceptable greybox envelope?
3. No tags column (§8) — ratified?
4. Are FLOOR_EDGE / WALL_MOUNT / DOOR_PLUG / CORRIDOR_END / PROP_FLOOR socket
   names sufficient for the realizer's snap logic, or does it need more?
