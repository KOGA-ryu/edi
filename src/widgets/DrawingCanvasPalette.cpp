#include "widgets/DrawingCanvasPalette.h"

#include "widgets/ShellTheme.h"

namespace drawing_canvas {

DrawingCanvasPalette deriveCanvasPalette(const edi::shell::ShellTheme &theme)
{
    DrawingCanvasPalette p;
    p.backdrop = QColor(QStringLiteral("#17191f"));
    p.boardFill = QColor(QStringLiteral("#222630"));
    p.boardOutline = QColor(QStringLiteral("#3d4452"));
    p.gridMajor = QColor(QStringLiteral("#465162"));
    p.gridMinor = QColor(QStringLiteral("#313744"));

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
