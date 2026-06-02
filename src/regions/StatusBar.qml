import QtQuick
import QtQuick.Layouts
import "../style"

Rectangle {
    id: statusBar
    property var controller: null

    color: UiStyle.colorStatusBar
    border.width: UiStyle.borderNone

    Rectangle {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        height: 1
        color: UiStyle.colorPanelAlt
    }

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: UiStyle.space10
        anchors.rightMargin: UiStyle.space10
        spacing: UiStyle.space12

        Text {
            text: "mode: " + (statusBar.controller ? statusBar.controller.activityMode : "unknown")
            color: UiStyle.colorTextMuted
            font.family: UiStyle.fontMono
            font.pixelSize: UiStyle.fontSizeXs
        }
        Text {
            Layout.fillWidth: true
            text: statusBar.controller ? statusBar.controller.breadcrumbText(statusBar.controller.selectedRouteId) : ""
            color: UiStyle.colorText
            font.family: UiStyle.fontSans
            font.pixelSize: UiStyle.fontSizeXs
            elide: Text.ElideRight
        }
        Text {
            text: "notes: " + (statusBar.controller ? statusBar.controller.allNotes(statusBar.controller.revision).length : 0)
            color: UiStyle.colorTextMuted
            font.family: UiStyle.fontMono
            font.pixelSize: UiStyle.fontSizeXs
        }
        Text {
            text: statusBar.controller && statusBar.controller.writeDisabled ? "writes disabled" : "writes enabled"
            color: statusBar.controller && statusBar.controller.writeDisabled ? UiStyle.colorWarning : UiStyle.colorSuccess
            font.family: UiStyle.fontMono
            font.pixelSize: UiStyle.fontSizeXs
        }
    }
}
