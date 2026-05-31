import QtQuick
import QtQuick.Layouts
import "../style"

Rectangle {
    id: panel

    property string dataUi: "panel"
    property string dataState: "secondary"
    property string surfaceRecipeId: "panel_surface"
    property color panelColor: UiStyle.colorPanel
    property color panelBorder: UiStyle.colorBorderMajor
    property int panelRadius: UiStyle.radiusSm
    property int panelPadding: UiStyle.space10
    default property alias content: contentSlot.data

    color: panelColor
    border.width: UiStyle.borderThin
    border.color: panelBorder
    radius: panelRadius
    clip: true

    Item {
        id: contentSlot
        anchors.fill: parent
        anchors.margins: panel.panelPadding
    }
}
