import QtQuick
import QtQuick.Layouts
import "../style"

Item {
    id: row

    property string label: ""
    property string value: ""
    property int maxValueLines: 3

    implicitHeight: content.implicitHeight

    ColumnLayout {
        id: content
        anchors.left: parent.left
        anchors.right: parent.right
        spacing: UiStyle.space0

        Text {
            Layout.fillWidth: true
            text: row.label
            color: UiStyle.colorTextFaint
            font.family: UiStyle.fontSans
            font.pixelSize: UiStyle.fontSizeXs
            elide: Text.ElideRight
        }

        Text {
            Layout.fillWidth: true
            visible: row.value.length > 0
            text: row.value
            color: UiStyle.colorTextMuted
            font.family: UiStyle.fontSans
            font.pixelSize: UiStyle.fontSizeXs
            wrapMode: Text.WordWrap
            maximumLineCount: row.maxValueLines
            elide: Text.ElideRight
        }
    }
}
