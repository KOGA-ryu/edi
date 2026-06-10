#pragma once

#include <QColor>

namespace edi::shell {
struct ShellTheme;
}

namespace drawing_canvas {

// Every color the canvas painters use for *chrome and feedback* — board,
// grid, snap markers, selection, previews. Object stroke colors are NOT here:
// those are document data (chosen by the user, serialized with the drawing)
// and flow through the projected object style instead. Theming must never
// rewrite a document, only the glass it is viewed through.
struct DrawingCanvasPalette {
    // Surfaces.
    QColor backdrop;          // widget area outside the board; also dimension label plates
    QColor boardFill;
    QColor boardOutline;
    QColor gridMajor;
    QColor gridMinor;

    // Theme-token backed (derived from ShellTheme).
    QColor drawableBounds;    // accent — the dashed drawable-area rectangle
    QColor originMarker;      // warning
    QColor snapFree;          // textMuted — pointer snapped to nothing
    QColor snapGrid;          // accent
    QColor snapObject;        // success
    QColor snapOutside;       // danger — pointer outside the drawable area
    QColor plotTravel;        // warning — pen-up travel moves in plot preview
    QColor safetyOk;          // success — plot bounds overlay, in bounds
    QColor safetyWarning;     // danger — plot bounds overlay / blocked objects

    // Canvas-specific hues with no shell counterpart.
    QColor selection;         // amber: selected strokes, editable handles, in-bounds selection
    QColor preview;           // blue: tool previews, marquee, plot pen-down strokes
    QColor snapGuide;         // teal: guide snaps and guide drag intent
    QColor handleOutline;
    QColor handleFrozen;      // non-editable handle fill
    QColor guideFallback;     // a guide whose stored color fails to parse
    QColor guideLocked;
    QColor guideIntersection;
    QColor construction;      // unselected construction lines
    QColor dimension;         // unselected dimension lines and labels
    QColor pointFill;         // unselected point objects
};

// Resolve the palette from a shell theme. Pure: token-backed members copy the
// theme's resolved tokens; canvas-specific members are fixed values, the same
// way ShellTheme fixes its semantic colors.
DrawingCanvasPalette deriveCanvasPalette(const edi::shell::ShellTheme &theme);

} // namespace drawing_canvas
