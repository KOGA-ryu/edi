// Phase-1 DETERMINISM GATE (brief 069) — "same input → same dungeon".
//
// WHY a DOUBLE-RUN test owns determinism that a single golden cannot:
//
//   The regression-lock canary (map_regression_lock_tests) checks that one run
//   matches a pre-recorded golden.  If the dungeon happened to be consistent
//   (e.g. the unordered_map hash collisions always fell the same way on this
//   build) the canary would stay green even though a DIFFERENT machine or
//   build produces a different dungeon.  A double-run test sidesteps that
//   entirely: run createMapFromSpec TWICE with IDENTICAL input, then diff the
//   exported TOON strings.  If they differ, non-determinism is proven regardless
//   of what the golden says.  This test owns the invariant globally.
//
// WHY unordered_map iteration was the culprit (and why std::map fixes it):
//
//   createMapFromSpec populates `roomRectByName` (room name → footprint) as an
//   unordered_map.  When building the obstacle list for each corridor it iterates
//   that map, whose order is NOT specified by the C++ standard — it depends on the
//   hash function, load-factor, and even address-space layout randomisation.  After
//   switching roomRectByName to std::map (ordered by key = room name, lexicographic)
//   the obstacle list is always in alphabetical room-name order, independent of
//   platform or run-to-run entropy.
//
// HALT PROTOCOL: if this test fails (the two TOON strings differ), DO NOT work
// around it — report the non-determinism source to edi-dungeon-map-planner.

#include "core/DrawingCore.h"
#include "drafting/DraftingDocument.h"
#include "drafting/DraftingRoom.h"
#include "io/MapToonExport.h"
#include "io/RoomSpecStore.h"

#include <QCoreApplication>

#include "EdiAssert.h"
#include <fstream>
#include <string>

#ifndef EDI_TESTS_DATA_DIR
#error "EDI_TESTS_DATA_DIR must be defined by CMakeLists.txt"
#endif

static std::string readFile(const std::string &path)
{
    std::ifstream f(path);
    EDI_CHECK(f.is_open() && "test fixture file not found — check EDI_TESTS_DATA_DIR");
    return std::string(std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>());
}

int main(int argc, char **argv)
{
    // DrawingDocumentController's serial counter and Qt object system need a
    // QCoreApplication — same requirement as map_regression_lock_tests.
    QCoreApplication app(argc, argv);

    const std::string toml = readFile(EDI_TESTS_DATA_DIR "/dungeon.map.toml");

    // Parse ONCE — the spec itself is deterministic (a std::vector of NamedRoomSpec
    // in TOML order); both controllers will receive identical input.
    const edi::io::MapSpecParseResult parsed = edi::io::parseMapSpecToml(toml, 1.0);
    EDI_CHECK(parsed.ok && "dungeon.map.toml must parse without errors");

    // -------------------------------------------------------------------------
    // Run 1: fresh controller A.
    // -------------------------------------------------------------------------
    DrawingDocumentController ctrlA;
    const bool okA = ctrlA.createMapFromSpec(parsed.spec, 1.0);
    EDI_CHECK(okA && "run A: createMapFromSpec must succeed");

    const std::string toonA = edi::io::exportMapToToon(ctrlA.draftingDocument(), "dungeon");
    const std::size_t countA = ctrlA.draftingDocument().objects.size();

    // -------------------------------------------------------------------------
    // Run 2: independent fresh controller B — no shared state with A.
    // -------------------------------------------------------------------------
    DrawingDocumentController ctrlB;
    const bool okB = ctrlB.createMapFromSpec(parsed.spec, 1.0);
    EDI_CHECK(okB && "run B: createMapFromSpec must succeed");

    const std::string toonB = edi::io::exportMapToToon(ctrlB.draftingDocument(), "dungeon");
    const std::size_t countB = ctrlB.draftingDocument().objects.size();

    // -------------------------------------------------------------------------
    // Assert byte-identical TOON output and equal object counts.
    //
    // toonA == toonB is the primary determinism assertion: identical input must
    // produce an IDENTICAL TOON string, character by character.
    //
    // countA == countB is a secondary sanity-check: if geometry creation were
    // non-deterministic (a branch that randomly creates or skips an object), the
    // counts would differ.  The primary check catches ordering non-determinism;
    // the count check catches creation non-determinism — they are complementary.
    // -------------------------------------------------------------------------
    EDI_CHECK(countA == countB && "object counts differ between run A and run B — non-deterministic creation");
    EDI_CHECK(toonA  == toonB  && "TOON output differs between run A and run B — non-deterministic ordering");

    return 0;
}
