# edi lessons — a data-oriented C++ course, mined from this repo's own commits

This folder is the teaching that was trapped in commit messages. edi's commit bodies
were written to explain *why* each design was chosen, what the alternative was, and why
it lost — but a body you read once and scroll past teaches nothing. So the lessons were
lifted out of 223 commits (the teaching era, 2026-06-10 → `e7885cc`), de-duplicated, and
re-ordered by **concept** instead of by date. The result is a course.

## How to read it

- **It's a path, not a reference.** Read the chapters in order the first time. Each one
  teaches a single idea from the codebase you already own.
- **Every lesson cites its commit.** The short hash next to a lesson is the primary
  source — run `git show <hash>` and read the full body. This folder is the map; the
  commits are the territory.
- **When a chapter names a file, open it.** Reading the lesson without reading the code is
  recognition, not learning. The whole point is to make you go look.
- **Answer the "check yourself" before moving on.** They ask you to *predict*, not recall.
  If you can't, re-read the chapter and the commit.

## The reading path

Chapters 1–5 are the spine — the same five ideas the architecture map turns on, and
everything else is a consequence of them. Read those in order. Chapters 6–10 are how the
codebase stays *correct*; 11–14 are formats, the framework, performance, and process —
sample those as you need them.

> Start at **[Chapter 1 — variation is data](01-variation-as-data.md)**. It's the rule the
> entire `src/drafting/` core is built on, and the one that transfers most directly to the
> large-scale data work down the road: branch on a tag, no virtual dispatch in the hot loop.

## Chapters

**Part I — data-oriented design (the heart)**
1. [variation is data, not subclasses](01-variation-as-data.md)
2. [the command variant — one way in](02-the-command-variant.md)
3. [plan structs and the resolve→plan→apply→emit seam](03-plan-structs-and-the-seam.md)
4. [registries and callables, not class hierarchies](04-registries-and-callables.md)
5. [the pure core and the Qt boundary](05-the-pure-core-boundary.md)

**Part II — staying correct**
6. [a test you haven't mutated is a hope](06-mutation-testing.md)
7. [the stale object, and reading the source not the artifact](07-stale-objects-and-artifacts.md)
8. [numbers are a contract](08-numbers-are-a-contract.md)
9. [refusals name their offender](09-refusals-and-boundaries.md)
10. [the adversarial review](10-the-adversarial-review.md)

**Part III — formats and boundaries**
11. [one format per job, never JSON — and the machine split](11-formats-and-the-machine-split.md)

**Part IV — the framework**
12. [Qt craft — QPalette, the cascade, and pixels that tell the truth](12-qt-craft.md)
13. [performance — sample before you optimize](13-performance.md)

**Part V — how the work is done**
14. [slices, behavior-preserving refactors, and writing decisions down](14-process-and-slices.md)

## Where this came from

Mined from `git log 2026-06-10..e7885cc` in five passes over the history; 134 lesson-cards
distilled into these chapters. If a chapter and a commit body ever disagree, **the commit
body wins** — fix the chapter. The architecture these lessons build is drawn in the
dependency map of `src/`; chapters 1–5 are that map's load-bearing ideas in prose.
