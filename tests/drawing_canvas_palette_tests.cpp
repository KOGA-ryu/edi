#include "widgets/DrawingCanvasPalette.h"
#include "widgets/ShellTheme.h"

#include <cassert>

using drawing_canvas::DrawingCanvasPalette;
using drawing_canvas::deriveCanvasPalette;
using edi::shell::ShellTheme;
using edi::shell::ShellThemeInputs;
using edi::shell::deriveShellTheme;

int main()
{
    const ShellTheme theme = deriveShellTheme(ShellThemeInputs{});
    const DrawingCanvasPalette p = deriveCanvasPalette(theme);

    // Token-backed members copy the theme's resolved tokens — the canvas and
    // the shell chrome must agree on what "accent" or "danger" looks like.
    assert(p.drawableBounds == QColor(theme.accent));
    assert(p.snapGrid == QColor(theme.accent));
    assert(p.originMarker == QColor(theme.warning));
    assert(p.plotTravel == QColor(theme.warning));
    assert(p.snapObject == QColor(theme.success));
    assert(p.safetyOk == QColor(theme.success));
    assert(p.snapOutside == QColor(theme.danger));
    assert(p.safetyWarning == QColor(theme.danger));
    assert(p.snapFree == QColor(theme.textMuted));

    // Behavior preservation: under the default theme, every member is exactly
    // the literal the painters used before the palette existed. This is the
    // contract that made extracting the palette a pure refactor.
    assert(p.backdrop == QColor("#17191f"));
    assert(p.boardFill == QColor("#222630"));
    assert(p.boardOutline == QColor("#3d4452"));
    assert(p.gridMajor == QColor("#465162"));
    assert(p.gridMinor == QColor("#313744"));
    assert(p.drawableBounds == QColor("#8fb4d8"));
    assert(p.originMarker == QColor("#d5bb78"));
    // Not the painter's old "#9aa8b6": that literal copied the legacy QML's
    // *declared default*, which UiStyle.applyTheme() always overwrote with
    // mix(surface, text, 0.62). The derived token is the canonical value; the
    // painter literal was drift, and adopting the token is the unification.
    assert(p.snapFree == QColor("#9199a1"));
    assert(p.snapGrid == QColor("#8fb4d8"));
    assert(p.snapObject == QColor("#91c89b"));
    assert(p.snapOutside == QColor("#d98b8b"));
    assert(p.plotTravel == QColor("#d5bb78"));
    assert(p.safetyOk == QColor("#91c89b"));
    assert(p.safetyWarning == QColor("#d98b8b"));
    assert(p.selection == QColor("#f6c65b"));
    assert(p.preview == QColor("#75c7ff"));
    assert(p.snapGuide == QColor("#54d2c6"));
    assert(p.handleOutline == QColor("#1d1f26"));
    assert(p.handleFrozen == QColor("#79828f"));
    assert(p.guideFallback == QColor("#83aeca"));
    assert(p.guideLocked == QColor("#6f8295"));
    assert(p.guideIntersection == QColor("#b7d7e8"));
    assert(p.construction == QColor("#9fb2c7"));
    assert(p.dimension == QColor("#b6d28f"));
    assert(p.pointFill == QColor("#d7dde8"));

    // Re-deriving from a re-tuned theme moves the token-backed members and
    // leaves the canvas-specific ones alone.
    ShellThemeInputs pink;
    pink.accent = QStringLiteral("#d46ca1");
    const DrawingCanvasPalette q = deriveCanvasPalette(deriveShellTheme(pink));
    assert(q.snapGrid == QColor("#d46ca1"));
    assert(q.snapGrid != p.snapGrid);
    assert(q.selection == p.selection);
    assert(q.boardFill == p.boardFill);

    return 0;
}
