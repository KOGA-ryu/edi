# Handoff — dungeon-map-20260619-provingground

> Per-campaign state. Each gate appends its result; the NEXT gate reads this first.
> Agents hand off THROUGH this file — they cannot message each other.

- **Campaign**: dungeon-map-20260619-provingground
- **Department**: edi-dungeon-map
- **Goal (one line)**: author a NEUTRAL "first-playable proving-ground" map (spawn → two-turn
  corridor → branch to RED dead-end → center locked door + alcove pickup → upper-right NPC+patrol →
  far-right goal behind a final locked door + props) and export it to TOON so the user's game engine
  can prove its behaviors against the fixture. **Map + export only — NO render.**
- **Boundary (the question the reviewer gate must settle)**: is the new marker/patrol/lock layer
  STRICTLY NEUTRAL (records + tags, ZERO behavior)? ADDITIVE (MessagePack tolerant, no version
  bump, byte-identical for existing maps)? And does each piece belong in `src/drafting`
  (DraftingMapTypes / DraftingRoom RoomFeature / DraftingDeclaredConnection) vs the map-doc /
  authoring layer (RoomSpecStore parser, MapSpec, MapToonExport)?

## THE LAW (three-tier — restated so no gate forgets)
edi RECORDS the neutral map + entity markers; the ENGINE owns ALL behavior — movement, camera
follow, collision/wall-blocking, patrol stepping, pickup state, key-before-door sequencing, win.
**edi implements NONE of them.** Every marker is a record + a neutral tag, never a rule. The
`locked`/`key_id` on a door is a TAG the engine interprets, NOT a gate edi enforces — edi will
still "let" anything through because edi does not simulate movement at all.

## Planner's proposed schema (the thing the reviewer gate rules on)

### 1. Marker — REUSE `RoomFeature`, extended additively (recommended; minimal)
`RoomFeature` already exists in `src/drafting/DraftingRoom.h`: `{ double x,y (room-local authored
feet); std::string type; std::string name; }`, parsed by `parseRoomFeatures` as
`room.feature.<i>.{x,y,type,name}` (RoomSpecStore.cpp ~L214). It is ALREADY a neutral interior
point marker. The brief's marker `{ id, role, anchor, metadata }` maps onto it with TWO additive
fields:
- `role` → existing `type` (neutral open vocab: spawn/pickup/npc/goal/…). No change.
- `anchor` → existing `x,y` (room-local authored feet). No change.
- `id` → **ADD** `std::string id;` — stable identifier. Needed because a pickup's id IS the
  door's `key_id`, and an npc references a patrol path by id. Default empty ⇒ byte-identical.
- `metadata` → **ADD** `std::vector<std::pair<std::string,std::string>> metadata;` — free-form
  neutral k/v (e.g. npc `patrol=patrol_guard`). Default empty ⇒ omit key ⇒ byte-identical.
- TOML authoring: add `room.feature.<i>.id` and `room.feature.<i>.meta.<j>.{key,value}`
  (the existing contiguous-indexed-sublist dialect). Keep `.type` as the role field.
