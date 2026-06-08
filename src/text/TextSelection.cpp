#include "text/TextSelection.h"

#include <algorithm>
#include <utility>

namespace edi::text {

TextRangeValidation TextRangeValidation::accepted()
{
    return {true, TextResultCode::None, {}};
}

TextRangeValidation TextRangeValidation::rejected(std::string message)
{
    return {false, TextResultCode::InvalidRange, std::move(message)};
}

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

TextRangeValidation validateInsertionOffset(std::size_t offset, std::size_t textLength)
{
    if (offset > textLength) {
        return TextRangeValidation::rejected("insert offset is past end of document");
    }
    return TextRangeValidation::accepted();
}

TextRangeValidation validateTextRange(TextRange range, std::size_t textLength)
{
    if (range.start > range.end) {
        return TextRangeValidation::rejected("range start must be less than or equal to range end");
    }
    if (range.end > textLength) {
        return TextRangeValidation::rejected("range end is past end of document");
    }
    return TextRangeValidation::accepted();
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
