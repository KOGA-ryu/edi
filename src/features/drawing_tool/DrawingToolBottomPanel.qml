import QtQuick
import QtQuick.Layouts
import "../../style"
import "../../components"

Item {
    id: drawingBottomPanel

    property string dataUi: "drawing_tool_bottom_panel"
    property string dataState: "draftsman_native_drawing"
    property var controller: null

    function activeTab() {
        return controller && String(controller.selectedShelfTab || "").length > 0 ? String(controller.selectedShelfTab) : "Log"
    }

    function tabActive(name) {
        return activeTab().toLowerCase() === String(name).toLowerCase()
    }

    RowLayout {
        anchors.fill: parent
        spacing: UiStyle.space8

        UiPanel {
            visible: drawingBottomPanel.tabActive("Log")
            Layout.fillWidth: true
            Layout.fillHeight: true
            panelColor: UiStyle.mix(UiStyle.colorPanel, UiStyle.colorText, 0.03)
            panelBorderWidth: UiStyle.borderThin
            panelBorder: UiStyle.colorBorderMinor

            RowLayout {
                anchors.fill: parent
                spacing: UiStyle.space8

                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    spacing: UiStyle.space4

                    UiSectionHeader {
                        title: "Native Run Log"
                        Layout.fillWidth: true
                    }
                    Repeater {
                        model: drawingBottomPanel.controller ? drawingBottomPanel.controller.drawingLogRows(drawingBottomPanel.controller.revision) : []
                        delegate: UiListRow {
                            Layout.fillWidth: true
                            label: modelData.label
                            meta: modelData.value
                            metaMaxWidth: 260
                        }
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    spacing: UiStyle.space4

                    UiSectionHeader {
                        title: "Validation"
                        Layout.fillWidth: true
                    }
                    Repeater {
                        model: drawingBottomPanel.controller ? drawingBottomPanel.controller.drawingValidationRows(drawingBottomPanel.controller.revision) : []
                        delegate: UiListRow {
                            Layout.fillWidth: true
                            label: modelData.label || modelData.id
                            meta: modelData.value || modelData.status
                            metaMaxWidth: 280
                        }
                    }
                }
            }
        }

        UiPanel {
            visible: drawingBottomPanel.tabActive("Exports")
            Layout.fillWidth: true
            Layout.fillHeight: true
            panelColor: UiStyle.mix(UiStyle.colorPanel, UiStyle.colorText, 0.03)
            panelBorderWidth: UiStyle.borderThin
            panelBorder: UiStyle.colorBorderMinor

            RowLayout {
                anchors.fill: parent
                spacing: UiStyle.space8

                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    spacing: UiStyle.space4

                    UiSectionHeader {
                        title: "Available Exports"
                        Layout.fillWidth: true
                    }
                    Repeater {
                        model: drawingBottomPanel.controller ? drawingBottomPanel.controller.drawingExportRows(drawingBottomPanel.controller.revision) : []
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

        UiPanel {
            visible: drawingBottomPanel.tabActive("Manifest")
            Layout.fillWidth: true
            Layout.fillHeight: true
            panelColor: UiStyle.mix(UiStyle.colorPanel, UiStyle.colorText, 0.03)
            panelBorderWidth: UiStyle.borderThin
            panelBorder: UiStyle.colorBorderMinor

            RowLayout {
                anchors.fill: parent
                spacing: UiStyle.space8

                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    spacing: UiStyle.space4

                    UiSectionHeader {
                        title: "Manifest"
                        Layout.fillWidth: true
                    }
                    Repeater {
                        model: drawingBottomPanel.controller ? drawingBottomPanel.controller.drawingManifestRows(drawingBottomPanel.controller.revision) : []
                        delegate: UiListRow {
                            Layout.fillWidth: true
                            label: modelData.label
                            meta: modelData.value
                            metaMaxWidth: 300
                        }
                    }
                }
            }
        }
    }
}
