import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../../style"
import "../../components"

Item {
    id: drawingRightContext

    property string dataUi: "drawing_tool_right_context"
    property string dataState: "draftsman_native_drawing"
    property var controller: null
    property var editRows: controller ? controller.drawingObjectEditRows(controller.revision) : []
    property var toolParameterRows: controller ? controller.drawingToolParameterEditRows(controller.revision) : []

    Flickable {
        anchors.fill: parent
        clip: true
        boundsBehavior: Flickable.StopAtBounds
        contentWidth: width
        contentHeight: content.implicitHeight

        ScrollBar.vertical: ScrollBar {
            policy: ScrollBar.AsNeeded
            width: 5
        }

        ColumnLayout {
            id: content
            width: parent.width
            spacing: UiStyle.space10

            UiPanel {
                visible: drawingRightContext.controller ? !drawingRightContext.controller.hasSelectedDrawingExternalTool(drawingRightContext.controller.revision) : true
                Layout.fillWidth: true
                Layout.preferredHeight: visible ? 138 : 0
                panelColor: UiStyle.mix(UiStyle.colorPanel, UiStyle.colorText, 0.035)
                panelBorderWidth: UiStyle.borderThin
                panelBorder: UiStyle.colorBorderMinor

                ColumnLayout {
                    anchors.fill: parent
                    spacing: UiStyle.space6

                    UiSectionHeader {
                        title: "Selection"
                        Layout.fillWidth: true
                    }
                    Repeater {
                        model: drawingRightContext.controller ? drawingRightContext.controller.drawingInspectorRows(drawingRightContext.controller.revision) : []
                        delegate: UiInspectorRow {
                            Layout.fillWidth: true
                            label: modelData.label
                            value: modelData.value
                            maxValueLines: 4
                        }
                    }
                }
            }

            UiPanel {
                visible: drawingRightContext.controller ? drawingRightContext.editRows.length > 0 : false
                Layout.fillWidth: true
                Layout.preferredHeight: visible ? Math.max(108, 42 + drawingRightContext.editRows.length * 34) : 0
                panelColor: UiStyle.mix(UiStyle.colorPanel, UiStyle.colorText, 0.035)
                panelBorderWidth: UiStyle.borderThin
                panelBorder: UiStyle.colorBorderMinor

                ColumnLayout {
                    anchors.fill: parent
                    spacing: UiStyle.space6

                    UiSectionHeader {
                        title: "Geometry Edit"
                        Layout.fillWidth: true
                    }
                    Repeater {
                        model: drawingRightContext.editRows
                        delegate: RowLayout {
                            Layout.fillWidth: true
                            spacing: UiStyle.space6

                            Text {
                                Layout.preferredWidth: 78
                                text: modelData.label
                                color: UiStyle.colorTextFaint
                                font.family: UiStyle.fontSans
                                font.pixelSize: UiStyle.fontSizeXs
                                elide: Text.ElideRight
                            }

                            UiTextField {
                                Layout.fillWidth: true
                                Layout.preferredHeight: 26
                                text: modelData.value
                                inputMethodHints: modelData.numeric === false ? Qt.ImhNoPredictiveText : Qt.ImhFormattedNumbersOnly
                                property string lastCommittedText: modelData.value

                                function commitValue() {
                                    if (drawingRightContext.controller && text !== lastCommittedText) {
                                        lastCommittedText = text
                                        drawingRightContext.controller.updateSelectedDrawingObjectField(modelData.field, text)
                                    }
                                }

                                onAccepted: commitValue()
                                onEditingFinished: commitValue()
                            }
                        }
                    }
                }
            }

            UiPanel {
                visible: drawingRightContext.controller ? drawingRightContext.toolParameterRows.length > 0 : false
                Layout.fillWidth: true
                Layout.preferredHeight: visible ? Math.max(92, 42 + drawingRightContext.toolParameterRows.length * 34) : 0
                panelColor: UiStyle.mix(UiStyle.colorPanel, UiStyle.colorText, 0.035)
                panelBorderWidth: UiStyle.borderThin
                panelBorder: UiStyle.colorBorderMinor

                ColumnLayout {
                    anchors.fill: parent
                    spacing: UiStyle.space6

                    UiSectionHeader {
                        title: "Tool Parameters"
                        Layout.fillWidth: true
                    }
                    Repeater {
                        model: drawingRightContext.toolParameterRows
                        delegate: RowLayout {
                            Layout.fillWidth: true
                            spacing: UiStyle.space6

                            Text {
                                Layout.preferredWidth: 78
                                text: modelData.label
                                color: UiStyle.colorTextFaint
                                font.family: UiStyle.fontSans
                                font.pixelSize: UiStyle.fontSizeXs
                                elide: Text.ElideRight
                            }

                            UiTextField {
                                Layout.fillWidth: true
                                Layout.preferredHeight: 26
                                text: modelData.value
                                inputMethodHints: Qt.ImhFormattedNumbersOnly
                                property string lastCommittedText: modelData.value

                                function commitValue() {
                                    if (drawingRightContext.controller && text !== lastCommittedText) {
                                        lastCommittedText = text
                                        drawingRightContext.controller.updateDrawingToolParameterField(modelData.field, text)
                                    }
                                }

                                onAccepted: commitValue()
                                onEditingFinished: commitValue()
                            }
                        }
                    }
                }
            }

            UiPanel {
                visible: drawingRightContext.controller ? drawingRightContext.controller.hasSelectedDrawingExternalTool(drawingRightContext.controller.revision) : false
                Layout.fillWidth: true
                Layout.preferredHeight: visible ? 220 : 0
                panelColor: UiStyle.mix(UiStyle.colorPanel, UiStyle.colorText, 0.035)
                panelBorderWidth: UiStyle.borderThin
                panelBorder: UiStyle.colorBorderMinor

                ColumnLayout {
                    anchors.fill: parent
                    spacing: UiStyle.space6

                    UiSectionHeader {
                        title: "Image Workbench"
                        Layout.fillWidth: true
                    }
                    Repeater {
                        model: drawingRightContext.controller ? drawingRightContext.controller.drawingExternalToolRows(drawingRightContext.controller.revision) : []
                        delegate: UiInspectorRow {
                            Layout.fillWidth: true
                            label: modelData.label
                            value: modelData.value
                            maxValueLines: 3
                        }
                    }
                }
            }

            UiPanel {
                visible: drawingRightContext.controller ? !drawingRightContext.controller.hasSelectedDrawingExternalTool(drawingRightContext.controller.revision) : true
                Layout.fillWidth: true
                Layout.preferredHeight: visible ? 166 : 0
                panelColor: UiStyle.mix(UiStyle.colorPanel, UiStyle.colorText, 0.035)
                panelBorderWidth: UiStyle.borderThin
                panelBorder: UiStyle.colorBorderMinor

                ColumnLayout {
                    anchors.fill: parent
                    spacing: UiStyle.space6

                    UiSectionHeader {
                        title: "Model Objects"
                        Layout.fillWidth: true
                    }
                    Repeater {
                        model: drawingRightContext.controller ? drawingRightContext.controller.drawingModelObjectRows(drawingRightContext.controller.revision) : []
                        delegate: UiListRow {
                            Layout.fillWidth: true
                            label: modelData.label
                            meta: modelData.meta
                            selected: modelData.selected
                            clickable: modelData.id.length > 0
                            onClicked: if (drawingRightContext.controller && modelData.id.length > 0) drawingRightContext.controller.selectDrawingObject(modelData.id)
                        }
                    }
                }
            }

            UiPanel {
                visible: drawingRightContext.controller ? !drawingRightContext.controller.hasSelectedDrawingExternalTool(drawingRightContext.controller.revision) : true
                Layout.fillWidth: true
                Layout.preferredHeight: visible ? 154 : 0
                panelColor: UiStyle.mix(UiStyle.colorPanel, UiStyle.colorText, 0.035)
                panelBorderWidth: UiStyle.borderThin
                panelBorder: UiStyle.colorBorderMinor

                ColumnLayout {
                    anchors.fill: parent
                    spacing: UiStyle.space6

                    UiSectionHeader {
                        title: "Tool Settings"
                        Layout.fillWidth: true
                    }
                    Repeater {
                        model: drawingRightContext.controller ? drawingRightContext.controller.drawingToolSettingsRows(drawingRightContext.controller.revision) : []
                        delegate: UiInspectorRow {
                            Layout.fillWidth: true
                            label: modelData.label
                            value: modelData.value
                            maxValueLines: 3
                        }
                    }
                }
            }

        }
    }
}
