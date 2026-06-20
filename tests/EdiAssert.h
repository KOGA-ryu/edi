#pragma once

// EDI_CHECK — the edi test-suite assertion. Unlike <cassert>'s assert(), it
// ALWAYS evaluates its argument and aborts on failure, REGARDLESS of NDEBUG.
//
// Why this exists (D03/D01 — the Release-segfault foundation fix):
// plain assert() expands to nothing under -DNDEBUG (the default for a Release
// build). That has two consequences in a test suite:
//   1. Any SIDE-EFFECTING call wrapped in an assert — e.g. assert(addObject(...).ok)
//      — is ELIDED entirely. The object is never added, the document stays empty,
//      and the next line's objects[N] reads out of bounds → segfault. This is the
//      exact cause of the 7 Release crashes (drafting_hit_test/snap/layer_ops/
//      plot_plan/graph_sync, drawing_document_controller, edi_shell_window).
//   2. Even for pure predicates, the shipped Release test binary checks NOTHING.
//
// EDI_CHECK cures both: the condition is always evaluated (so a wrapped mutator
// always runs), and a failure prints the expression + location and aborts with a
// non-zero exit so ctest registers a failure in EVERY build config.
//
// Implementation note: the do/while(false) wrapper makes EDI_CHECK(x); a single
// statement that behaves correctly as the body of an unbraced if/else. The
// condition is cast to bool via !(...) so it works for any contextually-bool
// expression (pointers, optionals, result structs with operator bool, etc.).

#include <cstdio>
#include <cstdlib>

#define EDI_CHECK(cond)                                                         \
    do {                                                                        \
        if (!(cond)) {                                                          \
            std::fprintf(stderr,                                                \
                         "EDI_CHECK failed: %s\n  at %s:%d (%s)\n",             \
                         #cond, __FILE__, __LINE__, __func__);                  \
            std::abort();                                                       \
        }                                                                       \
    } while (false)
