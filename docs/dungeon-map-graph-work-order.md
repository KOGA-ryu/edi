# Work order — the Map Graph (plugs + declared connections)

Phase 2 of the dungeon-map track. Companion to `docs/dungeon-map-roadmap.md`
(what to build) and `docs/dungeon-map-seams.md` (which existing seams each piece
plugs into). Phase 1 (the Wall: M1.1 primitive, M1.2 miter, M1.3 neutral render
classification, mirror, creation-time thickness) is **complete and merged to
`master`**. This doc is the build plan for the next keystone: the **map graph**,
the connection layer the user's earlier "gameguy" prototype called its crown
jewel, never built in edi until now.

This design was produced by a fan-out understanding pass over the shipped Seam-A
substrate, the document/serialization seams, and the ownership constraints, then
adversarially reviewed. The review found **no hard-rule violations** and added
several missing slices (folded in below).

---

## What the map graph is, in edi terms

Two new **document-level arrays** that sit beside `objects` on `DraftingDocument`:

- **plugs** — named, neutral attachment points (a door / portal / threshold
  *socket*). A plug is not geometry of its own; it rides on an existing document
  object (a `Point` marker at an opening's gap-center) **by id**, the way a label
  rides on the thing it names.
- **declared connections** — neutral edges naming *"plug A links to plug B"*. They
  reference plugs **by id only**, never raw coordinates.

Both are plain structs in `src/drafting/`, mutated only through free functions
(`DraftingGraphOps`) wrapped in `DraftingCommand` variants and applied through the
existing `applyDraftingCommand` `std::visit` dispatcher — exactly how `objects`
and `layers` are mutated today.

### Why document-level vectors, not a `DraftingGeometry` variant

A plug is a **relation**, not per-object geometry. Modelling it as a
`DraftingGeometry` arm would force it through the full ~22-site geometry-visitor
recipe (`docs/adding-a-drafting-tool.md`) and mis-state what it is. As two sibling
vectors on `DraftingDocument` it mirrors the existing `layers` / `objects` /
`selectedObjectIds` layout exactly — and because it lives **inside**
`DraftingDocument`, it rides the existing `DocumentSnapshot` undo/redo for free,
with zero new undo plumbing. A plug renders and is selectable because its
`anchorObjectId` points at a `Point` marker that **is already a real document
object** (the proven ASCII-door `emitMarker` path); the plug record itself stays
invisible data.

## The neutral-only stance (the rule that keeps edi clean)

edi **records the declaration** and records nothing about what it *means*. The
ownership boundary, restated for a connection graph:

- recording *"plug A connects to plug B"* — **neutral, belongs in edi.**
- computing reachability, honoring a locked/secret door as a barrier, pathfinding,
  resolving a plug to an asset — **rules, belong to the game engine past Seam B**
  (asset binding is the later Asset Dex tier).

The reviewer's one-line test for the boundary: the connection struct has **no
`passable`, no `weight`, no `direction`, no `assetId`** — *that absence is the
design, not an omission.*

---

## Data layout

```cpp
// DraftingTypes.h — beside DraftingObjectId
using DraftingPlugId       = std::string;
using DraftingConnectionId = std::string;

// DraftingDocument.h — plain sibling structs (no methods)
struct DraftingPlug {
    DraftingPlugId   id;             // opaque, minted "plug_0001"
    DraftingObjectId anchorObjectId; // the doc object (a Point marker) this plug sits on
    std::string      name;           // authored label, e.g. "north_doorway"
    std::string      type;           // neutral open vocab: "door"/"portal"/... (edi never interprets)
    Point2D          anchor;         // cached gap-center, so draw/export need not re-derive
};

struct DraftingDeclaredConnection {
    DraftingConnectionId id;         // opaque, minted "conn_0001"
    DraftingPlugId       plugA;
    DraftingPlugId       plugB;      // references plugs by id only — never raw coords
    std::string          type;       // neutral role tag, default empty ("corridor"/...)
};

// on DraftingDocument, after `objects`:
std::vector<DraftingPlug>               plugs;
std::vector<DraftingDeclaredConnection> connections;
```

---

## Build slices

Each is independently committable with the working tree green between, one vein per
commit, teaching commit body — the edi discipline.

| #  | Slice | Size | Depends |
|----|-------|------|---------|
| **S0** | Structs + id aliases + the two vectors — pure data, no behavior. **The keystone.** | small | — |
| **S1** | `DraftingGraphOps` free fns: add/remove plug, declare/undeclare connection; `removePlug` cascades to drop orphan edges; validate ids + anchor exist; bump `revision`. | medium | S0 |
| **S2** | `DraftingCommand` variants + `applyDraftingCommand` arms (`CreatePlug` / `DeletePlug` / `DeclareConnection` / `DeleteConnection`). Caller supplies pre-minted ids, like objects. | medium | S1 |
| **S3** | MessagePack round-trip — two tolerant arrays; bump doc version 2→3, hold `MinReadVersion` at 1 (old `.edidraw` load with an empty graph). No JSON. | medium | S0 |
| **S4** | **Object-delete → plug/connection cascade** — close the dangling-reference hole in the existing `DeleteObjectCommand` / `removeObject` path (mirror `normalizeSelection`'s prune). | small | S2 |
| **S5** | Room-spec authoring: parse `room.plug.<i>.{name,edge,at,type}`, emit a `Point` marker per plug (`emitMarker`, provenance `"plug"`, tag `plug:<name>`), **and the controller wiring that mints the plug** (not deferred). Doc the new keys. | medium | S2 |
| **S6** | TOML `map.connection.<i>.{from,to,type}` → `DeclareConnectionCommand`; reject duplicate/unknown plug names rather than silently resolving. | small | S5 |

**Start with S0.** It is pure data with zero behavior, so it cannot break a test,
default-constructs empty, and commits as one clean vein. S1/S3/S5 depend only on
S0, so serialize and authoring can proceed in parallel with the ops work.

### Seam map (per slice)

| Slice | Seams it plugs into |
|-------|---------------------|
| S0 | `DraftingTypes.h` id aliases · `DraftingDocument.h` aggregate (layers/objects precedent) |
| S1 | `DraftingStore.h` free-fn-over-document pattern (`DraftingStoreResult`, `++revision`); `objectIdSet` for validation |
| S2 | `DraftingCommands.h` variant + `applyDraftingCommand` `std::visit`; `fromStoreResult`; undo via `DocumentSnapshot` |
| S3 | `DraftingSerialize.cpp` `draftingDocumentToValue`/`FromValue`; `MsgPackValue` map/array; version constants |
| S4 | the existing `DeleteObjectCommand`/`removeObject` path |
| S5 | `RoomSpecStore` flat dotted-key parser; `planDraftingRoom`; `DraftingAsciiMap` `emitMarker`; `docs/map-authoring-format.md` |
| S6 | `RoomSpecStore` parser + the S2 `DeclareConnectionCommand` |

---

## Decisions (resolved, open to override)

1. **Plug attaches to a `Point` marker at the gap center**, not directly to a wall
   segment. A room emits *many* wall spans with no single "the wall"; the marker
   gives the plug a real coordinate and makes it selectable. Reuses `emitMarker`.
2. **Two sibling vectors**, not a wrapping `DraftingMapGraph` struct — buys nothing
   until there is graph-level data to hold; mirrors `layers` / `objects`.
3. **Bump the document version 2→3, hold `MinReadVersion` at 1** — the reader is
   tolerant either way, but the bump is the honest record (the M1.3 `wall_visual`
   precedent). Done in S3.
4. **Caller mints ids, ops only validate** — keeps `DraftingGraphOps` Qt-free and
   deterministic to test, matching how `DraftingObject`s arrive pre-minted at
   `addObject`.

## Risks & what the adversarial review added

- **Object-delete → graph cascade.** The first-pass design opened a
  dangling-reference hole (a plug whose anchor object is deleted) and never closed
  it. Now **S4**.
- **Authoring creates the plug end-to-end.** S5 originally deferred the controller
  marker→`CreatePlugCommand` wiring — the half that actually mints a plug. Folded
  back into S5.
- **Duplicate plug names.** Connections reference plugs by authored `name` but the
  document keys by id; two plugs sharing a name is ambiguous. S6 must **reject**,
  not silently pick one.
- **Seam-B superset.** S3's serialized fields (`id`, `anchor`, `name`, `type`,
  `plugA`, `plugB`) must be a superset of what a future TOON/Seam-B exporter needs,
  so the deferred exporter is never blocked by a missing field. They are.
- **Scope creep toward rules.** A reviewer may ask for a `passable` or `locked`
  flag on connections, or asset binding on plugs. These belong past Seam B / in the
  Asset Dex tier and must be refused in edi's tier-1 model.

## Out of scope (but un-blocked)

- The **TOON Seam-B exporter** that projects the graph for AI/engine handoff — a
  separate later slice; S3 proves the MessagePack round-trip, not the TOON one.
- The **Asset Dex tier** (asset/socket tables, CSV-cell mapping). The graph uses
  object ids as handles and free-form tags, so the tier can layer on the same ids
  without rework.
