import QtQuick
import QtQuick.Controls
import "../style"

Rectangle {
    id: button

    property string dataUi: "button"
    property string dataState: selected ? "selected" : hovered ? "hover" : "idle"
    property string label: "Button"
    property string iconText: ""
    property bool selected: false
    property bool compact: false
    signal clicked()

    implicitWidth: compact ? UiStyle.railButtonSize : 96
    implicitHeight: compact ? UiStyle.railButtonSize : UiStyle.rowHeight

    radius: UiStyle.radiusSm
    opacity: enabled ? 1.0 : 0.42
    color: selected ? UiStyle.colorSelected : hovered && enabled ? UiStyle.colorControlHover : UiStyle.colorTransparent
    border.width: selected ? UiStyle.borderThin : UiStyle.borderNone
    border.color: selected ? UiStyle.colorAccentSoft : UiStyle.colorTransparent

    property bool hovered: false

    Text {
        anchors.centerIn: parent
        text: button.compact && button.iconText.length > 0 ? button.iconText : button.label
        color: !button.enabled ? UiStyle.colorDisabled : button.selected ? UiStyle.colorText : UiStyle.colorTextMuted
        font.family: UiStyle.fontSans
        font.pixelSize: button.compact ? UiStyle.fontSizeBody : UiStyle.fontSizeSm
        font.weight: button.selected ? UiStyle.fontWeightSemiBold : UiStyle.fontWeightMedium
        elide: Text.ElideRight
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        width: parent.width - UiStyle.space8
    }

    MouseArea {
        anchors.fill: parent
        hoverEnabled: true
        enabled: button.enabled
        cursorShape: button.enabled ? Qt.PointingHandCursor : Qt.ArrowCursor
        onEntered: button.hovered = true
        onExited: button.hovered = false
        onClicked: if (button.enabled) button.clicked()
    }
}