- **Open question for reviewer:** new heavy `DraftingMarker` type vs this RoomFeature extension.
  Recommend the extension (brief: "reuse Point markers + metadata where it fits rather than
  inventing heavy new types").

### 2. Patrol path — NEW neutral polyline at MAP level (genuinely new)
No existing concept. Proposed: a `MapSpec`-level vector (DraftingRoom.h, beside `connections`,
`blocks`):
```
struct MapPatrolPath {
    std::string id;                 // referenced by an npc marker's metadata patrol=<id>
    std::vector<Point2D> waypoints; // ordered; ABSOLUTE authored feet
    bool closed = true;             // a loop (default) vs an open path — NEUTRAL geometry tag
};
```
**Closed-loop contract (research):** `closed=true` means LOOP; the first waypoint is **NOT**
repeated at the end (Tiled polygon convention) — the engine closes the loop. The proving-ground
npc patrol is a rectangular CLOSED loop of ~4 waypoints around the NPC-room center.

`MapPatrolPath` is NOT a DraftingGeometry variant (same reasoning as plugs/nodes: it is a relation/annotation, not a
renderable shape). TOML: `map.patrol.<i>.id`, `map.patrol.<i>.closed`, `map.patrol.<i>.point.<j>.{x,y}`.
- **Open question for reviewer:** MapSpec-only (authoring→TOON path, sufficient for this campaign)
  vs also a `DraftingDocument` vector (for the live-document Seam-C overload). Recommend
  MapSpec-only first — the proving ground is authored from ONE `.map.toml` and exported directly;
  no document round-trip is required by the deliverable.

### 3. Lock + key_id — a GENERIC neutral tag on BOTH a connection AND a marker (REVISED per HUB)
ONE key (`gold_key`) gates TWO things: the final goal-room DOOR (a connection) and a locked CHEST
(an interactable marker). So `locked`/`key_id` is NOT door-only — it is a generic neutral tag
expressible on either host. The intent of the connection's "deliberately no `locked`" comment is
**no RULE**; a TAG the engine interprets is allowed by the mandate. Two hosts, same vocabulary:
- **On a connection (the door):** ADD additive typed fields to `DraftingDeclaredConnection`
  (DraftingMapTypes.h) and `MapConnectionSpec` (DraftingRoom.h):
  - `bool locked = false;`   // NEUTRAL tag — lock STATE the engine reads; edi enforces nothing
  - `std::string keyId;`     // which key (matches a pickup marker's `id`); empty default
  Update the struct comment: these are engine-interpreted TAGS, not edi-enforced rules.
  TOML: `map.connection.<i>.locked = "true"`, `map.connection.<i>.key_id = "gold_key"`.
- **On a marker (the chest):** NO new typed field needed — the chest is a marker with role/type =
  `locked` (or `chest`) and its lock rides in the §1 free-form `metadata` bag:
  `key_id=gold_key` (and optionally `locked=true`, redundant with the role). This is exactly the
  genericity the hub asked for: the SAME `{locked, key_id}` vocabulary, typed columns on a
  connection, k/v metadata on a marker. The engine reads both.
- **Open question for reviewer:** connection-level typed fields (recommended — clean TOON column,
  the lock gates the PASSAGE) vs riding the plug's existing `flags`. AND: is the chest's lock fine as
  marker metadata, or should the export project marker `key_id` into a dedicated column too?
  Recommend: typed columns on connection; marker lock via metadata, projected in the markers[] meta.

### 4. TOON export — extend the export to carry markers + patrol + lock
`exportMapToToon(const MapSpec&, …)` (Seam B, MapToonExport.cpp) emits three tabular arrays
(rooms / plugs / connections). Extend additively:
- `markers[N]{room,id,role,x,y}` (+ a metadata projection — TBD form, reviewer rules)
- `patrols[N]{id,closed,points}` (points as a compact polyline)
- add `locked,key_id` columns to the existing `connections[]` array (default false/empty ⇒
  every existing export must stay byte-identical — pin with the golden test).
- **Open question for reviewer:** which overload carries markers — the `MapSpec` overload
  (recommended, the authored one-file product) or the `DraftingDocument` overload (the brief
  loosely calls the marker-carrying export "Seam C"; the charter calls the document overload
  Seam C and the MapSpec overload Seam B). Resolve the Seam B/C naming and pick the overload.

## Map layout — AUTHORITATIVE (`~/dept-bus/hub/briefs/provingground-layout.md`, from the user's render)
Relative positions + topology are FIXED; exact coords/dims are the dept's to parameterize as NAMED
DATA (no hardcoded dims). Rough cross/plus with branches. +x EAST, +y SOUTH. The M2 builder picks
coordinates that pass the non-overlap validator + render.
- **SPAWN room** — lower-LEFT — `spawn` marker (room center). Props: torch (wall), barrels + crate.
- **DEAD-END room** — upper-LEFT — room tagged `dead_end` (RED failure room), rubble props; ONE
  entrance (a door), NO onward exit (the blocked-path test).
- **KEY alcove** — CENTER, small — `pickup` marker `id="gold_key"`; one gated entrance off the
  central corridor.
- **NPC room** — upper-RIGHT — `npc` marker (room center, metadata `patrol=<patrol id>`) + a
  `patrol` path = rectangular CLOSED loop of ~4 waypoints around the room center; props barrels,
  torch; a door at its lower edge → central corridor/goal side.
- **GOAL room** — lower-RIGHT — `goal` marker (green tile) AND a `locked` **chest** marker
  (`key_id="gold_key"`); props barrels + crates, torch; a door at its upper edge.

**Corridors / connections (doors wooden unless noted):**
- SPAWN → a **two-turn corridor** (up, then turn toward center) — movement/camera/collision test.
- Off that corridor, a branch NW → the DEAD-END room (door, red-trimmed) — the blocked-path branch.
- KEY alcove opens off the central corridor (a gated/grille entrance).
- Corridor continues to the NPC room (door) and down to the GOAL room (door).
- The FINAL door into the GOAL room is `locked` (`key_id="gold_key"`) — key-before-door sequencing.

**Two-turn corridor (research-verified):** the spawn door and the next door face each other with
horizontal normals at DIFFERENT y → `corridorCenterline` emits the Z `{a,{xm,a.y},{xm,b.y},b}` (two
turns). The y-offset is NAMED DATA ≥ (corridor width + side-wall thickness) so the two legs read
distinctly. The NW branch to the dead-end is a 3rd plug + independent connection on the junction —
branching is DATA, not a routing mode.

## What the engine receives (the M4 manifest — drafted here, finalized at closeout)
- **markers:** `spawn`(spawn room) · `pickup` id `gold_key`(key alcove) · `npc`(npc room,
  metadata `patrol=<id>`) · `goal`(goal room) · `locked`/chest marker(goal room, metadata
  `key_id=gold_key`).
- **patrol:** one closed loop (~4 waypoints) referenced by the npc marker.
- **doors/locks:** the final GOAL-room door connection `locked=true, key_id=gold_key`. ONE key
  (`gold_key`) gates BOTH that door AND the chest.
- **room tag:** DEAD-END room tagged `dead_end`.
- **geometry:** 5 rooms + two-turn corridor + branch + gated alcove + the door connections.
- **props:** `blocks[]` asset placements — crates · barrels · torches (+ rubble in the dead-end).

## Gate log

### Research gate — 2026-06-19 — edi-dungeon-map-researcher (CLOSED ✅)
Doc: `docs/research/marker-patrol-lock-prior-art.md`. All four questions verdict **MATCH** vs prior
art (Tiled object layers / polylines / custom properties primary; Godot Marker2D; UVTT = confirmed
anti-pattern — JSON + VTT-centric + behavior baked in, learn-don't-adopt).
- **Markers** ✅ — canonical neutral shape `{id, type/role, position, free-form properties}` is a 1:1
  fit for `RoomFeature + id + metadata`. Keep string-only metadata; NO heavy `DraftingMarker` type.
- **Patrol** ✅ — `{id, waypoints[], closed}` referenced by npc id is canonical. **Contract:**
  `closed=true` means LOOP and the first waypoint is **NOT** repeated (Tiled polygon convention).
- **Lock** ✅ — `locked`/`key_id` as a recorded TAG (engine owns the rule) is standard; pickup-id ==
  door-key_id is the standard indirection. Connection-level is the clean neutral analogue of Tiled's
  door-object (cleaner than plug.flags).
- **Two-turn corridor** ✅ (verified against `DraftingCorridor.cpp`) — two doors with horizontal
  normals facing each other at different y emit the Z `{a,{xm,a.y},{xm,b.y},b}`; colinear y collapses
  to straight. **Layout rule:** to read as two distinct legs the y-offset must be NAMED DATA
  ≥ (corridor width + side-wall thickness). The 3-way junction/branch is just a 3-plug room with
  three independent connections — branching is DATA, not a routing mode.
- **Carry into reviewer gate:** keep all four proposals; adjust only (a) patrol closed-loop contract
  (no repeated first point), (b) connection comment must say "engine-interpreted TAGS, not edi rules",
  (c) the safe↔junction y-offset is named data ≥ corridor-width + side-wall-thickness.

### Refinements folded in — 2026-06-19 — HUB (canonical layout + 2 schema refinements)
Authoritative layout: `~/dept-bus/hub/briefs/provingground-layout.md` (from the user's reference render).
1. The pickup is the **gold key**, `id="gold_key"` — there is ONE key.
2. That ONE key gates **TWO** things: the final GOAL-room DOOR (a connection) AND a locked **chest**
   (an interactable MARKER) in the goal room. ⇒ the lock tag must be **GENERIC across both hosts**,
   not door-only (see revised §3). This SIMPLIFIES the original draft (was two keys key_red/key_green;
   now one gold_key gating door + chest).

### Reviewer gate — 2026-06-19 — edi-dungeon-map-reviewer (CLOSED ✅ BOUNDARY SETTLED: YES)
All four pieces ruled neutral, additive, correctly owned, achievable with NO version bump / NO
golden break. Rulings:
- **Q1 marker** ✅ ALLOWED — add `id` + `metadata` to `RoomFeature`; NO heavy `DraftingMarker`.
  `RoomFeature` is never MessagePack-serialized, so binary byte-identity is automatic. Chest lock
  rides `metadata` (`key_id=gold_key`); do NOT add a typed marker field; chest role rides `type`.
  Markers are VALUES in a vector (not object-id relations) ⇒ NO delete-cascade obligation.
- **Q2 patrol** ✅ ALLOWED — `MapPatrolPath {id,waypoints,closed}` on `MapSpec`; MapSpec-only
  (no document vector, no codec). `closed`=loop, no repeated first point = a DOCUMENTED INVARIANT.
- **Q3 lock** ✅ ALLOWED — typed `locked`+`keyId` on the connection does NOT violate the
  "deliberately no locked" comment (that guards a RULE; a TAG is fine) — but the builder MUST
  rewrite that comment to "engine-interpreted TAGS, not edi rules". Connection-level beats
  plug.flags. The generic split (typed cols on connection, k/v metadata on marker) is ACCEPTED —
  shared VOCABULARY, host-appropriate encoding; do NOT force unification.
- **Q4 export** ✅ — use the **Seam B (MapSpec) overload** (markers/patrols live only there). Seam
  B/C drift RESOLVED: charter wins — markers ship on Seam B; the brief's "Seam C" label is wrong,
  do not propagate. Columns: `markers[N]{room,id,role,x,y,meta}` (meta = `·`-joined `k=v`),
  `patrols[N]{id,closed,points}` (`·`-joined `x,y` pairs), and `locked,key_id` appended to
  `connections[]` CONDITIONALLY (pre-scan; absent when none locked). Existing reference golden
  stays byte-identical; new golden cases pin the marker/patrol/lock bytes.

### Planner fork decision — 2026-06-19 (drives reviewer guardrail #1)
**Add lock to `MapConnectionSpec` ONLY; DEFER the `DraftingDeclaredConnection` document twin.**
*Why:* the deliverable ships through the Seam B / MapSpec authoring path (`.map.toml` → TOON); the
document twin is NOT consumed by this campaign and its `.edidraw` codec needs the fiddly conditional-
emission proof (the `level` precedent always-writes ⇒ would break byte-identity). Deferring removes
guardrail #1's risk from M1 entirely. Named future slice: "lock on DraftingDeclaredConnection +
conditional `.edidraw` codec" — only when a live-document export needs it.

### Builder batch — pending (the ratified M1→M4 below; commit per slice; don't pause unless blocked)
**M1 — model spine:** `RoomFeature.id` + `RoomFeature.metadata`; `MapPatrolPath` + `MapSpec.patrols`;
`locked`+`keyId` on `MapConnectionSpec` (twin deferred). *Accept:* builds, ctest green, existing
`map_toon_export_tests` golden UNCHANGED.
**M2 — author + parse:** parser for `room.feature.<i>.{id,meta.<j>.{key,value}}`,
`map.patrol.<i>.{id,closed,point.<j>.{x,y}}`, `map.connection.<i>.{locked,key_id}`; author
`tests/data/provingground.map.toml` (5-room layout, y-offset NAMED data). *Accept:* parser
round-trips into structs; map passes non-overlap validator; offscreen render succeeds.
**M3 — export carries markers/patrols/lock:** Seam B overload emits the new sections, all conditional.
*Accept:* new golden cases pin the bytes; reference golden byte-identical.
**M4 — verify + manifest:** end-to-end author→export test asserting the manifest; `gold_key` matches
across pickup `id` / door `keyId` / chest meta `key_id`. *Accept:* manifest test green.

### Builder batch — 2026-06-19 — edi-dungeon-map-builder (COMPLETE ✅) + planner verification
SHAs: **M1 `8f240b5`** (model spine) · **M2 `68e3b94`** (parse + author `tests/data/provingground.map.toml`)
· **M3 `1f93288`** (Seam B TOON markers/patrols/lock) · **M4 `a5a4f13`** (manifest round-trip test).
- **Planner-verified (independent re-run, not trusted):** build clean (Release + Debug); code tree
  clean; map/marker/manifest/golden tests ALL GREEN (`map_spec_store`, `map_toon_export`,
  `map_provingground_manifest`, `map_regression_lock`, `map_determinism`); reference-dungeon golden
  BYTE-IDENTICAL (`connections[12]{from,to,type}`, no markers section — conditional-absence proven);
  scan clean (no js/qml, no json outside .claude, no QtQml/QtQuick); BOTH renders OK (map.png 152KB,
  pg.png 158KB); TOON export carries the full neutral doc — `connections` locked goal door
  `corridor,true,gold_key`, `markers[6]` (spawn/dead_end/pickup gold_key/npc+patrol/goal/chest+lock),
  `patrols[1]` `guard_loop,true,"64,8·74,8·74,16·64,16"` (closed loop, no repeated first pt).
- **Neutrality audit (planner read the M1/M2 diff):** AIRTIGHT — explicit "edi acts on neither,
  engine owns behavior" comments; patrol not a geometry variant; lock = TAG not RULE (old "no locked"
  comment reconciled). Three-tier law honored.
- **Builder deviation (audited, ACCEPTED):** added a `map.block.<i>.{asset_ref,x,y,rotation,scale,name}`
  TOML parser — `MapSpec::blocks` existed but had no authoring path; props required it. Additive,
  neutral (asset_ref records, realizer owns meshes), coordinate frame correct (authored feet UNSCALED,
  matching RoomFeature; createMapFromSpec applies the scale). Sound.
- **Two PRE-EXISTING issues surfaced (NOT this campaign's regressions — for HUB triage):**
  1. **7-test Release-build SEGFAULT** in drafting-core (`drafting_snap/hit_test/layer_ops/plot_plan/
     graph_sync`, `drawing_document_controller`, `edi_shell_window`). PROVEN pre-existing: fresh
     Debug build at HEAD passes all; fresh Release build at HEAD **and** at pristine pre-campaign
     `e34e773` both SEGFAULT identically ⇒ a Release optimization/UB in the drafting core, predating
     this campaign. Debug = 118/118 green. **Implication:** the charter green gate (`cmake --build
     build` defaults Release) needs this drafting-core UB triaged separately.
  2. **`--export-map` CLI aborts on process teardown** (exit 134) AFTER correctly writing the file —
     reference dungeon export aborts identically ⇒ pre-existing teardown crash, not in the new code.
     Export CONTENT is correct + complete; library-path tests (which call the function directly) are
     green. Likely the same Release teardown issue.

## Open questions / blockers
- ALL boundary questions RESOLVED by the reviewer gate (see above). No open blockers for THIS campaign.
- TWO pre-existing drafting-core issues flagged for HUB triage (Release SEGFAULTs + export teardown
  abort) — out of this department's ownership; deliverables verified via Debug build + library tests.
- Builder guardrails (from reviewer): (1) lock on MapConnectionSpec only this campaign (twin
  deferred — no codec trap); (2) TOON conditional emission everywhere (new sections absent when
  empty, lock columns absent when none locked); (3) no magic dims — corridor y-offset ≥ (corridor
  width + side-wall thickness) as NAMED data; (4) id-uniqueness + dangling-ref validation in the
  parser (markers/patrols/key matching), but edi enforces NO lock rule; (5) name it Seam B not C;
  (6) no JSON/qml/subclassing.

## Next
- Open the BUILDER BATCH for M1→M4 (boundary settled). Commit per slice; green gate + reference-
  dungeon render per commit; NO push. Then closeout with the final manifest.
</content>
</invoke>
