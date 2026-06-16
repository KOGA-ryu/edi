# edi agent workflow — departments of planner + specialists

edi is split into **departments**, each a self-contained workflow of four agents
("claudes") — a planner and its builder, reviewer, and researcher. You enter a
department by clicking its **planner**; the planner orchestrates the other three
within that department's scope. Ported from the user's Codex setup, adapted to
how Claude Code actually handles context (read "the one hard truth").

## The shape

```
.claude/agents/
  edi-drafting/      planner.md(name: edi-drafting)  builder.md  reviewer.md  researcher.md
  edi-blender-lab/   planner.md(name: edi-blender-lab)  builder.md  reviewer.md  researcher.md
  edi-ui/            planner.md(name: edi-ui)  builder.md  reviewer.md  researcher.md
  edi-dungeon-map/   planner.md(name: edi-dungeon-map)  builder.md  reviewer.md  researcher.md
docs/departments/    edi-drafting.md  edi-blender-lab.md  edi-ui.md  edi-dungeon-map.md   ← charters
```

- A **department** = a folder of four agents + a **charter** (`docs/departments/edi-<dept>.md`).
  The charter is the single source of the department's DOMAIN — scope, the docs to
  read, file ownership, backlog, how to verify. All four agents read it.
- The **planner** is named exactly `edi-<dept>` — it is the entry point and the
  department's persistent brain. Its specialists are `edi-<dept>-builder` /
  `-reviewer` / `-researcher`.

## The departments

| Department (planner) | Scope | Charter |
| --- | --- | --- |
| `edi-drafting` | the pure 2D drafting core — `src/drafting`, `src/core` | `docs/departments/edi-drafting.md` |
| `edi-blender-lab` | the recipe lab / Seam A — `src/recipe`, `tools/blender`, the composer | `docs/departments/edi-blender-lab.md` |
| `edi-ui` | the widget shell + look — `src/widgets`, theming | `docs/departments/edi-ui.md` |
| `edi-dungeon-map` | the map graph, corridors, neutral export | `docs/departments/edi-dungeon-map.md` |

## The four roles (identical across departments)

- **planner** (`edi-<dept>`) — holds the plan, writes self-contained briefs,
  delegates, integrates. The entry point. Does trivial edits inline.
- **builder** (`edi-<dept>-builder`) — coding only. A settled slice → the green
  gate → a change report.
- **reviewer** (`edi-<dept>-reviewer`) — read-only. Ownership map + adversarial
  audit + cost steward.
- **researcher** (`edi-<dept>-researcher`) — looks outward. Cited sources,
  open-source scouting, library evaluation, educational docs.

## The one hard truth (why this differs from Codex)

A subagent starts with a **fresh, isolated context**. It cannot see the planner's
conversation, the files already read, or the prior plan. The only channel
planner → specialist is the **brief** (the Agent-tool prompt); the only channel
back is the specialist's **single final message**.

So the planner must: (1) write **self-contained briefs** — exact paths, the spec,
the constraints, what "done" means; (2) treat FILES as the durable state (the
charter, `docs/`, the memory system) because the planner's own memory resets
between sessions; (3) RELAY between specialists — the reviewer sees the builder's
diff, never its context. The charter is what lets a fresh specialist act with
full domain knowledge from one read.

## The loop (per department)

```
        ┌─────── edi-<dept>-researcher (outward knowledge, before/at planning)
        ▼
  PLAN ──► brief ──► edi-<dept>-builder ──► change + green gate
   ▲                                            │
   │                                            ▼
  integrate ◄──── findings ◄──── edi-<dept>-reviewer (ownership + adversarial audit)
```

1. Plan the next slice from the charter's backlog.
2. Brief the builder — one self-contained slice.
3. Hand the diff + the builder's report to the reviewer.
4. Integrate, then persist the plan/board so a fresh session recovers it.

## Entering a department

- **Click / select the planner** `edi-<dept>` in the agent picker, or `@`-mention
  it (`@edi-drafting do X`), or boot a dedicated session into it from the CLI:
  `claude --agent edi-drafting`. The session then works as that department's
  planner, delegating to its specialists.
