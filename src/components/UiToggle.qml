import QtQuick
import QtQuick.Layouts
import "../style"

Rectangle {
    id: toggle
    property string label: "Toggle"
    property bool checked: false
    signal toggled(bool checked)

    implicitHeight: UiStyle.rowHeight
    implicitWidth: 132
    radius: UiStyle.radiusSm
    color: checked ? UiStyle.colorSelected : UiStyle.colorControl
    border.width: UiStyle.borderNone

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: UiStyle.space8
        anchors.rightMargin: UiStyle.space8
        spacing: UiStyle.space8
        Text {
            Layout.fillWidth: true
            text: toggle.label
            color: UiStyle.colorText
            font.family: UiStyle.fontSans
            font.pixelSize: UiStyle.fontSizeSm
            elide: Text.ElideRight
        }
        Rectangle {
            width: 28
            height: 14
            radius: 7
            color: checked ? UiStyle.colorAccentSoft : UiStyle.colorPanelRaised
            Rectangle {
                width: 10
                height: 10
                radius: 5
                y: 2
                x: checked ? 16 : 2
                color: UiStyle.colorText
            }
        }
    }

    MouseArea {
        anchors.fill: parent
        cursorShape: Qt.PointingHandCursor
        onClicked: {
            toggle.checked = !toggle.checked
            toggle.toggled(toggle.checked)
        }
    }
}
