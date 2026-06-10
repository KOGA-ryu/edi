# Shell architecture — a modular feature host

## Vision

edi is not a drafting app with panels; it is a **shell that hosts composable
features**. A *feature* (drafting canvas, text editor, ASCII preview, asset-zoo
taxonomy, script composer…) mounts into a *slot* (left / right / bottom / main).
Which feature sits in which slot — plus panel sizes and collapse state — is
*data* (a **workspace**), redefinable per job and switchable at runtime. The
drafting tool built in R1–R6 is **feature #1**, not the application.

Target jobs are pipelines across features: e.g. define shapes on the canvas →
compose a Blender build script (order + methods) → preview it as ASCII art →
commit to the 3D build. Different jobs want different layouts; the shell must
let features be mixed and matched, and let layouts be redefined.

This was real once: the scrapped QML app drove `main_workspace.feature` /
`left_panel` / `right_panel` / `bottom_panel` from project profiles, switched
workspaces via the activity rail, and `blender_recipe_lab` composed the drafting
canvas + text editor into slots. Mine it (`git show ce0b751:<path>`) as a
behavioral reference; do not port QML/JS.

## The four concepts (data + callables, no subclassing — per CLAUDE.md)

```cpp
// A region the shell offers. (Activity rail / status bar may join later.)
enum class ShellSlot { Main, Left, Right, Bottom };

// A feature is a registered descriptor, NOT a subclass. Variation is the
// factory callable, exactly like the controller's kind-and-callable helpers.
struct FeatureDescriptor {
    QString id;                 // "drafting", "text_editor", "ascii_preview", ...
    QString label;
    std::vector<ShellSlot> slots;   // slots this feature can fill
    // Builds the widget for one slot, wired to the shared context. Pure factory:
    // no global state, everything it needs arrives through FeatureContext.
    std::function<QWidget *(ShellSlot, FeatureContext &)> buildPanel;
};

// Plain data. TOML-serializable. This is a "job".
struct SlotBinding { ShellSlot slot; QString featureId; };
struct PanelGeometry { /* per-slot width/height, collapsed, auto-hide */ };
struct WorkspaceLayout {
    QString id;                 // "drafting", "blender_recipe", ...
    QString label;
    std::vector<SlotBinding> bindings;
    PanelGeometry panels;
};
```

- **FeatureRegistry** — a table of `FeatureDescriptor` (vector or id-map). Adding
  a feature = registering a descriptor. No shell code changes per feature.
- **ShellHost** — the generic window. Given a `WorkspaceLayout` + the registry +
  the context, it instantiates each bound feature's widget into its slot and
  owns chrome, resize, collapse, presets, theme. It knows about *slots*, never
  about *drafting*. Switching workspace = tear down slot widgets, rebuild from a
  different layout.

## The hard 70%: the feature context (cross-feature wiring)

Slotting widgets is the easy part. The jobs are *pipelines*, so features must
share a document and react to each other: the canvas publishes shapes, the
script composer consumes them, the ASCII preview renders the script's output.

Design (preserves the pure-core / thin-Qt-shell split):

- **Documents stay pure** — plain structs in `src/drafting/`-style modules
  (`DraftingDocument` today; a future `ScriptDocument`, `AssetTaxonomy`). No Qt.
- **`FeatureContext` is the thin Qt bus** that *owns* the live documents and
  emits a change signal per document. It generalizes the existing
  `DrawingDocumentController` (which already owns the drawing document and emits
  `modelChanged`). Features read/write documents through typed accessors on the
  context and connect to its signals; they do **not** hold pointers to each
  other.
- A feature declares what it **consumes** and **produces** by which context
  documents it touches — coupling is through shared data, not through feature
  references. ASCII-preview consumes `ScriptDocument`, re-renders on its change.

**Build it incrementally — do not design the full bus before the second feature
exists.** With one feature the context is just today's controller. The bus
earns its generality when feature #2 (the script composer or ASCII preview)
needs to read what feature #1 produced. Add exactly the seam that the second
feature forces, and no more — over-abstracting a one-feature host is the mirror
mistake of under-abstracting it.

## Data management policy (robust by default; JSON only at forced boundaries)

| Data | Format | Why |
|---|---|---|
| Workspace layouts (slot bindings, panel geometry), app settings | **TOML** | human-editable, diffable; the reader/writer exist in `src/formats/` |
| Feature documents (drawing, scripts, taxonomies) | **MessagePack** | compact binary; the value codec + `EDIM` envelope exist; each feature owns its schema + serializer (drafting: `DraftingSerialize`) |
| AI / external handoffs (job descriptions, build plans for an agent) | **TOON** | the handoff format decision; `ToonExport` exists |
| Live cross-feature context (during a session) | **in-memory C++ structs + Qt signals** | never serialized while live; persisted per-feature as MessagePack on save |

