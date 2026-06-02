import QtQuick
import QtQuick.Controls
import "../style"

Rectangle {
    id: action

    property string label: "Action"
    property bool selected: false
    property bool hovered: false
    signal clicked()

    implicitWidth: Math.max(42, labelText.implicitWidth + UiStyle.space12)
    implicitHeight: 22
    radius: UiStyle.radiusSm
    opacity: enabled ? 1.0 : 0.42
    color: selected ? UiStyle.mix(UiStyle.colorBase, UiStyle.colorAccent, 0.22)
        : hovered && enabled ? UiStyle.colorControlHover
        : UiStyle.colorTransparent
    border.width: UiStyle.borderNone

    Text {
        id: labelText
        anchors.centerIn: parent
        width: parent.width - UiStyle.space8
        text: action.label
        color: !action.enabled ? UiStyle.colorDisabled : action.selected ? UiStyle.colorText : UiStyle.colorTextMuted
        font.family: UiStyle.fontSans
        font.pixelSize: UiStyle.fontSizeXs
        font.weight: action.selected ? UiStyle.fontWeightSemiBold : UiStyle.fontWeightMedium
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }

    MouseArea {
        anchors.fill: parent
        hoverEnabled: true
        enabled: action.enabled
        cursorShape: action.enabled ? Qt.PointingHandCursor : Qt.ArrowCursor
        onEntered: action.hovered = true
        onExited: action.hovered = false
        onClicked: action.clicked()
    }
}
