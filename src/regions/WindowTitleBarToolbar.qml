import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../style"
import "../components"

RowLayout {
    id: toolbar

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
        return toolbar.controller && toolbar.controller.activityMode === "text_editor"
    }

    function drawingToolActive() {
        return toolbar.controller && toolbar.controller.activityMode === "drawing_tool"
    }

    spacing: 0

    RowLayout {
        Layout.preferredHeight: UiStyle.toolbarHeight
        visible: !toolbar.hostWindow || toolbar.hostWindow.width >= 720
        spacing: UiStyle.space2

        UiMenuButton {
            label: "File"
            visible: toolbar.textEditorActive()
            dynamicActions: toolbar.controller ? toolbar.controller.menuCustomActions("File", toolbar.controller.revision) : []
            onDynamicActionTriggered: function(action) { toolbar.controller.runCustomAction(action.id) }

            Action {
                text: "New Document"
                enabled: toolbar.controller
                onTriggered: toolbar.controller.newTextEditorDocument()
            }
            Action {
                text: "Rename Document"
                enabled: toolbar.controller
                onTriggered: toolbar.controller.renameActiveTextEditorDocument()
            }
            Action {
                text: "Duplicate Document"
                enabled: toolbar.controller
                onTriggered: toolbar.controller.duplicateTextEditorDocument()
            }
            Action {
                text: toolbar.controller && toolbar.controller.textEditorActiveDocumentPinned() ? "Unpin Document" : "Pin Document"
                enabled: toolbar.controller
                onTriggered: toolbar.controller.toggleTextEditorActiveDocumentPin()
            }
            Action {
                text: toolbar.controller ? "Role: " + toolbar.controller.textEditorActiveDocumentRole() : "Role"
                enabled: toolbar.controller
                onTriggered: toolbar.controller.cycleTextEditorActiveDocumentRole()
            }
            Action {
                text: "Close Document"
                enabled: toolbar.controller && toolbar.controller.textEditorDocuments.length > 1
                onTriggered: toolbar.controller.closeActiveTextEditorDocument()
            }
            MenuSeparator {}
            Action {
                text: "Save Document"
                enabled: toolbar.controller
                onTriggered: toolbar.controller.saveTextEditorDocument()
            }
            Action {
                text: "Save All"
                enabled: toolbar.controller
                onTriggered: toolbar.controller.saveAllTextEditorDocuments()
            }
            MenuSeparator {}
            Action {
                text: "Close Window"
                enabled: toolbar.hostWindow
                onTriggered: toolbar.hostWindow.close()
            }
        }

        UiMenuButton {
            label: "File"
            visible: !toolbar.textEditorActive()
            dynamicActions: toolbar.controller ? toolbar.controller.menuCustomActions("File", toolbar.controller.revision) : []
            onDynamicActionTriggered: function(action) { toolbar.controller.runCustomAction(action.id) }

            Action {
                text: "Save Layout"
                enabled: toolbar.controller && toolbar.controller.shellLayoutDirty
                onTriggered: toolbar.controller.saveShellLayout()
            }
            Action {
                text: "Reset Layout"
                enabled: toolbar.controller
                onTriggered: toolbar.controller.resetShellLayout()
            }
            MenuSeparator {}
            Action {
                text: "Close Window"
                enabled: toolbar.hostWindow
                onTriggered: toolbar.hostWindow.close()
            }
        }

        UiMenuButton {
            label: "Edit"
            visible: toolbar.textEditorActive()
            dynamicActions: toolbar.controller ? toolbar.controller.menuCustomActions("Edit", toolbar.controller.revision) : []
            onDynamicActionTriggered: function(action) { toolbar.controller.runCustomAction(action.id) }

            Action {
                text: "Undo"
                enabled: toolbar.controller
                onTriggered: toolbar.controller.requestTextEditorCommand("undo")
            }
            Action {
                text: "Redo"
                enabled: toolbar.controller
                onTriggered: toolbar.controller.requestTextEditorCommand("redo")
            }
            MenuSeparator {}
            Action {
                text: "Select All"
                enabled: toolbar.controller
                onTriggered: toolbar.controller.requestTextEditorCommand("select_all")
            }
            Action {
                text: "Clear"
                enabled: toolbar.controller
                onTriggered: toolbar.controller.requestTextEditorCommand("clear")
            }
            Action {
                text: "Find / Replace"
                enabled: toolbar.controller
                onTriggered: toolbar.controller.requestTextEditorCommand("find_replace")
            }
            MenuSeparator {}
            Action {
                text: toolbar.controller && toolbar.controller.textEditorWrapEnabled ? "Disable Wrap" : "Enable Wrap"
                enabled: toolbar.controller
                onTriggered: toolbar.controller.toggleTextEditorWrap()
            }
            Action {
                text: toolbar.controller && toolbar.controller.textEditorLineNumbersVisible ? "Hide Line Numbers" : "Show Line Numbers"
                enabled: toolbar.controller
                onTriggered: toolbar.controller.toggleTextEditorLineNumbers()
            }
            Action {
                text: toolbar.controller && toolbar.controller.textEditorSplitEnabled ? "Close Split" : "Open Split"
                enabled: toolbar.controller
                onTriggered: toolbar.controller.requestTextEditorCommand("split")
            }
        }

        UiMenuButton {
            label: "Edit"
            visible: toolbar.drawingToolActive()
            dynamicActions: toolbar.controller ? toolbar.controller.menuCustomActions("Edit", toolbar.controller.revision) : []
            onDynamicActionTriggered: function(action) { toolbar.controller.runCustomAction(action.id) }

            Action {
                text: "Undo"
                enabled: toolbar.controller && toolbar.controller.drawingCanUndoCommand
                onTriggered: toolbar.controller.undoDrawingCommand()
            }
            Action {
                text: "Redo"
                enabled: toolbar.controller && toolbar.controller.drawingCanRedoCommand
                onTriggered: toolbar.controller.redoDrawingCommand()
            }
        }

        UiMenuButton {
            label: "Edit"
            visible: !toolbar.textEditorActive() && !toolbar.drawingToolActive()
            dynamicActions: toolbar.controller ? toolbar.controller.menuCustomActions("Edit", toolbar.controller.revision) : []
            onDynamicActionTriggered: function(action) { toolbar.controller.runCustomAction(action.id) }

            Action {
                text: "Mark Pending"
                enabled: toolbar.controller && toolbar.controller.hasReviewSubject
                onTriggered: toolbar.controller.runInspectorAction("status_pending", toolbar.controller.selectedRouteId)
            }
            Action {
                text: "Accept Selection"
                enabled: toolbar.controller && toolbar.controller.hasReviewSubject
                onTriggered: toolbar.controller.runInspectorAction("status_accepted", toolbar.controller.selectedRouteId)
            }
            Action {
                text: "Mark Rework"
                enabled: toolbar.controller && toolbar.controller.hasReviewSubject
                onTriggered: toolbar.controller.runInspectorAction("status_needs_rework", toolbar.controller.selectedRouteId)
            }
            Action {
                text: "Reject Selection"
                enabled: toolbar.controller && toolbar.controller.hasReviewSubject
                onTriggered: toolbar.controller.runInspectorAction("status_rejected", toolbar.controller.selectedRouteId)
            }
        }

        UiMenuButton {
            label: "View"
            dynamicActions: toolbar.controller ? toolbar.controller.menuCustomActions("View", toolbar.controller.revision) : []
            onDynamicActionTriggered: function(action) { toolbar.controller.runCustomAction(action.id) }

            Action {
                text: toolbar.controller && toolbar.controller.panelManualCollapsed("left") ? "Show Left Panel" : "Hide Left Panel"
                enabled: toolbar.controller
                onTriggered: toolbar.controller.toggleLeftPanel()
            }
            Action {
                text: toolbar.controller && toolbar.controller.panelManualCollapsed("right") ? "Show Right Panel" : "Hide Right Panel"
                enabled: toolbar.controller
                onTriggered: toolbar.controller.toggleRightPanel()
            }
            Action {
                text: toolbar.controller && toolbar.controller.panelManualCollapsed("bottom") ? "Show Bottom Panel" : "Hide Bottom Panel"
                enabled: toolbar.controller
                onTriggered: toolbar.controller.toggleBottomPanel()
            }
            Action {
                text: toolbar.controller && toolbar.controller.textEditorStatsEnabled ? "Hide Stats" : "Show Stats"
                enabled: toolbar.controller && toolbar.textEditorActive()
                onTriggered: toolbar.controller.toggleTextEditorStats()
            }
            MenuSeparator {}
            Action {
                text: "Full Layout"
                enabled: toolbar.controller
                onTriggered: toolbar.controller.applyLayoutPreset("full")
            }
            Action {
                text: "Review Layout"
                enabled: toolbar.controller
                onTriggered: toolbar.controller.applyLayoutPreset("review")
            }
            Action {
                text: "Focus Layout"
                enabled: toolbar.controller
                onTriggered: toolbar.controller.applyLayoutPreset("focus")
            }
            Action {
                text: "Tiny Layout"
                enabled: toolbar.controller
                onTriggered: toolbar.controller.applyLayoutPreset("tiny")
            }
        }

        UiMenuButton {
            label: "Tools"
            dynamicActions: toolbar.controller ? toolbar.controller.menuCustomActions("Tools", toolbar.controller.revision) : []
            onDynamicActionTriggered: function(action) { toolbar.controller.runCustomAction(action.id) }

            Action {
                text: "Settings"
                enabled: toolbar.controller && toolbar.controller.hasActivityMode("settings")
                onTriggered: toolbar.controller.setActivityMode("settings")
            }
            Action {
                text: "Theme"
                enabled: toolbar.controller && toolbar.controller.hasActivityMode("settings")
                onTriggered: {
                    toolbar.controller.setActivityMode("settings")
                    toolbar.controller.setSettingsPage("theme")
                }
            }
            Action {
                text: "Panels"
                enabled: toolbar.controller && toolbar.controller.hasActivityMode("settings")
                onTriggered: {
                    toolbar.controller.setActivityMode("settings")
                    toolbar.controller.setSettingsPage("panels")
                }
            }
        }

        UiMenuButton {
            label: "Window"
            dynamicActions: toolbar.controller ? toolbar.controller.menuCustomActions("Window", toolbar.controller.revision) : []
            onDynamicActionTriggered: function(action) { toolbar.controller.runCustomAction(action.id) }

            Action {
                text: "Minimize"
                enabled: toolbar.hostWindow
                onTriggered: toolbar.hostWindow.showMinimized()
            }
            Action {
                text: "Zoom"
                enabled: toolbar.hostWindow
                onTriggered: toolbar.toggleZoom()
            }
        }
    }

    RowLayout {
        spacing: UiStyle.space6
        visible: toolbar.controller && toolbar.controller.activityMode === "review"

        UiIconButton {
            iconText: "<"
            label: "Back"
            tooltip: "Back"
            selected: false
            enabled: toolbar.controller && toolbar.controller.backStack.length > 0
            onClicked: toolbar.controller.goBack()
        }

        UiIconButton {
            iconText: ">"
            label: "Forward"
            tooltip: "Forward"
            selected: false
            enabled: toolbar.controller && toolbar.controller.forwardStack.length > 0
            onClicked: toolbar.controller.goForward()
        }

        UiIconButton {
            iconText: "H"
            label: "Home"
            tooltip: "Home"
            selected: false
            onClicked: toolbar.controller.goHome()
        }
    }

    Item {
        Layout.fillWidth: true
    }

    RowLayout {
        spacing: UiStyle.space6
        visible: !toolbar.hostWindow || toolbar.hostWindow.width >= 560

        UiIconButton {
            label: "S"
            tooltip: "Settings"
            visible: toolbar.controller && toolbar.controller.hasActivityMode("settings")
            selected: toolbar.controller && toolbar.controller.activityMode === "settings"
            onClicked: toolbar.controller.setActivityMode("settings")
        }

        UiIconButton {
            label: "P"
            tooltip: "Proof"
            visible: toolbar.controller && toolbar.controller.hasActivityMode("proof")
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
            panelState: toolbar.controller ? toolbar.controller.panelState("bottom") : "visible"
            tooltip: toolbar.panelTooltip("bottom", "Bottom panel")
            selected: panelState === "visible"
            onClicked: toolbar.controller.toggleBottomPanel()
        }

        UiPanelToggleButton {
            edge: "right"
            panelState: toolbar.controller ? toolbar.controller.panelState("right") : "visible"
            tooltip: toolbar.panelTooltip("right", "Right panel")
            selected: panelState === "visible"
            onClicked: toolbar.controller.toggleRightPanel()
        }
    }
}
