import QtQuick
import QtQuick.Controls
import "../style"

Rectangle {
    id: button

    property string edge: "left"
    property bool selected: false
    property string tooltip: ""
    signal clicked()

    implicitWidth: 30
    implicitHeight: 30
    radius: UiStyle.radiusSm
    color: mouseArea.containsMouse ? UiStyle.colorControlHover : selected ? UiStyle.colorSelected : UiStyle.colorControl
    border.color: selected ? UiStyle.colorBorderFocus : UiStyle.colorBorderMinor
    border.width: UiStyle.borderThin

    ToolTip.visible: mouseArea.containsMouse && tooltip.length > 0
    ToolTip.text: tooltip

    Rectangle {
        id: frame
        anchors.centerIn: parent
        width: 16
        height: 14
        radius: 2
        color: UiStyle.colorTransparent
        border.color: UiStyle.colorTextMuted
        border.width: UiStyle.borderThin
    }

    Rectangle {
        visible: button.edge === "left"
        x: frame.x + 1
        y: frame.y + 1
        width: 5
        height: frame.height - 2
        radius: 1
        color: selected ? UiStyle.colorAccent : UiStyle.colorTextFaint
    }

    Rectangle {
        visible: button.edge === "right"
        x: frame.x + frame.width - 6
        y: frame.y + 1
        width: 5
        height: frame.height - 2
        radius: 1
        color: selected ? UiStyle.colorAccent : UiStyle.colorTextFaint
    }

    Rectangle {
        visible: button.edge === "bottom"
        x: frame.x + 1
        y: frame.y + frame.height - 5
        width: frame.width - 2
        height: 4
        radius: 1
        color: selected ? UiStyle.colorAccent : UiStyle.colorTextFaint
    }

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        hoverEnabled: true
        onClicked: button.clicked()
    }
}
