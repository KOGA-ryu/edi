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

    implicitHeight: 58
    radius: UiStyle.radiusSm
    color: hovered ? UiStyle.colorControlHover : UiStyle.colorTransparent
    border.width: UiStyle.borderNone
    property bool hovered: false

    ColumnLayout {
        anchors.fill: parent
        anchors.leftMargin: UiStyle.space6
        anchors.rightMargin: UiStyle.space6
        anchors.topMargin: UiStyle.space4
        anchors.bottomMargin: UiStyle.space4
        spacing: UiStyle.space2
        RowLayout {
            Layout.fillWidth: true
            Text {
                Layout.fillWidth: true
                text: card.title
                color: UiStyle.colorText
                font.family: UiStyle.fontSans
                font.pixelSize: UiStyle.fontSizeSm
                font.weight: UiStyle.fontWeightSemiBold
                elide: Text.ElideRight
            }
            UiStatusChip { status: card.status; compact: true }
        }
        Text {
            Layout.fillWidth: true
            text: card.summary
            color: UiStyle.colorTextMuted
            font.family: UiStyle.fontSans
            font.pixelSize: UiStyle.fontSizeXs
            wrapMode: Text.WordWrap
            maximumLineCount: 1
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
