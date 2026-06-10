#include "widgets/ShellTheme.h"

#include <cassert>

using namespace edi::shell;

int main()
{
    // mixHex: exact, hand-verifiable goldens (black<->white).
    assert(mixHex("#000000", "#ffffff", 0.0) == "#000000");
    assert(mixHex("#000000", "#ffffff", 1.0) == "#ffffff");
    assert(mixHex("#000000", "#ffffff", 0.5) == "#808080"); // round(127.5) -> 128
    assert(mixHex("#000000", "#ffffff", 0.08) == "#141414"); // round(20.4) -> 20
    assert(mixHex("#000000", "#ffffff", 0.20) == "#333333"); // round(51.0) -> 51
    // ratio clamps; '#' optional; bad hex degrades to black.
    assert(mixHex("#ffffff", "#000000", -1.0) == "#ffffff");
    assert(mixHex("ffffff", "000000", 0.0) == "#ffffff");
    assert(mixHex("not-a-color", "#ffffff", 0.0) == "#000000");

    const ShellThemeInputs in; // defaults
    const ShellTheme t = deriveShellTheme(in);

    // The four inputs pass through unchanged.
    assert(t.base == in.base);
    assert(t.surface == in.surface);
    assert(t.accent == in.accent);
    assert(t.text == in.text);

    // Derived tokens use the right (source, target, ratio) — recomputed live, so
    // a wrong ratio or wrong pair in deriveShellTheme is caught.
    assert(t.surfaceRaised == mixHex(in.surface, in.text, 0.08));
    assert(t.workspaceBody == mixHex(in.surface, in.text, 0.06));
    assert(t.controlHover == mixHex(in.surface, in.text, 0.10));
    assert(t.selected == mixHex(in.surface, in.accent, 0.22));
    assert(t.textMuted == mixHex(in.surface, in.text, 0.62));
    assert(t.borderFocus == mixHex(in.surface, in.accent, 0.36));
    assert(t.accentSoft == mixHex(in.surface, in.accent, 0.26));
    assert(t.disabled == mixHex(in.surface, in.text, 0.30));

    // Semantic colors are fixed points, independent of the inputs.
    assert(t.success == "#91c89b");
    assert(t.warning == "#d5bb78");
    assert(t.danger == "#d98b8b");

    // Traffic lights: fixed platform fills, edges pulled 20% toward black.
    assert(t.trafficClose == "#ff5f57");
    assert(t.trafficMinimize == "#ffbd2e");
    assert(t.trafficZoom == "#28c840");
    assert(t.trafficCloseEdge == mixHex(t.trafficClose, "#000000", 0.2));
    assert(t.trafficZoomEdge == mixHex(t.trafficZoom, "#000000", 0.2));

    // A custom theme re-derives: change accent, selected/accentSoft follow it,
    // text-derived tokens do not.
    ShellThemeInputs pink = in;
    pink.accent = QStringLiteral("#d46ca1");
    const ShellTheme p = deriveShellTheme(pink);
    assert(p.selected == mixHex(in.surface, pink.accent, 0.22));
    assert(p.selected != t.selected);
    assert(p.textMuted == t.textMuted); // text unchanged -> token unchanged

    // Typography derives around the body size and clamps.
    assert(t.fontSizeSm == t.fontSizeBody - 1);
    assert(t.fontSizeXs == t.fontSizeBody - 2);
    assert(t.fontSizeTitle == t.fontSizeBody + 1);
    ShellThemeInputs huge = in;
    huge.uiFontSize = 999;
    assert(deriveShellTheme(huge).fontSizeBody == 28); // clamped

    // The stylesheet builder is pure and substitutes NAMED tokens; a leftover
    // "@" means a marker had no table entry. Every themed value must actually
    // appear in the sheet — Qt's numbered %N args once shifted silently when a
    // placeholder was deleted (font sizes received hex colors, nothing was
    // left for a contains("%") check to find), so presence is asserted
    // per-value, not per-marker.
    const QString qss = buildShellStyleSheet(t);
    assert(!qss.contains(QStringLiteral("@")));
    for (const QString &value : {t.base, t.surface, t.surfaceRaised, t.control, t.controlHover,
             t.selected, t.text, t.textMuted, t.borderMajor, t.borderMinor, t.borderFocus,
             t.accent, t.accentSoft, t.danger, t.disabled, t.trafficClose, t.trafficCloseEdge,
             t.trafficMinimize, t.trafficMinimizeEdge, t.trafficZoom, t.trafficZoomEdge, t.uiFont}) {
        assert(qss.contains(value));
    }
    // Sizes land as sizes, not as colors (the exact bug the %N shift caused).
    assert(qss.contains(QStringLiteral("font-size: %1px").arg(t.fontSizeBody)));
    assert(qss.contains(QStringLiteral("font-size: %1px").arg(t.fontSizeTitle)));
    assert(qss.contains(QStringLiteral("font-size: %1px").arg(t.fontSizeSm)));

    // The sheet covers every selector the shell relies on — these names are the
    // contract between the builder and the widgets' objectName() values, so a
    // dropped block (e.g. during a refactor) fails here instead of silently
    // unstyling a region of the app.
    for (const char *selector : {
             "#shellRoot", "#activityRail", "#leftPanel", "#rightPanel",
             "#workspaceColumn", "#bottomPanel",
             "#panelTitle", "#chromeStatus", "#sectionLabel", "#valueLabel",
             "#bottomStatus", "#editErrorLabel", "#fieldLabel", "#geometryField",
             "#railButton", "QPushButton", "QComboBox::drop-down",
             "QCheckBox::indicator:checked", "QSplitter::handle",
             "#rightPanelGrip", "#bottomPanelGrip",
             "#titleBar", "#trafficClose", "#trafficMinimize", "#trafficZoom"}) {
        assert(qss.contains(QLatin1String(selector)));
    }
    // Error surfaces are danger-token tinted, never bespoke hex.
    assert(qss.contains(t.danger));

    return 0;
}
