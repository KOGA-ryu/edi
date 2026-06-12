#include "text/TextEncoding.h"

namespace edi::text {

namespace {

// How many bytes the UTF-8 sequence starting at `lead` occupies. A
// continuation byte (10xxxxxx) or an invalid lead answers 1, so a walk
// over malformed input still terminates and lands on every byte at most
// once — garbage degrades to "each byte is a character", never a hang.
std::size_t sequenceLength(unsigned char lead)
{
    if ((lead & 0x80) == 0x00) {
        return 1; // 0xxxxxxx — ASCII
    }
    if ((lead & 0xE0) == 0xC0) {
        return 2; // 110xxxxx
    }
    if ((lead & 0xF0) == 0xE0) {
        return 3; // 1110xxxx
    }
    if ((lead & 0xF8) == 0xF0) {
        return 4; // 11110xxx — the surrogate-pair case in UTF-16
    }
    return 1;
}

// A 4-byte UTF-8 sequence encodes a code point beyond the Basic
// Multilingual Plane, which UTF-16 can only express as TWO code units
// (a surrogate pair). Everything shorter is one unit.
std::size_t utf16Units(std::size_t sequenceBytes)
{
    return sequenceBytes == 4 ? 2 : 1;
}

bool isContinuation(unsigned char byte)
{
    return (byte & 0xC0) == 0x80;
}

} // namespace

std::size_t byteOffsetForUtf16(const std::string &utf8, std::size_t utf16Offset)
{
    std::size_t byte = 0;
    std::size_t utf16 = 0;
    while (byte < utf8.size() && utf16 < utf16Offset) {
        const std::size_t bytes = sequenceLength(static_cast<unsigned char>(utf8[byte]));
        utf16 += utf16Units(bytes);
        byte += bytes;
    }
    // A utf16Offset landing INSIDE a surrogate pair (between its two units)
    // has already advanced past the whole character above — the caret snaps
    // to the character boundary, the only honest position.
    return byte < utf8.size() ? byte : utf8.size();
}

std::size_t utf16OffsetForByte(const std::string &utf8, std::size_t byteOffset)
{
    std::size_t byte = 0;
    std::size_t utf16 = 0;
    while (byte < utf8.size() && byte < byteOffset) {
        const std::size_t bytes = sequenceLength(static_cast<unsigned char>(utf8[byte]));
        // Stop BEFORE overshooting: a byteOffset inside this sequence maps
        // to the sequence's own UTF-16 position (snap to character start).
        if (byte + bytes > byteOffset) {
            break;
        }
        utf16 += utf16Units(bytes);
        byte += bytes;
    }
    return utf16;
}

std::size_t previousCharBoundary(const std::string &utf8, std::size_t byteOffset)
{
    if (byteOffset == 0 || utf8.empty()) {
        return 0;
    }
    std::size_t byte = byteOffset > utf8.size() ? utf8.size() : byteOffset;
    --byte;
    // Walk back over continuation bytes to the sequence's lead byte. The
    // bound is 3 steps (a UTF-8 sequence is at most 4 bytes), but malformed
    // input is tolerated by the byte > 0 guard.
    while (byte > 0 && isContinuation(static_cast<unsigned char>(utf8[byte]))) {
        --byte;
    }
    return byte;
}

std::size_t nextCharBoundary(const std::string &utf8, std::size_t byteOffset)
{
    if (byteOffset >= utf8.size()) {
        return utf8.size();
    }
    const std::size_t bytes = sequenceLength(static_cast<unsigned char>(utf8[byteOffset]));
    const std::size_t next = byteOffset + bytes;
    return next < utf8.size() ? next : utf8.size();
}

} // namespace edi::text
