# Scale-knob design — one S dials the whole crypt (PROPOSAL, reviewer gate)

**Status:** FORK BLESSED by the hub → **(c)** (scaled-feet canonical + ONE neutral
`scale` scene-scalar in the TOON header; the realizer scales its OWN constants by S).
Remaining gates fold IMPLEMENTATION detail: reviewer 045 audits (c) for standing-rule
+ exact contract wording + grid-alignment; blender-lab 046 confirms the realizer-S
table. Hub ask: the user dials S via a **single flag on the generator entry**, NOT a
TOON hand-edit (see "User-facing knob" below).

## RESOLVED (fork c) — the implementation
- **Generator:** `buildCryptMapSpec(double scale=1.0)` scales base-table dims by S
  (canonical SCALED feet in the doc). `exportMapToToon(doc, title, units, double
  sceneScale=1.0)` gains a `sceneScale` param and writes ONE header line
  `scale: <sceneScale>` (%g; default 1; missing ⇒ 1 for old TOONs — additive/tolerant).
- **Realizer (blender-lab):** reads the `scale` header meta and multiplies its greybox
  DATA TABLE by it — MODULE=5·S, CORRIDOR_W=5·S, DOOR_W=4·S, WALL_H=12·S, WALL_T/
  FLOOR_T·S, brazier light range/energy·S; rooms placed at the LITERAL scaled feet (NOT
  re-scaled). Same tile COUNT, bigger tiles. Camera auto-frames (already).
- **User-facing knob (hub ask):** a single flag on the generator CLI entry —
  RECOMMENDED `edi --generate-crypt <out.toon> --scale <S>` (writes the scaled TOON +
  `scale:` meta; the render daemon watching /tmp/m0 renders + auto-opens). The CLI flag
  lives in `app/main.cpp` (edi-ui-owned) → coordinated with edi-ui (brief 047). The
  user types ONE command; NO TOON hand-edit.

**Goal (hub brief 044):** replace per-change re-briefing ("double again") with ONE
SCALE knob `S`. One number scales the whole dungeon — rooms, corridor, brazier light —
coherently. The user dials x1/x2/x4 directly. Standing rule holds: **S scales DATA;
base dims stay named data; S is the only user-facing number.** Keep the WIRE NEUTRAL.

## Base (S=1) layout — the data tables hold THIS (= today ÷ 4)
The chain currently runs at the twice-doubled size; that becomes **S=4**. So base S=1:
- entrance 15×15 @ (0,5); crypt 25×25 @ (35,0) (stone). Straight corridor (plug
  midpoints colinear at world y=12.5).
- blocks: sarcophagus (47.5,12.5), brazier (41,6), stair (38,4).
- TOON golden: `~/dept-bus/dungeon-map/crypt_base.toon` (S=1),
  `~/dept-bus/dungeon-map/crypt_doubled2.toon` (S=4 = today).

## Generator half (dungeon-map) — settled shape
`edi::drafting::MapSpec buildCryptMapSpec(double scale = 1.0);`
- Multiplies every BASE-table dimension (room origins + sizes + block positions) by
  `scale`. Plug `at` re-derives from the SCALED room dims (already midpoint-derived —
  follows for free). Corridor length follows from the scaled positions.
