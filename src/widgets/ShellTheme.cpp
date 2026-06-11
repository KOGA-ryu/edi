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
    t.rowSelected = mixHex(in.base, in.accent, 0.14);
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

    // Traffic lights are platform colors, also fixed; each edge is the fill
    // pulled 20% toward black (the legacy's "1px darkened border").
    t.trafficClose = QStringLiteral("#ff5f57");
    t.trafficMinimize = QStringLiteral("#ffbd2e");
    t.trafficZoom = QStringLiteral("#28c840");
    t.trafficCloseEdge = mixHex(t.trafficClose, QStringLiteral("#000000"), 0.2);
    t.trafficMinimizeEdge = mixHex(t.trafficMinimize, QStringLiteral("#000000"), 0.2);
    t.trafficZoomEdge = mixHex(t.trafficZoom, QStringLiteral("#000000"), 0.2);

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
    // Named tokens, substituted by a loop — NOT QString's numbered %N args.
    // Lesson learned the hard way: chained multi-.arg() fills the LOWEST
    // remaining placeholder, so deleting the last use of %8 silently shifted
    // every later value one slot (font sizes got hex colors) while leaving no
    // literal "%" behind for a test to catch. Named markers cannot shift, a
    // leftover "@" is detectable, and the QSS reads as documentation.
    QString sheet = QStringLiteral(R"(
        /* Stylesheet fonts do NOT propagate to children the way setFont
           does — a font on #shellRoot styles only the root widget itself.
           The universal selector is the QSS-native way to reach every
           widget; per-widget rules below still override size/weight. */
        QWidget {
            font-family: "@uiFont@", "Inter", sans-serif;
            font-size: @fontBody@px;
            font-weight: 400;
        }
        #shellRoot {
            background: @base@;
            color: @text@;
        }
        #activityRail {
            background: @base@;
            border-right: 1px solid @borderMajor@;
        }
        #leftPanel, #rightPanel {
            background: @surface@;
            border-right: 1px solid @borderMajor@;
            border-left: 1px solid @borderMinor@;
        }
        #workspaceColumn {
            background: @base@;
        }
        #bottomPanel {
            background: @base@;
            border-top: 1px solid @borderMajor@;
        }
        QScrollArea {
            background: transparent;
            border: none;
        }
        QLabel {
            color: @text@;
        }
        #panelTitle {
            color: @text@;
            font-size: @fontTitle@px;
            font-weight: 600;
        }
        #chromeStatus {
            color: @textMuted@;
            font-size: @fontSm@px;
        }
        /* Section headers per spec §4: UPPERCASE, xs, textFaint, semibold —
           quiet signposts, not accented controls. The fold toggle keeps a
           hover affordance because it IS a control. */
        #sectionToggle {
            background: transparent;
            border: none;
            color: @textFaint@;
            font-size: @fontXs@px;
            font-weight: 600;
            padding: 8px 0px 0px 0px;
            text-align: left;
        }
        #sectionToggle:hover {
            color: @text@;
        }
        #sectionLabel {
            color: @textFaint@;
            font-size: @fontXs@px;
            font-weight: 600;
            padding-top: 8px;
            text-transform: uppercase;
        }
        #valueLabel, #bottomStatus {
            color: @textMuted@;
            background: transparent;
            border: none;
            padding: 2px 0;
        }
        #bottomStatus {
            font-family: "@codeFont@", monospace;
            font-size: @fontSm@px;
        }
        #fieldLabel {
            color: @textMuted@;
        }
        #geometryField {
            color: @text@;
            background: @control@;
            border: 1px solid @borderMajor@;
            border-radius: 5px;
            padding: 4px 6px;
        }
        #geometryField[editInvalid="true"] {
            color: @danger@;
            background: @surfaceRaised@;
            border: 1px solid @danger@;
        }
        #editErrorLabel {
            color: @danger@;
            background: @surfaceRaised@;
            border: 1px solid @danger@;
            border-radius: 5px;
            padding: 6px 8px;
        }
        QPushButton {
            color: @text@;
            background: transparent;
            border: 1px solid transparent;
            border-radius: 5px;
            padding: 4px 8px;
            text-align: left;
            font-weight: 500; /* spec: controls are medium weight */
        }
        QPushButton:hover {
            background: @controlHover@;
            border-color: @borderMajor@;
        }
        QPushButton:pressed, QPushButton:checked {
            background: @selected@;
            border-color: @borderFocus@;
            font-weight: 600; /* spec: selected/active is semibold */
        }
        QPushButton:disabled {
            color: @disabled@;
            border-color: transparent;
        }
        #railButton {
            min-width: 32px;
            text-align: center;
            padding-left: 6px;
            padding-right: 6px;
        }
        QComboBox, QLineEdit, QDoubleSpinBox, QSpinBox {
            color: @text@;
            background: @control@;
            border: 1px solid @borderMajor@;
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
        QListWidget {
            color: @text@;
            background: @surface@;
            border: 1px solid @borderMinor@;
            border-radius: 5px;
        }
        QListWidget::item {
            min-height: 24px;
            padding: 0px 6px;
            border-left: 2px solid transparent;
        }
        QListWidget::item:hover {
            background: @controlHover@;
        }
        QListWidget::item:selected {
            color: @text@;
            background: @rowSelected@;
            border-left: 2px solid @accent@;
        }
        #objectListEmpty {
            color: @textFaint@;
            font-size: @fontSm@px;
        }
        QCheckBox {
            color: @text@;
            spacing: 8px;
        }
        QCheckBox::indicator {
            width: 15px;
            height: 15px;
            border-radius: 3px;
        }
        QCheckBox::indicator:unchecked {
            background: @control@;
            border: 1px solid @borderMajor@;
        }
        QCheckBox::indicator:checked {
            background: @accent@;
            border: 1px solid @accent@;
        }
        QSplitter::handle {
            background: transparent;
        }
        QSplitter::handle:hover, QSplitter::handle:pressed {
            background: @accentSoft@;
        }
        #rightPanelGrip, #bottomPanelGrip {
            background: transparent;
        }
        #rightPanelGrip:hover, #bottomPanelGrip:hover {
            background: @accentSoft@;
        }
        #floatingPalette {
            background: transparent;
        }
        #paletteGrip {
            background: @borderMajor@;
            border-radius: 3px;
        }
        #paletteGrip:hover {
            background: @accentSoft@;
        }
        #titleBar {
            background: @base@;
            border-bottom: 1px solid @borderMajor@;
        }
        #titleBar QPushButton {
            background: transparent;
            border: none;
            border-radius: 5px;
            padding: 4px 8px;
            text-align: center;
        }
        #titleBar QPushButton:hover {
            background: @controlHover@;
        }
        #titleBar QPushButton:checked {
            background: @selected@;
        }
        #titleBar QPushButton:disabled {
            color: @disabled@;
        }
        /* Specificity lesson: the selectors must repeat the '#titleBar
           QPushButton' prefix. Bare '#trafficClose' (one id) LOSES to
           '#titleBar QPushButton' (id + type) on every shared property —
           QSS uses CSS2 specificity, where the generic chrome-button rule
           above silently erased these fills and the traffic lights rendered
           invisible. Two ids + a type also outranks the ':hover' generic
           (id count dominates pseudo-classes), so no hover variant needed. */
        /* 12px content + 1px border each side = a 14px border-box, so
           radius 7 closes a true circle and the hit area IS the dot.
           (QStyleSheetStyle::polish enforces these as the widget's min/max
           size — a setFixedSize in code would just be overwritten.) */
        #titleBar QPushButton#trafficClose,
        #titleBar QPushButton#trafficMinimize,
        #titleBar QPushButton#trafficZoom {
            min-width: 12px;
            max-width: 12px;
            min-height: 12px;
            max-height: 12px;
            border-radius: 7px;
            padding: 0;
        }
        #titleBar QPushButton#trafficClose {
            background: @trafficClose@;
            border: 1px solid @trafficCloseEdge@;
        }
        #titleBar QPushButton#trafficMinimize {
            background: @trafficMinimize@;
            border: 1px solid @trafficMinimizeEdge@;
        }
        #titleBar QPushButton#trafficZoom {
            background: @trafficZoom@;
            border: 1px solid @trafficZoomEdge@;
        }
    )");

    const std::pair<const char *, QString> tokens[] = {
        {"@base@", t.base},
        {"@surface@", t.surface},
        {"@surfaceRaised@", t.surfaceRaised},
        {"@control@", t.control},
        {"@controlHover@", t.controlHover},
        {"@selected@", t.selected},
        {"@rowSelected@", t.rowSelected},
        {"@text@", t.text},
        {"@textMuted@", t.textMuted},
        {"@textFaint@", t.textFaint},
        {"@borderMajor@", t.borderMajor},
        {"@borderMinor@", t.borderMinor},
        {"@borderFocus@", t.borderFocus},
        {"@accent@", t.accent},
        {"@accentSoft@", t.accentSoft},
        {"@danger@", t.danger},
        {"@disabled@", t.disabled},
        {"@trafficClose@", t.trafficClose},
        {"@trafficCloseEdge@", t.trafficCloseEdge},
        {"@trafficMinimize@", t.trafficMinimize},
        {"@trafficMinimizeEdge@", t.trafficMinimizeEdge},
        {"@trafficZoom@", t.trafficZoom},
        {"@trafficZoomEdge@", t.trafficZoomEdge},
        {"@uiFont@", t.uiFont},
        {"@codeFont@", t.codeFont},
        {"@fontBody@", QString::number(t.fontSizeBody)},
        {"@fontSm@", QString::number(t.fontSizeSm)},
        {"@fontXs@", QString::number(t.fontSizeXs)},
        {"@fontTitle@", QString::number(t.fontSizeTitle)},
    };
    for (const auto &[marker, value] : tokens) {
        sheet.replace(QLatin1String(marker), value);
    }
    return sheet;
}

} // namespace edi::shell
