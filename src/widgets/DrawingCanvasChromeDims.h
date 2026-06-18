#pragma once

// View-layer DIMENSIONS for the canvas object painter's chrome (arrowheads,
// selection handles, snap markers, guide labels, dimension ticks/labels).
//
// Per the project hard rule "no hardcoded dimensions — every dimension is
// DATA", every pixel size / radius / pen width / offset the painter draws with
// is a named constant here, not a bare literal in the paint logic. These are
// SCREEN pixels (not canvas units): they stay constant under zoom on purpose so
// chrome remains legible at any scale, which is exactly why they live in
// src/widgets (the VIEW layer) and never in src/drafting (the Qt-free core,
// which knows nothing about pixels).
//
// Exempt from this rule and therefore NOT here: epsilons/tolerances, the unset
// 0.0 sentinel, and non-dimension numbers (loop indices, colour channels,
// alpha, spread ratios, the miter limit). Handle box size (handle.sizePx) is
// already carried as DATA on the projected handle, so it is not duplicated here.

namespace drawing_canvas {

// Arrowhead: length in px of each of the two strokes back from a line's tip.
inline constexpr double kArrowHeadLengthPx = 11.0;

// Selection handles: outline pen width in px for handle boxes and their
// anchor leader lines.
inline constexpr double kHandleOutlinePenPx = 2.0;

// Snap / guide-intersection markers: pen width, dot radius, and the half-extent
// of the crosshair arms, all in px.
inline constexpr double kSnapMarkerPenPx = 1.0;
inline constexpr double kSnapDotRadiusPx = 3.0;
inline constexpr double kSnapCrosshairHalfPx = 5.0;

// Guide labels: text offset in px from the guide's edge anchor. The horizontal
// guide nudges its label UP (negative Y), the vertical guide nudges DOWN
// (positive Y), so the two Y offsets deliberately differ.
inline constexpr double kGuideLabelOffsetXPx = 8.0;
inline constexpr double kGuideLabelHorizLabelOffsetYPx = -6.0;
inline constexpr double kGuideLabelVertLabelOffsetYPx = 16.0;

// Dimension chrome: pen width of the tick/leader strokes (a heavier pen when
// selected), the half-extent in px of the end-cross arms, and the px offset
// used to nudge the dimension's preview label off its anchor point.
inline constexpr double kDimTickPenPx = 1.5;
inline constexpr double kDimLeaderPenPx = 2.0;
inline constexpr double kDimCrossHalfPx = 6.0;
inline constexpr double kDimLabelOffsetPx = 6.0;

// Dimension LABEL pill: the backing rounded-rect that sits behind the committed
// dimension's text. Padding X is added to each side of the text width, padding Y
// to its height; the label is lifted by the top offset so it floats just above
// the anchor; the corner radius rounds the pill. (X padding is applied as half
// on the left + the full width padding, so the text stays centred.)
inline constexpr double kDimLabelPadXPx = 6.0;
inline constexpr double kDimLabelPadWidthPx = 12.0;
inline constexpr double kDimLabelPadHeightPx = 8.0;
inline constexpr double kDimLabelTopOffsetPx = 8.0;
inline constexpr double kDimLabelCornerRadiusPx = 4.0;

// Guide line pen widths in px: a heavier stroke when the guide is selected, a
// lighter one otherwise.
inline constexpr double kGuideSelectedPenPx = 2.0;
inline constexpr double kGuidePenPx = 1.25;

// Construction line pen widths in px (committed render): heavier when selected.
inline constexpr double kConstructionSelectedPenPx = 2.0;
inline constexpr double kConstructionPenPx = 1.0;

// Point object: radius in px of the filled disc that IS the point's body.
inline constexpr double kPointFillRadiusPx = 4.0;

// Plot-diagnostic warning box: the minimum px size below which the box is
// inflated, the px amount each edge is adjusted outward when inflated, the pen
// width in px of the dashed warning outline, and the px offset of the warning
// label text off the box's top-left corner.
inline constexpr double kPlotWarnBoxMinPx = 12.0;
inline constexpr double kPlotWarnBoxAdjustPx = 6.0;
inline constexpr double kPlotWarnBoxPenPx = 1.5;
inline constexpr double kPlotWarnLabelOffsetPx = 6.0;

// Preview chrome pen widths in px: the dashed live-preview outline, and the
// lighter dotted construction-line preview.
inline constexpr double kPreviewPenPx = 2.0;
inline constexpr double kPreviewConstructionPenPx = 1.5;

} // namespace drawing_canvas
