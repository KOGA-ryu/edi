// Find-next with wrap-around (E3) — core arithmetic over the document's
// own bytes; the host converts to the view's UTF-16 selection elsewhere.
#include "text/TextSearch.h"

#include <cassert>
#include <string>

using namespace edi::text;

int main()
{
    const std::string text = "the cat sat on the mat";

    // Forward find from the start, then from past the first match.
    const auto first = findNext(text, "the", 0);
    assert(first.has_value() && first->start == 0 && first->end == 3);
    const auto second = findNext(text, "the", first->end);
    assert(second.has_value() && second->start == 15);

    // Wrap-around: from past the last match, the search comes home.
    const auto wrapped = findNext(text, "cat", 16);
    assert(wrapped.has_value() && wrapped->start == 4);

    // A match AT the from-offset is found (find-next is inclusive of the
    // caret position — the caret sits at a previous match's END, so this
    // never re-finds the same occurrence).
    const auto at = findNext(text, "sat", 8);
    assert(at.has_value() && at->start == 8);

    // Miss: nothing anywhere.
    assert(!findNext(text, "dog", 0).has_value());

    // The empty needle matches NOTHING — an empty needle "matching"
    // everywhere is the classic find-bar infinite loop.
    assert(!findNext(text, "", 0).has_value());
    assert(!findNext({}, "x", 0).has_value());

    // From-offset past the end: clamped, wraps once, still finds.
    const auto clamped = findNext(text, "the", 999);
    assert(clamped.has_value() && clamped->start == 0);

    // Case-sensitive v1, on purpose and documented.
    assert(!findNext(text, "The", 0).has_value());

    return 0;
}
