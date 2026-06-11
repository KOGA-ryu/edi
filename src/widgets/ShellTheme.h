#pragma once

#include <QString>

namespace edi::shell {

// The four colors the whole palette is derived from, plus typography. This is
// the entire tunable surface of the theme — every other token is a deterministic
// mix of these (see deriveShellTheme), mirroring the legacy UiStyle.applyTheme().
struct ShellThemeInputs {
    QString base = QStringLiteral("#101418");
    QString surface = QStringLiteral("#171d24");
    QString accent = QStringLiteral("#8fb4d8");
    QString text = QStringLiteral("#dce5ee");
    QString uiFont = QStringLiteral("Avenir Next");
    QString codeFont = QStringLiteral("Menlo");
    int uiFontSize = 12;   // body; small/xs/title derived around it
    int codeFontSize = 13;
};

// Every resolved token. Colors are #rrggbb strings (cheap to drop into QSS).
struct ShellTheme {
    QString base;
    QString surface;
    QString surfaceRaised;
    QString workspaceBody;
    QString control;
    QString controlHover;
    QString selected;
    // List-row selection (spec §4): mixed from BASE, not surface, so a selected
    // row reads as a darker notch inside a surface-colored list well.
    QString rowSelected;
    QString text;
    QString textMuted;
    QString textFaint;
    QString borderMajor;
    QString borderMinor;
    QString borderFocus;
    QString accent;
    QString accentSoft;
    QString success;
    QString warning;
    QString danger;
    QString pending;
    QString disabled;
    // Window controls (macOS traffic lights) — fixed platform colors with a
    // slightly darkened edge each, per the legacy chrome.
    QString trafficClose;
    QString trafficCloseEdge;
    QString trafficMinimize;
    QString trafficMinimizeEdge;
    QString trafficZoom;
    QString trafficZoomEdge;
    QString uiFont;
    QString codeFont;
    int fontSizeXs = 10;
    int fontSizeSm = 11;
    int fontSizeBody = 12;
    int fontSizeTitle = 13;
    int fontSizeEditor = 13;
};

// Linear interpolate two #rrggbb colors by ratio in [0,1] (ratio 0 -> a, 1 -> b),
// matching the legacy mix(): per-channel, Math.round. Invalid hex falls back to
// black so a bad input degrades to a visible-but-safe color rather than crashing.
QString mixHex(const QString &a, const QString &b, double ratio);

// Resolve the full token set from the four inputs using the legacy mix ratios.
ShellTheme deriveShellTheme(const ShellThemeInputs &inputs);

// Build the application stylesheet (QSS) from a resolved theme. Pure: same theme
// in, same string out — no widget state touched.
// (The QPalette adapter for self-painting widgets lives in ShellWidgetHelpers —
// this module stays QtGui-free so shell_theme_tests links Core only.)
QString buildShellStyleSheet(const ShellTheme &theme);

// The one app-scope sheet: QToolTip is a top-level widget the window sheet
// cannot reach. Kept minimal and separate so app- and window-level styling
// never compete over the same selectors.
QString buildToolTipStyleSheet(const ShellTheme &theme);

} // namespace edi::shell
