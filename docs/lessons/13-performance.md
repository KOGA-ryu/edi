# Chapter 13 — performance: sample before you optimize

> Measure where the time actually goes. The intuitive suspect is usually wrong.

**The idea.** The headline lesson is a humbling one. Stage 1 of the canvas-perf work cached
a "typed scene" — converting the document to structs once per mutation instead of per frame
— the obvious, intuitive fix. It bought *almost nothing*. The next sample showed **97% of
frame time inside antialiased line rasterization** (`QPainter::drawLine →
gray_render_scanline`): the real cost was software-rasterizing hundreds of full-board lines
every frame. Stage 2 rendered the static backdrop + grid + objects into a pixmap once per
(generation, board, device-size) and *blit* it each frame; only live chrome paints. 148.8 →
6.8 ms/frame at 1000 objects. Each stage was chosen by **sampling, not intuition**
(`5e5e536`). And every "faster" came with a benchmark instrument (`--paint-bench`,
`--bench-objects`) so the number was a measurement, not an adjective.

**A cheap-looking accessor that secretly rebuilds shared state turns O(1) into
O(everything).** The canvas's coordinate mappers re-derived `boardRect()` on every call, and
that chain rebuilt the *entire* document projection — twice for the plot plan. The grid
makes two mapper calls per line, so one frame rebuilt the projection ~200 times, scaling
*multiplicatively* with grid-lines × objects. The fix: compute model and board *once* at the
top of `paintEvent` and thread `board` down into pure helpers. 34.9 → 3.1 ms/frame
(`d41f480`).

**Fork an overloaded signal by change-kind.** One mouse-move emitted `modelChanged`, which
fanned out to five listeners including a full inspector rebuild — *per pointer tick*. The
fix expresses the distinction as a *second signal*, `pointerChanged` for transient
pointer/preview state vs `modelChanged` for real mutations, so listeners subscribe to
exactly what they need. A flag parameter on the one signal was rejected — a second signal
*types* the distinction (`5b41d4c`).

| lesson | commit | result |
|---|---|---|
| sample first — the cost was AA rasterization, not data conversion | `5e5e536` | 148.8 → 6.8 ms/frame |
| a cheap accessor that rebuilds shared state is O(everything) | `d41f480` | 34.9 → 3.1 ms/frame |
| fork an overloaded signal by change-kind | `5b41d4c` | full rebuild → 4 `setText` |
| tie a cache's invalidation to the one signal everyone trusts | `5e2cebe` | can't be staler than un-cached |
| build the index once, membership O(1) | `8f26e56` | 2 s → 30 ms, 67× |

**Two more that recur:** tie a cache's invalidation to the *one* signal everyone already
trusts (a generation counter bumped by a self-connection to `modelChanged`), so a path that
forgot to emit was already leaving listeners stale — the cache is no *more* wrong than the
system was (`5e2cebe`). And kill accidental quadratics by building an index once: a 9800-
object batch cost ~2s from three per-id linear scans; an id hash-set built once made every
membership test O(1), 2s → 30ms (`8f26e56`).

**Why it matters past edi.** "Profile before you optimize" and "don't recompute what didn't
change" are the *entire game* at 30TB. The static-layer blit is "cache the rendered surface,
not just the data behind it"; the O(1) index is the difference between a scan that finishes
and one that doesn't. Cache-aware, measure-first thinking starts exactly here, at 6.8 ms a
frame.

**Check yourself.** A list view feels sluggish when you drag the cursor across it. Before
changing anything, what's your *first* action — and name two things you would refuse to
"optimize" until that action tells you to.
