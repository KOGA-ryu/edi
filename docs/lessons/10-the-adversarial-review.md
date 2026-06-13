# Chapter 10 — the adversarial review

> A good reviewer finds real defects in good work, every time. Budget for it, and read the
> findings as free education.

**The idea.** Every phase in edi closes with a structured adversarial pass — independent
agents trying to *refute* the work — and it reports **confirmed vs refuted** counts: 27/0,
10-of-17, 56/0, 20, 17. The striking part is that it finds real defects in *good* work
every single time. Even more telling: a *second* pass over the *fixes* routinely finds bugs
in the fixes (17 agents found 12 more, 1 refuted) (`3089ba8`). The pass is not a formality
you pass through; it's a load-bearing part of the process, and you should budget for it.

**It catches defects regardless of who authored them.** The same bar that found 56 issues
in the port also caught planner errors in the build specs and builder gaps in the
implementations — "receipts over trust, both directions" (`4c6f1b2`). Nobody's work is
above the review, including the reviewer's own prior pass.

**A review log is a ledger, not a diff.** Three kinds of entry (`83c6b2d`):
- **Applied** — the real findings, fixed, each pinned by a test that can now fail.
- **Deferred, with a reason** — debts you're consciously not paying yet (e.g. "opacity is
  stored but unread; generalize only when a consumer exists"). A deferral with a stated
  reason is a decision; a silent skip is a leak.
- **Refuted, with counter-evidence** — false finder claims, kept on the record with *why*
  they're wrong, so nobody re-litigates them.

And the reviewer *reproduces the bug before fixing it* — the dead `:disabled` QSS rule was
reproduced pixel-for-pixel before the one-line fix, so the fix is provably aimed at the real
defect (`9c471c2`).

| lesson | commit |
|---|---|
| confirm/refute accounting; a second pass finds bugs in the fixes | `3089ba8` |
| the bar catches defects regardless of author | `4c6f1b2` |
| the log is a ledger: applied / deferred-with-reason / refuted | `83c6b2d` |
| reproduce the defect before fixing it | `9c471c2` |
| recurs every phase | `9201d96`, `cf50804`, `060d6ed`, `200ec94` |

**Why it matters past edi.** The habit underneath the ritual is the one that matters most
for research: *try to refute your own result before you believe it.* A finding you've
attacked from three angles and that survived is worth a hundred that merely looked right.
This is the same instinct as Chapter 6's mutation testing, pointed at conclusions instead of
tests — and it's the whole difference between a discovery and a coincidence.

**Check yourself.** You run a review and it returns zero findings on a 600-line change. What
are the two most likely explanations — and which one should you assume first, given this
chapter's track record of the reviewer finding defects in *every* phase so far?
