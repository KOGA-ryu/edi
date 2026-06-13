# Chapter 12 — Qt craft: QPalette, the cascade, and pixels that tell the truth

> A styling system has limits and footguns. Know where it can't reach, and where it lies.

This chapter is framework-specific — Qt — but the *shape* of each lesson (a declarative
system with cascade rules and a box model you must compute, not guess) recurs in CSS and
every other styling engine.

**QSS can't paint what a custom `paintEvent` draws.** A stylesheet styles standard widget
parts; it cannot reach pixels your own paint code lays down. Two escape hatches: push a
`QPalette` (the channel QSS can't be) so a custom paint reads theme colors from palette
roles (`5fc836f`), or make the look a **pure data-to-paint function** — a two-color icon
that no font glyph and no QSS rule can compose becomes `panelToggleFace(slot, colors, dpr)
→ pixmap`, allocated at device resolution so a 1px frame doesn't blur on a 2× display
(`ee03c00`).

**QSS specificity is real CSS cascade — total ordering you compute, not guess.**
`#titleBar QPushButton` (specificity 0-1-0-1) silently out-ranked the bare `#trafficClose`
(0-1-0-0), so the Close dot painted transparent — *invisible and still clickable*. Only
pixels caught it; the fix is to out-specify, `#titleBar QPushButton#trafficClose`
(`0b991e8`). And when two rules *tie* on specificity, document order wins — so a
`::indicator:disabled` rule that's co-ranked by every `:checked`/`:unchecked` rule is
**dead**, and must be raised to `:checked:disabled` to fire (`9c471c2`).

**The box model bites in two ways.** QSS `width` / `min-height` bound the *content* box, so
a 30px button needs `min-height: 20` (20 content + 8 padding + 2 border) — off-by-border
math that passes property reads and only shows in pixels (`d1d7987`). And stylesheet
geometry *overwrites* `setFixedSize` — `polish` sets the widget's min/max from the rule — so
keep one source of truth: size in the sheet *or* in code, never both (`0b991e8`,
`d1d7987`). A related stability trick: an idle border that appears on hover should be
`transparent`, not `none`, so the 1px box is already reserved and nothing reflows when the
border fades in (`e39cdf1`).

| lesson | commit |
|---|---|
| QPalette is the channel QSS can't reach for custom paint | `5fc836f` |
| when QSS can't express a look, make it a data-to-paint function | `ee03c00` |
| specificity is a computed total order; ties break by source order | `0b991e8`, `9c471c2` |
| QSS sizes the content box; polish overwrites `setFixedSize` | `d1d7987` |
| transparent border reserves space, kills layout-jump | `e39cdf1` |
| Qt `#define`s `slots` — never name a member `slots` | `1be3186` |
| `unique_ptr` to a forward-declared type needs an out-of-line dtor | `8676595` |

**Two concrete landmines, each worth a one-line comment in code.** In any TU that includes
Qt, `slots` is a macro that expands to nothing — a member named `slots` is silently
corrupted, so the field is `supportedSlots` (`1be3186`). And a `unique_ptr<Feature>` member
with only a *forward declaration* compiles, but the destructor must see the complete type,
so `~EdiShellWindow()` is defined out-of-line in a `.cpp` where the full type is visible
(`8676595`).

**Why it matters past edi.** Every declarative system you'll meet — CSS, a templating
engine, a query DSL — has a cascade and a box model with rules you must *compute*. The
meta-lesson is the render-proof one from Chapter 7: when the system can fail silently, assert
on the output it actually produces, not on your beliefs about it.

**Check yourself.** A toggle's `:disabled` style never appears, even though the selector
looks correct and the widget *is* disabled. Before touching code, what's your first
hypothesis from this chapter, and what one-line change would you try?
