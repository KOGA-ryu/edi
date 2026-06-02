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

    implicitHeight: 84
    radius: UiStyle.radiusSm
    color: hovered ? UiStyle.colorPanelRaised : UiStyle.colorPanel
    border.width: UiStyle.borderNone
    property bool hovered: false

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: UiStyle.space8
        spacing: UiStyle.space4
        RowLayout {
            Layout.fillWidth: true
            Text {
                Layout.fillWidth: true
                text: card.title
                color: UiStyle.colorText
                font.family: UiStyle.fontSans
                font.pixelSize: UiStyle.fontSizeBody
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
            maximumLineCount: 2
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
