#pragma once

// No Qt includes — this header is department-owned pure C++ so the generator
// can be used without a QApplication (tests, future CLI integration).
#include "drafting/DraftingRoom.h"

namespace edi::io {

// M0 crypt MapSpec in AUTHORED FEET, on the 5 ft grid (socket contract §1/§3):
// 2 rooms + 1 plug at each facing-edge MIDPOINT + 1 connection + 3 block props.
// All LAYOUT DIMENSIONS come from the file-scope constexpr tables in
// CryptGenerator.cpp (standing rule 2026-06-18: every dimension is DATA,
// never a magic literal in build logic).  Plug offsets (`at`) are DERIVED
// from the room dimensions; block positions are DATA in kCryptBlocks.
//
// `scale` multiplies EVERY base dimension uniformly (room origins + sizes +
// block positions).  `at` re-derives from the scaled room dims for free.
// Base S=1: entrance 15×15@(0,5), crypt 25×25@(35,0), straight corridor.
// S is a parameter (not a literal) — standing-rule compliant.
// Integer S preserves the 5 ft grid; non-integer S is permitted.
//
// TOON contract: exportMapToToon(controller.draftingDocument(), "crypt",
// "feet", scale) must byte-equal the matching golden fixture:
//   S=1 → ~/dept-bus/dungeon-map/crypt_base.toon     (no scale: line)
//   S=2 → ~/dept-bus/dungeon-map/crypt_doubled.toon  (scale: 2)
//   S=4 → ~/dept-bus/dungeon-map/crypt_doubled2.toon (scale: 4)
edi::drafting::MapSpec buildCryptMapSpec(double scale = 1.0);

} // namespace edi::io
