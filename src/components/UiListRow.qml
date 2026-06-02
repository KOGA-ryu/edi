import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../style"

Rectangle {
    id: row

    property string dataUi: "list_row"
    property string dataState: selected ? "selected" : "idle"
    property string label: "Row"
    property string meta: ""
    property string tooltip: meta.length > 0 ? label + " - " + meta : label
    property bool selected: false
    property int indent: 0
    property int hideMetaBelowWidth: 0
    property int metaMaxWidth: 92
    property bool clickable: false
    signal clicked()

    implicitHeight: 24
    property bool hovered: false
    color: selected ? UiStyle.mix(UiStyle.colorPanel, UiStyle.colorAccent, 0.14)
        : hovered && clickable ? UiStyle.colorControlHover
        : UiStyle.colorTransparent
    radius: UiStyle.radiusSm

    Rectangle {
        visible: row.selected
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        width: 2
        radius: 1
        color: UiStyle.colorAccent
    }

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: UiStyle.space6 + row.indent
        anchors.rightMargin: UiStyle.space6
        spacing: UiStyle.space6

        Text {
            id: labelText
            Layout.fillWidth: true
            text: row.label
            color: selected ? UiStyle.colorText : UiStyle.colorTextMuted
            font.family: UiStyle.fontSans
            font.pixelSize: UiStyle.fontSizeSm
            font.weight: selected ? UiStyle.fontWeightSemiBold : UiStyle.fontWeightRegular
            elide: Text.ElideRight
            verticalAlignment: Text.AlignVCenter
        }

        Text {
            id: metaText
            visible: row.meta.length > 0 && (row.hideMetaBelowWidth <= 0 || row.width >= row.hideMetaBelowWidth)
            Layout.preferredWidth: Math.min(Math.max(42, implicitWidth), row.metaMaxWidth)
            Layout.maximumWidth: row.metaMaxWidth
            text: row.meta
            color: UiStyle.colorTextFaint
            font.family: UiStyle.fontSans
            font.pixelSize: UiStyle.fontSizeXs
            horizontalAlignment: Text.AlignRight
            verticalAlignment: Text.AlignVCenter
            elide: Text.ElideRight
        }
    }

    ToolTip.visible: row.hovered && row.tooltip.length > 0 && (labelText.truncated || metaText.truncated || row.clickable)
    ToolTip.text: row.tooltip
    ToolTip.delay: 2400
    ToolTip.timeout: 8000

    MouseArea {
        anchors.fill: parent
        enabled: row.clickable || row.tooltip.length > 0
        hoverEnabled: row.clickable || row.tooltip.length > 0
        cursorShape: row.clickable ? Qt.PointingHandCursor : Qt.ArrowCursor
        onEntered: row.hovered = true
        onExited: row.hovered = false
        onClicked: if (row.clickable) row.clicked()
    }
}
