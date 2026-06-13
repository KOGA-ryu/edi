# Chapter 6 — a test you haven't mutated is a hope

> A green test proves nothing until you've watched it go red.

**The idea.** This is edi's signature discipline, and the first of the project's field
notes. After you write a test, deliberately break the code it covers — zero the decoded
radius, disable the constraint, turn the travel move into a stroke, flip the no-emit flag —
and confirm the test **aborts** (a failed assertion exits 134). A test that *survives* the
mutation is testing nothing; it is a vacuous pin, green on garbage. The mutation is run
per slice, as a habit, not occasionally (`5e5e536` and a long tail: `dea2781`, `734e7ade`,
`8f26e56`, `0262c85`, all four editor slices…).

What does a vacuous pin look like? Two recurring shapes (`060d6ed`): asserting a value at
its *constructor default* (so the assertion holds whether or not your code ran), and a
*tautological* equality. The sharpest example: a flute-cutter clamp at 24 columns painted
the same 11 columns no matter what, so the obvious test asserted nothing — the fix asserts
at 96 columns, where the clamp actually changes the output (`3089ba8`).

**The deeper move: when a mutation *doesn't* change the obvious signal, your assertion is
on the wrong thing.** Removing the forward-trail truncation didn't change any button's
enablement, because a push always lands at the end — so the test was strengthened to assert
*where Back lands* (the stale trail entry is the real observable difference), not the arrow
states (`7257be1`). A test that can't be made to fail by breaking the behavior isn't
covering the behavior; go find the actual consequence and assert on *that*.

| lesson | commit |
|---|---|
| mutate every slice; the suite must abort | `5e5e536`, `dea2781`, … |
| vacuous pins: constructor-default and tautology | `060d6ed`, `3089ba8` |
| strengthen to the real observable difference | `7257be1` |
| a self-modifying test must exit RED while it rewrites its oracle | `b3e47c7` |
| kill the mutant, don't just describe it (comment-only "proofs") | `060d6ed` |

**A wrinkle worth seeing once:** the golden-render "bless" path is a test that rewrites its
own reference image. It must *fail* while doing so — otherwise a leftover environment
variable could silently re-bless forever, the lock quietly becoming a no-op (`b3e47c7`). A
safety mechanism that can disable itself isn't one.

**Why it matters past edi.** A false-green test is structurally identical to the central
failure of quantitative research: a backtest that looks profitable because of lookahead
bias, survivorship bias, or an overfit parameter. You are training the exact reflex —
*try to make the result fail before you believe it* — that separates a real signal from a
data-mined ghost. The mutation habit here is the same muscle the data work will live on.

**Check yourself.** You write a test that a circle's serialized radius round-trips through
the codec, and it's green. What is the one-line mutation that proves the test isn't
vacuous? And what would a *vacuous* version of this exact test look like — one that stays
green even when serialization is broken?
