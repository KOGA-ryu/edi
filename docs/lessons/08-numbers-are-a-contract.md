# Chapter 8 — numbers are a contract

> How a number is parsed, formatted, and rounded is part of your data format — never
> incidental.

**The idea.** In a system where files round-trip and two languages must agree, the *text of
a number* is a contract, not cosmetics. edi is strict about it on three fronts.

**Parsing.** Use `from_chars` / `std::to_chars` only. `strtod` is `LC_NUMERIC`-sensitive —
a comma-decimal locale silently failed every load — and `strtol` clamps overflow into
valid-looking garbage. "Parsing behavior is data, not ambient process state" (`3089ba8`).
`std::to_chars` also gives the shortest text that round-trips a double exactly, so the
written number *is* the value (`9d27a66`).

**Formatting.** Generated artifacts must be deterministic so they diff clean in git and can
be golden-tested line by line: whole values bare, fractional at a fixed precision (`%.9g`
for scripts, fixed 3-decimal `snprintf` for G-code), *never* `%f` with platform-dependent
digits (`b656e7b`, `23ae80e`). And `%.9g` absorbs binary noise — `0.105 × 12` prints
`1.26`, not a float-tail (`e091871`).

**Rounding mode is a cross-language trap.** Python's `round()` is banker's rounding
(ties-to-even); C++ `lround` is half-away-from-zero. Map coordinates through
`std::nearbyint` under `FE_TONEAREST`, or every exact-`.5` cell shifts and the ASCII
projections stop being byte-identical to the prototype (`fee80fe`). Porting *means the
numbers transfer* — the moulding compiler asserts all 31 doric points verbatim to 4-decimal
rounding, because "a re-derivation that disagreed by a thousandth would be a quiet fork of
the user's working semantics" (`a877d84`).

**Quantize once, decide once.** A comparison that decides *identity* must use the same
quantization as the output. An SVG emitter compared the raw double for path-grouping while
printing `formatNumber()` text, so `0.9996` split its own path *and* printed
`stroke-opacity="1"` — two answers to one question. Both now read the same formatted text
(`060d6ed`).

| front | commit | rule |
|---|---|---|
| locale-safe, overflow-honest parsing | `3089ba8`, `9d27a66` | `from_chars` / `to_chars`, never `strtod`/`strtol` |
| deterministic output | `b656e7b`, `23ae80e`, `e091871` | fixed precision, never `%f` |
| matching rounding modes across languages | `fee80fe`, `a877d84` | `nearbyint` under `FE_TONEAREST`; pin verbatim |
| one quantization for compare and print | `060d6ed` | decide identity at output resolution |
| byte goldens pin all of the above | `b656e7b`, `06d12a9`, `77fcce3` | whole-output compare; tolerance band tuned to intent |

**Byte goldens are how you enforce it.** A code generator's golden compares the *whole*
output byte-for-byte — "contains" lets formatting drift hide (`b656e7b`). A real binary
codec is tested at the *width-band boundaries* where the encoding changes size
(int8…int64), not just the middle (`77fcce3`). And an image golden uses a tolerance *band*
(channel ±8, 0.5% pixel budget) tuned to lock the look without flaking on a rasterizer
patch — though a coarse whole-image budget can hide a small swapped icon, so localized
elements still need their own pixel probe (`06d12a9`).

**Why it matters past edi.** Market data is numbers crossing locales, precisions, and
languages. The bugs that quietly poison a dataset — a comma-decimal feed, a half-up vs
half-even tick, a float printed at platform precision — are *exactly* this chapter, and
determinism is what makes a result reproducible enough to trust.

**Check yourself.** Two machines parse the same `1,5` from a price feed and one gets `1.5`
while the other gets `1`. What's the bug, which function caused it, and what's the fix? Then:
why would byte-comparing the *whole* generated file have caught it, where a "contains 1.5"
assertion would not?
