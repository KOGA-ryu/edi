#pragma once

#include "text/TextDocument.h"

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

struct TextRangeValidation {
    bool ok = false;
    TextResultCode code = TextResultCode::InvalidRange;
    std::string message;

    static TextRangeValidation accepted();
    static TextRangeValidation rejected(std::string message);
};

TextRange normalizeRange(TextRange range);
TextRange clampRange(TextRange range, std::size_t textLength);
TextRangeValidation validateInsertionOffset(std::size_t offset, std::size_t textLength);
TextRangeValidation validateTextRange(TextRange range, std::size_t textLength);
bool isCollapsed(TextRange range);
std::string selectedText(const std::string &text, TextRange range);

} // namespace edi::text
