#pragma once

// No Qt includes — this header is department-owned pure C++ so the generator
// can be used without a QApplication (tests, future CLI integration).
#include "drafting/DraftingRoom.h"

namespace edi::io {

// M0 crypt MapSpec in AUTHORED FEET, on the 5 ft grid (socket contract §1/§3):
// 2 rooms + 1 plug at each facing-edge MIDPOINT + 1 connection joining them.
// Block props (sarcophagus, brazier, stair) are G2.
//
// Layout dimensions come from the file-scope data tables in CryptGenerator.cpp
// (standing rule 2026-06-18: every dimension is DATA, never a magic literal in
// build logic).  Plug offsets (`at`) are DERIVED from the room dimensions.
//
// Contract: the TOON emitted by exportMapToToon(controller.draftingDocument())
// after createMapFromSpec(buildCryptMapSpec(), 1.0) must match the fixture
// ~/dept-bus/dungeon-map/crypt_doubled.toon (rooms/plugs/connections sections).
edi::drafting::MapSpec buildCryptMapSpec();

} // namespace edi::io
