#include "widgets/DrawingCanvasPalette.h"

#include "widgets/ShellTheme.h"

namespace drawing_canvas {

DrawingCanvasPalette deriveCanvasPalette(const edi::shell::ShellTheme &theme)
{
    DrawingCanvasPalette p;
    // Structural surfaces follow the theme (spec §1: unify canvas colors onto
    // the tokens). Ratios are chosen so the DEFAULT inputs land within a few
    // units/channel of the legacy fixed family (#17191f / #222630 / #3d4452 /
    // #465162 / #313744). Exact reproduction is impossible: the legacy family
    // was warmer than any single token ray reaches, so the lines run slightly
    // cooler — imperceptible at 1px weight, and in exchange a custom theme
    // finally reaches the canvas instead of stopping at its edge.
    p.backdrop = QColor(edi::shell::mixHex(theme.surface, theme.base, 0.35));
    p.boardFill = QColor(theme.workspaceBody); // the token NAMED for this, previously dead
    p.boardOutline = QColor(theme.accentSoft);
    p.gridMajor = QColor(edi::shell::mixHex(theme.surface, theme.accent, 0.35));
    p.gridMinor = QColor(edi::shell::mixHex(theme.surface, theme.accent, 0.17));

    p.drawableBounds = QColor(theme.accent);
    p.originMarker = QColor(theme.warning);
    p.snapFree = QColor(theme.textMuted);
    p.snapGrid = QColor(theme.accent);
    p.snapObject = QColor(theme.success);
    p.snapOutside = QColor(theme.danger);
    p.plotTravel = QColor(theme.warning);
    p.safetyOk = QColor(theme.success);
    p.safetyWarning = QColor(theme.danger);

    p.selection = QColor(QStringLiteral("#f6c65b"));
    p.preview = QColor(QStringLiteral("#75c7ff"));
    p.snapGuide = QColor(QStringLiteral("#54d2c6"));
    p.handleOutline = QColor(QStringLiteral("#1d1f26"));
    p.handleFrozen = QColor(QStringLiteral("#79828f"));
    p.guideFallback = QColor(QStringLiteral("#83aeca"));
    p.guideLocked = QColor(QStringLiteral("#6f8295"));
    p.guideIntersection = QColor(QStringLiteral("#b7d7e8"));
    p.construction = QColor(QStringLiteral("#9fb2c7"));
    p.dimension = QColor(QStringLiteral("#b6d28f"));
    p.pointFill = QColor(QStringLiteral("#d7dde8"));
    return p;
}

} // namespace drawing_canvas
