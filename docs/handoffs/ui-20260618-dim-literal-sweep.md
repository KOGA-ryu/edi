# Handoff — ui-20260618-dim-literal-sweep

> edi-ui's sweep of magic DIMENSION literals in `src/widgets`, per the new
> CLAUDE.md hard rule "no hardcoded dimensions — every dimension is DATA"
> (reviewer-enforced; existing magic dims are defects to sweep). Source: the
> edi-ui-reviewer audit of 2026-06-18 (commit 6520a6d review + sweep).

- **Campaign**: ui-20260618-dim-literal-sweep
- **Department**: edi-ui
- **Goal**: retire magic dimension literals in `src/widgets`, moving each to
  named DATA (a named const / struct / ShellTheme token), BEHAVIOR-PRESERVING
  (byte-identical values, just named). VIEW dims stay in `src/widgets` (never
  `src/drafting`, which is Qt-free core).

## Reviewer findings (prioritized worst-first)

### P1 — Canvas chrome marker/handle pixels (highest traffic; edi-ui chrome) — SLICE 1
`src/widgets/DrawingCanvasObjectPainter.cpp`:
- L78 arrow `headLength = 11.0`; L95 handle outline pen `2`; L153/158/159 snap
  marker pen `1.0` / dot radius `3.0` / crosshair `5.0`; L373/379 guide-label
  offsets `(8.0,-6.0)`/`(8.0,16.0)`; L410/418-421/426 dim tick pen `1.5/2.0`,
  cross half-extent `6.0`, label offset `6.0`. (L102 `handle.sizePx` already DATA.)
- HOME: a `CanvasChromeDims` struct / named consts (`kArrowHeadLengthPx`,
  `kHandleOutlinePenPx`, `kSnapDotRadiusPx`, `kSnapCrosshairPx`, `kGuideLabelOffsetPx`,
  `kDimTickPenPx`, `kDimCrossHalfPx`…) in `DrawingCanvasObjectPainter.h` or a new
  `src/widgets/DrawingCanvasChromeDims.h`. VIEW dims → stay in src/widgets.

### P2 — Shell layout geometry literals — SLICE 2
- `EdiShellWindow.cpp` L1304/L1311 overlay grip hit-width `4` + band thickness `8`;
  L1139 `setIconSize(QSize(16,14))`.
- `EdiShellWindowPanels.cpp` L23 rail `setFixedWidth(52)`, L76 chrome bar
  `setFixedHeight(42)`, L211 status bar `setFixedHeight(28)`; margins/spacing
  `8/6/10/12` at L26-27/79-80/181-182/213-214.
- `ShellWidgetHelpers.cpp` L103-104 margins `12`/spacing `8`; L123 button
  `setMinimumHeight(30)`; L140 spacing `4`.
- `FloatingPalette.cpp` L29 grip `setFixedSize(10,28)`, L20 spacing `3`.
- HOME: strong candidates to join `ShellTheme` as LAYOUT tokens (`railWidth`,
  `chromeBarHeight`, `statusBarHeight`, `panelPadding`, `panelSpacing`,
  `buttonMinHeight`) — ShellTheme today holds only color + font-size tokens.
  Note `buttonMinHeight=30` is duplicated as a literal here AND in QSS — unify.

### P3 — QSS dimension literals (single template; larger brief, needs judgment) — SLICE 3
- `ShellTheme.cpp` bakes DIMENSIONS as literals in the QSS string while it already
  substitutes named COLOR tokens: `border-radius` 3/5/7/8px, button min-height,
  rail 32px squares, scrollbar 8px/handle 24px, toggle 26x12/knob 10px, separators
  1px / margins. HOME: extend the token substitution to dimensions (`@radius@`,
  `@railSquare@`, `@scrollbarWidth@`, `@toggleW/H@`, `@knob@`) fed from ShellTheme
  int fields. Reserve for an Opus builder (substitution plumbing needs judgment).

## NOT flagged (already DATA / exempt)
`panelSpec` sizes, `handle.sizePx`, the viewport fit knobs, `DraftingCanvasDims.h`;
`setContentsMargins(0,0,0,0)` zeros (unset/zero defaults exempt).

## Gate log
- Reviewer audit 2026-06-18 — findings above (read-only). PART 3 coherence: the
  viewport `kViewportFitPaddingFraction` (widget, pixel space) and drafting's
  `kAsciiBoardFillFraction` (core, canvas-unit, baked into geometry) are NOT a
  duplication — separate layers; leave as-is.

## Next
- SLICE 1 (P1 canvas chrome) dispatched to edi-ui-builder — behavior-preserving.
- SLICE 2 (P2 shell layout → ShellTheme tokens) + SLICE 3 (P3 QSS) queued.