**JSON rule (the "only when absolutely necessary"):** JSON never touches our own
persistence or internal data. It is permitted *only* as an interchange format at
a boundary with a foreign system we do not control (e.g. a third-party 3D/web
API that speaks only JSON). When forced, it lives in a thin `*Bridge` adapter at
that exact boundary, is converted to/from our structs immediately, and is never
stored. The earlier codebase died because JSON was the *internal* substrate;
here it is quarantined to egress/ingress with systems we can't dictate to.

## Migration — seam early, retrofit never

The earlier collapse came from "we'll fix the architecture later." So introduce
the host seam *before* it is strictly needed, with one feature in it:

1. The current `EdiShellWindow` is a drafting monolith. Extract the drafting UI
   into a `FeatureDescriptor` ("drafting") whose `buildPanel` returns the canvas
   (Main), the tool tree (Left), the inspector (Right), the status shelf
   (Bottom) — the panels that exist today, just produced by a feature factory.
2. `EdiShellWindow` becomes `ShellHost`: reads a one-binding `WorkspaceLayout`
   (drafting in all slots) and mounts it. Behavior-identical to today.
3. Everything after (theme tokens, real resize/collapse, presets, a second
   feature) builds on the seam instead of bolting onto a monolith.

A minimal one-feature host is cheap now and free to extend; modularizing at
feature #3 is not.

## Reshaped backlog (supersedes the U-phases)

- **H1 — Theme tokens as data.** Port `UiStyle.qml`'s `mix()` / `applyTheme()`
  into a pure `ShellTheme` struct + `deriveTokens()` free function + QSS builder;
  golden-test the derivations (values in `docs/ui_restoration_spec.md` §1, now
  validated against the canonical `UiStyle.qml`). Unify canvas colors onto
  tokens. (Was U1; unchanged, still first — foundational, feature-independent.)
- **H2 — Host seam.** `ShellSlot`, `FeatureDescriptor`, `FeatureRegistry`,
  `WorkspaceLayout` structs; `ShellHost` that mounts a layout. Extract drafting
  into the "drafting" feature descriptor. DoD: the app is behavior-identical to
  today but assembled from a registry + a one-binding layout; shell tests still
  green; a unit test mounts a fake 2-slot feature to prove the host is generic.
- **H3 — Panel system.** Real resize (8px splitters), collapse, auto-hide, the
  presets — now operating on `ShellHost` slots and persisted in the
  `WorkspaceLayout` (TOML). (Was U3, regrounded on the host.)
- **H4 — Frameless chrome + activity rail + status bar.** Spec §3; the activity
  rail switches *workspaces* (layouts), which is its real job.
- **H5 — Workspace persistence + switching.** Load/save `WorkspaceLayout` as
  TOML; switch layouts at runtime (tear down + rebuild slots). DoD: define two
  layouts, switch between them, geometry persists across restart.
- **H6 — Component treatment pass.** Restyle factory output to spec §4.
- **H7 — Review cycle**, then **H8 — replenish** (next feature: ASCII preview or
  script composer forces the first real `FeatureContext` bus work).

## Chrome vs. activity rail (two distinct regions — do not merge)

The legacy shell has TWO top/left regions, confirmed against
`docs/ui_reference/activity_rail_drilldown_1280x820.png` and `default_shell`:

- **Top Chrome** — the 42px frameless title bar. Window controls (close / min /
  max) top-left; the `File/Edit/View/Tools/Window` menu; and the panel-collapse
  toggles positioned **left-panel toggle near the left** (beside the menu),
  **right-panel and bottom-panel toggles on the right**. (H4)
- **Activity Rail** — the thin far-left *vertical* icon strip. It is the global
  **workspace switcher** (legacy modes: Binder / Review / Settings / Proof), not
  a feature launcher and not part of the chrome. Switching it swaps the whole
  `WorkspaceLayout`. (H4 + H5)

Keep these as separate widgets/regions; H4 builds both.

## Resolved decisions

1. **Activity rail = workspace switcher**, a distinct region from Top Chrome
   (see above). Panel-collapse toggles live in Top Chrome, not the rail.
2. **Features are multi-slot.** One feature may occupy several slots at once
   (drafting = canvas in Main + tool tree in Left + inspector in Right + shelf
   in Bottom). A `WorkspaceLayout` may bind one feature to many slots, or
   compose several features across slots. `FeatureDescriptor.slots` lists every
   slot the feature can fill; a binding names (slot, featureId, role?) so one
   feature can render different panels per slot.

## Still open (decide when constraints are known — not blocking H1–H4)

- **Default layout** for the drafting job: the legacy intentionally kept the
  left panel blank and loaded the right panel first. Final per-job layouts wait
  until real constraints emerge — and because layout is data, this is a
  `WorkspaceLayout` choice, never a code change, so it never blocks shell work.
- **Tool-tree content**: rebuilt left panel shows the full taxonomy with unbuilt
  tools disabled, or only wired tools? Decide when the tool tree is built (H6+).
