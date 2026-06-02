import QtQuick
import QtQuick.Layouts
import "../style"

Rectangle {
    id: row
    property string path: ""
    property string role: "code"
    property bool hovered: false

    implicitHeight: 22
    color: hovered ? UiStyle.colorControlHover : UiStyle.colorTransparent
    radius: UiStyle.radiusSm
    border.width: UiStyle.borderNone

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: UiStyle.space6
        anchors.rightMargin: UiStyle.space6
        spacing: UiStyle.space6
        Text {
            Layout.fillWidth: true
            text: row.path
            color: UiStyle.colorTextMuted
            font.family: UiStyle.fontMono
            font.pixelSize: UiStyle.fontSizeXs
            elide: Text.ElideMiddle
        }
        Text {
            Layout.maximumWidth: 44
            text: row.role
            color: UiStyle.colorTextFaint
            font.family: UiStyle.fontSans
            font.pixelSize: UiStyle.fontSizeXs
            elide: Text.ElideRight
        }
    }

    MouseArea {
        anchors.fill: parent
        hoverEnabled: true
        onEntered: row.hovered = true
        onExited: row.hovered = false
    }
}
