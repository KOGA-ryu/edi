import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../../style"
import "../../components"

Rectangle {
    id: workspace
    property var controller: null

    color: UiStyle.colorWorkspace

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: UiStyle.space10
        spacing: UiStyle.space10

        RowLayout {
            Layout.fillWidth: true
            spacing: UiStyle.space2

            Repeater {
                model: workspace.controller ? workspace.controller.localTabs : []
                delegate: UiTab {
                    label: modelData.label
                    tooltip: modelData.tooltip
                    active: workspace.controller.selectedLocalTab === modelData.id
                    clickable: true
                    onClicked: workspace.controller.setLocalTab(modelData.id)
                }
            }
        }

        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true

            ColumnLayout {
                width: Math.max(parent.width - 18, 320)
                spacing: UiStyle.space10

                UiTaxonomyFocusPanel {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 172
                    controller: workspace.controller
                }

                ReviewStatusPanel {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 46
                    controller: workspace.controller
                }

                ReviewRouteCards {
                    visible: workspace.controller && (workspace.controller.selectedLocalTab === "overview" || workspace.controller.selectedLocalTab === "objects")
                    Layout.fillWidth: true
                    controller: workspace.controller
                }

                ReviewCodeRefs {
                    visible: workspace.controller && (workspace.controller.selectedLocalTab === "overview" || workspace.controller.selectedLocalTab === "code")
                    Layout.fillWidth: true
                    controller: workspace.controller
                }

                ColumnLayout {
                    visible: workspace.controller && workspace.controller.selectedLocalTab === "prompts"
                    Layout.fillWidth: true
                    spacing: UiStyle.space6
                    UiSectionHeader { title: "Review Prompts"; Layout.fillWidth: true }
                    Repeater {
                        model: workspace.controller ? workspace.controller.currentRoute().prompts : []
                        delegate: UiListRow {
                            Layout.fillWidth: true
                            label: modelData
                            meta: "prompt"
                        }
                    }
                }

                ReviewCommentThread {
                    visible: workspace.controller && (workspace.controller.selectedLocalTab === "overview" || workspace.controller.selectedLocalTab === "notes")
                    Layout.fillWidth: true
                    controller: workspace.controller
                }
            }
        }
    }
}
