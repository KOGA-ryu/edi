#pragma once

// No Qt includes — this header is department-owned pure C++ so the generator
// can be used without a QApplication (tests, future CLI integration).
#include "drafting/DraftingRoom.h"

namespace edi::io {

// Hardcoded M0 crypt MapSpec in AUTHORED FEET, on the 5 ft grid (socket
// contract §1/§3): 2 rooms + 1 plug on each facing edge + 1 connection
// joining them.  Block props (sarcophagus, brazier, stair) are G2, added
// once MapSpec::blocks field carries them.
//
// Contract: the TOON emitted by exportMapToToon(controller.draftingDocument())
// after createMapFromSpec(buildCryptMapSpec(), 1.0) must equal the fixture at
// samples/crypt_m0/crypt.toon (rooms/plugs/connections sections) — that file
// is the realizer's proven input (the 5090 render that passed M0 gate).
edi::drafting::MapSpec buildCryptMapSpec();

} // namespace edi::io
