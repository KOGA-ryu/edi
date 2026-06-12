#include "text/TextSearch.h"

namespace edi::text {

std::optional<TextRange> findNext(const std::string &text,
                                  const std::string &needle,
                                  std::size_t fromByte)
{
    if (needle.empty() || text.empty()) {
        return std::nullopt;
    }
    const std::size_t from = fromByte > text.size() ? text.size() : fromByte;
    std::size_t at = text.find(needle, from);
    if (at == std::string::npos && from > 0) {
        // Wrap once: search the prefix the first pass skipped. Searching
        // [0, from + needle - 1) would double-find a match straddling
        // `from`; plain find from 0 is enough because any match at or
        // after `from` was already found above.
        at = text.find(needle);
        if (at >= from) {
            return std::nullopt;
        }
    }
    if (at == std::string::npos) {
        return std::nullopt;
    }
    return TextRange{at, at + needle.size()};
}

} // namespace edi::text
