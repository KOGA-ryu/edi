import QtQuick
import QtQuick.Layouts
import "../style"

Rectangle {
    id: row

    property string dataUi: "list_row"
    property string dataState: selected ? "selected" : "idle"
    property string label: "Row"
    property string meta: ""
    property bool selected: false
    property int indent: 0
    property bool clickable: false
    signal clicked()

    implicitHeight: UiStyle.rowHeightCompact
    property bool hovered: false
    color: selected ? UiStyle.colorSelected : hovered && clickable ? UiStyle.colorControlHover : UiStyle.colorTransparent
    radius: UiStyle.radiusSm

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: UiStyle.space8 + row.indent
        anchors.rightMargin: UiStyle.space8
        spacing: UiStyle.space8

        Text {
            Layout.fillWidth: true
            text: row.label
            color: selected ? UiStyle.colorText : UiStyle.colorTextMuted
            font.family: UiStyle.fontSans
            font.pixelSize: UiStyle.fontSizeBody
            font.weight: selected ? UiStyle.fontWeightSemiBold : UiStyle.fontWeightRegular
            elide: Text.ElideRight
        }

        Text {
            visible: row.meta.length > 0
            text: row.meta
            color: UiStyle.colorTextFaint
            font.family: UiStyle.fontSans
            font.pixelSize: UiStyle.fontSizeXs
            elide: Text.ElideRight
        }
    }

    MouseArea {
        anchors.fill: parent
        enabled: row.clickable
        hoverEnabled: row.clickable
        cursorShape: row.clickable ? Qt.PointingHandCursor : Qt.ArrowCursor
        onEntered: row.hovered = true
        onExited: row.hovered = false
        onClicked: row.clicked()
    }
}
