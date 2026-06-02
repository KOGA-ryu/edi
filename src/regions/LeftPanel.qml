import QtQuick
import QtQuick.Layouts
import "../style"
import "../components"
import "../features/ui_taxonomy"
import "../features/settings"

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
                text: "Draftsman"
                color: UiStyle.colorText
                font.family: UiStyle.fontSans
                font.pixelSize: UiStyle.fontSizeTitle
                font.weight: UiStyle.fontWeightSemiBold
                elide: Text.ElideRight
            }

            UiButton { label: "New"; implicitWidth: 54; enabled: false }
        }

        ReviewLeftNav {
            controller: leftPanel.controller
            visible: !leftPanel.controller || leftPanel.controller.activityMode === "review"
            Layout.fillWidth: true
            Layout.fillHeight: visible
        }

        SettingsLeftNav {
            controller: leftPanel.controller
            visible: leftPanel.controller && leftPanel.controller.activityMode === "settings"
            Layout.fillWidth: true
            Layout.fillHeight: visible
        }

        Item {
            visible: leftPanel.controller && leftPanel.controller.activityMode !== "review" && leftPanel.controller.activityMode !== "settings"
            Layout.fillWidth: true
            Layout.fillHeight: visible
            ColumnLayout {
                anchors.fill: parent
                spacing: UiStyle.space8
                UiSectionHeader { title: "Mode"; Layout.fillWidth: true }
                UiListRow {
                    Layout.fillWidth: true
                    label: leftPanel.controller ? leftPanel.controller.activityMode : "binder"
                    meta: "reserved"
                }
            }
        }
    }
}
