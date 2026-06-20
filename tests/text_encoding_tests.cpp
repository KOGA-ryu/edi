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

#include "EdiAssert.h"
#include <string>

using namespace edi::text;

int main()
{
    const std::string text = "\x61\xC3\xA9\xE6\xB0\x97\xF0\x9F\x9C\xB2\x62";
    EDI_CHECK(text.size() == 11);

    // ---- UTF-16 position -> byte offset, every character start. ----
    EDI_CHECK(byteOffsetForUtf16(text, 0) == 0);
    EDI_CHECK(byteOffsetForUtf16(text, 1) == 1);  // after a
    EDI_CHECK(byteOffsetForUtf16(text, 2) == 3);  // after é
    EDI_CHECK(byteOffsetForUtf16(text, 3) == 6);  // after 気
    // Position 4 lands BETWEEN the surrogate pair's two units — there is no
    // byte for "half a character", so the caret snaps past the whole 🜲.
    EDI_CHECK(byteOffsetForUtf16(text, 4) == 10);
    EDI_CHECK(byteOffsetForUtf16(text, 5) == 10); // after 🜲
    EDI_CHECK(byteOffsetForUtf16(text, 6) == 11); // after b == end
    EDI_CHECK(byteOffsetForUtf16(text, 99) == 11); // clamped

    // ---- byte offset -> UTF-16 position, including mid-sequence snaps. ----
    EDI_CHECK(utf16OffsetForByte(text, 0) == 0);
    EDI_CHECK(utf16OffsetForByte(text, 1) == 1);
    EDI_CHECK(utf16OffsetForByte(text, 2) == 1);  // inside é -> é's own position
    EDI_CHECK(utf16OffsetForByte(text, 3) == 2);
    EDI_CHECK(utf16OffsetForByte(text, 6) == 3);
    EDI_CHECK(utf16OffsetForByte(text, 7) == 3);  // inside 🜲
    EDI_CHECK(utf16OffsetForByte(text, 10) == 5);
    EDI_CHECK(utf16OffsetForByte(text, 11) == 6);
    EDI_CHECK(utf16OffsetForByte(text, 99) == 6); // clamped

    // ---- The round trip holds at every character boundary. ----
    for (const std::size_t byte : {std::size_t(0), std::size_t(1), std::size_t(3),
                                   std::size_t(6), std::size_t(10), std::size_t(11)}) {
        EDI_CHECK(byteOffsetForUtf16(text, utf16OffsetForByte(text, byte)) == byte);
    }

    // ---- Backspace's question: where does the previous character start? ----
    EDI_CHECK(previousCharBoundary(text, 11) == 10); // b
    EDI_CHECK(previousCharBoundary(text, 10) == 6);  // 🜲 — all four bytes
    EDI_CHECK(previousCharBoundary(text, 6) == 3);   // 気
    EDI_CHECK(previousCharBoundary(text, 3) == 1);   // é
    EDI_CHECK(previousCharBoundary(text, 1) == 0);   // a
    EDI_CHECK(previousCharBoundary(text, 0) == 0);   // at the start: stay
    EDI_CHECK(previousCharBoundary(text, 8) == 6);   // from INSIDE 🜲: its start

    // ---- Delete's question: where does the next character start? ----
    EDI_CHECK(nextCharBoundary(text, 0) == 1);
    EDI_CHECK(nextCharBoundary(text, 1) == 3);
    EDI_CHECK(nextCharBoundary(text, 3) == 6);
    EDI_CHECK(nextCharBoundary(text, 6) == 10);
    EDI_CHECK(nextCharBoundary(text, 10) == 11);
    EDI_CHECK(nextCharBoundary(text, 11) == 11); // at the end: stay

    // ---- ASCII degenerates to the identity, byte == unit. ----
    const std::string ascii = "hello";
    for (std::size_t i = 0; i <= ascii.size(); ++i) {
        EDI_CHECK(byteOffsetForUtf16(ascii, i) == i);
        EDI_CHECK(utf16OffsetForByte(ascii, i) == i);
    }

    // ---- Malformed input terminates and degrades, never hangs: a lone
    // continuation byte counts as one character. ----
    const std::string garbage = "\x80\x80";
    EDI_CHECK(utf16OffsetForByte(garbage, 2) == 2);
    EDI_CHECK(previousCharBoundary(garbage, 1) == 0);

    // ---- Empty text: every function answers 0. ----
    EDI_CHECK(byteOffsetForUtf16({}, 5) == 0);
    EDI_CHECK(utf16OffsetForByte({}, 5) == 0);
    EDI_CHECK(previousCharBoundary({}, 5) == 0);
    EDI_CHECK(nextCharBoundary({}, 5) == 0);

    return 0;
}
