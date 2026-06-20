# Neutral map TOON wire — schema & canonical column order (dungeon-map OWNS this)

The ONE source of truth for the Seam-B/C TOON column order + growth rule. dungeon-map is
the canonical column-order owner (Phase-2 decision). The realizer (`tools/blender/
edi_realize.py`) reads cells BY NAME from each section header; both sides obey the rules.
Live ratified contract bus copy: `~/dept-bus/edi-dungeon-map/briefs/074-blender-lab-wire-
contract-FINAL-plus-sample.md` (with a sample extended TOON).

## The two laws
1. **Header-as-truth** — the realizer indexes each cell by the header field name, tolerant
   of column count/order. No version line.
2. **Conditional emission** — every NEW section/column is emitted ONLY when non-default
   data exists. ⇒ a node-less / single-level / fully-walled / placed-only map is
   **BYTE-IDENTICAL** to today (the `map_regression_lock_tests` canary guards this).

## Canonical order (emitted in this order; each present-only)
- header metas: `kind` · `title` · `units` [· `feet_per_band`]
- [`levels[K]{index,elevation}`]  (multi-level only)
- sections: `rooms` · `plugs` · `connections` · `nodes` · `blocks` · `vertical` (future)

### rooms columns
`name,origin,size,material` [`,level`] [`,derivation`] [`,bounded_by`] [`,walls`] [`,kind`] [`,ceiling`] [`,floor`]
- `level` int (band) — conditional on some room level≠0. `derivation` `placed`/`span_derived`
  — conditional on some span room. `bounded_by` `·`-joined node-NAME list — conditional.
  `walls` 4-char NESW mask (present=letter, absent `-`) — conditional on a non-full mask.
  `kind` `enclosed`/`open` — conditional. `ceiling`/`floor` bool — conditional.

### plugs columns
`room,name,edge,type,connected,flags` [`,level`]   (level conditional)

### nodes columns
`name,anchor,type,radius`   (anchor authored feet; radius authored DATA)

### blocks columns
`room,asset,origin,scale,rotation`   (unchanged)

### vertical[] (future, after blocks)
`room,kind,footprint,from_level,to_level,type`

## Build status (Phase 2, wire slices — conditional + canary-guarded)
- A1 `nodes[]` ✓ (426b379, merged). A2 `level` ✓ (5fa7583, merged). A3 nodes `radius` +
  rooms `derivation`/`bounded_by` + `DraftingMapRoom.boundedBy` field ✓ (288571d).
- TODO: A4 `walls`/`kind`/`ceiling`/`floor` (carry onto `DraftingMapRoom`). A5
  `feet_per_band` meta + `levels[]` manifest. Then `vertical[]`.
- Realizer (blender-lab) built P2-C to this contract; FIRST INVERTED RENDER `/tmp/m0/
  inverted.png` @07afe8f.
