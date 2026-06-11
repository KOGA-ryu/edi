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

    // Structural surfaces derive from the theme (spec §1 unification). The
    // ratios were chosen so the default inputs land within a few units per
    // channel of the legacy fixed family (#17191f/#222630/#3d4452/#465162/
    // #313744) — goldens below are the hand-computed derived values, which
    // supersede the old behavior-preservation literals deliberately.
    assert(p.backdrop == QColor(edi::shell::mixHex(theme.surface, theme.base, 0.35)));
    assert(p.boardFill == QColor(theme.workspaceBody));
    assert(p.boardOutline == QColor(theme.accentSoft));
    assert(p.gridMajor == QColor(edi::shell::mixHex(theme.surface, theme.accent, 0.35)));
    assert(p.gridMinor == QColor(edi::shell::mixHex(theme.surface, theme.accent, 0.17)));
    assert(p.backdrop == QColor("#151a20"));     // legacy #17191f, off (2,1,1)
    assert(p.boardFill == QColor("#232930"));    // legacy #222630, off (1,3,0)
    assert(p.boardOutline == QColor("#364453")); // legacy #3d4452, off (7,0,1)
    assert(p.gridMajor == QColor("#415263"));    // legacy #465162, off (5,1,1)
    assert(p.gridMinor == QColor("#2b3743"));    // legacy #313744, off (6,0,1)
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

    // Re-deriving from a re-tuned theme moves the token-backed members AND
    // the accent-ray structural lines; text/base-derived surfaces and the
    // canvas-specific hues hold still.
    ShellThemeInputs pink;
    pink.accent = QStringLiteral("#d46ca1");
    const DrawingCanvasPalette q = deriveCanvasPalette(deriveShellTheme(pink));
    assert(q.snapGrid == QColor("#d46ca1"));
    assert(q.snapGrid != p.snapGrid);
    assert(q.selection == p.selection);
    assert(q.boardFill == p.boardFill);   // workspaceBody is text-derived
    assert(q.backdrop == p.backdrop);     // base/surface unchanged
    assert(q.gridMajor != p.gridMajor);   // accent ray follows the accent
    assert(q.gridMinor != p.gridMinor);
    assert(q.boardOutline != p.boardOutline);

    return 0;
}
