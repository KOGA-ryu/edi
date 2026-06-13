# Chapter 2 — the command variant: one way in

> Every change to the document is a value you can name, apply, and undo.

**The idea.** edi never lets a widget reach in and mutate the document. Instead, a change
is a **command** — a plain struct in a `std::variant` called `DraftingCommand` — and there
is exactly one function, `applyDraftingCommand`, that takes the document and a command and
returns the new state. The UI's job ends at *building* a command; applying it is the core's
job. This single choke point is what makes undo, dirty-tracking, and testing almost free.

Undo is the first payoff: because the document is plain copyable data, "undo" is a stack of
snapshots — `beginEdit` copies, `commitEdit` pushes — not a hand-written inverse for every
command (`2c95516`). The choke point is also where the **transaction** boundary goes: a cut
is a copy-then-delete that lands as *one* undo step, and a live drag that fires a command
per mouse-move is bracketed so the whole gesture collapses to one step (`b2f4b2f`,
`e2e7a5c`). The rule "one user action = one undo step" comes from guarding the few choke
points, not from being careful at every call site.

**Commands-as-data has a quiet superpower: later features turn out to be *translations*,
not additions.** When the text editor grew selection-editing, it needed *zero* new
commands — `ReplaceTextRangeCommand` had existed, unused, since the core was first written;
type-over-selection was just the gesture that finally asked for it (`d9df0ab`). When the
script view shipped, even the window — which owns both stores — mutated the editor document
only through that same `ReplaceTextRangeCommand` (`24d050b`). A core designed as
commands-as-data lets you build new features by *composing existing commands*.

| lesson | commit | files |
|---|---|---|
| snapshot undo is free over plain data | `2c95516` | DrawingCore.h (controller) |
| coalesce a drag into one undo step | `b2f4b2f` | DrawingCore.h, DrawingCanvasWidget |
| atomic cut = copy + delete, one step | `e2e7a5c` | DrawingDocumentController |
| validate-then-splice batch command | `8f26e56` | CreateObjectsCommand |
| features become translations of existing commands | `d9df0ab`, `24d050b` | TextEncoding, TextSearch, ReplaceTextRangeCommand |
| classify by the variant, don't re-derive identity | `5e2cebe` | commitEdit (`std::holds_alternative`) |

**The atomic-batch lesson earns its own line.** The old "create N objects" loop could
half-commit — objects 1..k land, k+1 fails — leaving the controller to reconcile the view
with reality. `CreateObjectsCommand` validates *everything* into a staging vector first,
then appends in one splice with one revision bump, so undo always removes exactly one whole
array (`8f26e56`). Validate-before-mutate buys atomicity and undo-correctness for free; the
alternative — a loop plus manual rollback — re-implements transactionality at every call
site.

**The alternative that lost.** A protected `virtual` hook overridden per command kind —
rejected everywhere in edi, because subclassing-for-behavior scatters policy across files
and hides the variation in a vtable. The variant keeps every command kind visible in one
place and dispatch in one function.

**Why it matters past edi.** "One logical edit = one apply = one undo" is the foundation of
any document app, but the deeper habit is *make state changes first-class values*. An
event-sourced ledger, a database write-ahead log, a redux reducer — they are all this idea:
don't mutate in place; name the change and apply it through one door.

**Check yourself.** You're asked to add "rotate selection 90°." Sketch it: what *is* the
command, where is it built, where is it applied, and why does undo work without you writing
a single line of undo code?
