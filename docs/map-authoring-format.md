# Map authoring format — the fill-out sheet

The human-facing form for authoring a dungeon/encounter map. You fill out a
selection of fields; edi turns them into geometry deterministically — the same
path an AI takes when it "prompts" a map. Fill this by hand, or hand it to the
AI; both produce the same result because both fill the same fields.

This is the **Seam A** contract (see `docs/layered-architecture.md` once written,
and `docs/dungeon-map-seams.md`). It maps 1:1 to `RoomSpec` in
`src/drafting/DraftingRoom.h` and is realised by
`DrawingDocumentController::createRoomFromSpec`.

## The one rule: this sheet is NEUTRAL

You write down *what is there* and *what kind of thing it is* — never *what it
does*. A secret door is `type: secret` (so edi can draw it flush); whether it
blocks line of sight is a **rule your game engine assigns later**, not a field on
this sheet. Keep game logic out; the sheet is geometry + tags.

## How to read the sheet

- **Units** — write measurements in feet (`ft`) or grid squares (`sq`); declare
  which in the `map` block. One square = the grid scale (default 5 ft). Offsets
  along a wall are measured from the wall's **start corner** (named per edge
  below). `center` is shorthand for "centred on the wall".
- **Status** of each field — `[live]` builds today · `[M2.2]` parsed but the
  visual (door leaf / secret marker) lands with the openings slice · `[M3]`
  parsed but realised when the block library lands · `[tag]` stored as a neutral
  tag, no render yet.
- **Required** fields have no default and must be filled. Everything else has a
  sensible default and can be left blank.

---

## The sheet

### `map` — the board

| field   | values / unit            | default        | status | notes |
|---------|--------------------------|----------------|--------|-------|
| `grid`  | feet per square          | `5 ft / square`| [live] | sets the ft↔square scale |
| `units` | `ft` \| `sq`             | `ft`           | [live] | how the numbers below are written |
| `name`  | free text                | `untitled`     | [tag]  | map title |

### `room` — a rectangular space (repeatable)

| field       | values / unit                    | default   | status | notes |
|-------------|----------------------------------|-----------|--------|-------|
| `shape`     | `rectangle`                      | rectangle | [live] | only rectangle in v1 |
| `footprint` | `<W> × <H>`                      | —         | [live] | required; outer size in `units` |
| `origin`    | `<x>, <y>` of the NW corner      | `0, 0`    | [live] | position on the board |
| `walls`     | `material: <m> · thickness: <t>` | `stone`, `1 ft` | [live] | one material + thickness for all four walls |
| `floor`     | a material tag                   | (none)    | [tag]  | flagstone / dirt / water / … |
| `ceiling`   | a height                         | (none)    | [tag]  | for the engine; not drawn |

#### `openings` — one line per gap in a wall

Each edge is named by its two corners, **clockwise**: `N` (NW→NE) · `E` (NE→SE) ·
`S` (SE→SW) · `W` (SW→NW). An opening's offset is measured from the edge's
**first-named** corner; `center` = centred.

```
<edge>  <type>  <width>  @ <where>
```

| part     | values                                   | status | notes |
|----------|------------------------------------------|--------|-------|
| `edge`   | `N` \| `E` \| `S` \| `W`                | [live] | which wall |
| `type`   | see vocabulary below                     | [live]→[M2.2] | the gap is cut now; the leaf/marker is M2.2 |
| `width`  | length in `units`                        | [live] | must fit within the wall |
| `where`  | `center`, or `<dist> from <corner>`      | [live] | the gap's midpoint; `<corner>` is either of the edge's two named corners (e.g. `5 ft from SE`) |

A wall with no `openings` line is solid. Openings on the same wall must not
overlap, and must fit inside it (validation refuses otherwise).

### `features` — placed objects (blocks)

```
<kind>  ×<count>  <size>  @ <x>, <y>   facing: <dir>   tags: <...>
```

| part     | values                          | status | notes |
|----------|---------------------------------|--------|-------|
| `kind`   | a block id (vocabulary below)   | [M3]   | resolves to a prop in the engine |
| `count`  | integer                         | [M3]   | how many |
| `size`   | `<w> × <h>` in `units`          | [M3]   | footprint |
| `@`      | position in `units` from origin | [M3]   | placement |
| `facing` | `N` \| `E` \| `S` \| `W`        | [M3]   | orientation |
| `tags`   | free words                      | [tag]  | neutral classification |

