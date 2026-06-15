# edi — project map (the living board)

The one place the whole project has a shape. edi is **not** a drafting app; it is a
**production pipeline hosted in a modular shell**:

```
   Blender recipe lab  ──▶  3D ASSETS  ──▶  placed in DUNGEONS  ──▶  GAME ENGINE
        (Seam A)                (Seam B)            (Seam C)
```

The shell hosts every stage as a **feature** in **slots** (Main/Left/Right/Bottom)
and **workspaces** (layout = data). Everything needs (a) a spot on the UI and
(b) its own profile settings. **~30% done** — the shell and the drafting/dungeon
bench are real; the pipeline does not yet flow end to end and most features still
lack homes.

Cadence (decided 2026-06-15): **light by default** — plan, build, test, commit per
slice inline; run a design/skeptic pass only at the genuinely risky joints (new
seams, new formats). 70% is a lot of road; only a sustainable cadence finishes it.
Sub-backlogs this board sits over: `docs/dungeon-map-tool-backlog.md`,
`docs/shell_architecture.md`, `docs/direction.md`.

---

## 1. The pipeline — the spine (fix this first; it's what makes edi *one* thing)

| Seam | What it is | State | The connective work missing |
|------|-----------|:-----:|-----------------------------|
| **A — Blender → asset** | recipe TOML → bind/resolve → deterministic bpy → headless render | **engine solid · no UI home** | The recipe engine parses/binds/resolves (refuses to compile an unresolved stream — proof never guesses)/compiles to bpy, and `ProcessRunStore` spawns headless Blender → PNG. Missing: a **lab workspace** fusing the drafting grid (it supplies the parameters) + editor + ASCII proof + render, and the **ASCII preview** stage. Fires only via CLI/Build today. R2 mesh/OBJ proof designed (`edi-ui/.claude/plans/R2a-obj-proof.md`), unbuilt. |
| **B — asset → dungeon** | a Blender asset placed in a map as a block/symbol | **◐ REPAIRED (data path)** | ✅ `DraftingBlock.assetRef` (S0 `d0c8821`) · ✅ set at define time (S1 `f4ebb30`) · ✅ `BlockPlacementMetadata` snapshots the asset + an `instanceId` onto every FLATTEN-stamped object (S2 `4364719`). A placement is now traceable to its asset, and the N flattened objects of one stamp share an instance id. Remaining: the asset VALUES stay empty until the Blender lab produces real asset ids to link (Seam A's UI). |
| **C — dungeon → engine** | the neutral map crosses to the engine | **✅ FLOWS (with blocks)** | Rooms now live in the document (`e09f6e9`+`c1512bf`), the authoring scale is stored so the export speaks authored feet (`6189d15`), a document-based `exportMapToToon` emits rooms/plugs/connections **+ a `blocks[]{room,asset,origin,scale,rotation}`** section re-formed from S2 placement provenance (`4d6fadc`), and `--export-map` reads a saved `.edidraw` to carry the placed blocks (`db7f55f`). The full spine has a data path end to end. Remaining: real asset VALUES still wait on the Blender lab (P2); placement is translate-only (scale/rotation 1/0 until `transformGeometry`). |

**Recommended first move:** **Seam B → Seam C** (small, data-first, mostly mirrors
the plug/connection precedent). Done together, "Blender asset → dungeon → engine"
actually *flows* — the moment edi stops being three tools and becomes a pipeline.

---

## 2. Features → UI home → profile settings (every stage needs a spot)

Legend — state: ✅ done · ◐ partial · ○ planned/missing.  Profile: settings page present / partial / **missing**.

| Feature | UI home | Profile settings | State |
|---------|---------|:----------------:|:-----:|
| **Drafting** (canvas + object list + inspector + status) | Drafting workspace, all 4 slots | flat `edi.toml`, no F6 page → **missing** | ✅ |
| **Tool belt** (weapon-cross carousel) | Drafting, floating palette | F6 "Tool Belt" page | ✅ |
| **Block library** (define / stamp / palette) | Drafting, floating "Blocks" palette | tags only → **missing** | ◐ (Seam-B gap) |
| **Dungeon-map authoring** | **Map workspace** (rail "M" + real switch; reuses the drafting canvas + object list, Right slot = live map graph browser) | **missing** | ✅ logic / ◐ home |
| **Text editor** | Drafting + Blender, Bottom slot | none (highlighting/autosave) → **missing** | ✅ |
| **Settings (F6)** | pop-out `Qt::Tool` window | Theme / Tool Belt / Panels pages | ✅ |
| **Theme** | F6 Theme page | **real profiles** (`profiles/*.toml`) | ✅ |
| **Blender render preview** | Blender workspace, Right slot | `blender.executable_path`, hand-edited → **missing** | ◐ |
| **Blender recipe lab / script composer** | **NO HOME YET** (envisioned: canvas + editor + ASCII + script-format) | **missing** | ◐ engine only |
| **ASCII preview** (recipe proof) | **NO HOME YET** | **missing** | ○ |
| **Asset taxonomy / zoo** | **NO HOME YET** (`assetLibraryIds` is an unused placeholder) | **missing** | ○ |
| **Game-engine preview** (Seam-C consumer) | **NO HOME YET** | **missing** | ○ |
| Rail modes Text / Project / Planning | enum exists; fall through to drafting | — | ○ |

Two structural truths here: **only Theme has real profiles** (everything else is flat
`edi.toml` or nothing), and **the map tool + the Blender lab are the two biggest
missing homes**. The fix for profiles is one seam: a `buildSettingsPage` hook on
`FeatureDescriptor` so each feature self-registers its F6 page (append-a-row, the
pattern F6 already uses).

---

## 3. Tracks — the pullable backlog (pick in any order; priority is a suggestion)

**P1 · Pipeline continuity (do first — highest leverage)**

- **✅ Seam B repair — block ↔ asset link.** S0 `DraftingBlock.assetRef` (`d0c8821`) ·
  S1 set at define (`f4ebb30`) · S2 `BlockPlacementMetadata` snapshot + `instanceId` on
  each placed object (`4364719`). S3 (opt, deferred) named anchor points vs center-only.
- **✅ Seam C completion — blocks in the TOON export.** rooms-in-document
  (`e09f6e9` data + `c1512bf` populate) · authoring scale stored (`6189d15`) ·
  document-based `exportMapToToon` with the `blocks[]` section (`4d6fadc`) ·
  `--export-map` reads a `.edidraw` (`db7f55f`). The pipeline now flows end to end.

**P1 PIPELINE CONTINUITY: COMPLETE.** Blender asset → block (Seam B) → stamped in a
dungeon → exported to the engine with the placement (Seam C). The data spine is whole;
what fills it (real asset ids) waits on P2's Blender lab.

**P2 · The big UI homes (what "it all needs a spot" means)**

- **Map authoring as a first-class workspace.** ✅ *Core shipped (slices 1–2):* a `map`
  `FeatureDescriptor` + `mapWorkspaceLayout()` (reuses the drafting canvas — the map IS
  document content — and swaps the Right slot to a live `map_browser` graph view, mirroring
  Blender's Right-slot preview), a real rail "M" switch (`setWorkspaceMode` now public; the
  `--workspace` snapshot flag eyeballs any job). ○ *Remaining:* a map settings page (layers,
  block-library filter, dimensions, export options — overlaps P3's `buildSettingsPage` hook);
  view auto-fit; interior point-features; the Right overlay opens collapsed by default
  (a shell-chrome default shared with Blender — P4).
- **Blender recipe lab — Feature #3 + ASCII proof.** Register an `ascii_preview`
  feature + panel reading `RecipeOpsAscii`; define the lab `WorkspaceLayout`
  (canvas supplies params + editor/ASCII in Bottom + script-format in Right); a recipe
  settings page; build the R2 mesh/OBJ proof (deterministic OBJ from extracted mesh math,
  no Blender install needed).

**P3 · Settings depth (what "custom profile settings" means)**

- A per-feature `buildSettingsPage` hook on `FeatureDescriptor` (features self-register
  F6 pages). Group drafting grid/snap/plot into a real "drafting" profile (extend
  `ProfileStore` beyond theme). Give the Blender preview a page for `executable_path`.
  Scope grid/snap overrides per-workspace on reload.

**P4 · Shell chrome & component polish (H2–H6 — the look the spec wants)**

- H2 frameless chrome (traffic lights, `startSystemMove` drag, 42px bar, offscreen
  fallback bool). H3 real splitter resize (8px, collapse, auto-hide, presets). H4
  activity-rail restyle (52px / 34×34 / selected state). H6 component treatment pass
  (30px rhythm, radius 5) when a consumer forces it.

**P5 · Drafting bench depth (art-tool & precision gaps)**

- Wire **fill into SVG export** (data+paint live; export writes `fill='none'`) — *fastest
  high-value win in the codebase.* Color/swatch picker + line-style completeness.
  `transformGeometry` over the 14 kinds → unlocks block rotate/scale + radial-array picked
  centers. Advanced snaps + dimensions; Ellipse/Spline primitives polish.

**P6 · Stubs & remaining modes (clean up or formally retire)**

- Real `WorkspaceLayout`s for Text / Project / Planning. Implement or retire
  `DrawingRecentFilesStore` + `DrawingRuntimeCore` stubs. Asset-taxonomy browser feature
  (the shared library both the lab and the block palette read). Game-engine preview
  feature (depends on the map workspace + Seam C blocks).

---

## 4. Polish inventory (the "ton of small polish")

| Category | Item |
|----------|------|
| drafting-render | **Fill in SVG export** — paint live, export writes `fill='none'`. *Highest-leverage one-liner.* |
| drafting-render | Region/bucket fill (P3) — colour an enclosed room (needs fill render first) |
| drafting-render | Line-style completeness — add dash-dot / center / phantom / custom |
| drafting-ux | Color/swatch picker for stroke + fill (today free-hex text only) |
| shell-chrome | Frameless chrome / title bar (H2) |
| shell-chrome | Real panel splitter resize (H3) |
| shell-chrome | Activity-rail restyle (H4) |
| shell-chrome | Component treatment pass (H6) |
| shell-ux | Toast/popup notifications (status bar exists, no floating alerts) |
| shell-ux | Undo/redo history visualization |
| persistence | Belt pinned-rows persistence (held in data, not written to `workspace.toml`) |
| persistence | Block library `.edidraw` round-trip at scale (serial recovery untested at scale) |
| settings | Per-workspace settings overrides (grid/snap/plot don't reload per workspace) |
| settings | Layer defaults / custom grid-preset editor |
| map-ux | View auto-fit (P1) — frame the whole map to the viewport |
| map-authoring | Interior room features (P2) — `map.room.<i>.feature.<j>` markers |
| block-library | Block palette tagging/taxonomy + visual polish (user owns the look) |
| export | Export options UI (page size, resolution, coordinate system) |
| performance | Canvas perf — full scene rebuild per paint; no culling/LOD (fine ≤~200 objects) |
| stub | `DrawingRecentFilesStore` — load returns empty, never persists |
| stub | `DrawingRuntimeCore` — header-only stub, empty `.cpp` |

---

## 5. What the shipped ~30% already covers

- **The shell host.** `EdiShellWindow` + `ShellHost` mount features data-drivenly into
  four slots via a `FeatureRegistry` of `FeatureDescriptor`s (no subclassing). Two real
  workspaces (Drafting, Blender), workspace history (back/forward), TOML-persisted
  layout/belt/palette/panel assignments, live theme derivation, recent files, dirty
  guard, undo/redo.
- **The drafting bench (feature #1).** 14 geometry kinds, layers/plot styles,
  calibration/measurement, material+role+provenance metadata, SVG/HPGL/G-code export,
  arrays, snapping, dimensions.
- **The dungeon-map tool (Phases A–D).** `.map.toml` authoring → wall/room geometry →
  neutral plug+connection graph (rides inside `DraftingDocument` for free undo) → A*
  corridor routing → doors → block library (define/FLATTEN-stamp/palette) → TOON export
  of rooms/plugs/connections. A 5-map test corpus + the reference dungeon.
- **The Blender recipe engine (Seam A core).** TOML recipe ops parse/bind/resolve/compile
  to deterministic bpy; headless Blender spawns; render PNG returns to the preview.

95 tests green; no JSON in our data, no `.js`/`.qml`.

---

*Keep this current as slices land (mirror the per-track status). It's the top-level board;
the per-track detail lives in the sub-backlogs.*
