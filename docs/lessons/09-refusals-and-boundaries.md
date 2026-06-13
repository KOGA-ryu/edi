# Chapter 9 — refusals name their offender

> When input is wrong, fail at the earliest boundary you own, name the exact offender, and
> refuse the whole thing — on both sides of the file.

**The idea.** A refusal is a feature, and a good one points at the problem *by address*:
`op.3.radius: binding names unknown object 'gone'`, or `unknown recipe key:
step.0.param.size_zz.value`. The message is the diagnosis; wrapping it in friendlier prose
only blurs the pointer (`a6d7e10`).

**Refuse at the earliest boundary you own.** `range(2.5)` is a Python `TypeError` raised
hours later inside Blender — the worst possible place to discover a typo. So the emitter
refuses a fractional or non-positive count, and `setParamLiteral` rejects non-finite
numbers at one shared gate used by both the UI and the loader (`200ec94`). An error named at
authoring time is worth a hundred mysterious downstream failures.

**Both sides of the file.** The reader refuses what it can't consume *by name* — a
**consumption-audit loader** marks every key as it's read and rejects any leftover key,
which catches typos, gaps, and plural-mistakes with one mechanism and beats a
hand-maintained legal-key list that drifts (`9d27a66`, `9e3aca5`). And the writer refuses to
serialize what the reader would bounce — never write a file that can't reload (`6e3014d`).

**All-or-nothing, by predicate, reported all at once.** A partially-resolved stream is
*worse* than no stream, because it looks exactly like a healthy one — so resolution returns
a finding per failure and an empty result, never a half-baked payload (`76c0f97`). The "safe
to proceed?" check is a single explicit predicate covering *every* unresolved form
(`recipeOpsResolved()` means no bindings remain *and* no unlowered lathe op survives), and
when Export refuses it lists every stale binding at once so the drafter fixes the lot in one
pass (`4c6f1b2`).

| lesson | commit |
|---|---|
| name the offender by address; pass the diagnosis through verbatim | `a6d7e10`, `4c6f1b2` |
| validate at the earliest boundary you own | `200ec94` |
| consumption-audit loader (reject leftover keys) | `9d27a66`, `9e3aca5` |
| writer refuses what the reader would bounce | `6e3014d` |
| all-or-nothing resolve; gate is a predicate over all bad forms | `76c0f97`, `4c6f1b2` |
| one clamp at the trust boundary, not N downstream copies | `060d6ed`, `08f2fa7` |

**One gate, not N defensive copies.** Opacity was clamped at the setter and at canvas
extraction — but a crafted `.edidraw` is external input too, so the clamp moved to
*deserialization*, the single trust boundary all consumers sit behind (`060d6ed`). The same
posture caps an array length read from an untrusted file, so a corrupt `array32` header
can't trigger a multi-gigabyte allocation (`08f2fa7`). And distinguish *recoverable state*
from *unrecoverable data*: a session manifest naming a missing file skips that document and
says so; a manifest that won't parse refuses outright (`cc87c03`).

**Why it matters past edi.** This is the constitution of a robust ingest pipeline: reject
the corrupt row at the parse boundary, name it, and bound every allocation taken from
attacker-controlled lengths. "Degrade recoverable state; refuse unrecoverable data" is the
line you draw a thousand times when feeding 30TB of someone else's files.

**Check yourself.** A recipe TOML has `step.0.param.radius.vlaue = 6.0` (note the typo).
With a consumption-audit loader, what exactly happens, and what does the error say? Now
contrast: what would happen with a hand-maintained list of legal keys instead — and why is
that worse?
