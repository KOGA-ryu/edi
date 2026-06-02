import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../style"
import "../components"

Rectangle {
    id: titleBar

    property var controller: null
    property var hostWindow: null

    color: UiStyle.colorPanel
    border.color: UiStyle.colorBorderMajor
    border.width: UiStyle.borderThin

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
            spacing: UiStyle.space6

            UiPanelToggleButton {
                edge: "left"
                tooltip: titleBar.controller && titleBar.controller.leftPanelCollapsed ? "Show left panel" : "Collapse left panel"
                selected: titleBar.controller && !titleBar.controller.leftPanelCollapsed
                onClicked: titleBar.controller.toggleLeftPanel()
            }

            UiPanelToggleButton {
                edge: "bottom"
                tooltip: titleBar.controller && titleBar.controller.bottomPanelCollapsed ? "Show bottom panel" : "Collapse bottom panel"
                selected: titleBar.controller && !titleBar.controller.bottomPanelCollapsed
                onClicked: titleBar.controller.toggleBottomPanel()
            }

            UiPanelToggleButton {
                edge: "right"
                tooltip: titleBar.controller && titleBar.controller.rightPanelCollapsed ? "Show right panel" : "Collapse right panel"
                selected: titleBar.controller && !titleBar.controller.rightPanelCollapsed
                onClicked: titleBar.controller.toggleRightPanel()
            }
        }

        Text {
            Layout.fillWidth: true
            visible: !titleBar.hostWindow || titleBar.hostWindow.width >= 420
            text: titleBar.controller ? titleBar.controller.selectedSubjectLabel + " / " + titleBar.controller.currentRoute().label : "Draftsman"
            color: UiStyle.colorText
            font.family: UiStyle.fontSans
            font.pixelSize: UiStyle.fontSizeTitle
            font.weight: UiStyle.fontWeightSemiBold
            elide: Text.ElideRight
            horizontalAlignment: Text.AlignHCenter

            MouseArea {
                anchors.fill: parent
                acceptedButtons: Qt.LeftButton
                onDoubleClicked: titleBar.toggleZoom()
                onPressed: {
                    if (titleBar.hostWindow && titleBar.hostWindow.startSystemMove) {
                        titleBar.hostWindow.startSystemMove()
                    }
                }
            }
        }

        RowLayout {
            spacing: UiStyle.space6
            visible: !titleBar.hostWindow || titleBar.hostWindow.width >= 560
            Layout.preferredWidth: visible ? 126 : 0

            UiIconButton {
                label: "S"
                tooltip: "Settings"
                selected: titleBar.controller && titleBar.controller.activityMode === "settings"
                onClicked: titleBar.controller.setActivityMode("settings")
            }

            UiIconButton {
                label: "P"
                tooltip: "Proof"
                enabled: false
            }

            UiIconButton {
                label: "..."
                tooltip: "More"
                enabled: false
            }
        }
    }
}
