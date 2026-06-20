#include "widgets/DrawingCanvasPalette.h"
#include "widgets/ShellTheme.h"

#include "EdiAssert.h"

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
    EDI_CHECK(p.drawableBounds == QColor(theme.accent));
    EDI_CHECK(p.snapGrid == QColor(theme.accent));
    EDI_CHECK(p.originMarker == QColor(theme.warning));
    EDI_CHECK(p.plotTravel == QColor(theme.warning));
    EDI_CHECK(p.snapObject == QColor(theme.success));
    EDI_CHECK(p.safetyOk == QColor(theme.success));
    EDI_CHECK(p.snapOutside == QColor(theme.danger));
    EDI_CHECK(p.safetyWarning == QColor(theme.danger));
    EDI_CHECK(p.snapFree == QColor(theme.textMuted));

    // Structural surfaces derive from the theme (spec §1 unification). The
    // ratios were chosen so the default inputs land within a few units per
    // channel of the legacy fixed family (#17191f/#222630/#3d4452/#465162/
    // #313744) — goldens below are the hand-computed derived values, which
    // supersede the old behavior-preservation literals deliberately.
    EDI_CHECK(p.backdrop == QColor(edi::shell::mixHex(theme.surface, theme.base, 0.35)));
    EDI_CHECK(p.boardFill == QColor(theme.workspaceBody));
    EDI_CHECK(p.boardOutline == QColor(theme.accentSoft));
    EDI_CHECK(p.gridMajor == QColor(edi::shell::mixHex(theme.surface, theme.accent, 0.35)));
    EDI_CHECK(p.gridMinor == QColor(edi::shell::mixHex(theme.surface, theme.accent, 0.17)));
    EDI_CHECK(p.backdrop == QColor("#151a20"));     // legacy #17191f, off (2,1,1)
    EDI_CHECK(p.boardFill == QColor("#232930"));    // legacy #222630, off (1,3,0)
    EDI_CHECK(p.boardOutline == QColor("#364453")); // legacy #3d4452, off (7,0,1)
    EDI_CHECK(p.gridMajor == QColor("#415263"));    // legacy #465162, off (5,1,1)
    EDI_CHECK(p.gridMinor == QColor("#2b3743"));    // legacy #313744, off (6,0,1)
    EDI_CHECK(p.drawableBounds == QColor("#8fb4d8"));
    EDI_CHECK(p.originMarker == QColor("#d5bb78"));
    // Not the painter's old "#9aa8b6": that literal copied the legacy QML's
    // *declared default*, which UiStyle.applyTheme() always overwrote with
    // mix(surface, text, 0.62). The derived token is the canonical value; the
    // painter literal was drift, and adopting the token is the unification.
    EDI_CHECK(p.snapFree == QColor("#9199a1"));
    EDI_CHECK(p.snapGrid == QColor("#8fb4d8"));
    EDI_CHECK(p.snapObject == QColor("#91c89b"));
    EDI_CHECK(p.snapOutside == QColor("#d98b8b"));
    EDI_CHECK(p.plotTravel == QColor("#d5bb78"));
    EDI_CHECK(p.safetyOk == QColor("#91c89b"));
    EDI_CHECK(p.safetyWarning == QColor("#d98b8b"));
    EDI_CHECK(p.selection == QColor("#f6c65b"));
    EDI_CHECK(p.preview == QColor("#75c7ff"));
    EDI_CHECK(p.snapGuide == QColor("#54d2c6"));
    EDI_CHECK(p.handleOutline == QColor("#1d1f26"));
    EDI_CHECK(p.handleFrozen == QColor("#79828f"));
    EDI_CHECK(p.guideFallback == QColor("#83aeca"));
    EDI_CHECK(p.guideLocked == QColor("#6f8295"));
    EDI_CHECK(p.guideIntersection == QColor("#b7d7e8"));
    EDI_CHECK(p.construction == QColor("#9fb2c7"));
    EDI_CHECK(p.dimension == QColor("#b6d28f"));
    EDI_CHECK(p.pointFill == QColor("#d7dde8"));

    // Re-deriving from a re-tuned theme moves the token-backed members AND
    // the accent-ray structural lines; text/base-derived surfaces and the
    // canvas-specific hues hold still.
    ShellThemeInputs pink;
    pink.accent = QStringLiteral("#d46ca1");
    const DrawingCanvasPalette q = deriveCanvasPalette(deriveShellTheme(pink));
    EDI_CHECK(q.snapGrid == QColor("#d46ca1"));
    EDI_CHECK(q.snapGrid != p.snapGrid);
    EDI_CHECK(q.selection == p.selection);
    EDI_CHECK(q.boardFill == p.boardFill);   // workspaceBody is text-derived
    EDI_CHECK(q.backdrop == p.backdrop);     // base/surface unchanged
    EDI_CHECK(q.gridMajor != p.gridMajor);   // accent ray follows the accent
    EDI_CHECK(q.gridMinor != p.gridMinor);
    EDI_CHECK(q.boardOutline != p.boardOutline);

    return 0;
}
