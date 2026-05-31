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
    width: UiStyle.windowWidth
    height: UiStyle.windowHeight
    minimumWidth: UiStyle.windowMinWidth
    minimumHeight: UiStyle.windowMinHeight
    visible: true

    property string dataUi: "window"
    property string dataState: "active"
    property string surfaceRecipeId: "shell_surface"

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
                Layout.preferredWidth: UiStyle.leftPanelWidth
                Layout.fillHeight: true
            }

            ColumnLayout {
                id: centerColumn
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: 0

                MainWorkspace {
                    id: mainWorkspace
                    controller: runtimeController
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                }

                BottomPanel {
                    id: bottomPanel
                    controller: runtimeController
                    Layout.fillWidth: true
                    Layout.preferredHeight: UiStyle.bottomPanelHeight
                }
            }

            RightPanel {
                id: rightPanel
                controller: runtimeController
                visible: window.width >= 1160
                Layout.preferredWidth: visible ? UiStyle.rightPanelWidth : 0
                Layout.fillHeight: true
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
