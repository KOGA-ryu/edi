#!/usr/bin/env bash
# Lint: forbid bare assert() anywhere in the C++ test suite.
#
# Why (D03/D01): plain assert() from <cassert> expands to NOTHING under -DNDEBUG
# (the default in a Release build). In a test that means:
#   • a side-effecting setup call wrapped in assert (e.g. assert(addObject(...).ok))
#     is ELIDED — the setup never runs, later objects[N] reads out of bounds, and
#     the Release binary segfaults; and
#   • even a pure predicate checks nothing, so the shipped test binary is vacuous.
# The suite uses EDI_CHECK (tests/EdiAssert.h) instead, which always evaluates its
# argument and aborts regardless of NDEBUG. This lint keeps it that way.
#
# static_assert() is fine (compile-time, never elided) and is excluded. The
# EdiAssert.h header itself mentions assert() in its docs and is excluded.
set -u

root="${1:-.}"
testdir="${root}/tests"

# A bare assert( is "assert(" NOT preceded by a word char (so static_assert is
# excluded). Skip the EDI_CHECK header's own explanatory comments.
hits="$(grep -rnE '(^|[^[:alnum:]_])assert\(' "${testdir}" \
          --include='*.cpp' --include='*.h' \
          | grep -v 'EdiAssert\.h' || true)"

if [ -n "${hits}" ]; then
    echo "lint FAILED: bare assert() found in the test suite."
    echo "Use EDI_CHECK (tests/EdiAssert.h) — bare assert() is elided under -DNDEBUG."
    echo "${hits}"
    exit 1
fi

echo "lint OK: no bare assert() in tests/ (EDI_CHECK only)."
exit 0
