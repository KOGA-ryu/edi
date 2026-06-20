// M4 — proving-ground MANIFEST round-trip (Seam A -> Seam B).
//
// The end-to-end contract for the first-playable fixture: parse the AUTHORED
// tests/data/provingground.map.toml and project it through exportMapToToon (the
// MapSpec / Seam B overload), then assert the engine receives the full neutral
// manifest the campaign promises — every marker, the patrol, the locks, and the
// dead-end tag — AND that the single gold_key id is the SAME string across the
// pickup, the door, and the chest (the key-before-door indirection).
//
// THE LAW: every assertion here checks a RECORD or a NEUTRAL TAG. The test proves
// edi carried the data; it asserts NOTHING about behaviour (no gate is enforced, no
// patrol is walked) — those are the engine's, downstream of Seam B.
//
// This is the authored-file companion to map_regression_lock_tests (which pins the
// reference dungeon). It links only the parser + exporter — no controller, no Qt —
// because the manifest lives entirely in the MapSpec -> TOON projection.

#include "drafting/DraftingRoom.h"
#include "io/MapToonExport.h"
#include "io/RoomSpecStore.h"

#include "EdiAssert.h"
#include <fstream>
#include <string>

#ifndef EDI_TESTS_DATA_DIR
#error "EDI_TESTS_DATA_DIR must be defined by CMakeLists.txt"
#endif

static std::string readFile(const std::string &path)
{
    std::ifstream f(path);
    EDI_CHECK(f.is_open() && "fixture not found — check EDI_TESTS_DATA_DIR");
    return std::string(std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>());
}

// True iff `needle` appears in `hay` (a readable contains()).
static bool has(const std::string &hay, const std::string &needle)
{
    return hay.find(needle) != std::string::npos;
}

int main()
{
    const std::string toml = readFile(EDI_TESTS_DATA_DIR "/provingground.map.toml");

    // canvasPerUnit=1.0 keeps authored feet == the numbers in the TOON (the CLI's
    // export-map path uses the same), so the manifest assertions read in feet.
    const edi::io::MapSpecParseResult parsed = edi::io::parseMapSpecToml(toml, 1.0);
    EDI_CHECK(parsed.ok && "provingground.map.toml must parse without errors");
    EDI_CHECK(parsed.spec.rooms.size() == 6);
    EDI_CHECK(parsed.spec.connections.size() == 5);
    EDI_CHECK(parsed.spec.patrols.size() == 1);

    const std::string toon = edi::io::exportMapToToon(parsed.spec, "provingground");

    // --- markers: spawn / pickup / npc / goal / chest -------------------------
    EDI_CHECK(has(toon, "markers["));
    // spawn marker in the spawn room.
    EDI_CHECK(has(toon, "spawn,player_spawn,spawn,"));
    // pickup gold_key in the alcove.
    EDI_CHECK(has(toon, "alcove,gold_key,pickup,"));
    // npc with metadata patrol=guard_loop.
    EDI_CHECK(has(toon, "npc,guard,npc,"));
    EDI_CHECK(has(toon, "patrol=guard_loop"));
    // goal marker.
    EDI_CHECK(has(toon, "goal,exit_goal,goal,"));
    // chest marker carrying the lock metadata.
    EDI_CHECK(has(toon, "goal,treasure,chest,"));
    EDI_CHECK(has(toon, "key_id=gold_key"));

    // --- patrol: the closed loop the npc references ---------------------------
    EDI_CHECK(has(toon, "patrols[1]{id,closed,points}:"));
    EDI_CHECK(has(toon, "guard_loop,true,"));
    // ~4 waypoints: count the comma-separated x,y pairs in the patrol's points run.
    EDI_CHECK(parsed.spec.patrols[0].id == "guard_loop");
    EDI_CHECK(parsed.spec.patrols[0].closed);
    EDI_CHECK(parsed.spec.patrols[0].waypoints.size() == 4);

    // --- the FINAL goal-room door: locked + key_id gold_key -------------------
    EDI_CHECK(has(toon, "connections[5]{from,to,type,locked,key_id}:"));
    EDI_CHECK(has(toon, "junction.to_goal,goal.entry,corridor,true,gold_key"));

    // --- the dead-end room tag ------------------------------------------------
    // Authored as a neutral marker (type=dead_end) in the upper-left room.
    EDI_CHECK(has(toon, "deadend,deadend_tag,dead_end,"));

    // --- gold_key matches across pickup id / door keyId / chest meta key_id ---
    // The key-before-door indirection: ONE id string ties the three together.
    // Find each in the parsed structs (not just the text) so a typo'd id fails.
    const std::string kKey = "gold_key";

    // 1) the pickup marker's id.
    bool pickupMatch = false;
    for (const auto &room : parsed.spec.rooms) {
        for (const auto &feature : room.spec.features) {
            if (feature.type == "pickup" && feature.id == kKey) {
                pickupMatch = true;
            }
        }
    }
    EDI_CHECK(pickupMatch && "a pickup marker must have id gold_key");

    // 2) the locked door connection's keyId.
    bool doorMatch = false;
    for (const auto &c : parsed.spec.connections) {
        if (c.locked && c.keyId == kKey) {
            doorMatch = true;
        }
    }
    EDI_CHECK(doorMatch && "a locked connection must carry keyId gold_key");

    // 3) the chest marker's metadata key_id.
    bool chestMatch = false;
    for (const auto &room : parsed.spec.rooms) {
        for (const auto &feature : room.spec.features) {
            if (feature.type != "chest") {
                continue;
            }
            for (const auto &kv : feature.metadata) {
                if (kv.first == "key_id" && kv.second == kKey) {
                    chestMatch = true;
                }
            }
        }
    }
    EDI_CHECK(chestMatch && "the chest marker's metadata key_id must be gold_key");

    // All three resolved to the SAME id string — the manifest is internally
    // consistent (the parser's referential-integrity check guarantees this, and
    // this asserts it end-to-end on the shipped fixture).
    EDI_CHECK(pickupMatch && doorMatch && chestMatch);

    return 0;
}
