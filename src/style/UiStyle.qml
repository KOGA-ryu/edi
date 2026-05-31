pragma Singleton

import QtQuick

QtObject {
    id: style

    // Window geometry
    readonly property int windowWidth: 1280
    readonly property int windowHeight: 820
    readonly property int windowMinWidth: 960
    readonly property int windowMinHeight: 620

    // Major region sizes
    readonly property int railWidth: 52
    readonly property int leftPanelWidth: 260
    readonly property int rightPanelWidth: 330
    readonly property int bottomPanelHeight: 184
    readonly property int statusBarHeight: 28

    // Colors
    readonly property color colorBase: "#101418"
    readonly property color colorSurface: "#171d24"
    readonly property color colorSurfaceRaised: "#202832"
    readonly property color colorWindow: colorBase
    readonly property color colorRail: "#121920"
    readonly property color colorPanel: colorSurface
    readonly property color colorPanelAlt: "#1b232d"
    readonly property color colorPanelRaised: colorSurfaceRaised
    readonly property color colorWorkspace: "#111821"
    readonly property color colorWorkspaceBody: "#18222d"
    readonly property color colorBottomPanel: "#121920"
    readonly property color colorStatusBar: "#0c1116"
    readonly property color colorControl: "#202a35"
    readonly property color colorControlHover: "#283542"
    readonly property color colorSelected: "#304052"
    readonly property color colorAccent: "#8fb4d8"
    readonly property color colorAccentSoft: "#3a5168"
    readonly property color colorText: "#dce5ee"
    readonly property color colorTextMuted: "#9aa8b6"
    readonly property color colorTextFaint: "#708090"
    readonly property color colorBorderMajor: "#31404f"
    readonly property color colorBorderMinor: "#24313e"
    readonly property color colorBorderFocus: "#5e7892"
    readonly property color colorSuccess: "#91c89b"
    readonly property color colorWarning: "#d5bb78"
    readonly property color colorDanger: "#d98b8b"
    readonly property color colorPending: "#8aa4bf"
    readonly property color colorDisabled: "#55616e"
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
    readonly property int sectionHeaderHeight: 24

    // Borders and radius
    readonly property int borderNone: 0
    readonly property int borderThin: 1
    readonly property int radiusSm: 5
    readonly property int radiusMd: 8
    readonly property int radiusLg: 12

    // Typography
    readonly property string fontSans: "Avenir Next"
    readonly property string fontMono: "Menlo"
    readonly property int fontSizeXs: 10
    readonly property int fontSizeSm: 11
    readonly property int fontSizeBody: 12
    readonly property int fontSizeEditor: 13
    readonly property int fontSizeTitle: 13
    readonly property int fontWeightRegular: 400
    readonly property int fontWeightMedium: 500
    readonly property int fontWeightSemiBold: 600
}
