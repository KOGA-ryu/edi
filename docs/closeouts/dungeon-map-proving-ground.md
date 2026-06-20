# Closeout — dungeon-map-20260619-provingground (the neutral proving-ground map + marker layer)

Freezes the boundary so future work does not re-litigate it. Campaign: author a NEUTRAL
"first-playable proving-ground" map for the user's game-engine testing and export it to TOON.
**Map + export only — no render, no behavior.** Status: **COMPLETE ✅** (M1–M4 landed on
`dept/dungeon-map`; NOT pushed — hub owns the origin bridge).

## What shipped (SHAs on dept/dungeon-map)
- **M1 `8f240b5`** — neutral marker/patrol/lock MODEL spine.
- **M2 `68e3b94`** — TOML parser for the new fields + the authored `tests/data/provingground.map.toml`.
- **M3 `1f93288`** — Seam B (MapSpec) TOON export carries markers / patrols / connection lock.
- **M4 `a5a4f13`** — end-to-end manifest round-trip test.

## The frozen boundary (do not re-litigate)
Three-tier law: **edi RECORDS the neutral map + entity markers; the ENGINE owns ALL behavior**
(movement, camera, collision, patrol stepping, pickup state, key-before-door, win). Every field
added is a record or a neutral TAG; edi acts on none of it.

- **Marker** = the existing `RoomFeature` (`src/drafting/DraftingRoom.h`), extended ADDITIVELY with
  `std::string id` + `std::vector<std::pair<std::string,std::string>> metadata`. No heavy
  `DraftingMarker` type. `role` rides the existing `type`; `anchor` rides `x,y` (room-local authored
  feet). RoomFeature is never MessagePack-serialized ⇒ no on-disk byte-identity risk.
- **Patrol** = new `MapPatrolPath {id, waypoints[], closed}` on `MapSpec` (MapSpec-only; no document
  vector, no codec). **Closed-loop invariant:** `closed=true` = LOOP, first waypoint NOT repeated;
  the ENGINE closes it. Referenced by an npc marker's `metadata` `patrol=<id>`.
- **Lock** = a GENERIC neutral tag, ONE vocabulary on TWO hosts: typed `locked`+`keyId` columns on
  the connection (`MapConnectionSpec`); k/v `key_id=<id>` in a marker's `metadata` (the chest). The
  connection's old "deliberately no locked" comment meant no RULE — a TAG the engine interprets is
  inside the mandate. **DEFERRED:** the `DraftingDeclaredConnection` document twin + its conditional
  `.edidraw` codec (this campaign ships only the Seam B / MapSpec authoring path).
- **Export** = the **Seam B (MapSpec) overload** of `exportMapToToon` (`src/io/MapToonExport.cpp`).
  Naming: markers ship on **Seam B**, not Seam C (charter wins over the brief's loose label). All
  new sections/columns are CONDITIONAL — absent when empty, so maps without markers (the reference
  dungeon) stay byte-identical.

## THE MANIFEST — exactly what the engine receives (deliverable #4; wire your loader to this)
Authored from ONE file (`tests/data/provingground.map.toml`) → exported to neutral TOON. Sections:

```
kind: map · title: provingground · units: feet
rooms[6]{name,origin,size,material}        spawn, deadend(rubble), alcove, npc, goal, junction
plugs[10]{room,name,edge,type,connected,flags}   doors + the alcove portcullis (gated entrance)
connections[5]{from,to,type,locked,key_id}
  spawn.out → junction.to_spawn     corridor  (the TWO-TURN corridor; y-offset 13 ft, NAMED data)
  junction.to_deadend → deadend.in  corridor  (the branch to the RED dead-end)
  junction.to_alcove → alcove.gate  passage   (the gated key alcove)
  junction.to_npc → npc.down        corridor
  junction.to_goal → goal.entry     corridor  locked=true  key_id=gold_key   ← key-before-door
markers[6]{room,id,role,x,y,meta}
  spawn   player_spawn  spawn       (player start)
  deadend deadend_tag   dead_end    (the failure room tag)
  alcove  gold_key      pickup      (THE key — its id == every key_id below)
  npc     guard         npc         meta: patrol=guard_loop
  goal    exit_goal     goal        (the win tile)
  goal    treasure      chest       meta: locked=true·key_id=gold_key   ← locked interactable
patrols[1]{id,closed,points}
  guard_loop  closed=true  64,8·74,8·74,16·64,16   (rectangular loop, 4 waypoints, NPC room)
```

**Key indirection (the engine wires these; edi only records them):** the pickup `gold_key` (marker
`id`) is the `key_id` of BOTH the final goal-room door (connection) AND the chest (marker meta).
One key gates two things — the key-before-door sequencing test, ×2.

**Engine-reader notes:** a `·` (U+00B7) joins multi-value cells (marker `meta` `k=v` runs; patrol
`points` `x,y` pairs). Empty cells (`""`) are tolerated. `closed=true` means close the loop without
a repeated final point. `locked`/`key_id` are STATE + indirection edi records; edi enforces no gate.

## Verification (planner re-ran independently)
Debug build 118/118 green; map/marker/manifest/golden tests green; reference-dungeon golden
byte-identical (conditional-absence proven); scan clean; both maps render offscreen; TOON export
content correct + complete.

## Carried out of this campaign (for the HUB / future slices)
- **Pre-existing (NOT this campaign):** (1) a 7-test **Release-build SEGFAULT** in drafting-core
  (proven at pristine `e34e773` in Release; Debug is clean) — a drafting-core UB the optimizer
  exposes, needs separate triage; (2) `--export-map` **CLI aborts on teardown** after correctly
  writing the file (reference dungeon identical) — same Release teardown class. Neither is a marker
  regression.
- **Deferred future slice:** lock on `DraftingDeclaredConnection` + conditional `.edidraw` codec —
  only when a live-document (not authored-`.map.toml`) export needs the lock tag.
- **Not done (out of scope):** patrol-path rendering in the Map workspace (records only; no crash).
- **Prior art:** `docs/research/marker-patrol-lock-prior-art.md` (Tiled/Godot validated the schema).
</content>
