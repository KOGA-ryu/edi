#include "widgets/ShellTheme.h"

#include <algorithm>
#include <array>
#include <cmath>

namespace edi::shell {
namespace {

// Parse "#rrggbb" (tolerating a missing '#') into 0-255 channels. Returns false
// on anything malformed so callers can fall back deterministically.
bool parseHex(const QString &value, std::array<int, 3> &rgb)
{
    QString text = value.trimmed();
    if (text.startsWith(QLatin1Char('#'))) {
        text = text.mid(1);
    }
    if (text.size() != 6) {
        return false;
    }
    bool ok = false;
    const int packed = text.toInt(&ok, 16);
    if (!ok) {
        return false;
    }
    rgb = {(packed >> 16) & 0xff, (packed >> 8) & 0xff, packed & 0xff};
    return true;
}

QString toHex(int r, int g, int b)
{
    const auto clampByte = [](int v) { return std::clamp(v, 0, 255); };
    return QStringLiteral("#%1%2%3")
        .arg(clampByte(r), 2, 16, QLatin1Char('0'))
        .arg(clampByte(g), 2, 16, QLatin1Char('0'))
        .arg(clampByte(b), 2, 16, QLatin1Char('0'));
}

} // namespace

QString mixHex(const QString &a, const QString &b, double ratio)
{
    std::array<int, 3> sa{0, 0, 0};
    std::array<int, 3> sb{0, 0, 0};
    parseHex(a, sa); // on failure, black — a safe, visible degradation
    parseHex(b, sb);
    const double amount = std::clamp(ratio, 0.0, 1.0);
    const auto channel = [&](int i) {
        // Math.round semantics (half up) to match the legacy palette exactly.
        return static_cast<int>(std::floor(sa[i] + (sb[i] - sa[i]) * amount + 0.5));
    };
    return toHex(channel(0), channel(1), channel(2));
}

ShellTheme deriveShellTheme(const ShellThemeInputs &in)
{
    ShellTheme t;
    // The four inputs pass through unchanged.
    t.base = in.base;
    t.surface = in.surface;
    t.accent = in.accent;
    t.text = in.text;

    // Everything else is a fixed mix of (surface, text) or (surface, accent) —
    // the same ratios the legacy applyTheme() used, so a custom 4-color theme
    // produces the same family of derived tones it did in the old UI.
    t.surfaceRaised = mixHex(in.surface, in.text, 0.08);
    t.workspaceBody = mixHex(in.surface, in.text, 0.06);
    t.control = mixHex(in.surface, in.text, 0.08);
    t.controlHover = mixHex(in.surface, in.text, 0.10);
    t.selected = mixHex(in.surface, in.accent, 0.22);
    t.textMuted = mixHex(in.surface, in.text, 0.62);
    t.textFaint = mixHex(in.surface, in.text, 0.38);
    t.borderMajor = mixHex(in.surface, in.text, 0.12);
    t.borderMinor = mixHex(in.surface, in.text, 0.055);
    t.borderFocus = mixHex(in.surface, in.accent, 0.36);
    t.accentSoft = mixHex(in.surface, in.accent, 0.26);
    t.pending = mixHex(in.surface, in.accent, 0.62);
    t.disabled = mixHex(in.surface, in.text, 0.30);

    // Semantic colors are fixed points (not derived): their meaning must stay
    // legible regardless of how the four inputs are tuned.
    t.success = QStringLiteral("#91c89b");
    t.warning = QStringLiteral("#d5bb78");
    t.danger = QStringLiteral("#d98b8b");

    t.uiFont = in.uiFont;
    t.codeFont = in.codeFont;
    t.fontSizeBody = std::clamp(in.uiFontSize, 9, 28);
    t.fontSizeSm = std::clamp(t.fontSizeBody - 1, 8, 27);
    t.fontSizeXs = std::clamp(t.fontSizeBody - 2, 8, 26);
    t.fontSizeTitle = std::clamp(t.fontSizeBody + 1, 10, 29);
    t.fontSizeEditor = std::clamp(in.codeFontSize, 9, 28);
    return t;
}

QString buildShellStyleSheet(const ShellTheme &t)
{
    // One QSS string, every color sourced from the resolved theme. Metrics
    // (radii, padding) stay inline as literals for now; they become data when a
    // later phase needs to tune them.
    return QStringLiteral(R"(
        #shellRoot {
            background: %1;
            color: %2;
            font-family: "%3", "Inter", sans-serif;
            font-size: %4px;
        }
        #activityRail {
            background: %1;
            border-right: 1px solid %5;
        }
        #leftPanel, #rightPanel {
            background: %6;
            border-right: 1px solid %5;
            border-left: 1px solid %7;
        }
        #workspaceColumn {
            background: %1;
        }
        #workspaceHeader {
            background: %8;
            border-bottom: 1px solid %5;
        }
        #bottomPanel {
            background: %1;
            border-top: 1px solid %5;
        }
        QScrollArea {
            background: transparent;
            border: none;
        }
        QLabel {
            color: %2;
        }
        #panelTitle, #workspaceTitle {
            color: %2;
            font-size: %9px;
            font-weight: 600;
        }
        #sectionLabel {
            color: %10;
            font-size: %11px;
            font-weight: 600;
            padding-top: 8px;
            text-transform: uppercase;
        }
        #valueLabel, #bottomStatus {
            color: %12;
            background: %8;
            border: 1px solid %7;
            border-radius: 5px;
            padding: 6px 8px;
        }
        #fieldLabel {
            color: %12;
        }
        #geometryField {
            color: %2;
            background: %13;
            border: 1px solid %5;
            border-radius: 5px;
            padding: 4px 6px;
        }
        #geometryField[editInvalid="true"] {
            color: %14;
            background: %15;
            border: 1px solid %14;
        }
        #editErrorLabel {
            color: %14;
            background: %15;
            border: 1px solid %14;
            border-radius: 5px;
            padding: 6px 8px;
        }
        QPushButton {
            color: %2;
            background: %13;
            border: 1px solid %5;
            border-radius: 5px;
            padding: 7px 9px;
            text-align: left;
        }
        QPushButton:hover {
            background: %16;
        }
        QPushButton:pressed, QPushButton:checked {
            background: %17;
            border-color: %18;
        }
        QPushButton:disabled {
            color: %19;
            border-color: %7;
        }
        #railButton {
            min-width: 32px;
            text-align: center;
            padding-left: 6px;
            padding-right: 6px;
        }
        QComboBox, QLineEdit, QDoubleSpinBox, QSpinBox {
            color: %2;
            background: %13;
            border: 1px solid %5;
            border-radius: 5px;
            padding: 4px 6px;
        }
        QComboBox {
            padding: 6px 8px;
        }
        QComboBox::drop-down {
            border: 0;
            width: 22px;
        }
        QCheckBox {
            color: %2;
            spacing: 8px;
        }
        QCheckBox::indicator {
            width: 15px;
            height: 15px;
            border-radius: 3px;
        }
        QCheckBox::indicator:unchecked {
            background: %13;
            border: 1px solid %5;
        }
        QCheckBox::indicator:checked {
            background: %10;
            border: 1px solid %10;
        }
    )")
        .arg(t.base, t.text, t.uiFont)                                  // 1,2,3
        .arg(t.fontSizeBody)                                            // 4
        .arg(t.borderMajor, t.surface, t.borderMinor, t.surfaceRaised)  // 5,6,7,8
        .arg(t.fontSizeTitle)                                           // 9
        .arg(t.accent)                                                  // 10
        .arg(t.fontSizeSm)                                              // 11
        .arg(t.textMuted, t.control, t.danger, t.surfaceRaised)         // 12,13,14,15
        .arg(t.controlHover, t.selected, t.borderFocus, t.disabled);    // 16,17,18,19
}

} // namespace edi::shell
