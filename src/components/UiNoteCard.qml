import QtQuick
import QtQuick.Layouts
import "../style"

Rectangle {
    id: card
    property string status: "pending"
    property string body: ""
    property string createdAt: ""

    implicitHeight: Math.max(84, content.implicitHeight + UiStyle.space20)
    color: UiStyle.colorPanel
    radius: UiStyle.radiusMd
    border.width: UiStyle.borderThin
    border.color: UiStyle.colorBorderMinor

    ColumnLayout {
        id: content
        anchors.fill: parent
        anchors.margins: UiStyle.space10
        spacing: UiStyle.space6
        RowLayout {
            Layout.fillWidth: true
            UiStatusChip { status: card.status }
            Item { Layout.fillWidth: true }
            Text {
                text: card.createdAt
                color: UiStyle.colorTextFaint
                font.family: UiStyle.fontSans
                font.pixelSize: UiStyle.fontSizeXs
            }
        }
        Text {
            Layout.fillWidth: true
            text: card.body
            color: UiStyle.colorTextMuted
            font.family: UiStyle.fontSans
            font.pixelSize: UiStyle.fontSizeSm
            wrapMode: Text.WordWrap
        }
    }
}