- `scale` is a PARAMETER, not a magic literal — satisfies the standing rule ("where a
  dimension varies (scale) it is a parameter"). Base dims stay named data.
- Grid alignment: base origins/sizes are 5 ft multiples ⇒ INTEGER S keeps them on the
  5 ft grid (the user dials 1/2/4). Non-integer S may land off-grid — DECISION for the
  gate: accept (props/feet tolerate it) vs snap. Recommend: accept; document that
  whole-number S preserves grid alignment.

## SETTLED (reviewer 045 + blender-lab 046 folded, 2026-06-18)
- **Wire = (c) scaled feet** (reject a/b). **`scale:` meta ADOPTED** (hub-blessed,
  blender-lab-implemented) WITH the reviewer's anti-double-scale FENCE: advisory presenter
  metadata, feet authoritative, realizer must NOT re-scale feet by it. (Reviewer preferred
  no-meta/derive-from-span; NOT taken — span ≠ scale for M1 varied dungeons, so an explicit
  shape-independent S is the right knob; the hub blessed it.) Contract §0/§5 amended.
- **Base = S=1 = 15×15/25×25** is CONFIRMED CORRECT on the live tip (commit 1d0a487);
  today = S=4 = 60/100 is consistent & on-grid. (The reviewer's "base is 30/50 ⇒ today=S=2"
  BLOCKER was a STALE read of the pre-refactor tables — moot.)
- **Grid (reviewer item 3 — ACCEPT, do not snap):** integer S preserves the 5 ft grid
  (base is 5-ft-aligned); non-integer S is permitted but rooms may land off-grid (props
  always float). Documented in contract §1 spirit.
- **Realizer (blender-lab 046, fork c LIVE @4ee013e):** `GreyboxDims.scaled(S)` ×S on the
  envelope; `tile`=5 FIXED; light density invariant (scales S² via wire area, no S input);
  rooms at literal feet. S=1/2/4 demo rendered (/tmp/m0/crypt_S{1,2,4}.png, OptiX/5090).
  Realizer reads `scale` header meta OR `--scale` CLI (CLI overrides).

## The FORK — how does S reach the REALIZER? (wire stays neutral)
The realizer can't derive corridor/door WIDTH or light range from neutral feet (the
wire carries no widths). So S must reach it. Three mechanisms:

| | Wire | Generator | Realizer | One knob? | Contract |
| --- | --- | --- | --- | --- | --- |
| **(a)** out-of-band param | UNCHANGED (scaled feet) | scales feet by S | gets S via a shared chain CLI/arg | only if a unified runner threads S to both | no change |
| **(b)** base feet + `scale` meta | base feet + `scale:S` header | emits BASE + writes meta | multiplies EVERYTHING (rooms+widths+light) by meta | YES (TOON carries S) | +header meta |
| **(c)** scaled feet + `scale` meta | scaled feet + `scale:S` header | scales feet by S + writes meta | places rooms at feet as-is; scales ONLY its constants (width/height/light) by meta | YES (TOON carries S) | +header meta |

**Recommendation: (c).** The TOON is the existing channel between generator and
realizer (the tools are decoupled — no unified CLI today), so carrying S as a single
neutral scalar meta in the header is a true one-knob without a shared runner. (c)
keeps the TOON literally showing the dungeon at scale (SCALE-POLICY invariant 1:
canonical feet are the real, scaled feet) while the realizer reads the meta only to
scale its own greybox constants + light (NOT to re-scale rooms — those are already
scaled in feet). Wire stays neutral: one scalar `scale:` line, no per-element widths.
- (a) is cleanest on the wire but needs a unified chain-runner CLI that does not exist
  (generator CLI is deferred to edi-ui); without it "one knob" means passing S to two
  tools — desync risk.
- (b) is simplest for the realizer (uniform multiply) but the canonical TOON feet are
  the BASE crypt, not the scaled one — weaker invariant-1 story.
- **Open to (b) if blender-lab finds the uniform-multiply materially simpler** — the
  realizer owns that half; (b) vs (c) is its ergonomics call.

## Realizer half (blender-lab) — coordinate
Whichever of (b)/(c): the realizer's greybox + light DATA TABLES multiply by S —
CORRIDOR_W = 5·S, DOOR_W = 4·S, WALL_H = 12·S, WALL_T/FLOOR_T·S, brazier light
range/energy scaled for coherent lighting; camera auto-frames the span (already).
Reads S from the TOON `scale` header meta (b/c) or a shared arg (a).

## Contract impact
(b)/(c) add a single additive, tolerant header meta `scale: S` (default 1, missing ⇒
1, no version bump — the established discipline). A neutral scene scalar, NOT a
per-element width ⇒ wire stays neutral. Frozen contract §0 gains the meta; §5 notes
the realizer constants multiply by `scale`. (a) needs no contract change.

## Deliverable (brief 044)
One command renders the crypt at any S; prove S=1, 2, 4 (auto-opens on the desktop).
S=4 reproduces today. Green gate.
