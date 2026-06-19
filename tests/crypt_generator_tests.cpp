// G2 crypt generator end-to-end test — pins the full chain:
//   buildCryptMapSpec(S) -> createMapFromSpec -> exportMapToToon(doc, ..., S)
// for three uniform scale values S ∈ {1, 2, 4}.
//
// WHY three scales, not just S=1?
//   The scale knob (brief 047) is the contract: the same named data tables must
//   produce three byte-identical TOON fixtures.  Testing all three proves that
//   EVERY dimension (room origin, room size, block position) is multiplied by S
//   and that `at` re-derives correctly — a shear would be caught by the anchor
//   colinearity check; a forgotten `* scale` would be caught by the full-string
//   golden match.
//
// WHY assert the FULL TOON string, not just individual fields?
//   The `%g` format of `num()` hides precision surprises (47.5*2 = 95, not 95.0).
//   Byte-matching against the fixture files guarantees formatting AND geometry.
//
// Golden fixtures:
//   S=1 → crypt_base.toon     (no `scale:` line; blocks[3] at base positions)
//   S=2 → crypt_doubled.toon  (`scale: 2`; rooms ×2; blocks ×2)
//   S=4 → crypt_doubled2.toon (`scale: 4`; rooms ×4; blocks ×4)

#include "core/DrawingCore.h"
#include "drafting/DraftingDocument.h"
#include "drafting/DraftingRoom.h"
#include "io/CryptGenerator.h"
#include "io/MapToonExport.h"

#include <QCoreApplication>

#include <cassert>
#include <cmath>
#include <string>

// Helper: compute the expected colinear Y for plugs at a given scale.
// entrance East-edge midpoint: anchor.y = origin.y + at = 5*S + 7.5*S = 12.5*S
// crypt    West-edge midpoint: anchor.y = origin.y + height - at = 0 + 25*S - 12.5*S = 12.5*S
// Both equal 12.5*S for any S → straight corridor, no bend.
static double expectedColinearY(double S) { return 12.5 * S; }

