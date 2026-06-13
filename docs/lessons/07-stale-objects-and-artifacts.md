# Chapter 7 — the stale object, and reading the source not the artifact

> Verify against the thing that actually runs — not a cached binary, not a generated
> sample, not your beliefs about the pixels.

This chapter is three traps that share a spine: you think you're checking reality, but
you're checking a stand-in.

**Trap 1 — the stale object file.** Mutation testing (Chapter 6) is worthless if the binary
didn't actually rebuild. Incremental build systems key on file modification times with
coarse granularity, and that will happily run a *mutated* `.o` through a green suite, or a
*restored* source through a stale mutated object — a false green either way. This trap has
struck repeatedly enough to earn a field note: in any mutate/restore cycle,
`find build -name '<file>.cpp.o' -delete` before rebuilding. It "claimed its fifth victim"
before the note finally dispatched it (`3089ba8`, `2d87c04`, recurs through `d9df0ab`).

**Trap 2 — the generated artifact instead of the source.** The prototype port was first
written against the *generated* Blender script rather than the *library source*, and it
produced two confident "v0 never did X" claims that were both false — v0's code did lay
cylinders along x/y and did widen rings by overhang; the one generated sample just never
exercised those paths (`3089ba8`). Study the *source* of a system you're reimplementing,
not one sample of its output. (The same commit replaced `strtod`/`strtol` with
`from_chars`, because parsing behavior should be data you control, not ambient process
state — see Chapter 8.)

**Trap 3 — property reads instead of pixels.** A stylesheet rule can be silently ignored —
a plain `QWidget` without `WA_StyledBackground`, a palette fill covering a styled frame —
while every property query you make returns the correct value. Only the rendered image
tells the truth. So edi grew `edi --snapshot out.png --probe x,y`, which settles the window
offscreen and prints the *actual* pixel color plus the owning widget's parent chain
(`9ed35fa`). It caught a QSS specificity bug that made the Close button invisible *and*
clickable, and a `QScrollArea` palette leak — both invisible to code review, both obvious
to a pixel probe (`c90e60b`, `0b991e8`).

| trap | commit | the rule |
|---|---|---|
| stale `.o` masks the mutation | `3089ba8`, `2d87c04` | delete object files in mutate/restore cycles |
| ported from the generated script | `3089ba8` | read the source, not one artifact of it |
| property reads looked correct | `9ed35fa`, `c90e60b` | assert on the shipped pixels |

**Why it matters past edi.** When you ingest a 30TB feed, the schema doc is an artifact and
the vendor's example file is an artifact — the bytes you actually parse are the source of
truth, and a locale-independent, overflow-honest parser (`from_chars`, not `strtod`) is how
you avoid silently trusting the wrong stand-in.

**Check yourself.** You mutate a function, rebuild, run the suite — and it's still green.
Name two *different* reasons that could happen, one from this chapter and one from
Chapter 6, and the check you'd run to tell them apart.