---

## Vocabularies — the selections

These are open lists (add your own); the ones below are the starting kit. edi
stores any value; rendering and engine-meaning attach to known ones over time.

- **wall material** — `stone` · `brick` · `wood` · `earth` · `rubble` · `metal`
- **opening type** — `door` · `corridor` · `archway` · `window` · `secret` ·
  `portcullis` · `gate`
- **floor** — `flagstone` · `dirt` · `cobble` · `water` · `chasm` · `grass`
- **feature kind** — `brazier` · `statue` · `pillar` · `altar` · `chest` ·
  `table` · `door-leaf` · `stairs` · `rubble-pile`
- **facing** — `N` · `E` · `S` · `W`

---

## The fillable file — TOML

The tables above describe the fields; the file you actually fill out and hand to
edi is **TOML**, per `docs/format_strategy.md` (human-authored static config) and
the same flat dialect the recipe files use. Three dialect rules:

- **Dotted keys, no `[section]` headers** — `room.width = "30"`, not `[room]`.
- **Every value is quoted** — numbers included: `room.width = "30"`. The typed
  reads parse them.
- **Lists are indexed keys** — openings are `room.opening.0.*`, `room.opening.1.*`,
  … read until the first gap (the recipe op-stream convention).

Build it: `edi --room-file <path>.room.toml` (distances are in feet; the loader
scales feet → canvas). A ready example lives at
`docs/examples/guard-antechamber.room.toml`.

### Blank template — copy, fill the quotes

```toml
room.width = "___"
room.height = "___"
room.origin_x = "0"
room.origin_y = "0"
room.wall_material = "stone"
room.wall_thickness = "1"

room.opening.0.edge = "N"
room.opening.0.type = "corridor"
room.opening.0.width = "___"
room.opening.0.at = "center"
```

Add or remove `room.opening.<i>.*` blocks (keep `i` contiguous from 0). A wall
with no opening block is solid. `at` is `center` or a number (offset in feet from
the edge's start corner).

---

## Worked example — the guard antechamber

The prompt *"a 30×20 stone guard antechamber, oak door south, secret door east,
corridor north"* as the filled file
(`docs/examples/guard-antechamber.room.toml`):

```toml
room.width = "30"
room.height = "20"
room.origin_x = "5"
room.origin_y = "5"
room.wall_material = "stone"
room.wall_thickness = "1"

room.opening.0.edge = "N"
room.opening.0.type = "corridor"
room.opening.0.width = "10"
room.opening.0.at = "center"

room.opening.1.edge = "E"
room.opening.1.type = "secret"
room.opening.1.width = "3"
room.opening.1.at = "12.5"

room.opening.2.edge = "S"
room.opening.2.type = "door"
room.opening.2.width = "5"
room.opening.2.at = "center"
```

`edi --room-file docs/examples/guard-antechamber.room.toml` builds it: the four
`stone` walls at 30×20, mitred at the corners, with the three doorway **gaps**
cut where the openings are (the `W` wall solid). The door leaf, the secret-door
marker, and the braziers/statue land with M2.2 and M3.

---

## How it maps to edi (for the curious / the parser)

| sheet field            | becomes |
|------------------------|---------|
| `footprint` + `origin` | the room rectangle (canvas units, via the grid scale) |
| `walls.thickness`      | `WallGeometry.thickness` on every segment |
| `walls.material`       | a neutral `ObjectMetadata.material` tag |
| an `opening`           | a **gap**: the wall edge splits into solid segments around it |
| `opening.type`         | a neutral tag (drives the leaf/marker render in M2.2) |
| `feature`              | a block instance (M3) — id + transform + tags |

The conversion from physical units (ft/sq on this sheet) to edi's canvas units
happens at ingest using `grid`. Today the ingest step is the AI: fill the sheet,
hand it over, and it calls `createRoomFromSpec`. A text parser that reads this
format directly (so no AI is needed in the loop) is the next Seam-A slice.
