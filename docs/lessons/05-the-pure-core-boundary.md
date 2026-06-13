# Chapter 5 — the pure core and the Qt boundary

> The compiler should refuse to let a framework type into your logic.

**The idea.** edi is split hard: `src/drafting`, `src/formats`, `src/recipe`, `src/text`
are pure C++ with no Qt types at all; the Qt widgets, the orchestration, and the IO sit
above them. This isn't a style preference — it's *enforced by the build*. The pure test
targets link only `Qt6::Core` (or nothing at all), so the first time someone put a
`QPalette` adapter into the pure `ShellTheme` module, the pure test refused to compile. The
layering rule enforced itself mechanically; the adapter moved to the widget-helper layer
where QtGui belongs (`5fc836f`). Make your logic's test target refuse to link the
framework, and the compiler becomes the guard.

Above that line, the discipline is: **a widget is a projection of state, and data flows one
way.** Verbs rewrite the state and re-project; the only feedback path (a splitter drag)
flows drag → clamp → model, never the reverse (`2b1c24a`). Panels are recomputed *whole*
from the document on each change, not incrementally patched, so they can't drift out of
sync — with a `QSignalBlocker` during the rebuild so a programmatic refresh can't re-enter
as a user event (`929912b`). And features never hold pointers to each other: they couple
through *shared documents on a bus* and talk to the shell through a struct of
`std::function` verbs, so the recipe lab will never hold a `DraftingFeature*` (`8676595`,
`4592c33`).

| lesson | commit | files |
|---|---|---|
| the Core-only test target is the layering guard | `5fc836f` | ShellTheme vs ShellWidgetHelpers |
| widget = projection, one-way data flow | `2b1c24a` | EdiShellWindow, ShellPanels |
| recompute-whole + signal-blocker kills re-entrancy | `929912b` | DraftingFeatureInspector |
| feature talks to shell via callable struct, not back-pointer | `8676595` | DraftingFeature (ShellActions) |
| features couple through shared documents, never references | `4592c33` | FeatureContext bus, RecipeController |
| host the user's core untouched; mutate only via commands | `d132292` | text editor host |

**The text editor is the boundary's purest demonstration.** Its view is a *read-only*
`QPlainTextEdit` — read-only kills every built-in Qt mutation path (typing, paste,
drag-drop) in one switch — and `keyPressEvent` translates editing keys into the user's own
`TextEditorCommands`. "The editor of record is your code or nothing" (`d132292`). The widget
shows the document; it is never allowed to *be* the document.

**The alternative that lost.** An editable widget mirrored back into the model — two
sources of truth and a set of unaudited mutation paths. Every "the UI and the data
disagree" bug lives in that gap. One source of truth, with the view derived from it, closes
the gap by construction.

**Why it matters past edi.** Separating pure logic from the framework is what makes logic
testable, portable, and *movable to another machine* — which is literally edi's plan: the
pure recipe core emits TOML that runs on a Linux box with no C++ at all (Chapter 11). Keep
your computation framework-free and it survives every UI you'll ever wrap around it.

**Check yourself.** The text view is read-only, yet find-in-document highlights a match. How
does a selection appear on screen without violating "the widget never mutates the
document"? (Hint: what *kind* of state is a selection?)
