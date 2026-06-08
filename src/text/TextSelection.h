#pragma once

#include <cstddef>
#include <string>

namespace edi::text {

struct TextCursor {
    std::size_t offset = 0;
};

struct TextRange {
    std::size_t start = 0;
    std::size_t end = 0;
};

struct TextSelection {
    TextRange range;
};

TextRange normalizeRange(TextRange range);
TextRange clampRange(TextRange range, std::size_t textLength);
bool isCollapsed(TextRange range);
std::string selectedText(const std::string &text, TextRange range);

} // namespace edi::text
