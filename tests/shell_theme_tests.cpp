#include "widgets/ShellTheme.h"

#include "EdiAssert.h"

using namespace edi::shell;

int main()
{
    // mixHex: exact, hand-verifiable goldens (black<->white).
    EDI_CHECK(mixHex("#000000", "#ffffff", 0.0) == "#000000");
    EDI_CHECK(mixHex("#000000", "#ffffff", 1.0) == "#ffffff");
    EDI_CHECK(mixHex("#000000", "#ffffff", 0.5) == "#808080"); // round(127.5) -> 128
    EDI_CHECK(mixHex("#000000", "#ffffff", 0.08) == "#141414"); // round(20.4) -> 20
    EDI_CHECK(mixHex("#000000", "#ffffff", 0.20) == "#333333"); // round(51.0) -> 51
    // ratio clamps; '#' optional; bad hex degrades to black.
    EDI_CHECK(mixHex("#ffffff", "#000000", -1.0) == "#ffffff");
    EDI_CHECK(mixHex("ffffff", "000000", 0.0) == "#ffffff");
    EDI_CHECK(mixHex("not-a-color", "#ffffff", 0.0) == "#000000");

    const ShellThemeInputs in; // defaults
    const ShellTheme t = deriveShellTheme(in);

    // The four inputs pass through unchanged.
    EDI_CHECK(t.base == in.base);
    EDI_CHECK(t.surface == in.surface);
    EDI_CHECK(t.accent == in.accent);
    EDI_CHECK(t.text == in.text);

    // Derived tokens use the right (source, target, ratio) — recomputed live, so
    // a wrong ratio or wrong pair in deriveShellTheme is caught.
    EDI_CHECK(t.surfaceRaised == mixHex(in.surface, in.text, 0.08));
    EDI_CHECK(t.workspaceBody == mixHex(in.surface, in.text, 0.06));
    EDI_CHECK(t.controlHover == mixHex(in.surface, in.text, 0.10));
    EDI_CHECK(t.selected == mixHex(in.surface, in.accent, 0.22));
    // rowSelected is the one token mixed from BASE (spec §4 list rows), so a
    // selected row reads as a darker notch inside a surface-colored list.
    EDI_CHECK(t.rowSelected == mixHex(in.base, in.accent, 0.14));
    EDI_CHECK(t.rowSelected == "#222a33"); // hand-computed golden for the defaults
    EDI_CHECK(t.textMuted == mixHex(in.surface, in.text, 0.62));
    EDI_CHECK(t.borderFocus == mixHex(in.surface, in.accent, 0.36));
    EDI_CHECK(t.accentSoft == mixHex(in.surface, in.accent, 0.26));
    EDI_CHECK(t.disabled == mixHex(in.surface, in.text, 0.30));

    // Semantic colors are fixed points, independent of the inputs.
    EDI_CHECK(t.success == "#91c89b");
    EDI_CHECK(t.warning == "#d5bb78");
    EDI_CHECK(t.danger == "#d98b8b");

    // Traffic lights: fixed platform fills, edges pulled 20% toward black.
    EDI_CHECK(t.trafficClose == "#ff5f57");
    EDI_CHECK(t.trafficMinimize == "#ffbd2e");
    EDI_CHECK(t.trafficZoom == "#28c840");
    EDI_CHECK(t.trafficCloseEdge == mixHex(t.trafficClose, "#000000", 0.2));
    EDI_CHECK(t.trafficZoomEdge == mixHex(t.trafficZoom, "#000000", 0.2));

    // A custom theme re-derives: change accent, selected/accentSoft follow it,
    // text-derived tokens do not.
    ShellThemeInputs pink = in;
    pink.accent = QStringLiteral("#d46ca1");
    const ShellTheme p = deriveShellTheme(pink);
    EDI_CHECK(p.selected == mixHex(in.surface, pink.accent, 0.22));
    EDI_CHECK(p.selected != t.selected);
    EDI_CHECK(p.rowSelected == mixHex(in.base, pink.accent, 0.14)); // follows accent
    EDI_CHECK(p.rowSelected != t.rowSelected);
    EDI_CHECK(p.textMuted == t.textMuted); // text unchanged -> token unchanged

    // Typography derives around the body size and clamps.
    EDI_CHECK(t.fontSizeSm == t.fontSizeBody - 1);
    EDI_CHECK(t.fontSizeXs == t.fontSizeBody - 2);
    EDI_CHECK(t.fontSizeTitle == t.fontSizeBody + 1);
    ShellThemeInputs huge = in;
    huge.uiFontSize = 999;
    EDI_CHECK(deriveShellTheme(huge).fontSizeBody == 28); // clamped

    // The stylesheet builder is pure and substitutes NAMED tokens; a leftover
    // "@" means a marker had no table entry. Every themed value must actually
    // appear in the sheet — Qt's numbered %N args once shifted silently when a
    // placeholder was deleted (font sizes received hex colors, nothing was
    // left for a contains("%") check to find), so presence is asserted
    // per-value, not per-marker.
    const QString qss = buildShellStyleSheet(t);
    EDI_CHECK(!qss.contains(QStringLiteral("@")));
    for (const QString &value : {t.base, t.surface, t.surfaceRaised, t.control, t.controlHover,
             t.selected, t.rowSelected, t.text, t.textMuted, t.borderMajor, t.borderMinor, t.borderFocus,
             t.accent, t.accentSoft, t.danger, t.disabled, t.trafficClose, t.trafficCloseEdge,
             t.trafficMinimize, t.trafficMinimizeEdge, t.trafficZoom, t.trafficZoomEdge, t.uiFont}) {
        EDI_CHECK(qss.contains(value));
    }
    // Sizes land as sizes, not as colors (the exact bug the %N shift caused).
    EDI_CHECK(qss.contains(QStringLiteral("font-size: %1px").arg(t.fontSizeBody)));
    EDI_CHECK(qss.contains(QStringLiteral("font-size: %1px").arg(t.fontSizeTitle)));
    EDI_CHECK(qss.contains(QStringLiteral("font-size: %1px").arg(t.fontSizeSm)));
    EDI_CHECK(qss.contains(QStringLiteral("font-size: %1px").arg(t.fontSizeXs)));
    EDI_CHECK(qss.contains(t.codeFont)); // the mono consumer (bottom-shelf readout)

    // The sheet covers every selector the shell relies on — these names are the
    // contract between the builder and the widgets' objectName() values, so a
    // dropped block (e.g. during a refactor) fails here instead of silently
    // unstyling a region of the app.
    for (const char *selector : {
             "#shellRoot", "#activityRail", "#leftPanel", "#rightPanel",
             "#workspaceColumn", "#bottomPanel",
             "#statusBar", "#statusMode", "#statusFile[documentDirty=\"true\"]",
             "#panelTitle", "#sectionLabel", "#valueLabel",
             "#bottomStatus", "#editErrorLabel", "#fieldLabel", "#geometryField",
             "#railButton", "QPushButton", "QComboBox::drop-down",
             "QComboBox QAbstractItemView", "QMenu::item:selected",
             "QFrame[chromePopup=\"true\"]", "#settingsWindow",
             "QListWidget::item:selected", "#objectListEmpty",
             "QScrollBar::handle:vertical", "QAbstractSpinBox::up-button",
             "QComboBox:disabled", "QCheckBox::indicator:checked:disabled",
             "#titleBar QPushButton[panelState=\"auto_hidden\"]",
             "#titleBar QPushButton#toggleLeftPanel",
             "QSplitter::handle:horizontal:hover",
             "QCheckBox::indicator:checked", "QSplitter::handle",
             "#rightPanelGrip", "#bottomPanelGrip",
             "#titleBar", "#trafficClose", "#trafficMinimize", "#trafficZoom"}) {
        EDI_CHECK(qss.contains(QLatin1String(selector)));
    }
    // Error surfaces are danger-token tinted, never bespoke hex.
    EDI_CHECK(qss.contains(t.danger));

    // Splitter-line composites: #AARRGGBB strings assembled from the tokens
    // (alpha 55% idle / 90% hot), present in the sheet.
    EDI_CHECK(qss.contains(QStringLiteral("#8c") + t.borderMajor.mid(1)));
    EDI_CHECK(qss.contains(QStringLiteral("#e6") + t.accentSoft.mid(1)));

    // The tooltip sheet is the one app-scope sheet (top-level widgets are
    // out of the window sheet's reach) — tokens substituted, no markers.
    const QString tipSheet = buildToolTipStyleSheet(t);
    EDI_CHECK(tipSheet.contains(QStringLiteral("QToolTip")));
    EDI_CHECK(tipSheet.contains(t.surfaceRaised));
    EDI_CHECK(tipSheet.contains(t.text));
    EDI_CHECK(tipSheet.contains(t.borderMajor));
    EDI_CHECK(!tipSheet.contains(QStringLiteral("%1")));

    return 0;
}
