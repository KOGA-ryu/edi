# Chapter 14 — slices, behavior-preserving refactors, and writing decisions down

> Ship one verified slice at a time, make refactors provable, and record every decision and
> every loss.

**The idea.** edi grows in **slices**, and the first slice of a feature is always *pure
data*: the struct, its serialization round-trip, and a "vandalism" test that proves a
malformed/duplicate/unknown row is dropped on decode — *before* any wiring exists
(`abff897`). Crucially, the defaults reproduce today's behavior bit-for-bit, so the golden
test passing *is* the proof of behavior preservation. The downstream wiring then consumes a
format that's already tested and forgiving.

**A behavior-preserving refactor must be provable by inspection.** Break a 2351-line god
file by moving code *verbatim*, so the source's diff is pure deletion plus a couple of
wiring lines (an include and a `using`) — a reviewer confirms nothing changed without
re-running anything. (Bonus C++ fact: multiple `.cpp` files can define methods of the *same*
class and link fine.) Cut the boundaries where the next phase's seams will fall, so the
split is a stepping stone, not throwaway (`3d3054e`).

**Mark deliberate changes as decisions, not refactors.** When paste became atomic
("whole clipboard or nothing"), the commit *flags* that it's a behavior change, not a
refactor — the distinction is part of the discipline (`c3beaa9`). The same honesty applies
to removals: **retire a subsystem against its own benchmark.** Before deleting pipeline A,
a probe ran its resolver over the real drafted profiles and captured every number at full
precision; the replacement reproduces those numbers byte-for-byte (XOR one byte of the
drafted doc and the test dies), *then* A was deleted — and what's lost (the `bevel`/`array`
shapers) is written down, "because a capability deleted in silence is amnesia and one
deleted in writing is a decision" (`0262c85`). Deletion's classic failure mode is prose that
outlives the code — orphaned comments, an empty menu fence, a stale roadmap — so those get
swept too.

| lesson | commit |
|---|---|
| pure-data-first slice: struct + round-trip + vandalism test | `abff897` |
| verbatim move; diff is pure deletion (god-file split) | `3d3054e` |
| mark deliberate semantics changes explicitly | `c3beaa9` |
| retire against the old benchmark; document the losses | `0262c85` |
| extract a contract before the code embodying it retires | `c442f61` |
| don't abstract before the second client | `75ad340`, `83c6b2d` |
| repo truth beats the plan, whoever wrote it | `4c6f1b2` |

**Two governing rules close the loop.** *Don't abstract before the second client:* the
`FeatureContext` bus stayed a one-pointer struct until feature #2 actually needed more —
building generality before it has a shape is just speculation (`75ad340`). And *repo truth
beats the plan, whoever wrote it:* a build order once specified a test "via grid settings"
that was impossible on this tree (the sanitizer replaces non-finite dimensions with 1.0);
the builder verified the claim against the code, found it false, **stopped, routed around
it, and flagged it loudly** (`4c6f1b2`). A plan is a hypothesis; verify it before building,
and stop-and-flag when reality contradicts it — regardless of who authored it.

**Why it matters past edi.** This is the operation's spine — *receipts over trust, every
claim checkable* — and it's why the work can be trusted at all. Small verified slices, a
golden that doubles as a regression oracle, and decisions/losses written where the next
person will read them: that's how a codebase stays legible to a learner and honest to its
future self.

**Check yourself.** You're about to delete a function you're sure is dead. From this
chapter, what do you do *before* deleting it to prove the deletion is safe, and what do you
do *after* to keep the codebase honest about what's gone?