- You can also click any specialist directly (e.g. `edi-ui-builder`) for a
  one-off, but the planner is the orchestrator.
- **Agent files load at session START** — after adding or editing one, restart
  Claude Code (or use `/agents`) before it can be invoked.

## Adding a department

Copy a department folder to `.claude/agents/edi-<newdept>/`, rename the four
`name:`s to `edi-<newdept>` / `edi-<newdept>-builder` / `-reviewer` /
`-researcher`, and write its charter at `docs/departments/edi-<newdept>.md`
(scope, docs, ownership, backlog, verify). That's the whole cost of a new
department.

## Cost discipline (and its honest limit)

A CLI subagent **cannot read a live token/$ meter** — that data exists only in the
Agent SDK or external tools (`ccusage`, OTEL). So each reviewer stewards cost
*qualitatively* (flags expensive patterns, recommends the cheapest model tier).
To save real money, pin a cheaper model per agent in its frontmatter (e.g.
`model: haiku` on a researcher, `model: sonnet` on a reviewer; builders usually
want the session's full strength). For true per-token numbers, use `/cost` and
paste them to the reviewer, or move to the Agent SDK — say so rather than
pretending.

## When NOT to orchestrate

Delegation has overhead (a fresh context, a round trip). For a trivial or
mechanical change the planner just does it inline. Reach for the specialists when
the work is substantial, needs an independent audit, or needs outside knowledge.

## Relationship to `/goal` and `/ui-goal`

Those commands encode the *campaign* protocol — the phase backlog, one verified
slice at a time. This workflow says WHO does each slice and within WHICH
department. They compose: a charter's backlog points at `/goal` (or `/ui-goal`
for `edi-ui`); the planner decides whether to build a slice inline or brief the
builder, and whether to gate it through the reviewer.

## Parallel departments — git worktrees

Each **domain** department works in its own **git worktree** (a separate working
directory sharing this one repository) so departments edit in parallel without
colliding in the file system. `master` is the integration line; the agents +
charters live on `master`, so every worktree inherits them.

`edi-ui` is the exception: it works **on `master`**, not a worktree, because it
OWNS the shared shell (`EdiShellWindow*`, `CMakeLists.txt`, `app/main.cpp`).
Making the shell's owner the integration line keeps those shared files
single-author — the domain departments rebase shell changes DOWN from master
rather than fighting edi-ui over them.

Current layout:

| Department | Working dir | Branch |
| --- | --- | --- |
| edi-ui (+ integration) | `/Users/kogaryu/edi` | `master` |
| edi-drafting | `/Users/kogaryu/edi-drafting` | `dept/drafting` |
| edi-blender-lab | `/Users/kogaryu/edi-blender-lab` | `dept/blender-lab` |
| edi-dungeon-map | `/Users/kogaryu/edi-dungeon-map` | `dept/dungeon-map` |

Each domain worktree needs its own `build/` once: `cmake -S . -B build` (CMake
build trees are per-worktree, not shared).

**The rebase contract (this is what keeps departments from tripping).** Three
files are shared by ALL departments and worktrees cannot isolate them:
`src/widgets/EdiShellWindow*.cpp` (the monolith that hosts every department's
panels), `CMakeLists.txt`, and `app/main.cpp`. So:
- **Rebase on `master` at the START of every iteration** (`git rebase master`)
  before building the next slice.
- **Small slices, merge often** — land each verified slice to `master` quickly so
  the shared files don't drift far between departments.
- **At a conflict in a shared file, `master`'s version wins**; re-apply your
  department's small addition on top.
- The lasting cure is to make the shell PLUGGABLE — extract each department's
  panels into its own feature file via the FeatureContext bus — so departments
  stop editing the shared `EdiShellWindow`. Until then, the rebase discipline is
  how they stay out of each other's way.

Manage: `git worktree list` · `git worktree add /Users/kogaryu/edi-<dept> -b dept/<dept>` ·
`git worktree remove <path>`.

## Communication topology (the honest mechanics)

The **hub is this main session** — the user's single window. Everything funnels
through the hub to the user; the user talks only to the hub.

What Claude Code actually supports here (verified, not assumed):

