import QtQuick
import QtQuick.Layouts
import "../../style"
import "../../components"

Item {
    id: drawingBottomPanel

    property string dataUi: "drawing_tool_bottom_panel"
    property string dataState: "draftsman_native_drawing"
    property var controller: null

    function asArray(value) {
        if (!value) {
            return []
        }
        if (Array.isArray(value)) {
            return value
        }
        return []
    }

    function logRows() {
        return drawingBottomPanel.controller ? drawingBottomPanel.controller.drawingLogRows(drawingBottomPanel.controller.revision) : []
    }

    function validationRows() {
        return drawingBottomPanel.controller ? drawingBottomPanel.controller.drawingValidationRows(drawingBottomPanel.controller.revision) : []
    }

    function exportRows() {
        return drawingBottomPanel.controller ? drawingBottomPanel.controller.drawingExportRows(drawingBottomPanel.controller.revision) : []
    }

    function manifestRows() {
        return drawingBottomPanel.controller ? drawingBottomPanel.controller.drawingManifestRows(drawingBottomPanel.controller.revision) : []
    }

    function hasLogHistory() {
        if (!drawingBottomPanel.controller) {
            return false
        }
        if (drawingBottomPanel.controller.drawingNativeController) {
            var model = drawingBottomPanel.controller.drawingNativeController.modelDocument()
            var commandLog = asArray(model ? model.command_log : [])
            return commandLog.length > 0
        }
        return false
    }

    function hasBottomRows() {
        return hasLogHistory() && logRows().length > 0
    }

    function activeTab() {
        if (!drawingBottomPanel.controller) {
            return ""
        }
        var tabs = asArray(drawingBottomPanel.controller.shelfTabs)
        var selected = String(drawingBottomPanel.controller.selectedShelfTab || "")
        if (selected.length > 0 && tabs.indexOf(selected) >= 0) {
            return selected
        }
        if (tabs.length > 0) {
            return String(tabs[0])
        }
        return hasBottomRows() ? "Log" : ""
    }

    function tabActive(name) {
        return activeTab().toLowerCase() === String(name).toLowerCase()
    }

    visible: hasBottomRows()

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 32
            visible: drawingBottomPanel.controller && drawingBottomPanel.controller.shelfTabs.length > 0
            color: UiStyle.colorPanelAlt
            border.width: UiStyle.borderNone

            Rectangle {
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                height: 1
                color: UiStyle.colorPanelRaised
            }

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: UiStyle.space8
                anchors.rightMargin: UiStyle.space8
                spacing: UiStyle.space2

                Repeater {
                    model: drawingBottomPanel.controller ? drawingBottomPanel.controller.shelfTabs : []
                    delegate: UiTab {
                        label: modelData
                        active: drawingBottomPanel.tabActive(modelData)
                        clickable: true
                        onClicked: drawingBottomPanel.controller.setShelfTab(modelData)
                    }
                }
                Item { Layout.fillWidth: true }
            }
        }

        UiPanel {
            visible: drawingBottomPanel.tabActive("Log")
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.margins: UiStyle.space6
            panelColor: UiStyle.mix(UiStyle.colorPanel, UiStyle.colorText, 0.03)
            panelBorderWidth: UiStyle.borderThin
            panelBorder: UiStyle.colorBorderMinor

            ColumnLayout {
                anchors.fill: parent
                spacing: UiStyle.space4

                UiSectionHeader {
                    title: "Native Run Log"
                    Layout.fillWidth: true
                }
                Repeater {
                    model: drawingBottomPanel.logRows()
                    delegate: UiListRow {
                        Layout.fillWidth: true
                        label: modelData.label
                        meta: modelData.value
                        metaMaxWidth: 260
                    }
                }
            }
        }
    }
}
