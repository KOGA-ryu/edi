# UI restoration spec — legacy QML look in pure C++ widgets

Design spec for reproducing the scrapped QML shell's look and behavior in Qt
Widgets. Mined from `ce0b751` (read originals via `git show ce0b751:<path>`,
e.g. `src/style/UiStyle.qml`, `src/Main.qml`, `src/regions/*.qml`,
`src/components/Ui*.qml`). Visual targets: `docs/ui_reference/*.png`.
**No QML, no JS** — this spec exists precisely so the technology does not come
back with the look.

## 1. Design tokens (the palette is data — model it that way)

The legacy theme derived every color from four inputs (base, surface, accent,
text) via linear mixing. Reproduce that: a `ShellTheme` struct holding the four
inputs + a pure function computing the derived tokens + a QSS builder consuming
them. Defaults (dark, the only mode):

| Token | Value | Derivation |
|---|---|---|
| base (background/rail/status) | `#101418` | input |
| surface | `#171D24` | input |
| surfaceRaised | `#202832` | surface ⊕ text 8% |
| workspaceBody | `#18222D` | surface ⊕ text 6% |
| control (buttons/inputs) | `#202A35` | surface ⊕ text 8% |
| controlHover | `#283542` | surface ⊕ text 10% |
| selected | `#304052` | surface ⊕ accent 22% |
| text | `#DCE5EE` | input |
| textMuted | `#9AA8B6` | surface ⊕ text 62% |
| textFaint | `#708090` | surface ⊕ text 38% |
| borderMajor | `#31404F` | surface ⊕ text 12% |
| borderMinor | `#24313E` | surface ⊕ text 5.5% |
| borderFocus | `#5E7892` | surface ⊕ accent 36% |
| accent | `#8FB4D8` | input |
| accentSoft | `#3A5168` | surface ⊕ accent 26% |
| success | `#91C89B` | fixed |
| warning | `#D5BB78` | fixed |
| danger | `#D98B8B` | fixed |
| pending | `#8AA4BF` | surface ⊕ accent 62% |
| disabled | `#55616E` | surface ⊕ text 30% |

(⊕ N% = linear interpolation toward the second color by N. Current canvas
colors in `DrawingCanvasWidget`/`ObjectPainter` already match several of these
— `#17191f`/`#222630`/`#8fb4d8`/`#d5bb78`/`#91c89b`/`#d98b8b` — convergence,
not coincidence; unify them onto the tokens.)

Typography: UI font Avenir Next, code font Menlo. Sizes: xs 10, sm 11,
body 12, editor/title 13. Weights: 400 regular, 500 medium (controls),
600 semibold (selected/active). Disabled = opacity 0.42 + `disabled` text
color, never desaturation.

Spacing scale: 0/2/4/6/8/10/12/16/20. Panel padding 10. Radii: sm 5 (buttons,
inputs, tabs, chips), md 8 (cards), border 1px.

## 2. Layout constants

- Window: default 900×760, min 520×420. Frameless (custom chrome).
- Title bar 42px; activity rail 52px fixed; status bar 28px; toolbar 34px;
  tab 30px; standard row 30px; compact row 26px; section header 20px.
- Left panel: 260 default, 180–520, auto-hide when window width < 640.
- Right panel: 300 default, 160+, never auto-hidden.
- Bottom panel: 132 default, 96–1000, auto-hide when window height < 520.
- Initial: left open, right closed, bottom closed.
- Splitters: 8px invisible hit zone, 1px visible line (borderMajor @55%
  idle → accentSoft @90% on hover/drag), SplitH/SplitV cursors.
- Layout presets: `full` (all open, sizes reset), `focus`/`tiny` (all
  collapsed), `review` (left open, right+bottom collapsed). Manual collapse,
  auto-hide, and presets are three independent inputs to one
  `panelState() ∈ {visible, collapsed, auto_hidden}` — model as data.

## 3. Shell behaviors

- **Title bar:** frameless window; left: three 14px traffic lights
  (`#FF5F57`/`#FFBD2E`/`#28C840`, 1px darkened border, radius 7) → close/min/
  max; center: drag region calling `windowHandle()->startSystemMove()`;
  right: panel toggle buttons. 1px bottom divider.
- **Panel toggle buttons** (30×30): a 16×14 inner frame with a 5×12 edge
  indicator bar — accent when visible, faint when collapsed, warning when
  auto-hidden. Tooltips name the action.
- **Activity rail:** 34×34 icon buttons (letter glyphs ok), selected state =
  `selected` background; bottom stretch. Current `buildActivityRail` exists —
  restyle to spec.
- **Status bar:** mode label (mono, xs, textMuted); drawing mode shows dirty
  state (warning color when dirty); elides right.

## 4. Component treatments (build on demand — only with a consumer)

Existing factories (`makeActionButton`, `makeToggle`, `makeDataCombo`,
`makeConditionalButton`, `makeSectionLabel`, `makeValueLabel`) stay the API;
this spec restyles their output. Notable treatments:

- Buttons: 30px tall, radius 5, transparent idle / controlHover hover /
  selected+semibold active; pointing-hand cursor.
- Tabs: 30px, radius 5, active = selected bg + semibold.
- Section headers: UPPERCASE, xs, textFaint, semibold (current
  `sectionLabel` ≈ right).
- Status chip (new widget when needed): 20px pill, xs semibold text, bg =
  status color @ ~24% alpha, text = status color (success/warning/danger/
  pending mapping).
- List rows: 24px, selected = base ⊕ accent 14% + 2px accent left bar;
  meta text right-aligned, drops when narrow.
- Toggle switch (style upgrade for `makeToggle` output): 28×14 track,
  radius 7, 10px knob, accentSoft track when on. QSS `QCheckBox::indicator`
  may be sufficient; custom paint only if not.
- Identity traits to preserve: dense 30px rhythm, no animations (state
  changes instant), accent used only for selection/focus/affordance — never
  as surface fill, 1px chrome everywhere.

## 5. What NOT to port

Light theme (never existed), animations (never existed), the QML component
zoo without consumers, theme *editing* UI (later, with R6's TOML — the
`ShellTheme` struct is designed to be TOML-loadable when that day comes).
