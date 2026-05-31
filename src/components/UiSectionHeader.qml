import QtQuick
import "../style"

Item {
    id: header

    property string dataUi: "section_header"
    property string title: "Section"

    implicitHeight: UiStyle.sectionHeaderHeight

    Text {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.verticalCenter: parent.verticalCenter
        text: header.title.toUpperCase()
        color: UiStyle.colorTextFaint
        font.family: UiStyle.fontSans
        font.pixelSize: UiStyle.fontSizeXs
        font.weight: UiStyle.fontWeightSemiBold
        elide: Text.ElideRight
    }
}
