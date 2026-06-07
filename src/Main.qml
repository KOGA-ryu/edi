// GENERATED-SAFE SOURCE FILE.
// Human-editable shell composer.
// Main.qml composes major regions only. Region internals live in regions/*.qml.
// Shared styling lives in style/UiStyle.qml.

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import "style"
import "regions"
import "runtime"

ApplicationWindow {
    id: window

    title: runtimeController.projectTitle
    flags: Qt.Window | Qt.FramelessWindowHint
    width: initialWindowDimension("width", UiStyle.windowWidth, 520, 2400)
    height: initialWindowDimension("height", UiStyle.windowHeight, 420, 1800)
    visible: true

    property string dataUi: "window"
    property string dataState: "active"
    property string surfaceRecipeId: "shell_surface"
    property int dragStartX: 0
    property int dragStartY: 0
    property int dragStartSize: 0
    property bool closeDiscardConfirmed: false
    property bool effectiveLeftPanelVisible: runtimeController.panelVisible("left")
    property bool effectiveRightPanelVisible: runtimeController.panelVisible("right")
    property bool effectiveBottomPanelVisible: runtimeController.panelVisible("bottom")

    function initialWindowDimension(key, fallback, low, high) {
        var document = typeof initialShellLayout === "undefined" ? ({}) : initialShellLayout
        var settings = document && document.window ? document.window : ({})
        var value = Number(settings[key] || fallback)
        return Math.max(low, Math.min(high, Math.round(value)))
    }

    RuntimeController {
        id: runtimeController
        objectName: "runtimeController"
        targetRoot: window
    }

    onClosing: function(close) {
        if (window.closeDiscardConfirmed) {
            return
        }
        if (runtimeController.activityMode === "drawing_tool" && runtimeController.drawingDocumentDirty) {
            close.accepted = false
            discardCloseDialog.open()
        }
    }

    Dialog {
        id: discardCloseDialog
        title: "Discard unsaved drawing?"
        modal: true
        anchors.centerIn: parent
        width: 360
        standardButtons: Dialog.Cancel | Dialog.Discard

        Text {
            text: "The current drawing has unsaved changes."
            color: UiStyle.colorText
            font.family: UiStyle.fontSans
            font.pixelSize: UiStyle.fontSizeSm
            width: 300
            wrapMode: Text.WordWrap
        }

        onDiscarded: {
            window.closeDiscardConfirmed = true
            window.close()
        }
    }

    background: Rectangle {
        color: UiStyle.colorWindow
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        WindowTitleBar {
            id: windowTitleBar
            controller: runtimeController
            hostWindow: window
            Layout.fillWidth: true
            Layout.preferredHeight: UiStyle.titleBarHeight
        }

        RowLayout {
            id: shellBody
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 0

            ActivityRail {
                id: activityRail
                controller: runtimeController
                Layout.preferredWidth: UiStyle.railWidth
                Layout.fillHeight: true
            }

            Item {
                id: contentOverlayHost
                Layout.fillWidth: true
                Layout.fillHeight: true

                MainWorkspace {
                    id: mainWorkspace
                    controller: runtimeController
                    anchors.fill: parent
                    z: 0
                }

                LeftPanel {
                    id: leftPanel
                    controller: runtimeController
                    visible: window.effectiveLeftPanelVisible
                    width: visible ? runtimeController.leftPanelWidth : 0
                    anchors.left: parent.left
                    anchors.top: parent.top
                    anchors.bottom: parent.bottom
                    z: 10
                }

                Rectangle {
                    id: leftResizeHandle
                    visible: window.effectiveLeftPanelVisible
                    width: visible ? UiStyle.splitterHitSize : 0
                    anchors.left: leftPanel.right
                    anchors.top: parent.top
                    anchors.bottom: parent.bottom
                    z: 11
                    color: UiStyle.colorTransparent

                    Rectangle {
                        anchors.horizontalCenter: parent.horizontalCenter
                        anchors.top: parent.top
                        anchors.bottom: parent.bottom
                        width: UiStyle.splitterLineSize
                        color: leftResizeMouse.containsMouse || leftResizeMouse.dragging ? UiStyle.colorAccentSoft : UiStyle.colorBorderMajor
                        opacity: leftResizeMouse.containsMouse || leftResizeMouse.dragging ? 0.9 : 0.55
                    }

                    MouseArea {
                        id: leftResizeMouse
                        property bool dragging: false
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.SplitHCursor
                        onPressed: function(mouse) {
                            dragging = true
                            window.dragStartX = leftResizeHandle.mapToItem(null, mouse.x, mouse.y).x
                            window.dragStartSize = runtimeController.leftPanelWidth
                        }
                        onReleased: dragging = false
                        onCanceled: dragging = false
                        onPositionChanged: function(mouse) {
                            if (!dragging) {
                                return
                            }
                            var currentX = leftResizeHandle.mapToItem(null, mouse.x, mouse.y).x
                            runtimeController.setLeftPanelWidth(window.dragStartSize + currentX - window.dragStartX)
                        }
                    }
                }

                Rectangle {
                    id: rightResizeHandle
                    visible: window.effectiveRightPanelVisible
                    width: visible ? UiStyle.splitterHitSize : 0
                    anchors.top: parent.top
                    anchors.bottom: parent.bottom
                    anchors.right: rightPanel.left
                    z: 11
                    color: UiStyle.colorTransparent

                    Rectangle {
                        anchors.horizontalCenter: parent.horizontalCenter
                        anchors.top: parent.top
                        anchors.bottom: parent.bottom
                        width: UiStyle.splitterLineSize
                        color: rightResizeMouse.containsMouse || rightResizeMouse.dragging ? UiStyle.colorAccentSoft : UiStyle.colorPanelRaised
                        opacity: rightResizeMouse.containsMouse || rightResizeMouse.dragging ? 0.9 : 0.55
                    }

                    MouseArea {
                        id: rightResizeMouse
                        property bool dragging: false
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.SplitHCursor
                        onPressed: function(mouse) {
                            dragging = true
                            window.dragStartX = rightResizeHandle.mapToItem(null, mouse.x, mouse.y).x
                            window.dragStartSize = runtimeController.rightPanelWidth
                        }
                        onReleased: dragging = false
                        onCanceled: dragging = false
                        onPositionChanged: function(mouse) {
                            if (!dragging) {
                                return
                            }
                            var currentX = rightResizeHandle.mapToItem(null, mouse.x, mouse.y).x
                            runtimeController.setRightPanelWidth(window.dragStartSize - (currentX - window.dragStartX))
                        }
                    }
                }

                RightPanel {
                    id: rightPanel
                    controller: runtimeController
                    visible: window.effectiveRightPanelVisible
                    width: visible ? runtimeController.rightPanelWidth : 0
                    anchors.top: parent.top
                    anchors.right: parent.right
                    anchors.bottom: parent.bottom
                    z: 10
                }

                Rectangle {
                    id: bottomResizeHandle
                    visible: window.effectiveBottomPanelVisible
                    height: visible ? UiStyle.splitterHitSize : 0
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.bottom: bottomPanel.top
                    z: 21
                    color: UiStyle.colorTransparent

                    Rectangle {
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.left: parent.left
                        anchors.right: parent.right
                        height: UiStyle.splitterLineSize
                        color: bottomResizeMouse.containsMouse || bottomResizeMouse.dragging ? UiStyle.colorAccentSoft : UiStyle.colorPanelRaised
                        opacity: bottomResizeMouse.containsMouse || bottomResizeMouse.dragging ? 0.9 : 0.55
                    }

                    MouseArea {
                        id: bottomResizeMouse
                        property bool dragging: false
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.SplitVCursor
                        onPressed: function(mouse) {
                            dragging = true
                            window.dragStartY = bottomResizeHandle.mapToItem(null, mouse.x, mouse.y).y
                            window.dragStartSize = runtimeController.bottomPanelHeight
                        }
                        onReleased: dragging = false
                        onCanceled: dragging = false
                        onPositionChanged: function(mouse) {
                            if (!dragging) {
                                return
                            }
                            var currentY = bottomResizeHandle.mapToItem(null, mouse.x, mouse.y).y
                            runtimeController.setBottomPanelHeight(window.dragStartSize - (currentY - window.dragStartY))
                        }
                    }
                }

                BottomPanel {
                    id: bottomPanel
                    controller: runtimeController
                    visible: window.effectiveBottomPanelVisible
                    height: visible ? runtimeController.bottomPanelHeight : 0
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.bottom: parent.bottom
                    z: 20
                }
            }
        }

        StatusBar {
            id: statusBar
            controller: runtimeController
            Layout.fillWidth: true
            Layout.preferredHeight: UiStyle.statusBarHeight
        }
    }
}
