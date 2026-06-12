// The UTF-8/UTF-16 offset mapping (E3) — the arithmetic that lets Qt's
// caret (UTF-16 code units) and the core's documents (UTF-8 bytes) tell
// each other the truth. The fixture covers every sequence length:
//
//   "a é 気 🜲 b"   (no spaces in the actual string)
//    a   = 1 byte   1 unit    bytes [0,1)    units [0,1)
//    é   = 2 bytes  1 unit    bytes [1,3)    units [1,2)
//    気  = 3 bytes  1 unit    bytes [3,6)    units [2,3)
//    🜲  = 4 bytes  2 units   bytes [6,10)   units [3,5)  <- surrogate PAIR
//    b   = 1 byte   1 unit    bytes [10,11)  units [5,6)
#include "text/TextEncoding.h"

#include <cassert>
#include <string>

using namespace edi::text;

int main()
{
    const std::string text = "\x61\xC3\xA9\xE6\xB0\x97\xF0\x9F\x9C\xB2\x62";
    assert(text.size() == 11);

    // ---- UTF-16 position -> byte offset, every character start. ----
    assert(byteOffsetForUtf16(text, 0) == 0);
    assert(byteOffsetForUtf16(text, 1) == 1);  // after a
    assert(byteOffsetForUtf16(text, 2) == 3);  // after é
    assert(byteOffsetForUtf16(text, 3) == 6);  // after 気
    // Position 4 lands BETWEEN the surrogate pair's two units — there is no
    // byte for "half a character", so the caret snaps past the whole 🜲.
    assert(byteOffsetForUtf16(text, 4) == 10);
    assert(byteOffsetForUtf16(text, 5) == 10); // after 🜲
    assert(byteOffsetForUtf16(text, 6) == 11); // after b == end
    assert(byteOffsetForUtf16(text, 99) == 11); // clamped

    // ---- byte offset -> UTF-16 position, including mid-sequence snaps. ----
    assert(utf16OffsetForByte(text, 0) == 0);
    assert(utf16OffsetForByte(text, 1) == 1);
    assert(utf16OffsetForByte(text, 2) == 1);  // inside é -> é's own position
    assert(utf16OffsetForByte(text, 3) == 2);
    assert(utf16OffsetForByte(text, 6) == 3);
    assert(utf16OffsetForByte(text, 7) == 3);  // inside 🜲
    assert(utf16OffsetForByte(text, 10) == 5);
    assert(utf16OffsetForByte(text, 11) == 6);
    assert(utf16OffsetForByte(text, 99) == 6); // clamped

    // ---- The round trip holds at every character boundary. ----
    for (const std::size_t byte : {std::size_t(0), std::size_t(1), std::size_t(3),
                                   std::size_t(6), std::size_t(10), std::size_t(11)}) {
        assert(byteOffsetForUtf16(text, utf16OffsetForByte(text, byte)) == byte);
    }

    // ---- Backspace's question: where does the previous character start? ----
    assert(previousCharBoundary(text, 11) == 10); // b
    assert(previousCharBoundary(text, 10) == 6);  // 🜲 — all four bytes
    assert(previousCharBoundary(text, 6) == 3);   // 気
    assert(previousCharBoundary(text, 3) == 1);   // é
    assert(previousCharBoundary(text, 1) == 0);   // a
    assert(previousCharBoundary(text, 0) == 0);   // at the start: stay
    assert(previousCharBoundary(text, 8) == 6);   // from INSIDE 🜲: its start

    // ---- Delete's question: where does the next character start? ----
    assert(nextCharBoundary(text, 0) == 1);
    assert(nextCharBoundary(text, 1) == 3);
    assert(nextCharBoundary(text, 3) == 6);
    assert(nextCharBoundary(text, 6) == 10);
    assert(nextCharBoundary(text, 10) == 11);
    assert(nextCharBoundary(text, 11) == 11); // at the end: stay

    // ---- ASCII degenerates to the identity, byte == unit. ----
    const std::string ascii = "hello";
    for (std::size_t i = 0; i <= ascii.size(); ++i) {
        assert(byteOffsetForUtf16(ascii, i) == i);
        assert(utf16OffsetForByte(ascii, i) == i);
    }

    // ---- Malformed input terminates and degrades, never hangs: a lone
    // continuation byte counts as one character. ----
    const std::string garbage = "\x80\x80";
    assert(utf16OffsetForByte(garbage, 2) == 2);
    assert(previousCharBoundary(garbage, 1) == 0);

    // ---- Empty text: every function answers 0. ----
    assert(byteOffsetForUtf16({}, 5) == 0);
    assert(utf16OffsetForByte({}, 5) == 0);
    assert(previousCharBoundary({}, 5) == 0);
    assert(nextCharBoundary({}, 5) == 0);

    return 0;
}
