#include "text/TextSelection.h"

#include <algorithm>

namespace edi::text {

TextRange normalizeRange(TextRange range)
{
    if (range.start > range.end) {
        std::swap(range.start, range.end);
    }
    return range;
}

TextRange clampRange(TextRange range, std::size_t textLength)
{
    range = normalizeRange(range);
    range.start = std::min(range.start, textLength);
    range.end = std::min(range.end, textLength);
    return range;
}

bool isCollapsed(TextRange range)
{
    range = normalizeRange(range);
    return range.start == range.end;
}

std::string selectedText(const std::string &text, TextRange range)
{
    range = clampRange(range, text.size());
    return text.substr(range.start, range.end - range.start);
}

} // namespace edi::text
