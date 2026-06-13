# Chapter 11 — one format per job, never JSON — and the machine split

> Pick a serialization by its consumer; make readers forgiving and writers strict; let the
> document be the wire between machines.

**The idea.** There is no universal format. edi chose three, each for its consumer
(`62e9552`):
- **TOML** for human-editable, diffable settings — a person reads and hand-edits it.
- **MessagePack** for compact binary document data — a machine reads it fast and small.
- **TOON** for AI handoffs — a model reads it.
- **JSON** is banned, because it's none of those *well*.

**A real codec earns its tests at the edges.** A hand-written MessagePack codec encodes the
smallest representation but pins one thing for stability (doubles always `float64`), and its
decoder is defensive: cursor-based, rejecting truncation *and* trailing garbage, requiring
full-buffer consumption — and tested at the *width-band boundaries* where the encoding
switches size, not just the middle (`77fcce3`). Critically, never `reserve()` a count read
straight from an untrusted length field — a corrupt `array32` header could ask for a
multi-gigabyte allocation, so the reserve is capped by bytes-remaining (`08f2fa7`).

**Decode forgiving, encode strict.** Travel enums by *name* so files survive reordering;
clamp hand-typed values through the same band the runtime uses; drop a malformed row but
keep the rest; carry an `ok` flag so "missing or unusable" means "use the built-in default"
instead of throwing (`5d33f7e`). New fields get a **default-and-sentinel migration plan** so
old files round-trip with their original meaning intact — per-object stroke uses an empty
color as "the layer decides," and the decoder maps the old non-writable defaults onto that
sentinel so a pre-existing drawing keeps its look instead of turning black (`734e7ade`,
`c8244488`).

**And then the payoff: the strict document is the transport between machines.** edi's plan
is two computers — a Mac proving ground that drafts and resolves, and a Linux factory (the
RTX 5090) that renders. The factory needs *only* Python + Blender + the resolved TOML; there
is zero C++ on it. That's possible because the recipe core stores a **reference** and
resolves fresh — a `ShaperStep` holds the profile's object id and pulls its exact points at
resolve time, so editing the drafted profile and re-resolving picks up the new numbers.
Copying the points into the recipe would create a second source of truth — the exact thing
the project exists to forbid (`fc9858d`). Export resolves against the *live* document and
refuses to write *any* file when a binding is stale: "a partial script on disk would be a
guess wearing a file extension" (`a6d7e10`).

| lesson | commit |
|---|---|
| one format per job; never JSON | `62e9552` |
| codec tested at width-band boundaries; defensive decode | `77fcce3` |
| cap reserves from untrusted length fields | `08f2fa7` |
| decode forgiving (enums by name, clamp, drop-row, `ok`) | `5d33f7e` |
| default-and-sentinel migration; old files keep their meaning | `734e7ade`, `c8244488` |
| store a reference, resolve fresh — never copy the numbers | `fc9858d` |
| refuse to write any artifact when inputs are stale | `a6d7e10` |
| the factory runs on python + TOML, zero C++ | `d1aaa74` |

**Why it matters past edi.** mmap-friendly columnar formats, schema evolution, tolerance for
dirty inputs, and "store an id and resolve against the authority on demand" are the daily
bread of large-scale data work. The Mac/Linux split is a preview: a strict, portable
document is what lets computation cross a machine boundary without dragging its toolchain
along.

**Check yourself.** You add a new optional field `bevelDepth` to a serialized shape. What
default value makes every existing `.edidraw` load unchanged, and why is choosing that
default a *format* decision rather than a code detail? When would an empty/sentinel value be
the right default instead of `0.0`?
