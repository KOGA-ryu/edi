// Phase-1 CANARY — the regression-lock golden for the reference dungeon.
//
// WHY this test exists (and why it comes FIRST in the Phase-1 additive campaign):
//   Phase 1 adds ~12 fields to the map data structs (DraftingMapRoom, DraftingPlug,
//   DraftingDeclaredConnection, etc.) using an ADDITIVE, no-version-bump strategy:
//   every new field has a neutral default so that existing documents, exports, and
//   TOON outputs are byte-identical before and after.  The promise is easy to state
//   but easy to accidentally break — a new field with the wrong default can silently
//   shift a TOON number or add/drop an object.
//
//   This test is the SAFETY NET.  It pins two invariants that must hold through every
//   Phase-1 slice:
//     1. The reference-dungeon TOON (12 rooms, 26 plugs, 12 connections) must be
//        BYTE-IDENTICAL to the baseline captured at master @0d5ca60.  sha256 of the
//        1685-byte golden = 6c632293229e43c9d57cb6aae443f6d4f8c93927c3196e001fae8f2c3012b0e3
//     2. The DraftingObject count produced by createMapFromSpec on the same dungeon
//        must equal the pinned value (walls, door markers, plug anchors, door leaves,
//        corridor walls).  This count is ORDER-INDEPENDENT; it catches a new field
//        that accidentally causes a branch to create or skip a geometry object.
//
// HALT PROTOCOL:  if either assertion drifts after a Phase-1 additive slice, DO NOT
// re-bless this file to hide it.  A drift means the new field's default is NOT
// neutral → the change is not behavior-preserving → STOP and report to edi-dungeon-map-planner.
//
// Source: ~/dept-bus/SPATIAL-MODEL-BACKLOG.md (Phase-1 EXECUTION); edi-ui canary
//         baseline in docs/handoffs/ui-20260618-spatial-phase1.md.

#include "core/DrawingCore.h"
#include "drafting/DraftingDocument.h"
#include "drafting/DraftingRoom.h"
#include "io/MapToonExport.h"
#include "io/RoomSpecStore.h"

#include <QCoreApplication>

#include <cassert>
#include <fstream>
#include <string>

// EDI_TESTS_DATA_DIR is injected by CMakeLists.txt as the absolute path to
// tests/data/.  This avoids a hardcoded path and lets the test find its
// fixture regardless of which directory ctest is invoked from.
#ifndef EDI_TESTS_DATA_DIR
#error "EDI_TESTS_DATA_DIR must be defined by CMakeLists.txt"
#endif

static std::string readFile(const std::string &path)
{
    // std::ifstream for text TOML — no Qt required here.  std::string::assign
    // with an istreambuf_iterator is the idiomatic single-call file-to-string.
    std::ifstream f(path);
    assert(f.is_open() && "test fixture file not found — check EDI_TESTS_DATA_DIR");
    return std::string(std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>());
}

