import QtQuick
import QtQuick.Layouts
import "../style"

Rectangle {
    id: row
    property string path: ""
    property string role: "code"

    implicitHeight: UiStyle.rowHeight
    color: UiStyle.colorControl
    radius: UiStyle.radiusSm
    border.width: UiStyle.borderThin
    border.color: UiStyle.colorBorderMinor

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: UiStyle.space8
        anchors.rightMargin: UiStyle.space8
        spacing: UiStyle.space8
        Text {
            Layout.fillWidth: true
            text: row.path
            color: UiStyle.colorTextMuted
            font.family: UiStyle.fontMono
            font.pixelSize: UiStyle.fontSizeXs
            elide: Text.ElideMiddle
        }
        Text {
            text: row.role
            color: UiStyle.colorTextFaint
            font.family: UiStyle.fontSans
            font.pixelSize: UiStyle.fontSizeXs
        }
    }
}

