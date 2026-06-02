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
        anchors.margins: UiStyle.space8
        spacing: UiStyle.space6

        Flickable {
            id: localTabStrip
            Layout.fillWidth: true
            Layout.preferredHeight: UiStyle.tabHeight
            clip: true
            contentWidth: localTabRow.implicitWidth
            contentHeight: height
            boundsBehavior: Flickable.StopAtBounds

            function revealTab(tabItem) {
                if (!tabItem || contentWidth <= width) {
                    contentX = 0
                    return
                }
                var leftEdge = tabItem.x
                var rightEdge = tabItem.x + tabItem.width
                if (leftEdge < contentX) {
                    contentX = Math.max(0, leftEdge - UiStyle.space8)
                } else if (rightEdge > contentX + width) {
                    contentX = Math.min(contentWidth - width, rightEdge - width + UiStyle.space8)
                }
            }

            Row {
                id: localTabRow
                height: parent.height
                spacing: UiStyle.space2

                Repeater {
                    model: workspace.controller ? workspace.controller.localTabs : []
                    delegate: UiTab {
                        height: localTabRow.height
                        label: modelData.label
                        tooltip: modelData.tooltip
                        active: workspace.controller.selectedLocalTab === modelData.id
                        clickable: true
                        onActiveChanged: if (active) localTabStrip.revealTab(this)
                        onClicked: {
                            workspace.controller.setLocalTab(modelData.id)
                            localTabStrip.revealTab(this)
                        }
                    }
                }
            }
        }

        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true

            ColumnLayout {
                width: Math.max(parent.width - 18, 320)
                spacing: UiStyle.space6

                UiTaxonomyFocusPanel {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 108
                    controller: workspace.controller
                }

                ReviewStatusPanel {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 24
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
                    spacing: UiStyle.space2
                    Repeater {
                        model: workspace.controller ? workspace.controller.currentRoute().prompts : []
                        delegate: UiListRow {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 22
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
