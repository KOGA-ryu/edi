#pragma once

#include <cstddef>
#include <string>

namespace edi::text {

// UTF-8 <-> UTF-16 offset arithmetic (E3) — the seam where Qt's text world
// meets the core's.
//
// THE PROBLEM, concretely: the document model stores UTF-8 bytes
// (std::string); Qt widgets count caret positions in UTF-16 code units
// (QString). For plain ASCII the two scales agree — one character, one
// byte, one unit — which is why E1 could ship with a gate instead of a
// mapping. They disagree the moment text gets interesting:
//
//   character   UTF-8 bytes   UTF-16 units
//   "a"         1             1
//   "é"         2             1
//   "気"        3             1
//   "🜲"        4             2   <- a SURROGATE PAIR: one character,
//                                    TWO Qt caret positions
//
// These functions convert between the scales by walking the UTF-8 lead
// bytes (0xxxxxxx = 1 byte, 110xxxxx = 2, 1110xxxx = 3, 11110xxx = 4;
// 10xxxxxx marks a continuation byte, never a character start). A 4-byte
// sequence is the only one that costs two UTF-16 units.
//
// All functions CLAMP rather than refuse: an offset past the end maps to
// the end, and an offset landing inside a multi-byte sequence snaps to
// that sequence's start. The callers are caret arithmetic — for a caret,
// the nearest legal position is the honest answer; refusal is for
// DOCUMENT mutations, and those happen downstream through the command
// layer's own range validation.

// The byte offset corresponding to a UTF-16 position (clamped to [0, size]).
std::size_t byteOffsetForUtf16(const std::string &utf8, std::size_t utf16Offset);

// The UTF-16 position corresponding to a byte offset (snapped to the
// containing character's start, clamped to the end).
std::size_t utf16OffsetForByte(const std::string &utf8, std::size_t byteOffset);

// The byte offset of the character BEFORE byteOffset (0 if at the start).
// This is what Backspace means: delete one CHARACTER, never one byte —
// half-deleting a multi-byte sequence is exactly the corruption E1's
// ASCII gate existed to prevent, now impossible by construction.
std::size_t previousCharBoundary(const std::string &utf8, std::size_t byteOffset);

// The byte offset of the character AFTER the one at byteOffset (size if at
// or past the last character). What Delete means.
std::size_t nextCharBoundary(const std::string &utf8, std::size_t byteOffset);

} // namespace edi::text
