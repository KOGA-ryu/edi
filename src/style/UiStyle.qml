pragma Singleton

import QtQuick

QtObject {
    id: style

    // Window geometry
    readonly property int windowWidth: 1280
    readonly property int windowHeight: 820

    // Major region sizes
    readonly property int railWidth: 52
    readonly property int leftPanelWidth: 260
    readonly property int rightPanelWidth: 330
    readonly property int bottomPanelHeight: 132
    readonly property int titleBarHeight: 42
    readonly property int statusBarHeight: 28

    // Colors
    property color colorBase: "#101418"
    property color colorSurface: "#171d24"
    property color colorSurfaceRaised: "#202832"
    property color colorWindow: colorBase
    property color colorRail: "#121920"
    property color colorPanel: colorSurface
    property color colorPanelAlt: "#1b232d"
    property color colorPanelRaised: colorSurfaceRaised
    property color colorWorkspace: "#111821"
    property color colorWorkspaceBody: "#18222d"
    property color colorBottomPanel: "#121920"
    property color colorStatusBar: "#0c1116"
    property color colorControl: "#202a35"
    property color colorControlHover: "#283542"
    property color colorSelected: "#304052"
    property color colorAccent: "#8fb4d8"
    property color colorAccentSoft: "#3a5168"
    property color colorText: "#dce5ee"
    property color colorTextMuted: "#9aa8b6"
    property color colorTextFaint: "#708090"
    property color colorBorderMajor: "#31404f"
    property color colorBorderMinor: "#24313e"
    property color colorBorderFocus: "#5e7892"
    property color colorSuccess: "#91c89b"
    property color colorWarning: "#d5bb78"
    property color colorDanger: "#d98b8b"
    property color colorPending: "#8aa4bf"
    property color colorDisabled: "#55616e"
    readonly property color colorTransparent: "transparent"

    // Spacing
    readonly property int space0: 0
    readonly property int space2: 2
    readonly property int space4: 4
    readonly property int space6: 6
    readonly property int space8: 8
    readonly property int space10: 10
    readonly property int space12: 12
    readonly property int space16: 16
    readonly property int space20: 20

    // Sizing
    readonly property int rowHeightCompact: 26
    readonly property int rowHeight: 30
    readonly property int toolbarHeight: 34
    readonly property int tabHeight: 30
    readonly property int railButtonSize: 34
    readonly property int sectionHeaderHeight: 20

    // Borders and radius
    readonly property int borderNone: 0
    readonly property int borderThin: 1
    readonly property int radiusSm: 5
    readonly property int radiusMd: 8
    readonly property int radiusLg: 12

    // Typography
    property string fontSans: "Avenir Next"
    property string fontMono: "Menlo"
    property int fontSizeXs: 10
    property int fontSizeSm: 11
    property int fontSizeBody: 12
    property int fontSizeEditor: 13
    property int fontSizeTitle: 13
    readonly property int fontWeightRegular: 400
    readonly property int fontWeightMedium: 500
    readonly property int fontWeightSemiBold: 600

    function clamp(value, low, high) {
        return Math.max(low, Math.min(high, value))
    }

    function componentToHex(value) {
        var text = clamp(Math.round(value), 0, 255).toString(16)
        return text.length === 1 ? "0" + text : text
    }

    function normalizeHex(value, fallback) {
        var text = String(value || "").trim()
        if (text.charAt(0) !== "#") {
            text = "#" + text
        }
        return /^#[0-9a-fA-F]{6}$/.test(text) ? text.toUpperCase() : fallback
    }

    function mix(hex, targetHex, ratio) {
        var source = normalizeHex(hex, "#000000")
        var target = normalizeHex(targetHex, "#000000")
        var amount = clamp(ratio, 0, 1)
        var sr = parseInt(source.slice(1, 3), 16)
        var sg = parseInt(source.slice(3, 5), 16)
        var sb = parseInt(source.slice(5, 7), 16)
        var tr = parseInt(target.slice(1, 3), 16)
        var tg = parseInt(target.slice(3, 5), 16)
        var tb = parseInt(target.slice(5, 7), 16)
        return "#" + componentToHex(sr + (tr - sr) * amount)
            + componentToHex(sg + (tg - sg) * amount)
            + componentToHex(sb + (tb - sb) * amount)
    }

    function applyTheme(theme) {
        var colors = theme && theme.colors ? theme.colors : theme || ({})
        var typography = theme && theme.typography ? theme.typography : theme || ({})
        var base = normalizeHex(colors.base, "#333039")
        var surface = normalizeHex(colors.surface, "#373B4F")
        var accent = normalizeHex(colors.accent, "#D46CA1")
        var text = normalizeHex(colors.text, "#577DD1")

        colorBase = base
        colorSurface = surface
        colorSurfaceRaised = mix(surface, text, 0.08)
        colorWindow = base
        colorRail = mix(base, surface, 0.35)
        colorPanel = surface
        colorPanelAlt = mix(surface, text, 0.04)
        colorPanelRaised = colorSurfaceRaised
        colorWorkspace = mix(base, surface, 0.42)
        colorWorkspaceBody = mix(surface, text, 0.06)
        colorBottomPanel = mix(base, surface, 0.32)
        colorStatusBar = mix(base, "#000000", 0.22)
        colorControl = mix(surface, text, 0.08)
        colorControlHover = mix(surface, text, 0.10)
        colorSelected = mix(surface, accent, 0.22)
        colorAccent = accent
        colorAccentSoft = mix(surface, accent, 0.26)
        colorText = text
        colorTextMuted = mix(surface, text, 0.62)
        colorTextFaint = mix(surface, text, 0.38)
        colorBorderMajor = mix(surface, text, 0.12)
        colorBorderMinor = mix(surface, text, 0.055)
        colorBorderFocus = mix(surface, accent, 0.36)
        colorPending = mix(surface, accent, 0.62)
        colorDisabled = mix(surface, text, 0.30)

        var uiFontName = String(typography.ui_font || "").trim()
        var codeFontName = String(typography.code_font || "").trim()
        fontSans = uiFontName.length > 0 ? uiFontName : "Avenir Next"
        fontMono = codeFontName.length > 0 ? codeFontName : "Menlo"

        var uiSize = Number(typography.ui_font_size || 13)
        var codeSize = Number(typography.code_font_size || 12)
        fontSizeBody = clamp(uiSize, 9, 28)
        fontSizeSm = clamp(fontSizeBody - 1, 8, 27)
        fontSizeXs = clamp(fontSizeBody - 2, 8, 26)
        fontSizeTitle = clamp(fontSizeBody + 1, 10, 29)
        fontSizeEditor = clamp(codeSize, 9, 28)
    }

    function currentTheme() {
        return {
            theme_mode: "dark",
            base: String(colorBase),
            surface: String(colorSurface),
            accent: String(colorAccent),
            text: String(colorText),
            ui_font: fontSans === "Avenir Next" ? "" : fontSans,
            code_font: fontMono === "Menlo" ? "" : fontMono,
            ui_font_size: fontSizeBody,
            code_font_size: fontSizeEditor
        }
    }
}
