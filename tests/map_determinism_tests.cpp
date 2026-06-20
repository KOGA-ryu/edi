// Phase-1 DETERMINISM GATE (brief 069) — "same input → same dungeon".
//
// WHAT THIS TEST CATCHES — and what it CANNOT (read this before trusting it):
//
//   It runs createMapFromSpec TWICE in ONE process (ctrlA + ctrlB in the same
//   main()), then diffs the exported TOON strings.  So it catches SAME-PROCESS
//   ordering / creation non-determinism: any source whose result varies between
//   two runs that share the same address space and entropy.  That is exactly the
//   class of bug it was written to guard — the unordered_map -> std::map fix
//   below (the iteration order that decided corridor obstacle order).
//
//   It CANNOT catch CROSS-PROCESS / ASLR variance.  Two runs in one process see
//   the SAME address-space layout and the SAME process-lifetime hash seed, so an
//   unordered_map whose order depends on ASLR or a per-process seed would iterate
//   IDENTICALLY in both ctrlA and ctrlB — this test would stay green while a fresh
//   process (or a different build/machine) produced a different dungeon.  This
//   test does NOT own that guarantee.
//
//   The build-independent BYTE guarantee is owned elsewhere: map_regression_lock_tests
//   pins the reference-dungeon TOON to a RECORDED sha256
//   (6c632293...b0e3).  That fixed hash is what actually fails if a different build
//   emits different bytes — it is the cross-build anchor this in-process diff is not.
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

#include <cassert>
#include <fstream>
#include <string>

#ifndef EDI_TESTS_DATA_DIR
#error "EDI_TESTS_DATA_DIR must be defined by CMakeLists.txt"
#endif

static std::string readFile(const std::string &path)
{
    std::ifstream f(path);
    assert(f.is_open() && "test fixture file not found — check EDI_TESTS_DATA_DIR");
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
    assert(parsed.ok && "dungeon.map.toml must parse without errors");

    // -------------------------------------------------------------------------
    // Run 1: fresh controller A.
    // -------------------------------------------------------------------------
    DrawingDocumentController ctrlA;
    const bool okA = ctrlA.createMapFromSpec(parsed.spec, 1.0);
    assert(okA && "run A: createMapFromSpec must succeed");

    const std::string toonA = edi::io::exportMapToToon(ctrlA.draftingDocument(), "dungeon");
    const std::size_t countA = ctrlA.draftingDocument().objects.size();

    // -------------------------------------------------------------------------
    // Run 2: independent fresh controller B — no shared state with A.
    // -------------------------------------------------------------------------
    DrawingDocumentController ctrlB;
    const bool okB = ctrlB.createMapFromSpec(parsed.spec, 1.0);
    assert(okB && "run B: createMapFromSpec must succeed");

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
    assert(countA == countB && "object counts differ between run A and run B — non-deterministic creation");
    assert(toonA  == toonB  && "TOON output differs between run A and run B — non-deterministic ordering");

    return 0;
}
