// G1 crypt generator end-to-end test — pins the whole chain:
//   buildCryptMapSpec() -> createMapFromSpec -> exportMapToToon (Seam C)
//
// WHY a pure MapSpec builder + a controller-driven test (not a CLI):
//   The generator (CryptGenerator) is a plain C++ module owned by edi-dungeon-map.
//   It returns a MapSpec value; it has no Qt dependency, no argc/argv, no side
//   effects.  The controller-driven test validates the FULL chain — from authored
//   geometry to TOON wire — without requiring app/main.cpp or a CLI flag.  CLI
//   integration (wiring --generate-crypt into main.cpp) is an edi-ui step that
//   follows once the seam is proven here.
//
// Golden contract: the rooms/plugs/connections sections must match the fixture
// ~/dept-bus/dungeon-map/crypt_doubled.toon (the realizer's target input).
// The blocks section has 0 items in G1 (props are G2).

#include "core/DrawingCore.h"
#include "drafting/DraftingDocument.h"
#include "drafting/DraftingRoom.h"
#include "io/CryptGenerator.h"
#include "io/MapToonExport.h"

#include <QCoreApplication>

#include <cassert>
#include <cmath>
#include <string>

int main(int argc, char **argv)
{
    // QCoreApplication is required by DrawingDocumentController: it uses Qt
    // internals for id generation and signals even in a headless context.
    QCoreApplication app(argc, argv);

    // Step 1 — build the hardcoded M0 crypt MapSpec (pure C++, no Qt).
    edi::drafting::MapSpec spec = edi::io::buildCryptMapSpec();

    assert(spec.rooms.size() == 2);
    assert(spec.rooms[0].name == "entrance");
    assert(spec.rooms[1].name == "crypt");
    assert(spec.connections.size() == 1);
    assert(spec.blocks.empty()); // G1 — props are G2

    // Step 2 — drive through the authoring controller at 1:1 scale.
    // canvasPerAuthoredUnit = 1.0 → canvas units == authored feet, so the
    // document stores the coordinates verbatim and the TOON export recovers
    // them by dividing by 1.0.
    DrawingDocumentController controller;
    assert(controller.createMapFromSpec(spec, 1.0));

    const edi::drafting::DraftingDocument &doc = controller.draftingDocument();
    // Two rooms, two plugs, one connection — the full graph was minted.
    assert(doc.rooms.size() == 2);
    assert(doc.plugs.size() == 2);
    assert(doc.connections.size() == 1);

    // Step 3 — Seam C export: project the live document to the TOON wire.
    // The document overload derives plug edges from anchor positions (not
    // the spec), so this also validates that createMapFromSpec placed the
    // anchors correctly against the room footprints.
    const std::string toon = edi::io::exportMapToToon(doc, "crypt");

    // Step 4 — Pin the golden TOON contract (the "doubled crypt" geometry).
    //
    // Rooms/plugs/connections sections must be BYTE-IDENTICAL to the
    // fixture crypt_doubled.toon (the realizer's target input).
    // The blocks section is G1-specific (0 items; the fixture has 3).
    //
    // Number formatting via %g: 0→"0", 10→"10", 25→"25", 30→"30",
    //   50→"50", 70→"70"; coordinate cells are quoted (they contain commas).
    // The plug flag "crypt" has no delimiters → stays bare (not quoted).
    // connected=true/false is emitted directly (not via cell()).
    const std::string expected =
        "kind: map\n"
        "title: crypt\n"
        "units: feet\n"
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
        "blocks[0]{room,asset,origin,scale,rotation}:\n";

    assert(toon == expected);

    // Step 5 — Geometry sanity: verify the two plug anchors.
    //
    // entrance East-edge midpoint: world (30, 25)
    //   East edge NE(30,10)→SE(30,40); at=15 → {30+0*15, 10+1*15} = (30,25)
    // crypt    West-edge midpoint: world (70, 25)
    //   West edge SW(70,50)→NW(70,0);  at=25 → {70+0*25, 50−1*25} = (70,25)
    //
    // COLINEAR at y=25 → realizer routes a STRAIGHT corridor (no bend).
    for (const edi::drafting::DraftingPlug &plug : doc.plugs) {
        if (plug.name == "entrance.to_crypt") {
            assert(std::abs(plug.anchor.x - 30.0) < 1e-9);
            assert(std::abs(plug.anchor.y - 25.0) < 1e-9);
        } else if (plug.name == "crypt.to_entrance") {
            assert(std::abs(plug.anchor.x - 70.0) < 1e-9);
            assert(std::abs(plug.anchor.y - 25.0) < 1e-9);
        }
    }

    // Confirm colinear: both y-values equal 25 (straight corridor, no bend).
    double yEntrance = -1.0;
    double yCrypt    = -1.0;
    for (const edi::drafting::DraftingPlug &plug : doc.plugs) {
        if (plug.name == "entrance.to_crypt") { yEntrance = plug.anchor.y; }
        if (plug.name == "crypt.to_entrance") { yCrypt    = plug.anchor.y; }
    }
    assert(std::abs(yEntrance - 25.0) < 1e-9); // entrance midpoint at y=25
    assert(std::abs(yCrypt    - 25.0) < 1e-9); // crypt    midpoint at y=25
    assert(std::abs(yEntrance - yCrypt) < 1e-9); // colinear → straight corridor

    return 0;
}
