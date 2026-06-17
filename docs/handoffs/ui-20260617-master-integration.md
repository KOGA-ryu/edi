# Handoff — ui-20260617-master-integration

**Department:** edi-ui (owns `src/widgets`, the master integration line, the chrome bucket).
**Session:** edi-ui-planner. **Gate:** builder (ongoing integration + chrome).

## What this thread is
edi-ui owns three standing duties (HUB kick `~/dept-bus/edi-ui/briefs/000-kick-edi-ui.md`):
1. **Master integration line.** When a domain dept lands a verified op, the hub routes its
   merge here; edi-ui merges to master and keeps master green so other depts rebase down.
2. **The chrome bucket.** Wire each feature's UI surface per `docs/ui-surface/` as the
   domain op lands. Each surface is a new instance of an existing mechanism (belt cell,
   tool-option input, inspector field, picker, panel). Match the existing design exactly.
3. **Shell goldens** (H1 + DM-11) — re-bless the machine-local goldens on this box.

## State as of 2026-06-17

### Done
- **Master promoted** to `9967f0b` (cartography integration of all 3 depts), then
  **merged `dept/drafting` transformGeometry** (DR-01 keystone `167768e`) → master at
  `b4f2eed`. Clean merge (drafting core + 1 test + 1 CMakeLists line; no shell conflicts).
  Verified **97/97 ctest green + scan clean** on this Linux box. Reported to hub.
  → unblocks dungeon-map DM-14/15 (they rebase master).
- **UI-Integration dept + surface specs** committed to master (`6282a93`):
  `.claude/agents/edi-ui-integration/`, `docs/departments/ui-integration.md`,
  `docs/ui-surface/{INFRA.md, drafting/, dungeon-map/, blender-lab/}`.
- **H1 golden re-blessed** on this box (`e103097`): `tests/golden/default_shell_1100x760.png`.
  Was 6508px over a 4180 budget — pure font-AA drift (golden authored on the Mac).
  Verified render correct vs reference before blessing. `edi_shell_window_tests` now 100%
  green here. **Hub told to drop the `-E edi_shell_window_tests` exclusion** from the gate.

### Pending / waiting on upstream
- **Chrome bucket is idle by design** — no surface-bearing op has landed yet.
  DR-01 transformGeometry has **NO OWN SURFACE** (primitive; surfaces only via the future
  DR-10/11/12 transform actions — see `docs/ui-surface/drafting/DR-surfaces.md:50`). Next
  chrome work pipelines behind: DR-10/11/12 (drafting transforms), DM-03/10/11/14/15
  (dungeon-map), blender-lab's 2 File-menu actions.
- **DM-11 map-browser golden co-bless** — FUTURE coordinated event. When dungeon-map adds
  plugs+connections to `buildMapBrowserPanel` (M8, edi-ui-owned host/file, dungeon-map
  content), the `map` workspace golden changes and edi-ui must re-bless in that same change.
  edi-ui is READY. (`docs/ui-surface/dungeon-map/DM-surfaces.md:41`.)

## How a fresh session resumes
1. `git -C ~/edi log --oneline -5` — confirm master tip; `git status -sb` clean.
2. Watch the bus inbox for the next route-merge or "op landed, wire chrome" doorbell.
3. For a chrome slice: read the feature's `docs/ui-surface/<domain>/*-surfaces.md` entry,
   write a self-contained brief to `~/dept-bus/edi-ui/briefs/NNN-*.md`, hand to
   edi-ui-builder, green-gate (build + ctest + scan + a `--snapshot` look check), then
   hand the diff to edi-ui-reviewer (signal-safety / widget lifetimes are the risky joints).
