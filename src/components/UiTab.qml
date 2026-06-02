import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../style"

Rectangle {
    id: tab

    property string dataUi: "tab"
    property string dataState: active ? "active" : "inactive"
    property string label: "Tab"
    property string tooltip: ""
    property bool active: false
    property bool clickable: false
    signal clicked()

    implicitHeight: UiStyle.tabHeight
    implicitWidth: Math.max(58, labelText.implicitWidth + UiStyle.space16)
    color: active ? UiStyle.colorPanelRaised : UiStyle.colorControl
    border.width: active ? UiStyle.borderThin : UiStyle.borderNone
    border.color: active ? UiStyle.colorBorderFocus : UiStyle.colorTransparent
    radius: UiStyle.radiusSm

    Text {
        id: labelText
        anchors.centerIn: parent
        text: tab.label
        color: active ? UiStyle.colorText : UiStyle.colorTextMuted
        font.family: UiStyle.fontSans
        font.pixelSize: UiStyle.fontSizeSm
        font.weight: active ? UiStyle.fontWeightSemiBold : UiStyle.fontWeightMedium
        elide: Text.ElideRight
        width: parent.width - UiStyle.space12
    }

    ToolTip.visible: mouseArea.containsMouse && tooltip.length > 0
    ToolTip.text: tooltip
    ToolTip.delay: 2400
    ToolTip.timeout: 8000

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        enabled: tab.clickable
        hoverEnabled: tab.clickable
        cursorShape: tab.clickable ? Qt.PointingHandCursor : Qt.ArrowCursor
        onClicked: tab.clicked()
    }
}
