# Closeout — interactive map-graph authoring (corridors + doors in the live tool)

> Freezes the interactive-authoring boundary so future work does not re-litigate it.

- **Boundary**: the IN-APP interactive authoring of the map graph — placing plugs,
  connecting them into editable corridors, authoring door types, deleting, and
  re-routing — and the dungeon-map↔edi-ui chrome split for it.
- **Department**: edi-dungeon-map
- **Campaign**: dungeon-map-20260617-corridors-doors
- **Date**: 2026-06-17
- **Final green tip**: `693eac0` (+ this closeout commit) — handed to edi-ui to merge
  onto the current integration line (`17c716a`+).

## Scope ratified (HUB)
The corridor/door GEOMETRY already shipped via `.map.toml` (Phase A/B — verified, NOT
rebuilt; I flagged the dispatch/backlog discrepancy and the hub confirmed Option 2).
This campaign built the **interactive authoring loop in the live tool**. Mandate held
throughout: NEUTRAL in-app authoring of existing primitives — no game rules, no
generation; rules live downstream of Seam B.

## What shipped (the controller surface — edi-ui wires the chrome)
| Verb (`DrawingDocumentController`) | What |
| --- | --- |
| `beginPlugPick()` → `placePlugAtPoint` | free-click places a plug (snapped marker + `CreatePlugCommand`) |
| `beginConnectionPick()` → `connectPlugs` | TWO-click pick (plug A, plug B) → `DeclareConnection` + an editable corridor |
| `selectConnection(connId)` | Map-browser row selects a connection (a RELATION, not an object) |
| `setPlugType(plugId, type)` | sets the plug's neutral type → re-mints the door leaf (new `UpdatePlugCommand` arm) |
| `deletePlug` / `deleteConnection` | delete + cascade (graph records AND rendered objects) |
| `rerouteConnection(connId)` | re-route the corridor from the plugs' CURRENT anchors |

**Projection keys for edi-ui:** `has_connection_selection`, `active_connection_id`,
`active_object_is_plug` (+ the DM-15 `has_block_instance_selection`/`instance_id`).
**Inspector contexts:** `object_connection`{connection_summary, connection_verbs},
`object_plug`{plug_summary, plug_type, plug_verbs} (provisional names — edi-ui aligns
with DM2-surfaces). Full chrome contract:
`~/dept-bus/dungeon-map/replies/006-chrome-contract-batch2.md`.

## Decisions frozen (do NOT re-argue)
1. **Independent / editable corridors** (charter fork, hub-ratified). Each connection
   emits its OWN editable corridor; no Vazgriz merge. Corridor↔connection tie = a NEUTRAL
   provenance tag `connection:<connId>` on each corridor wall (open-vocab breadcrumb, no
   new field/codec). Edit/delete/re-route filter by that tag.
2. **Door leaf ↔ plug tie** = the tag `plug:<plugId>` on the leaf — minted by `setPlugType`
   AND backfilled on authored leaves in `createMapFromSpec` (so authored & interactive
   leaves are SYMMETRIC; `setPlugType` replaces / `deletePlug` cascades both). Plug `type`
   is a neutral open vocabulary; `wallTypeForPlugType` (in `DraftingMapQuery`) maps it to a
   RENDER `WallType` only (no rule).
3. **Relation-aware inspector** — `DraftingInspectorInput` gained `hasConnectionSelection`
   + `activeIsPlugAnchor`; two precedence branches ABOVE the kind branch (connection → plug
   → kind), no regression. The connection selection is controller VIEW-STATE
   (`m_activeConnectionId`), NOT a document field, NOT persisted; mutual-exclusion with the
   object selection is maintained at every selection entry (incl. the canvas-click path and
   undo/redo reconcile).
4. **The two-click connection pick** reuses the `m_pendingBlockId` cross-click idiom (one
   intent + one member), not a new two-stage capture mechanism.
5. **Plug-type update goes through a command** (`UpdatePlugCommand` arm + `updatePlug` op),
   NOT a bracket-only mutation — the command discipline is the codebase rule; the A1
   `static_assert` auto-forces the new branch.
6. **B2-3 door authoring INCLUDED** (hub-ratified); **batch-3 hardening** scoped by the hub
   to the cheap correctness items (done); the rest record-don't-gate.

## Out of scope / explicitly NOT allowed
- **No generation** (WFC/BSP) — tool-first stop-line; rules + generation downstream of Seam B.
- The corridor/door GEOMETRY (`.map.toml` Phase A/B) is NOT rebuilt — only driven by clicks.

## Parked (named, deferred)
- **Auto-re-route-on-plug-move** — v1 is the manual `rerouteConnection`; auto-sync is the
  parked `syncGraphForMovedObject` (also fixes the `plug.anchor` staleness + the corridor-vs-
  Seam-C-export anchor asymmetry). One future slice serves all three.
- **`m_activeConnectionId` in the DocumentSnapshot** — the long-term fix for the undo
  both-true corner (v1 ships a targeted `reconcileActiveConnection` patch).

## Carried NOTES (record, don't gate)
- **Locked-layer delete NIT** (B2-4): a gathered object on a locked layer → its
  `DeleteObjectCommand` is skipped → stray rendered object (graph stays consistent). Matches
  the codebase's trusted-sub-command pattern.
- **edi-ui: `active_plug_type` projection key** — the door-type picker needs the plug's
  CURRENT type to pre-select; add the key (mirror `active_object_is_plug`) IF the picker
  pre-selects, else write-only. Confirm UX.

## Quality record
8 feature slices + 2 fix slices, every one edi-gate GREEN and no-rebase. SIX reviewer
checkpoint audits; THREE caught real bugs the green gate missed — the B2-CTX mutual-exclusion
break on the primary mouse path, and the authored-leaf duplicate/orphan gap (which the
reviewer had predicted in gate 023) affecting both B2-3 and B2-4. The authored↔interactive
symmetry was the recurring untested seam. (Process lesson recorded: fold gate
recommendations into the implementing brief explicitly.)

## The rebase incident (now fleet practice)
A builder's slice-start `git rebase master` twice landed on the stale box-vs-Mac
`origin/master` (591e92c), once corrupting the branch (recovered by `reset --hard` to the
real master, no loss). Hardened rule, now HUB-RATIFIED FLEET-WIDE: **builders NEVER touch
git remote/history; the PLANNER owns all master-sync (onto LOCAL master only).** The hub
later reconciled `origin/master` to the real line; origin is now a current backup.

## Pointers
- Handoff (full gate log): `docs/handoffs/dungeon-map-20260617-corridors-doors.md`
- Design gates: replies `019` (the loop), `023` (relation context + plug-type mechanism)
- Charter: `docs/departments/edi-dungeon-map.md` · Arch: `docs/architecture/edi-dungeon-map.md`
- Prior closeouts: `h2-src-drafting-map-boundary.md`, `dungeon-map-20260617-feature-batch.md`
