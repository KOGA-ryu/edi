# Prior art — neutral entity markers, patrol paths, door-lock tags, two-turn corridors

> Research gate for campaign **dungeon-map-20260619-provingground**. Validates the
> planner's proposed marker / patrol / lock / export schema (see
> `docs/handoffs/dungeon-map-20260619-provingground.md`) against mature map/level
> formats BEFORE the reviewer settles the boundary. No code; data-shape validation only.
>
> THE LAW being checked: edi RECORDS neutral data + tags; the engine owns ALL behavior.
> A marker is a record + a tag, never a rule. Each finding ends with a verdict line.
>
> Verified-from-source vs asserted: every load-bearing claim below carries a URL and a
> one-line note on what it supports. Anything I could not confirm is marked UNVERIFIED.

---

## 1. Neutral entity-marker layers

**The established neutral shape.** Tiled's object layers store gameplay markers as
data-only objects. Every object carries: `id` (incremental, unique across the map),
`name` (free text), `type`/`class` (a free-form classification string), `x`/`y`
(position), and an array of **custom properties** — each a `{name, type, value}`
triple. The object can also be flagged `point` ("the simplest objects… only represent a
location… cannot be resized or rotated").

- Object fields incl. `id`/`name`/`type`(class)/`x`/`y`/`point` + properties array of
  `{name,type,value}`: https://doc.mapeditor.org/en/stable/reference/json-map-format/
- Point object = location-only, no behavior; custom properties carry per-object gameplay
  metadata ("a chest with defined contents or an NPC with defined script"):
  https://doc.mapeditor.org/en/stable/manual/objects/

Godot's **Marker2D** corroborates the "marker = position-only, behavior elsewhere"
split: it is documented as a "Generic 2D position hint for editing" with no gameplay
function, used "for marking specific locations like spawn points, waypoints" — exactly a
data-only anchor whose meaning lives in attached scripts/the engine.

- Marker2D is a position-only editor hint, no inherent behavior:
  https://docs.godotengine.org/en/stable/classes/class_marker2d.html

**Why UVTT is the anti-pattern (poor fit, by design).** UVTT/.dd2vtt is JSON, is
"an image with a bunch of extra data attached," measures everything in "Squares," and
its core vocabulary is VTT *presentation* semantics — line-of-sight walls, portals that
"block sight and movement but can be unlocked," and light sources. That bakes
rendering/lighting *behavior* into the document and is tabletop-VTT-centric, not
engine-neutral. The user runs their OWN engine, and edi forbids JSON — so UVTT is the
format to learn FROM (the anti-pattern), not adopt.

- UVTT = JSON, image-plus-data, "Squares" units, walls/portals/lights with block-sight
  semantics baked in: https://arkenforge.com/universal-vtt-files/ and
  https://wiki.roll20.net/Script:UniversalVTTImporter

**Comparison to the planner's `RoomFeature + id + metadata`.** The common neutral shape
across Tiled + Godot is `{id, type/role, position, free-form properties}`. The planner's
extended `RoomFeature` = `{id, type(role), x,y(anchor), name, metadata[]}`. That is a
1:1 match to the mature shape — `type` is the open-vocab role (spawn/pickup/npc/goal),
`x,y` is the anchor, `metadata[]` is the `{name,value}` property array (edi drops the
per-prop `type` column, which is fine: edi stores all tags as strings already, e.g.
`DraftingPlug::flags`). Nothing is missing; nothing is over-modeled. The one thing to
NOT add: a per-property typed value (Tiled's `type` on each property) — edi's string-only
metadata is simpler and consistent with the rest of the codebase.

> Verdict: ✅ planner proposal matches prior art. `{id, type/role, x,y, name, metadata[]}`
> is the canonical neutral marker shape; keep metadata as string k/v (do not add typed
> values). Reusing `RoomFeature` rather than a heavy `DraftingMarker` is the right call.

---

## 2. Patrol path / waypoint representation

**The established neutral shape.** Tiled stores ordered paths as **polyline** objects:
an object whose `polyline` field is an array of `{x,y}` points, position-relative. The
manual states polylines are "often used to represent paths to be followed." The
open-vs-closed distinction is structural and is the format's own convention:

- **polyline** = open path (array of points, "paths to be followed");
- **polygon** = closed path ("completed by clicking the first point again"); polygons
  require at least three points and reconnect to the start.

- polyline/polygon point arrays + open-vs-closed semantics, polyline = "paths to be
  followed": https://doc.mapeditor.org/en/stable/manual/objects/
- point arrays stored as `{x,y}` lists; polyline open vs polygon closed:
  https://doc.mapeditor.org/en/stable/reference/json-map-format/

**The reference-by-id pattern.** Tiled's **`object` property type** (since 1.4) is "a
reference to an object" — one object points at another by its id, with editor navigation
between them. This is the established way an NPC marker links to its patrol path: a
property on the npc holds the patrol object's id. So `npc.metadata patrol=<patrol_id>` is
exactly the mature pattern (edi expresses the reference as a string id rather than Tiled's
typed object-ref, consistent with edi's all-strings tags).

- `object` property type = reference one object by id, with jump-to navigation:
  https://doc.mapeditor.org/en/stable/manual/custom-properties/

**Comparison to the planner's `MapPatrolPath {id, waypoints[], closed}`.** This maps
cleanly onto a Tiled polyline/polygon object plus an id reference: `waypoints[]` = the
point array, `closed` = the polyline(open)/polygon(closed) distinction collapsed into one
explicit boolean, `id` = the reference target. ONE refinement on the closed-loop
convention: Tiled does NOT repeat the first point for a closed shape — closure is
implicit in being a *polygon* (a type), not a duplicated last point. The planner's
explicit `closed` boolean is the cleaner neutral encoding of the same idea, and it means
the builder must NOT append a duplicate first point to `waypoints` for loops — the engine
closes the loop because `closed=true`, not because the list repeats. Document that
contract so the engine reads it unambiguously.

> Verdict: ✅ planner proposal matches prior art — with one contract note: `closed` is the
> boolean that means "loop"; do NOT also repeat the first waypoint. `{id, waypoints[],
> closed}` referenced by an npc marker's `patrol=<id>` is the canonical pattern.

---

## 3. Door lock / key gating as a neutral tag

**The established neutral pattern.** In Tiled, a door is placed as an *object* (tiles in
tile layers cannot hold per-instance properties, so doors that need gameplay data go on an
object layer), and lock/key data is attached as **custom properties** — e.g. `locked`
(bool or enum) and `key_id`/`key_number` (the matching key). These properties are pure
data: "These properties would then be read by your game engine to determine door behavior
during gameplay." The format records the tag; the engine owns the unlock RULE.

- doors as objects with `locked` / `key_id` custom properties, "read by your game engine
  to determine door behavior": https://discourse.mapeditor.org/t/how-to-use-doors-in-tiled/5057
  and https://doc.mapeditor.org/en/stable/manual/custom-properties/

**Key-by-id matching is the standard indirection.** The conventional model is a string
(or int) `key_id` on the door that the engine matches against an inventory/pickup
identifier — the pickup's id and the door's `key_id` are the SAME value, and the engine
resolves the match. That is precisely the planner's "pickup id == door key_id" design.
(The Tiled forum thread on "procedural map generation with locked doors, keys" treats
key/door pairing as engine-side matching, not editor-enforced.)

- locked-doors/keys treated as engine-side pairing, editor only records:
  https://discourse.mapeditor.org/t/procedural-map-generation-with-locked-doors-keys/5021

**On WHERE the tag rides (connection vs plug.flags).** Both placements have prior art:
Tiled attaches the lock to the *door object* (the thing you pass through), and edi's door
is a plug+connection pair. The connection is the PASSAGE between two plugs, so
`locked`/`key_id` on the connection reads as "this passage is gated" — semantically the
closest analogue to Tiled's door-object. Riding `plug.flags` would scatter the lock across
two plug endpoints and lose the clean export column. The planner's connection-level choice
is the better neutral fit; the existing `DraftingDeclaredConnection` "deliberately no
`locked`" comment was guarding against a RULE, and a tag the engine interprets does not
violate that — update the comment to say so explicitly.

> Verdict: ✅ planner proposal matches prior art. `locked:bool + key_id:string` is the
> conventional neutral door tag; pickup-id == door-key_id is the standard indirection.
> Connection-level (not plug.flags) is the cleaner placement. Restate the struct comment:
> these are engine-interpreted tags, NOT edi-enforced rules.

---

## 4. Two-turn corridor + single-branch topology (layout rule, not algorithm)

This is a GEOMETRY/layout question answered directly from edi's own router
(`src/drafting/DraftingCorridor.cpp::corridorCenterline`), so it is verified-from-source
against the actual code, not asserted from a blog.

**How the router bends.** `corridorCenterline` branches on the two doors' outward
normals (`outwardNormal(edge)`):

- Both normals HORIZONTAL (e.g. door A faces East, door B faces West) → centerline is
  `{a, {xm, a.y}, {xm, b.y}, b}` where `xm = (a.x + b.x)/2`. That is an explicit
  TWO-BEND Z (out east, jog in y at the midpoint x, in west) whenever `a.y != b.y`.
- If `a.y == b.y` (doors colinear), `simplifyOrthogonal` drops the colinear interior
  points and the Z collapses to a STRAIGHT run.
- One vertical + one horizontal normal → a single-bend L: `{a, {a.x, b.y}, b}` or
  `{a, {b.x, a.y}, b}`.

(Source: `corridorCenterline`, the `!normalIsVertical(nA) && !normalIsVertical(nB)`
branch; `normalIsVertical` thresholds `|n.y| > 0.5`; `simplifyOrthogonal` removes
colinear interior points.)

**The layout rule for the proving ground.** To force the two-turn Z between safe_room and
the junction:

1. Make the two doors FACE EACH OTHER on the same axis — safe_room's plug on its **East**
   wall, the junction's plug on its **West** wall (both horizontal normals). This selects
   the two-bend branch.
2. Give the two doors a **non-zero y-offset** (`a.y != b.y`). ANY non-zero offset larger
   than the centerline epsilon (`kEps = 1e-9`) produces the second turn — there is no
   minimum "force" distance in the centerline math; the jog appears as soon as the y's
   differ. For a corridor that READS as a clear Z (the side walls of the two horizontal
   legs not overlapping), make the y-offset at least the corridor width plus a wall
   thickness — i.e. `|a.y - b.y| > width + wallThickness` (a dimension the M2 builder sets
   as data, not a literal). Below that the two legs visually merge into a fat single bend;
   above it you get a clean two-leg Z.
3. Keep the two doors at different x (the natural east/west separation) so `xm` lands
   between them and the vertical jog leg has length.

The planner's layout (safe_room E plug, junction W plug, at DIFFERENT y) already satisfies
1 and 2. The only thing to PIN as data is the y-offset magnitude for a clean read (rule:
offset ≥ corridor walkable width + side-wall thickness).

**Single-branch junction to a dead-end.** edi has no merged-vs-independent corridor
auto-router for hubs — a junction of degree N is simply an N-plug room with N separate
`connections`, each routed independently by `corridorCenterline`
(charter + `DraftingCorridor`). So the 3-way junction = a small room with three plugs (W
from safe, E to center, S to dead_end), three independent connections. The S→dead_end
branch is just one more connection to a room that has ONE plug and no onward exit — the
"branches once to a dead-end" topology is data (three connection rows), not a routing
mode. Offset the dead_end's entrance plug from the junction's S plug only if a bent
approach is wanted; a straight stub (colinear) is fine for a dead-end.

> Verdict: ✅ planner layout produces the intended two-turn Z — confirmed against the
> actual router. Adjust only this: PIN the safe↔junction y-offset as named data ≥
> (corridor width + side-wall thickness) so the Z reads as two distinct legs rather than a
> fat single bend. The branch-to-dead-end is three independent connections off a 3-plug
> junction room, not a special routing mode.

---

## Carry into the reviewer gate

- **Marker shape — KEEP.** Extend `RoomFeature` to `{id, type(role), x,y(anchor), name,
  metadata[]}`. This is the canonical Tiled/Godot neutral shape. Keep `metadata` as
  string k/v; do NOT add per-property typed values, and do NOT mint a heavy
  `DraftingMarker` type. (Q1: match.)
- **Patrol shape — KEEP, with a contract note.** `MapPatrolPath {id, waypoints[], closed}`
  matches Tiled polyline/polygon. ADD the contract: `closed=true` means "loop"; the
  builder must NOT also repeat the first waypoint. NPC links its path via
  `metadata patrol=<id>` (Tiled's `object` reference-by-id pattern, as a string). (Q2.)
- **Lock tag — KEEP at connection level.** `locked:bool + key_id:string` on the
  connection (not on plug.flags) is the cleaner neutral analogue of Tiled's door-object
  property. ADJUST the `DraftingDeclaredConnection` comment: state these are
  engine-interpreted TAGS, not edi-enforced rules (the old "deliberately no locked"
  guarded against a RULE, which still holds). Pickup-id == door-key_id is the standard
  matching indirection — keep it. (Q3.)
- **Export columns — KEEP additive.** `markers[]{room,id,role,x,y(+metadata projection)}`,
  `patrols[]{id,closed,points}`, and `locked,key_id` appended to `connections[]`. Defaults
  (empty id/metadata, locked=false, empty key_id) MUST keep existing exports
  byte-identical — pin with the golden test, exactly as the planner notes. Reviewer still
  to settle the metadata-projection form and the Seam B/C overload choice (no prior-art
  blocker either way — both are internal naming). (Q1/Q2/Q3.)
- **Layout rule — ADJUST (pin one dimension).** safe_room E plug ↔ junction W plug
  (facing, horizontal normals) at DIFFERENT y → router emits a two-bend Z (verified in
  `corridorCenterline`). PIN the y-offset as named data ≥ (corridor walkable width +
  side-wall thickness) so the Z reads as two distinct legs. The 3-way junction = a 3-plug
  room with three INDEPENDENT connections; branch-to-dead-end is data, not a routing mode.
  (Q4.)
