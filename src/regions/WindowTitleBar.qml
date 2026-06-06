import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import "../style"
import "../components"
import "."

Rectangle {
    id: titleBar

    property var controller: null
    property var hostWindow: null

    function toggleZoom() {
        if (!hostWindow) {
            return
        }
        if (hostWindow.visibility === Window.Maximized) {
            hostWindow.showNormal()
        } else {
            hostWindow.showMaximized()
        }
    }

    color: UiStyle.colorPanel
    border.width: UiStyle.borderNone

    Rectangle {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        height: 1
        color: UiStyle.colorPanelAlt
    }

    MouseArea {
        anchors.fill: parent
        acceptedButtons: Qt.LeftButton
        onPressed: {
            if (titleBar.hostWindow && titleBar.hostWindow.startSystemMove) {
                titleBar.hostWindow.startSystemMove()
            }
        }
    }

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: UiStyle.space12
        anchors.rightMargin: UiStyle.space12
        spacing: UiStyle.space10

        RowLayout {
            spacing: UiStyle.space8
            Layout.preferredWidth: 84

            Rectangle {
                Layout.preferredWidth: 14
                Layout.preferredHeight: 14
                radius: 7
                color: "#ff5f57"
                border.color: UiStyle.mix("#ff5f57", "#000000", 0.22)
                border.width: UiStyle.borderThin
                MouseArea {
                    anchors.fill: parent
                    onClicked: titleBar.hostWindow.close()
                }
            }

            Rectangle {
                Layout.preferredWidth: 14
                Layout.preferredHeight: 14
                radius: 7
                color: "#ffbd2e"
                border.color: UiStyle.mix("#ffbd2e", "#000000", 0.24)
                border.width: UiStyle.borderThin
                MouseArea {
                    anchors.fill: parent
                    onClicked: titleBar.hostWindow.showMinimized()
                }
            }

            Rectangle {
                Layout.preferredWidth: 14
                Layout.preferredHeight: 14
                radius: 7
                color: "#28c840"
                border.color: UiStyle.mix("#28c840", "#000000", 0.24)
                border.width: UiStyle.borderThin
                MouseArea {
                    anchors.fill: parent
                    onClicked: titleBar.toggleZoom()
                }
            }
        }

        RowLayout {
            spacing: UiStyle.space8

            UiPanelToggleButton {
                edge: "left"
                panelState: titleBar.controller ? titleBar.controller.panelState("left") : "visible"
                tooltip: titleBar.controller ? (function() {
                    var state = titleBar.controller.panelState("left")
                    if (state === "auto_hidden") {
                        return "Left panel is auto-hidden by window size"
                    }
                    if (state === "collapsed") {
                        return "Show left panel"
                    }
                    return "Collapse left panel"
                })() : "Left panel"
                selected: panelState === "visible"
                onClicked: titleBar.controller.toggleLeftPanel()
            }
        }

        WindowTitleBarToolbar {
            controller: titleBar.controller
            hostWindow: titleBar.hostWindow
            Layout.fillWidth: true
        }
    }
}
