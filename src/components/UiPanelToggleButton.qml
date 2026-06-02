import QtQuick
import QtQuick.Controls
import "../style"

Rectangle {
    id: button

    property string edge: "left"
    property bool selected: false
    property string panelState: selected ? "visible" : "collapsed"
    property string tooltip: ""
    signal clicked()

    implicitWidth: 30
    implicitHeight: 30
    radius: UiStyle.radiusSm
    color: mouseArea.containsMouse ? UiStyle.colorControlHover
        : panelState === "auto_hidden" ? UiStyle.mix(UiStyle.colorControl, UiStyle.colorWarning, 0.18)
        : panelState === "collapsed" ? UiStyle.colorControl
        : UiStyle.colorTransparent
    border.width: UiStyle.borderNone

    Rectangle {
        anchors.fill: parent
        visible: panelState === "visible" && !mouseArea.containsMouse
        radius: parent.radius
        color: UiStyle.mix(UiStyle.colorControl, UiStyle.colorAccent, 0.12)
        opacity: 0.45
    }

    Rectangle {
        anchors.fill: parent
        visible: panelState === "visible" && mouseArea.containsMouse
        radius: parent.radius
        color: UiStyle.mix(UiStyle.colorControlHover, UiStyle.colorAccent, 0.14)
        opacity: 0.9
    }

    ToolTip.visible: mouseArea.containsMouse && tooltip.length > 0
    ToolTip.text: tooltip

    Rectangle {
        id: frame
        anchors.centerIn: parent
        width: 16
        height: 14
        radius: 2
        color: UiStyle.mix(UiStyle.colorControl, UiStyle.colorText, 0.06)
        border.width: UiStyle.borderNone
    }

    Rectangle {
        visible: button.edge === "left"
        x: frame.x + 1
        y: frame.y + 1
        width: 5
        height: frame.height - 2
        radius: 1
        color: panelState === "visible" ? UiStyle.colorAccent : panelState === "auto_hidden" ? UiStyle.colorWarning : UiStyle.colorTextFaint
    }

    Rectangle {
        visible: button.edge === "right"
        x: frame.x + frame.width - 6
        y: frame.y + 1
        width: 5
        height: frame.height - 2
        radius: 1
        color: panelState === "visible" ? UiStyle.colorAccent : panelState === "auto_hidden" ? UiStyle.colorWarning : UiStyle.colorTextFaint
    }

    Rectangle {
        visible: button.edge === "bottom"
        x: frame.x + 1
        y: frame.y + frame.height - 5
        width: frame.width - 2
        height: 4
        radius: 1
        color: panelState === "visible" ? UiStyle.colorAccent : panelState === "auto_hidden" ? UiStyle.colorWarning : UiStyle.colorTextFaint
    }

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        hoverEnabled: true
        onClicked: button.clicked()
    }
}
