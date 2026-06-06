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
    border.width: UiStyle.borderNone

    Rectangle {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        height: 1
        color: UiStyle.colorPanelAlt
    }

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

    function panelTooltip(panelId, label) {
        if (!controller) {
            return label
        }
        var state = controller.panelState(panelId)
        if (state === "auto_hidden") {
            return label + " is auto-hidden by window size"
        }
        if (state === "collapsed") {
            return "Show " + label.toLowerCase()
        }
        return "Collapse " + label.toLowerCase()
    }

    function textEditorActive() {
        return titleBar.controller && titleBar.controller.activityMode === "text_editor"
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
                panelState: titleBar.controller ? titleBar.controller.panelState("left") : "visible"
                tooltip: titleBar.panelTooltip("left", "Left panel")
                selected: panelState === "visible"
                onClicked: titleBar.controller.toggleLeftPanel()
            }
        }

        RowLayout {
            Layout.preferredHeight: UiStyle.toolbarHeight
            visible: !titleBar.hostWindow || titleBar.hostWindow.width >= 720
            spacing: UiStyle.space2

            UiMenuButton {
                label: "File"
                visible: titleBar.textEditorActive()

                Action {
                    text: "New Document"
                    enabled: titleBar.controller
                    onTriggered: titleBar.controller.newTextEditorDocument()
                }
                Action {
                    text: "Rename Document"
                    enabled: titleBar.controller
                    onTriggered: titleBar.controller.renameActiveTextEditorDocument()
                }
                Action {
                    text: "Duplicate Document"
                    enabled: titleBar.controller
                    onTriggered: titleBar.controller.duplicateTextEditorDocument()
                }
                Action {
                    text: "Close Document"
                    enabled: titleBar.controller && titleBar.controller.textEditorDocuments.length > 1
                    onTriggered: titleBar.controller.closeActiveTextEditorDocument()
                }
                MenuSeparator {}
                Action {
                    text: "Save Document"
                    enabled: titleBar.controller
                    onTriggered: titleBar.controller.saveTextEditorDocument()
                }
                Action {
                    text: "Save All"
                    enabled: titleBar.controller
                    onTriggered: titleBar.controller.saveAllTextEditorDocuments()
                }
                MenuSeparator {}
                Action {
                    text: "Close Window"
                    enabled: titleBar.hostWindow
                    onTriggered: titleBar.hostWindow.close()
                }
            }

            UiMenuButton {
                label: "File"
                visible: !titleBar.textEditorActive()

                Action {
                    text: "Save Layout"
                    enabled: titleBar.controller && titleBar.controller.shellLayoutDirty
                    onTriggered: titleBar.controller.saveShellLayout()
                }
                Action {
                    text: "Reset Layout"
                    enabled: titleBar.controller
                    onTriggered: titleBar.controller.resetShellLayout()
                }
                MenuSeparator {}
                Action {
                    text: "Close Window"
                    enabled: titleBar.hostWindow
                    onTriggered: titleBar.hostWindow.close()
                }
            }

            UiMenuButton {
                label: "Edit"
                visible: titleBar.textEditorActive()

                Action {
                    text: "Undo"
                    enabled: titleBar.controller
                    onTriggered: titleBar.controller.requestTextEditorCommand("undo")
                }
                Action {
                    text: "Redo"
                    enabled: titleBar.controller
                    onTriggered: titleBar.controller.requestTextEditorCommand("redo")
                }
                MenuSeparator {}
                Action {
                    text: "Select All"
                    enabled: titleBar.controller
                    onTriggered: titleBar.controller.requestTextEditorCommand("select_all")
                }
                Action {
                    text: "Clear"
                    enabled: titleBar.controller
                    onTriggered: titleBar.controller.requestTextEditorCommand("clear")
                }
                Action {
                    text: "Find"
                    enabled: titleBar.controller
                    onTriggered: titleBar.controller.requestTextEditorCommand("find")
                }
                MenuSeparator {}
                Action {
                    text: titleBar.controller && titleBar.controller.textEditorWrapEnabled ? "Disable Wrap" : "Enable Wrap"
                    enabled: titleBar.controller
                    onTriggered: titleBar.controller.toggleTextEditorWrap()
                }
                Action {
                    text: titleBar.controller && titleBar.controller.textEditorLineNumbersVisible ? "Hide Line Numbers" : "Show Line Numbers"
                    enabled: titleBar.controller
                    onTriggered: titleBar.controller.toggleTextEditorLineNumbers()
                }
            }

            UiMenuButton {
                label: "Edit"
                visible: !titleBar.textEditorActive()

                Action {
                    text: "Mark Pending"
                    enabled: titleBar.controller && titleBar.controller.hasReviewSubject
                    onTriggered: titleBar.controller.runInspectorAction("status_pending", titleBar.controller.selectedRouteId)
                }
                Action {
                    text: "Accept Selection"
                    enabled: titleBar.controller && titleBar.controller.hasReviewSubject
                    onTriggered: titleBar.controller.runInspectorAction("status_accepted", titleBar.controller.selectedRouteId)
                }
                Action {
                    text: "Mark Rework"
                    enabled: titleBar.controller && titleBar.controller.hasReviewSubject
                    onTriggered: titleBar.controller.runInspectorAction("status_needs_rework", titleBar.controller.selectedRouteId)
                }
                Action {
                    text: "Reject Selection"
                    enabled: titleBar.controller && titleBar.controller.hasReviewSubject
                    onTriggered: titleBar.controller.runInspectorAction("status_rejected", titleBar.controller.selectedRouteId)
                }
            }

            UiMenuButton {
                label: "View"

                Action {
                    text: titleBar.controller && titleBar.controller.panelManualCollapsed("left") ? "Show Left Panel" : "Hide Left Panel"
                    enabled: titleBar.controller
                    onTriggered: titleBar.controller.toggleLeftPanel()
                }
                Action {
                    text: titleBar.controller && titleBar.controller.panelManualCollapsed("right") ? "Show Right Panel" : "Hide Right Panel"
                    enabled: titleBar.controller
                    onTriggered: titleBar.controller.toggleRightPanel()
                }
                Action {
                    text: titleBar.controller && titleBar.controller.panelManualCollapsed("bottom") ? "Show Bottom Panel" : "Hide Bottom Panel"
                    enabled: titleBar.controller
                    onTriggered: titleBar.controller.toggleBottomPanel()
                }
                MenuSeparator {}
                Action {
                    text: "Full Layout"
                    enabled: titleBar.controller
                    onTriggered: titleBar.controller.applyLayoutPreset("full")
                }
                Action {
                    text: "Review Layout"
                    enabled: titleBar.controller
                    onTriggered: titleBar.controller.applyLayoutPreset("review")
                }
                Action {
                    text: "Focus Layout"
                    enabled: titleBar.controller
                    onTriggered: titleBar.controller.applyLayoutPreset("focus")
                }
                Action {
                    text: "Tiny Layout"
                    enabled: titleBar.controller
                    onTriggered: titleBar.controller.applyLayoutPreset("tiny")
                }
            }

            UiMenuButton {
                label: "Tools"

                Action {
                    text: "Settings"
                    enabled: titleBar.controller && titleBar.controller.hasActivityMode("settings")
                    onTriggered: titleBar.controller.setActivityMode("settings")
                }
                Action {
                    text: "Theme"
                    enabled: titleBar.controller && titleBar.controller.hasActivityMode("settings")
                    onTriggered: {
                        titleBar.controller.setActivityMode("settings")
                        titleBar.controller.setSettingsPage("theme")
                    }
                }
                Action {
                    text: "Panels"
                    enabled: titleBar.controller && titleBar.controller.hasActivityMode("settings")
                    onTriggered: {
                        titleBar.controller.setActivityMode("settings")
                        titleBar.controller.setSettingsPage("panels")
                    }
                }
            }

            UiMenuButton {
                label: "Window"

                Action {
                    text: "Minimize"
                    enabled: titleBar.hostWindow
                    onTriggered: titleBar.hostWindow.showMinimized()
                }
                Action {
                    text: "Zoom"
                    enabled: titleBar.hostWindow
                    onTriggered: titleBar.toggleZoom()
                }
            }
        }

        RowLayout {
            spacing: UiStyle.space6
            visible: titleBar.controller && titleBar.controller.activityMode === "review"

            UiIconButton {
                iconText: "<"
                label: "Back"
                tooltip: "Back"
                selected: false
                enabled: titleBar.controller && titleBar.controller.backStack.length > 0
                onClicked: titleBar.controller.goBack()
            }

            UiIconButton {
                iconText: ">"
                label: "Forward"
                tooltip: "Forward"
                selected: false
                enabled: titleBar.controller && titleBar.controller.forwardStack.length > 0
                onClicked: titleBar.controller.goForward()
            }

            UiIconButton {
                iconText: "H"
                label: "Home"
                tooltip: "Home"
                selected: false
                onClicked: titleBar.controller.goHome()
            }
        }

        Item {
            Layout.fillWidth: true
        }

        RowLayout {
            spacing: UiStyle.space6
            visible: !titleBar.hostWindow || titleBar.hostWindow.width >= 560

            UiIconButton {
                label: "S"
                tooltip: "Settings"
                visible: titleBar.controller && titleBar.controller.hasActivityMode("settings")
                selected: titleBar.controller && titleBar.controller.activityMode === "settings"
                onClicked: titleBar.controller.setActivityMode("settings")
            }

            UiIconButton {
                label: "P"
                tooltip: "Proof"
                visible: titleBar.controller && titleBar.controller.hasActivityMode("proof")
                enabled: false
            }

            UiIconButton {
                label: "..."
                tooltip: "More"
                visible: false
                enabled: false
            }
        }

        RowLayout {
            spacing: UiStyle.space6

            UiPanelToggleButton {
                edge: "bottom"
                panelState: titleBar.controller ? titleBar.controller.panelState("bottom") : "visible"
                tooltip: titleBar.panelTooltip("bottom", "Bottom panel")
                selected: panelState === "visible"
                onClicked: titleBar.controller.toggleBottomPanel()
            }

            UiPanelToggleButton {
                edge: "right"
                panelState: titleBar.controller ? titleBar.controller.panelState("right") : "visible"
                tooltip: titleBar.panelTooltip("right", "Right panel")
                selected: panelState === "visible"
                onClicked: titleBar.controller.toggleRightPanel()
            }
        }
    }
}
