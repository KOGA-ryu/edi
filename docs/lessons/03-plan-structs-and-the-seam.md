# Chapter 3 — plan structs, and the resolve→plan→apply→emit seam

> Compute the change as a pure value first; commit it only if it's whole.

**The idea.** Between "the user did something" and "the document changed," edi inserts a
pure step: a **plan**. A planning function takes the current state and the request and
returns a plan struct — typically an `ok` flag plus a payload — *without touching
anything*. If the plan isn't ok, the document is never mutated; the bad input simply
doesn't produce a change. This is why a rejected edit in edi leaves the document
byte-identical instead of half-applied.

The controller's whole job is this pipeline, said the same way every time: **resolve**
inputs → **plan** (delegate to a pure function) → **apply** the command → **emit**
`modelChanged`. CLAUDE.md is explicit that you grow this by *extending* the shared
kind-and-callable helpers, not by re-inlining the sequence at each new call site. When
grid, radial, and repeat arrays arrived, they all flowed through one helper,
`createArrayFromActiveObject`, differing only in the planner callable handed to it — three
actions that *cannot* drift apart because they share one orchestration seam (`a849954`,
`2fd318f`).

The plan is also the natural home for **policy as a table**. Which inspector sections show
for a given (tool, selection, shape) is a pure function `planDraftingInspector()` returning
a list of section ids; the widgets build once and toggle visibility from that list
(`61bc877`, `4da318e`). Policy changes faster than mechanism, so encoding it as a
golden-tested data table means retuning is a table edit with zero widget churn — and a
subclass-per-context would scatter one policy across files.

| lesson | commit | files |
|---|---|---|
| reject leaves the document untouched | `10ad828` | RecipeDocument (move validates a copy) |
| one orchestration seam, callable per variant | `a849954`, `2fd318f` | DrawingDocumentController, array planners |
| row / grid / ring differ only by a vector of offsets | `2fd318f` | array planners, buildTranslatedCopies |
| paste planning as a pure value-in / value-out function | `a02c881` | DraftingClipboard |
| inspector context as a pure plan table | `61bc877`, `4da318e` | DraftingInspectorPlan |
| a pure function can still advance a counter | `a02c881` | planDraftingPaste (startSerial → nextSerial) |

**A subtle, powerful detail:** a pure function can still thread mutable bookkeeping if you
pass it *in and out*. `planDraftingPaste` mints fresh, collision-free ids by taking the
live serial as a parameter and returning the next one — deterministic and unit-testable,
without the function owning a counter (`a02c881`). Purity is about no *hidden* state, not no
state.

**The alternative that lost.** Mutating as you go and rolling back on failure. It
re-implements "leave it clean on reject" at every call site, and the first site that forgets
leaves a half-edited document. A plan that's computed whole, then applied whole, makes
partial states impossible by construction.

**Why it matters past edi.** "Compute the result as a value, validate it, then commit
atomically" is the shape of every robust transform — a database transaction, a compiler
pass, an ETL stage. When the data work comes, a scan that builds its result set before
publishing it (rather than streaming half-answers downstream) is this same seam at scale.

**Check yourself.** Why does edi recompute an object's bounding box from the *translated
geometry* during paste, instead of translating the source's stored bounds box? What class
of bug does that choice prevent?
