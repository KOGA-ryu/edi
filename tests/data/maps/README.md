# Test-map corpus (Seam B)

A small set of authored `.map.toml` dungeons spanning different graph shapes, each
exported to a `.toon` map document for exercising the game engine's map reader.
Distances are in feet; the `.toon` is the neutral Seam B handoff (rooms / plugs /
connections). See `docs/format_strategy.md` (TOON) and the exporter at
`src/io/MapToonExport.*`.

| Map | Shape it stresses |
|---|---|
| `minimal`  | 2 rooms, 1 corridor — the smallest valid graph |
| `hallway`  | 4 rooms in a line — a pure chain (degree-1 ends, degree-2 interior) |
| `loop`     | 4 rooms in a cycle — a path back to start, no dead ends |
| `hub`      | central room with 4 doors + 4 satellites — a degree-4 node |
| `secrets`  | secret-type plugs as unconnected dead ends (`type=secret, connected=false`) |

The larger reference dungeon lives at `tests/data/dungeon.map.toml`
(10 rooms + a 3-way junction + secret doors + 12 corridors).

## Regenerating the `.toon` outputs

The `.toon` files are generated from the `.map.toml` sources — regenerate any after
editing its source:

```
QT_QPA_PLATFORM=offscreen ./build/edi --map-file tests/data/maps/<name>.map.toml \
    --export-map tests/data/maps/<name>.toon
```
