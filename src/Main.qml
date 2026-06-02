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

    title: "Editable UI Shell"
    flags: Qt.Window | Qt.FramelessWindowHint
    width: UiStyle.windowWidth
    height: UiStyle.windowHeight
    visible: true

    property string dataUi: "window"
    property string dataState: "active"
    property string surfaceRecipeId: "shell_surface"
    property int dragStartX: 0
    property int dragStartY: 0
    property int dragStartSize: 0
    property bool effectiveLeftPanelVisible: runtimeController.panelVisible("left")
    property bool effectiveRightPanelVisible: runtimeController.panelVisible("right")
    property bool effectiveBottomPanelVisible: runtimeController.panelVisible("bottom")

    RuntimeController {
        id: runtimeController
        objectName: "runtimeController"
        targetRoot: window
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

            LeftPanel {
                id: leftPanel
                controller: runtimeController
                visible: window.effectiveLeftPanelVisible
                Layout.preferredWidth: visible ? runtimeController.leftPanelWidth : 0
                Layout.fillHeight: true
            }

            Rectangle {
                id: leftResizeHandle
                visible: window.effectiveLeftPanelVisible
                Layout.preferredWidth: visible ? 6 : 0
                Layout.fillHeight: true
                color: UiStyle.colorTransparent

                Rectangle {
                    anchors.horizontalCenter: parent.horizontalCenter
                    anchors.top: parent.top
                    anchors.bottom: parent.bottom
                    width: 1
                    color: leftResizeMouse.containsMouse || leftResizeMouse.dragging ? UiStyle.colorBorderFocus : UiStyle.colorBorderMajor
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

            ColumnLayout {
                id: workAreaColumn
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: 0

                RowLayout {
                    id: upperWorkArea
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    spacing: 0

                    MainWorkspace {
                        id: mainWorkspace
                        controller: runtimeController
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                    }

                    Rectangle {
                        id: rightResizeHandle
                        visible: window.effectiveRightPanelVisible
                        Layout.preferredWidth: visible ? 6 : 0
                        Layout.fillHeight: true
                        color: UiStyle.colorTransparent

                        Rectangle {
                            anchors.horizontalCenter: parent.horizontalCenter
                            anchors.top: parent.top
                            anchors.bottom: parent.bottom
                            width: 1
                            color: rightResizeMouse.containsMouse || rightResizeMouse.dragging ? UiStyle.colorBorderFocus : UiStyle.colorBorderMajor
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
                        Layout.preferredWidth: visible ? runtimeController.rightPanelWidth : 0
                        Layout.fillHeight: true
                    }
                }

                Rectangle {
                    id: bottomResizeHandle
                    visible: window.effectiveBottomPanelVisible
                    Layout.fillWidth: true
                    Layout.preferredHeight: visible ? 6 : 0
                    color: UiStyle.colorTransparent

                    Rectangle {
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.left: parent.left
                        anchors.right: parent.right
                        height: 1
                        color: bottomResizeMouse.containsMouse || bottomResizeMouse.dragging ? UiStyle.colorBorderFocus : UiStyle.colorBorderMajor
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
                    Layout.fillWidth: true
                    Layout.preferredHeight: visible ? runtimeController.bottomPanelHeight : 0
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
