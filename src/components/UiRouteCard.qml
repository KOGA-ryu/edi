import QtQuick
import QtQuick.Layouts
import "../style"

Rectangle {
    id: card
    property string title: "Route"
    property string summary: ""
    property string status: "pending"
    property string meta: ""
    property int noteCount: 0
    signal clicked()

    implicitHeight: 104
    radius: UiStyle.radiusMd
    color: hovered ? UiStyle.colorPanelRaised : UiStyle.colorPanel
    border.width: UiStyle.borderThin
    border.color: hovered ? UiStyle.colorBorderFocus : UiStyle.colorBorderMinor
    property bool hovered: false

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: UiStyle.space10
        spacing: UiStyle.space6
        RowLayout {
            Layout.fillWidth: true
            Text {
                Layout.fillWidth: true
                text: card.title
                color: UiStyle.colorText
                font.family: UiStyle.fontSans
                font.pixelSize: UiStyle.fontSizeTitle
                font.weight: UiStyle.fontWeightSemiBold
                elide: Text.ElideRight
            }
            UiStatusChip { status: card.status }
        }
        Text {
            Layout.fillWidth: true
            Layout.fillHeight: true
            text: card.summary
            color: UiStyle.colorTextMuted
            font.family: UiStyle.fontSans
            font.pixelSize: UiStyle.fontSizeSm
            wrapMode: Text.WordWrap
            elide: Text.ElideRight
        }
        Text {
            Layout.fillWidth: true
            text: card.meta + (card.noteCount > 0 ? "  |  notes " + card.noteCount : "")
            color: UiStyle.colorTextFaint
            font.family: UiStyle.fontMono
            font.pixelSize: UiStyle.fontSizeXs
            elide: Text.ElideRight
        }
    }

    MouseArea {
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onEntered: card.hovered = true
        onExited: card.hovered = false
        onClicked: card.clicked()
    }
}

