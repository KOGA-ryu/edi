#pragma once

#include "text/TextSelection.h"

#include <optional>
#include <string>

namespace edi::text {

// Find-next with wrap-around (E3, v1: case-sensitive, literal). Starts at
// fromByte; if nothing matches from there to the end, wraps once to the
// top. Returns the matched BYTE range, or nullopt for a miss or an empty
// needle (searching for nothing matches nothing — an empty needle
// "matching" everywhere is the classic find-bar infinite loop).
//
// Byte offsets on purpose: this is core arithmetic over the document's
// own representation; the HOST converts to UTF-16 for the view's
// selection, through TextEncoding, like every other offset crossing.
std::optional<TextRange> findNext(const std::string &text,
                                  const std::string &needle,
                                  std::size_t fromByte);

} // namespace edi::text
