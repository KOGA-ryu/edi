# Architecture — the dungeon-map subsystem

> **Status: DRAFT skeleton — pending the reviewer-gate survey**
> (campaign `dungeon-map-20260616-cartography`). Section bodies are filled from
> `edi-dungeon-map-reviewer`'s verified `file:line` findings; until then each
> section states only what the charter/seam docs already assert, marked
> _(unverified)_. This doc is the durable map of the subsystem — keep it current.

The map subsystem turns an authored `.map.toml` into a neutral drafting document
(rooms + walls + corridors + doors + blocks) and exports it across **Seam B** as a
neutral TOON map for the user's OWN game engine. **Layered law: edi records
geometry + neutral tags; it does NOT simulate. No game rules, no generation.**

## 1. Ownership boundary — ours vs the drafting core's

We SHARE `src/drafting` with edi-drafting. This table is the contract that keeps
the two departments from colliding; the hub arbitrates disputes.

| Owner | Files / types | Role |
| --- | --- | --- |
| _pending reviewer survey_ | | |

**Boundary risks:** _(pending)_

## 2. The plug / connection graph model
- `plugs` and `declared_connections` — 2 document-level vectors on
  `DraftingDocument` _(unverified — reviewer to confirm)_. A plug is a RELATION,
  not a `DraftingGeometry` variant.
- Neutral-only: no passable/weight/direction. _(pending struct defs + line anchors)_

## 3. The map arms of `DraftingCommand` + visit sites
- Command arms (`CreatePlug`/`DeletePlug`/`CreateConnection`/block arms…) and the
  `std::visit`/dispatch sites that must handle each. _(pending — incl. any
  non-exhaustive visit flagged.)_

## 4. The MessagePack codec for map data
- How plugs / declared_connections / blocks (de)serialize in
  `DraftingSerialize.cpp`. Contract: additive-tolerant — missing key ⇒ default,
  NO version bump (like `wall_visual`). _(pending — confirm + line anchors;
  flag any non-tolerant field.)_

## 5. What-calls-what: `.map.toml` → rendered objects
- `parseMapSpecToml` / `createMapFromSpec` (`io/RoomSpecStore`) → MapSpec →
  controller/CLI (`--map-file`) → document objects → projection → painter.
  _(pending compact call-graph.)_

## 6. Seams in / out
- **Seam B/C export:** `exportMapToToon` / `io/MapToonExport` / `--export-map` →
  TOON (never JSON, never UVTT), projecting the typed MapSpec. _(pending confirm.)_

## 7. Refactor candidates (behavior-preserving only)
- _(pending — ranked by value, each with `file:line`, the defect, and a
  one-line behavior-preserving fix sketch.)_

## 8. Known forward dependency (note, do not build)
- `transformGeometry` (rotate/scale over the 14 geometry kinds) does NOT exist
  yet and is **edi-drafting-owned**. Future per-instance block rotation/scale
  depends on it. Out of this campaign's scope.
