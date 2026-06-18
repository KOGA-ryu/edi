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
// The golden string must match the fixture samples/crypt_m0/crypt.toon
// (rooms + plugs + connections sections); that file was the realizer's proven
// input for the 5090 render that passed the M0 gate.

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
    assert(spec.rooms[1].name  == "crypt");
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

    // Step 4 — Pin the golden TOON contract.
    //
    // Rooms/plugs/connections sections must be BYTE-IDENTICAL to the
    // fixture samples/crypt_m0/crypt.toon (the realizer's proven input).
    // The blocks section is G1-specific (0 items; the fixture has 3).
    //
    // Number formatting: %g — 0→"0", 7.5→"7.5", 15→"15", 20→"20",
    //   25→"25", 35→"35"; coordinate cells quoted because they contain commas.
    // The plug flag "crypt" has no delimiters, so it stays bare (not quoted).
    // connected=true/false is emitted directly (not via cell()).
    const std::string expected =
        "kind: map\n"
        "title: crypt\n"
        "units: feet\n"
        "\n"
        "rooms[2]{name,origin,size,material}:\n"
        "  entrance,\"0,0\",\"15,15\",stone\n"
        "  crypt,\"35,25\",\"20,20\",stone\n"
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
    // entrance East-edge midpoint:  world (15, 7.5)
    //   East edge NE(15,0)→SE(15,15); at=7.5 → point {15+0*7.5, 0+1*7.5}
    // crypt    West-edge midpoint:  world (35, 35)
    //   West edge SW(35,45)→NW(35,25); at=10 → point {35+0*10, 45−1*10}
    //
    // Non-colinear (y=7.5 vs y=35) → realizer routes an L-shaped corridor.
    for (const edi::drafting::DraftingPlug &plug : doc.plugs) {
        if (plug.name == "entrance.to_crypt") {
            assert(std::abs(plug.anchor.x - 15.0) < 1e-9);
            assert(std::abs(plug.anchor.y -  7.5) < 1e-9);
        } else if (plug.name == "crypt.to_entrance") {
            assert(std::abs(plug.anchor.x - 35.0) < 1e-9);
            assert(std::abs(plug.anchor.y - 35.0) < 1e-9);
        }
    }

    // Confirm non-colinear: the two y-values differ by more than a tile.
    double yEntrance = 0.0;
    double yCrypt    = 0.0;
    for (const edi::drafting::DraftingPlug &plug : doc.plugs) {
        if (plug.name == "entrance.to_crypt")  { yEntrance = plug.anchor.y; }
        if (plug.name == "crypt.to_entrance")   { yCrypt    = plug.anchor.y; }
    }
    assert(std::abs(yEntrance - yCrypt) > 1.0); // 7.5 vs 35 → well above threshold

    return 0;
}