- **Subagents are ONE-SHOT.** A spawned agent runs in isolated context and returns
  ONE final message, then it is gone. There is no re-engaging it (`SendMessage` /
  agent-teams is an experimental flag, off here). "Re-running a role" means
  spawning a FRESH agent that re-reads the durable state below.
- **No reliable 3-tier nesting.** Do not count on a spawned agent spawning and
  managing its own workers. The orchestrator (the hub, or a department SESSION
  acting as hub) drives the one-shot workers directly.
- **Workers cannot talk to each other or to the user** — enforced by tools: the
  builder/reviewer/researcher have NO Agent tool, so they cannot message anyone;
  they only return their report to whoever spawned them. That IS "workers talk only
  to their planner."
- **No agent can force `/compact`.** Compaction is automatic (the harness, as
  context fills) or manual (the user types `/compact`). The hub stays lean by
  OFFLOADING heavy work to one-shot agents (their context never enters the hub) and
  via the **Compact Instructions** in `CLAUDE.md` (which steer what survives).

So the durable backbone is FILES, not agent memory:

- **The ledger** `docs/handoffs/LEDGER.md` — the registry the hub maintains: each
  thread/campaign's id, department, current gate, status, handoff doc, and session
  id. The map of who is doing what.
- **Handoff docs** `docs/handoffs/<campaign-id>.md` — the per-campaign state a gate
  writes for the next gate to read. Agents hand off THROUGH this file (they cannot
  message each other).
- **Closeout docs** `docs/closeouts/<boundary>.md` — freeze a boundary so future
  work does not re-litigate it.

## Persistent threads = sessions

A true persistent "planner thread" is a separate Claude Code SESSION — open one per
department (ideally in its worktree); it adopts that department's planner charter.
Sessions have stable IDs. The hub coordinates them through the ledger plus the
session tools: `list_sessions` / `search_session_transcripts` to SEE what a
department thread did, and `send_message` to hand off to one (this prompts the user
to confirm — it is not silent). The hub cannot CREATE a session; the user opens the
thread, it picks up its charter + the ledger, and runs its gates.

## Gates — the unit of work

Work moves in GATES, in order. The reviewer/research gates run BEFORE the builder
batch ON PURPOSE — to understand the boundary before building wrappers faster than
we understood it. The planner RECORDS each gate's result in the campaign's handoff
doc (a read-only reviewer returns its findings; the planner is the scribe), and the
next gate reads it first.

1. **Research gate** (researcher) — "how do other repos solve this?" Opened ONLY
   when the missing input is EXTERNAL / reference knowledge. NOT for an ownership
   question (that is the reviewer gate).
2. **Reviewer gate** (reviewer) — "where does this belong in edi, and what exactly
   is allowed?" Ownership, naming, repo fit, duplication risk, scope. BEFORE code.
   Its verdict is "boundary settled — yes/no."
3. **Builder batch** (builder) — "implement these slices, commit between them, do
   not pause unless blocked." Bounded batches, verified against the green gate,
   facts reported.
4. **Closeout doc** (planner) — "freeze the boundary so future work does not
   re-litigate it." Written to `docs/closeouts/`.

A planner opens gates in this order, folds each gate's handoff into the next, and
opens the builder batch ONLY once the reviewer gate has settled the boundary. The
planner keeps the long road coherent and decides which lane opens next; it does not
itself write large code (that is a builder batch). Because each worker is one-shot
and reads only its brief + its charter, the planner BRIEFS each worker with its
specific gate role — e.g. "you are the reviewer gate: settle ownership/naming/
duplication/scope for X and propose NO code," or "you are the builder batch:
implement these N slices, commit between them, don't pause unless blocked."

## What the hub does autonomously (and what it cannot)

- **Can, without asking:** spawn one-shot gate-agents; read/write the ledger,
  handoff, and closeout docs; read other sessions via `list_sessions` /
  `search_session_transcripts`.
- **Needs the user:** `send_message` to another session (confirmation prompt);
  creating a new session/thread (the user opens it); forcing a `/compact` (the user
  types it, or the harness auto-compacts).
- **The hub's context-hygiene move:** push heavy reading/building into one-shot
  agents so only their short reports return — that is what keeps this window lean
  between the user's manual compactions.