int main(int argc, char **argv)
{
    // QCoreApplication is required by DrawingDocumentController: Qt id generation
    // and signal machinery need an application context even when headless.
    QCoreApplication app(argc, argv);

    // -------------------------------------------------------------------------
    // S = 1  (base geometry, no `scale:` header line)
    // Golden: crypt_base.toon  — blocks[3] at base positions
    // -------------------------------------------------------------------------
    {
        // Step 1 — build the MapSpec at S=1 (purely C++, no Qt involved).
        // S=1: entrance 15×15@(0,5), crypt 25×25@(35,0), straight corridor.
        // The generator iterates kCryptRooms/kCryptPlugs/kCryptConns/kCryptBlocks;
        // with scale=1.0 the base positions pass through unchanged.
        edi::drafting::MapSpec spec = edi::io::buildCryptMapSpec(1.0);

        assert(spec.rooms.size() == 2);
        assert(spec.rooms[0].name == "entrance");
        assert(spec.rooms[1].name == "crypt");
        assert(spec.connections.size() == 1);
        assert(spec.blocks.size() == 3); // G2 adds sarcophagus / brazier / stair

        // Step 2 — materialize the document at 1:1 (canvas unit = authored foot).
        DrawingDocumentController controller;
        assert(controller.createMapFromSpec(spec, 1.0));
        const edi::drafting::DraftingDocument &doc = controller.draftingDocument();
        assert(doc.rooms.size() == 2);
        assert(doc.plugs.size() == 2);
        assert(doc.connections.size() == 1);

        // Step 3 — Seam C export with sceneScale=1.0 → NO `scale:` header line.
        // WHY omit at S=1? Engine treats missing => 1; omitting keeps existing TOON
        // consumers backward-compatible. THE FENCE: the feet ARE the authored values.
        const std::string toon = edi::io::exportMapToToon(doc, "crypt", "feet", 1.0);

        // Step 4 — pin the golden (crypt_base.toon byte-identical contract).
        // Number format: `%g` — 47.5→"47.5", 12.5→"12.5", 0→"0", 5→"5", etc.
        // Coordinate cells are double-quoted (they contain commas).
        // connected=true/false is bare; plug flag "crypt" stays bare (no delimiters).
        const std::string expected_s1 =
            "kind: map\n"
            "title: crypt\n"
            "units: feet\n"
            "\n"
            "rooms[2]{name,origin,size,material}:\n"
            "  entrance,\"0,5\",\"15,15\",stone\n"
            "  crypt,\"35,0\",\"25,25\",stone\n"
            "\n"
            "plugs[2]{room,name,edge,type,connected,flags}:\n"
            "  entrance,to_crypt,E,door,true,crypt\n"
            "  crypt,to_entrance,W,door,true,crypt\n"
            "\n"
            "connections[1]{from,to,type}:\n"
            "  entrance.to_crypt,crypt.to_entrance,corridor\n"
            "\n"
            "blocks[3]{room,asset,origin,scale,rotation}:\n"
            "  crypt,crypt.sarcophagus,\"47.5,12.5\",1,0\n"
            "  crypt,crypt.brazier,\"41,6\",1,0\n"
            "  crypt,crypt.stair,\"38,4\",1,0\n";

        assert(toon == expected_s1);

        // Step 5 — anchor geometry sanity: both plugs at y=12.5 (colinear → straight).
        for (const edi::drafting::DraftingPlug &plug : doc.plugs) {
            if (plug.name == "entrance.to_crypt") {
                assert(std::abs(plug.anchor.x - 15.0) < 1e-9);   // East wall at x=15
                assert(std::abs(plug.anchor.y - expectedColinearY(1.0)) < 1e-9);
            } else if (plug.name == "crypt.to_entrance") {
                assert(std::abs(plug.anchor.x - 35.0) < 1e-9);   // West wall at x=35
                assert(std::abs(plug.anchor.y - expectedColinearY(1.0)) < 1e-9);
            }
        }
    }

    // -------------------------------------------------------------------------
    // S = 2  (doubled, `scale: 2` header line)
    // Golden: crypt_doubled.toon  — rooms ×2, blocks ×2
    // -------------------------------------------------------------------------
    {
        // S=2: entrance 30×30@(0,10), crypt 50×50@(70,0); colinear y=25.
        // `at` re-derives from scaled room dims for free (h/2 = 30/2 = 15 → y=10+15=25).
        edi::drafting::MapSpec spec = edi::io::buildCryptMapSpec(2.0);

        assert(spec.rooms.size() == 2);
        assert(spec.connections.size() == 1);
        assert(spec.blocks.size() == 3);

        // Verify one scaled room dim to prove the table is multiplied, not re-typed.
        assert(std::abs(spec.rooms[0].spec.width  - 30.0) < 1e-9); // 15 * 2
        assert(std::abs(spec.rooms[1].spec.origin.x - 70.0) < 1e-9); // 35 * 2

        DrawingDocumentController controller;
        assert(controller.createMapFromSpec(spec, 1.0));
        const edi::drafting::DraftingDocument &doc = controller.draftingDocument();

        // sceneScale=2.0 → emits `scale: 2` (num(2.0) → "%g" → "2").
        const std::string toon = edi::io::exportMapToToon(doc, "crypt", "feet", 2.0);

        const std::string expected_s2 =
            "kind: map\n"
            "title: crypt\n"
            "units: feet\n"
            "scale: 2\n"
            "\n"
            "rooms[2]{name,origin,size,material}:\n"
            "  entrance,\"0,10\",\"30,30\",stone\n"
            "  crypt,\"70,0\",\"50,50\",stone\n"
            "\n"
            "plugs[2]{room,name,edge,type,connected,flags}:\n"
            "  entrance,to_crypt,E,door,true,crypt\n"
            "  crypt,to_entrance,W,door,true,crypt\n"
            "\n"
            "connections[1]{from,to,type}:\n"
            "  entrance.to_crypt,crypt.to_entrance,corridor\n"
            "\n"
            "blocks[3]{room,asset,origin,scale,rotation}:\n"
            "  crypt,crypt.sarcophagus,\"95,25\",1,0\n"
            "  crypt,crypt.brazier,\"82,12\",1,0\n"
            "  crypt,crypt.stair,\"76,8\",1,0\n";

        assert(toon == expected_s2);

        // Colinear anchor check at S=2: both plugs at y=25.
        for (const edi::drafting::DraftingPlug &plug : doc.plugs) {
            if (plug.name == "entrance.to_crypt") {
                assert(std::abs(plug.anchor.x - 30.0) < 1e-9);  // East wall x=0+30
                assert(std::abs(plug.anchor.y - expectedColinearY(2.0)) < 1e-9);
            } else if (plug.name == "crypt.to_entrance") {
                assert(std::abs(plug.anchor.x - 70.0) < 1e-9);  // West wall x=70
                assert(std::abs(plug.anchor.y - expectedColinearY(2.0)) < 1e-9);
            }
        }
    }

    // -------------------------------------------------------------------------
    // S = 4  (quadrupled, `scale: 4` header line)
    // Golden: crypt_doubled2.toon  — rooms ×4, blocks ×4
    // -------------------------------------------------------------------------
    {
        // S=4: entrance 60×60@(0,20), crypt 100×100@(140,0); colinear y=50.
        edi::drafting::MapSpec spec = edi::io::buildCryptMapSpec(4.0);

        assert(spec.rooms.size() == 2);
        assert(spec.connections.size() == 1);
        assert(spec.blocks.size() == 3);

        // Verify one scaled room and one scaled block to prove uniform multiply.
        assert(std::abs(spec.rooms[1].spec.width   - 100.0) < 1e-9); // 25 * 4
        assert(std::abs(spec.blocks[0].position.x  - 190.0) < 1e-9); // 47.5 * 4

        DrawingDocumentController controller;
        assert(controller.createMapFromSpec(spec, 1.0));
        const edi::drafting::DraftingDocument &doc = controller.draftingDocument();

        // sceneScale=4.0 → emits `scale: 4` (num(4.0) → "%g" → "4").
        const std::string toon = edi::io::exportMapToToon(doc, "crypt", "feet", 4.0);

        const std::string expected_s4 =
            "kind: map\n"
            "title: crypt\n"
            "units: feet\n"
            "scale: 4\n"
            "\n"
            "rooms[2]{name,origin,size,material}:\n"
            "  entrance,\"0,20\",\"60,60\",stone\n"
            "  crypt,\"140,0\",\"100,100\",stone\n"
            "\n"
            "plugs[2]{room,name,edge,type,connected,flags}:\n"
            "  entrance,to_crypt,E,door,true,crypt\n"
            "  crypt,to_entrance,W,door,true,crypt\n"
            "\n"
            "connections[1]{from,to,type}:\n"
            "  entrance.to_crypt,crypt.to_entrance,corridor\n"
            "\n"
            "blocks[3]{room,asset,origin,scale,rotation}:\n"
            "  crypt,crypt.sarcophagus,\"190,50\",1,0\n"
            "  crypt,crypt.brazier,\"164,24\",1,0\n"
            "  crypt,crypt.stair,\"152,16\",1,0\n";

        assert(toon == expected_s4);

        // Colinear anchor check at S=4: both plugs at y=50.
        for (const edi::drafting::DraftingPlug &plug : doc.plugs) {
            if (plug.name == "entrance.to_crypt") {
                assert(std::abs(plug.anchor.x - 60.0) < 1e-9);   // East wall x=0+60
                assert(std::abs(plug.anchor.y - expectedColinearY(4.0)) < 1e-9);
            } else if (plug.name == "crypt.to_entrance") {
                assert(std::abs(plug.anchor.x - 140.0) < 1e-9);  // West wall x=140
                assert(std::abs(plug.anchor.y - expectedColinearY(4.0)) < 1e-9);
            }
        }
    }

    return 0;
}
