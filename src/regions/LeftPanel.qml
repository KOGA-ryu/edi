import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../style"
import "../components"
import "../features/blank"
import "../features/ui_taxonomy"
import "../features/settings"
import "../features/csv_map_editor"
import "../features/drawing_tool"
import "../features/text_editor"

Rectangle {
    id: leftPanel

    property string dataUi: "left_panel"
    property string dataState: "secondary"
    property string placementRole: "resizable_left_panel"
    property string surfaceRecipeId: "left_panel_surface"
    property var controller: null

    color: UiStyle.colorPanel
    border.width: UiStyle.borderNone

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: UiStyle.space10
        spacing: UiStyle.space6

        RowLayout {
            Layout.fillWidth: true
            Layout.preferredHeight: UiStyle.toolbarHeight
            spacing: UiStyle.space8

            Text {
                Layout.fillWidth: true
                text: leftPanel.controller ? leftPanel.controller.projectTitle : "Draftsman"
                color: UiStyle.colorText
                font.family: UiStyle.fontSans
                font.pixelSize: UiStyle.fontSizeTitle
                font.weight: UiStyle.fontWeightSemiBold
                elide: Text.ElideRight
            }
        }

        Flickable {
            id: navScroll
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            boundsBehavior: Flickable.StopAtBounds
            contentWidth: width
            contentHeight: navContent.implicitHeight
            interactive: contentHeight > height

            ScrollBar.vertical: ScrollBar {
                id: verticalScrollBar
                policy: ScrollBar.AlwaysOn
                active: visible
                visible: navScroll.contentHeight > navScroll.height
                width: 5
                contentItem: Rectangle {
                    implicitWidth: 5
                    radius: 2
                    color: verticalScrollBar.pressed || verticalScrollBar.hovered ? UiStyle.colorAccentSoft : UiStyle.colorBorderMajor
                    opacity: verticalScrollBar.active ? 0.85 : 0.55
                }
                background: Rectangle {
                    color: UiStyle.colorTransparent
                }
            }

            Item {
                width: navScroll.width - (verticalScrollBar.visible ? verticalScrollBar.width + UiStyle.space4 : 0)
                height: navContent.implicitHeight

                ColumnLayout {
                    id: navContent
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.top: parent.top
                    spacing: UiStyle.space6

                    ReviewLeftNav {
                        controller: leftPanel.controller
                        visible: leftPanel.controller && leftPanel.controller.activityMode === "review"
                        Layout.fillWidth: true
                    }

                    SettingsLeftNav {
                        controller: leftPanel.controller
                        visible: leftPanel.controller && leftPanel.controller.activityMode === "settings"
                        Layout.fillWidth: true
                    }

                    CsvMapEditorLeftNav {
                        controller: leftPanel.controller
                        visible: leftPanel.controller && leftPanel.controller.activityMode === "map_editor"
                        Layout.fillWidth: true
                    }

                    DrawingToolLeftPanel {
                        controller: leftPanel.controller
                        visible: leftPanel.controller && leftPanel.controller.activityMode === "drawing_tool"
                        Layout.fillWidth: true
                    }

                    TextEditorLeftPanel {
                        controller: leftPanel.controller
                        visible: leftPanel.controller && leftPanel.controller.activityMode === "text_editor"
                        Layout.fillWidth: true
                    }

                    BlankPanel {
                        visible: leftPanel.controller
                            && leftPanel.controller.activityMode !== "review"
                            && leftPanel.controller.activityMode !== "settings"
                            && leftPanel.controller.activityMode !== "map_editor"
                            && leftPanel.controller.activityMode !== "drawing_tool"
                            && leftPanel.controller.activityMode !== "text_editor"
                        Layout.fillWidth: true
                    }
                }
            }
        }
    }
}