int main(int argc, char **argv)
{
    // DrawingDocumentController uses Qt's object system internally (id generation,
    // signals) even in a headless context.  QCoreApplication must be alive first.
    QCoreApplication app(argc, argv);

    const std::string toml = readFile(EDI_TESTS_DATA_DIR "/dungeon.map.toml");

    // -------------------------------------------------------------------------
    // Step 1 — Parse the reference dungeon (Seam A).
    // canvasPerUnit=1.0 keeps canvas coords == authored feet, same as the CLI.
    // -------------------------------------------------------------------------
    const edi::io::MapSpecParseResult parsed = edi::io::parseMapSpecToml(toml, 1.0);
    assert(parsed.ok && "dungeon.map.toml must parse without errors");
    assert(parsed.spec.rooms.size() == 12);
    assert(parsed.spec.connections.size() == 12);

    // -------------------------------------------------------------------------
    // Step 2 — TOON byte-lock (Seam B export, MapSpec overload).
    //
    // Title: "dungeon" (QFileInfo::baseName() of "dungeon.map.toml" strips
    // everything from the first '.' — matching the CLI at main.cpp:218).
    // Units: "feet" (default).  No sceneScale → no `scale:` header line.
    //
    // The golden string below is the exact 1685-byte output of:
    //   ./build/edi --map-file tests/data/dungeon.map.toml --export-map /tmp/c.toon
    // at master @0d5ca60.  sha256:
    //   6c632293229e43c9d57cb6aae443f6d4f8c93927c3196e001fae8f2c3012b0e3
    //
    // DO NOT re-bless this string if a Phase-1 slice changes the output —
    // report the drift (it means a field's default is not neutral).
    // -------------------------------------------------------------------------
    const std::string toon = edi::io::exportMapToToon(parsed.spec, "dungeon");

    // clang-format off
    const std::string expectedToon =
        "kind: map\n"
        "title: dungeon\n"
        "units: feet\n"
        "\n"
        "rooms[12]{name,origin,size,material}:\n"
        "  entrance,\"21,47.5\",\"6,2\",stone\n"
        "  room1,\"18,35\",\"12,11\",stone\n"
        "  room2,\"18,17\",\"12,14\",stone\n"
        "  room3,\"2,19\",\"11,11\",stone\n"
        "  room4,\"2,34\",\"12,12\",stone\n"
        "  room5,\"34,18\",\"13,13\",stone\n"
        "  room6,\"34,34\",\"13,4\",stone\n"
        "  room7,\"34,40\",\"13,4\",stone\n"
        "  room8,\"34,46\",\"13,3\",stone\n"
        "  room9,\"31,2\",\"16,13\",stone\n"
        "  room10,\"2,2\",\"12,12\",stone\n"
        "  junction,\"20,6\",\"5,4\",stone\n"
        "\n"
        "plugs[26]{room,name,edge,type,connected,flags}:\n"
        "  entrance,up,N,door,true,\"\"\n"
        "  room1,entrance,S,door,true,\"\"\n"
        "  room1,to_2,N,door,true,\"\"\n"
        "  room2,to_1,S,door,true,\"\"\n"
        "  room2,to_j,N,door,true,\"\"\n"
        "  room2,to_3,W,door,true,\"\"\n"
        "  room2,to_5,E,door,true,\"\"\n"
        "  room3,to_2,E,door,true,\"\"\n"
        "  room3,to_4,S,door,true,\"\"\n"
        "  room4,to_3,N,door,true,\"\"\n"
        "  room4,secret_s,S,secret,false,\"\"\n"
        "  room5,to_2,W,door,true,\"\"\n"
        "  room5,to_9,N,door,true,\"\"\n"
        "  room5,to_6,S,door,true,\"\"\n"
        "  room6,to_5,N,door,true,\"\"\n"
        "  room6,to_7,S,door,true,\"\"\n"
        "  room7,to_6,N,door,true,\"\"\n"
        "  room7,to_8,S,door,true,\"\"\n"
        "  room8,to_7,N,door,true,\"\"\n"
        "  room9,to_j,W,door,true,\"\"\n"
        "  room9,to_5,S,door,true,\"\"\n"
        "  room10,to_j,E,door,true,\"\"\n"
        "  room10,secret_ne,N,secret,false,\"\"\n"
        "  junction,to_2,S,door,true,\"\"\n"
        "  junction,to_10,W,door,true,\"\"\n"
        "  junction,to_9,E,door,true,\"\"\n"
        "\n"
        "connections[12]{from,to,type}:\n"
        "  entrance.up,room1.entrance,corridor\n"
        "  room1.to_2,room2.to_1,corridor\n"
        "  room2.to_3,room3.to_2,corridor\n"
        "  room2.to_5,room5.to_2,corridor\n"
        "  room2.to_j,junction.to_2,corridor\n"
        "  junction.to_10,room10.to_j,corridor\n"
        "  junction.to_9,room9.to_j,corridor\n"
        "  room3.to_4,room4.to_3,corridor\n"
        "  room5.to_9,room9.to_5,corridor\n"
        "  room5.to_6,room6.to_5,corridor\n"
        "  room6.to_7,room7.to_6,corridor\n"
        "  room7.to_8,room8.to_7,corridor\n";
    // clang-format on

    assert(toon == expectedToon);

    // -------------------------------------------------------------------------
    // Step 3 — Object-count lock (Seam A → in-memory document).
    //
    // createMapFromSpec materializes the MapSpec into DraftingObjects: room walls
    // (4 per room ± splits for openings), door-leaf bands (1 per connected plug),
    // plug anchor markers (1 per plug), and corridor side-walls (2 per centerline
    // segment per connection).  The count is order-independent — the unordered_map
    // corridor iteration affects wall geometry ORDER, not COUNT.
    //
    // DRIFT means a Phase-1 new-field default caused a branch to create or skip
    // an object — not behavior-preserving → HALT and report.
    // -------------------------------------------------------------------------
    DrawingDocumentController controller;
    assert(controller.createMapFromSpec(parsed.spec, 1.0));

    const std::size_t objCount = controller.draftingDocument().objects.size();

    // 170 objects: 12 rooms × (4 base walls + 1 per opening + 1 per plug anchor)
    // + 1 door-leaf per connected plug (24 total)
    // + corridor side-walls (2 per centerline segment per connection, routed
    //   around obstacles — some corridors use A* and get more than 1 segment).
    // Pinned by running the test at master @0d5ca60.  The backlog said "74" but
    // that was a pre-A*-routing estimate; 170 is the authoritative live value.
    // DRIFT here means a Phase-1 new-field default caused a branch to skip or
    // create a geometry object → not behavior-preserving → halt and report.
    assert(objCount == 170);

    // -------------------------------------------------------------------------
    // Step 4 (D15-A3) — the lock TAG survives author -> createMapFromSpec ->
    // document.  Before D15 the document twin DraftingDeclaredConnection had no
    // locked/keyId, so createMapFromSpec copied only `type` and the lock was
    // dropped silently.  Mark one authored connection locked, re-materialize,
    // and assert the document connection now carries it.  (Neutral tag copy —
    // edi never branches on it; the engine owns the rule past Seam B.)
    // -------------------------------------------------------------------------
    {
        edi::drafting::MapSpec lockedSpec = parsed.spec;
        assert(!lockedSpec.connections.empty());
        lockedSpec.connections[0].locked = true;
        lockedSpec.connections[0].keyId = "gold_key";

        DrawingDocumentController lockedController;
        assert(lockedController.createMapFromSpec(lockedSpec, 1.0));

        const auto &docConns = lockedController.draftingDocument().connections;
        // Exactly one connection carries the lock (the one we marked); the rest stay
        // neutral-default.  Order-independent: count by predicate, not by index.
        std::size_t lockedCount = 0;
        for (const auto &c : docConns) {
            if (c.locked) {
                ++lockedCount;
                assert(c.keyId == "gold_key");
            } else {
                assert(c.keyId.empty());
            }
        }
        assert(lockedCount == 1);
    }

    return 0;
}
