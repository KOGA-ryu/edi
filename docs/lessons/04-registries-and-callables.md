# Chapter 4 — registries and callables, not class hierarchies

> When you'd reach for a base class with virtual methods, reach for a table of structs with
> callable fields instead.

**The idea.** edi's feature system, its shaper vocabulary, its emitters, its settings
pages, its tool belt — all the places a "plugin would go" — are plain **tables**. A feature
is a `FeatureDescriptor`: an id, a label, the slots it can fill, and a `buildPanel`
**callable**. The registry is just a `std::vector` of these — adding a feature is appending
a row (`1be3186`). The alternative, a `Feature` base class with a virtual `buildPanel()`,
was rejected on purpose: inheritance hides the variation point in a vtable, while a value
table you can copy, inspect, and build in a test keeps it visible — a null callable is a
row you can *see* and read as "no behavior here" (`7361e48`).

Two refinements make this pattern sing. First, **a row's variation point can be a member
pointer.** Each color row in the settings editor stores a pointer-to-the-field it edits, so
four near-identical row builders collapse to one mechanism parameterized by data
(`536a41c`). That `fieldKey → member pointer` idea scales up: which op fields may carry a
measurement binding is a per-kind table of member pointers, and *one* registry serves both
the writer and the resolver — they cannot disagree about what's bindable (`6665ed1`).
Second, **lifecycle is just more optional callables.** Rather than virtual `create()` /
`destroy()`, a descriptor carries optional `recreateInstance` / `buildPalettes` /
`buildChromePanels` callables; an empty `std::function` reads as "no lifecycle" right at the
registration site (`7361e48`, `e4b6394`, `2f3112e`).

| lesson | commit | files |
|---|---|---|
| feature host as a value table + callable factory | `1be3186` | ShellHost (FeatureDescriptor, FeatureRegistry) |
| lifecycle as optional callables, not virtuals | `7361e48` | ShellHost, EdiShellWindow |
| a row's edit target is a member pointer | `536a41c` | SettingsFeature |
| one bind registry serves writer and resolver | `6665ed1` | RecipeOpsBind |
| ownership decided by who owns the vocabulary | `2f3112e` | DraftingFeature, ShellHost |
| emitters as a callable table keyed by id | `b656e7b` | RecipeEmit |

**A governance rule falls out of this:** decide *who owns a thing* by who owns its
vocabulary, not by what's convenient to wire. Snap modes are drafting vocabulary, so the
drafting feature builds the snap popup; undo/redo are document-lifecycle verbs the window
already owns, so the window builds those (`2f3112e`). Keep that line and the shell never has
to learn what any feature *is* — it just frames whatever any bound feature offers, and the
registry stays a table.

**Why it matters past edi.** Registry-as-data with callable factories is the data-oriented
answer to class-hierarchy plugins: extensible by appending rows, testable without
inheritance, and tolerant of a malformed config row (skip it, don't crash). The same shape
— a table of descriptors with function-valued fields — is how you'd register column types,
parsers, or aggregations in a query engine.

**Check yourself.** A new feature needs a panel, a floating palette, *and* a title-bar
popup, but no special teardown. Exactly which fields of its `FeatureDescriptor` do you set,
which do you leave empty, and how many lines of shell code change? Why is "zero" the right
answer to the last part?
