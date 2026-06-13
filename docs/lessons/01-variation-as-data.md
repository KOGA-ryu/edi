# Chapter 1 — variation is data, not subclasses

> Before you add a class, ask whether the difference is really just a value.

**The idea.** The instinct from most tutorials is: a new *kind* of thing is a new
*subclass*. edi almost never does that. When something varies — a rectangle that can be
rounded, a line that can wear an arrowhead, an array that can be a grid or a ring — the
variation is stored as **data** (a flag, a couple of fields, an enum, a row in a table) on
top of machinery that already exists. New behavior arrives as data over old code, not as a
new type with its own methods.

Why so insistent? Because edi's geometry is a `std::variant` (a closed sum type), and
every place that handles geometry — bounds, hit-test, snapping, serialize, paint, plot —
must handle *every* arm. Add a new arm and you pay a tax at every one of those sites. So
you add an arm only when the thing is genuinely a new **shape**; when it's a **decoration
on** an existing shape, you keep the sum type small and let a side field carry the
difference. An arrow is the cleanest case: it's a `Line` with an `endArrow` flag in its
visual metadata, so every algorithm that already understood a line understands an arrow for
free (`2f7f287`). A rounded or framed rectangle is two `double`s on the one
`RectangleGeometry`, not three new types — only the paint code learns the new axis
(`734e7ade`).

**The companion rule: closed sets get enums, open sets get strings.** An object's `role`
(Wall / Floor / Cutout / Collider) is an enum because the set is fixed and code switches on
it — an invalid role should be *unrepresentable*, not a mis-typed string. But `material`
and `tags` are free text, because those vocabularies are open and user-defined; forcing
them into an enum would mean editing the core every time someone names a new material
(`89e1a55`). And when one rule is shared by several paths — circle, arc, and polygon all
size their radius the same way — it's *one* field (`fixedRadius`), not three, because three
values that mean the same thing drift apart the first time someone updates only one
(`c6d45e3`).

**Where it shows up:**

| lesson | commit | files |
|---|---|---|
| arrow = line + metadata flag | `2f7f287` | DraftingTypes.h, DraftingToolCreation, DraftingSerialize |
| rounded / frame rect = two doubles | `734e7ade` | DraftingTypes.h, DraftingGeometry, DraftingSerialize |
| new tool, no new variant (polygon) | `6099210` | DraftingToolCreation |
| paying the tax honestly (arc primitive) | `36fdcc7` | DraftingGeometry + a `static_assert` on the variant count |
| closed → enum, open → string | `89e1a55` | DraftingTypes.h, DraftingSerialize |
| one shared rule = one field | `c6d45e3` | DraftingToolCreationRequest |
| even icons are data (`BeltFace` geometry) | `ebaa024` | belt widgets, draftingToolFace |
| even the theme is f(inputs) | `0cdc364` | ShellTheme |

**When you *do* add a variant** (the arc, `36fdcc7`), do it with eyes open: update every
exhaustive site, and add a `static_assert` on the variant's case count so the *next* person
who adds an arm gets a compile error if they miss a site. That single line turns "I forgot
a case" from a runtime bug into a build failure — the core safety property of a sum type.

**The alternative that lost.** A `RectVariant` enum spawning `RoundedRectGeometry` /
`FrameGeometry` types, or an `ArrowGeometry`. Each new geometry kind forces every
`std::visit` across the codebase to grow an arm — broad disturbance for a difference that
lives in one place. Adding a column to an existing record beats adding a new variant when
the variation is local.

**Why it matters past edi.** This is the bridge to the data work. A large columnar scan is
fast for exactly this reason: a record is a row with a tag, you branch on the tag, and
there's no vtable to chase in the inner loop. "Model the operation's actual inputs, not the
UI that presents them" (`c6d45e3`) is the same instinct as "store the columns the query
needs, not the shape of the form the user sees."

**Check yourself.** A teammate wants to add a "dashed leader line" tool. Under what
circumstance is that a new geometry arm, and under what circumstance is it a flag on
`Line`? What single question decides it — and what's the cost if you guess wrong?
