import QtQuick
import QtQuick.Layouts
import "../../style"
import "../../components"

Rectangle {
    id: drawingBottomPanel

    property string dataUi: "drawing_tool_bottom_panel"
    property string dataState: "draftsman_native_drawing"
    property var controller: null

    color: UiStyle.colorTransparent
    border.width: UiStyle.borderNone

    function styleLineStyle() {
        return String(drawingBottomPanel.controller ? drawingBottomPanel.controller.drawingLineStyle : "solid")
    }

    function snapEnabled() {
        return !!(drawingBottomPanel.controller && drawingBottomPanel.controller.drawingSnapGridEnabled)
    }

    function gridVisible() {
        return !!(drawingBottomPanel.controller && drawingBottomPanel.controller.drawingGridVisible)
    }

    function objectSnapEnabled() {
        return !!(drawingBottomPanel.controller && drawingBottomPanel.controller.drawingObjectSnapEnabled)
    }

    function setFieldValue(fieldId, value) {
        if (!drawingBottomPanel.controller) {
            return
        }
        if (fieldId === "stroke") {
            drawingBottomPanel.controller.setDrawingStrokeColor(value)
        } else if (fieldId === "fill") {
            drawingBottomPanel.controller.setDrawingFillColor(value)
        } else if (fieldId === "width") {
            drawingBottomPanel.controller.setDrawingLineThickness(value)
        } else if (fieldId === "opacity") {
            drawingBottomPanel.controller.setDrawingStrokeOpacity(value)
        }
    }

    component FormatField: RowLayout {
        property string label: ""
        property string fieldId: ""
        property string valueText: ""
        property int fieldWidth: 86
        property int labelWidth: 36
        spacing: UiStyle.space2

        Text {
            Layout.preferredWidth: parent.labelWidth
            text: parent.label
            color: UiStyle.colorTextFaint
            font.family: UiStyle.fontSans
            font.pixelSize: UiStyle.fontSizeXs
            elide: Text.ElideRight
        }

        UiTextField {
            Layout.preferredWidth: parent.fieldWidth
            Layout.preferredHeight: 24
            text: parent.valueText
            property string lastCommittedText: parent.valueText

            function commit() {
                if (text === lastCommittedText) {
                    return
                }
                lastCommittedText = text
                drawingBottomPanel.setFieldValue(parent.fieldId, text)
            }

            onAccepted: commit()
            onEditingFinished: commit()
        }
    }

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: UiStyle.space6
        anchors.rightMargin: UiStyle.space6
        spacing: UiStyle.space4

        FormatField {
            label: "Stroke"
            fieldId: "stroke"
            valueText: String(drawingBottomPanel.controller ? (drawingBottomPanel.controller.drawingStrokeColor || "") : "")
            fieldWidth: 70
            labelWidth: 42
        }

        FormatField {
            label: "Fill"
            fieldId: "fill"
            valueText: String(drawingBottomPanel.controller ? (drawingBottomPanel.controller.drawingFillColor || "") : "")
            fieldWidth: 60
            labelWidth: 22
        }

        FormatField {
            label: "W"
            fieldId: "width"
            valueText: String(Number(drawingBottomPanel.controller ? drawingBottomPanel.controller.drawingLineThickness : 2))
            fieldWidth: 38
            labelWidth: 12
        }

        FormatField {
            label: "O"
            fieldId: "opacity"
            valueText: String(Number(drawingBottomPanel.controller ? drawingBottomPanel.controller.drawingStrokeOpacity : 1))
            fieldWidth: 38
            labelWidth: 12
        }

        Rectangle {
            Layout.preferredWidth: 1
            Layout.fillHeight: true
            color: UiStyle.colorPanelRaised
            opacity: 0.7
        }

        UiButton {
            Layout.preferredWidth: 56
            Layout.preferredHeight: 24
            label: "Solid"
            selected: drawingBottomPanel.styleLineStyle() === "solid"
            onClicked: if (drawingBottomPanel.controller) drawingBottomPanel.controller.setDrawingLineStyle("solid")
        }

        UiButton {
            Layout.preferredWidth: 52
            Layout.preferredHeight: 24
            label: "Dash"
            selected: drawingBottomPanel.styleLineStyle() === "dashed"
            onClicked: if (drawingBottomPanel.controller) drawingBottomPanel.controller.setDrawingLineStyle("dashed")
        }

        UiButton {
            Layout.preferredWidth: 44
            Layout.preferredHeight: 24
            label: "Dot"
            selected: drawingBottomPanel.styleLineStyle() === "dotted"
            onClicked: if (drawingBottomPanel.controller) drawingBottomPanel.controller.setDrawingLineStyle("dotted")
        }

        Rectangle {
            Layout.preferredWidth: 1
            Layout.fillHeight: true
            color: UiStyle.colorPanelRaised
            opacity: 0.7
        }

        UiToggle {
            Layout.preferredWidth: 104
            Layout.preferredHeight: 24
            label: "Snap"
            checked: drawingBottomPanel.snapEnabled()
            onToggled: if (drawingBottomPanel.controller) drawingBottomPanel.controller.setDrawingSnapGrid(checked)
        }

        UiToggle {
            Layout.preferredWidth: 98
            Layout.preferredHeight: 24
            label: "Grid"
            checked: drawingBottomPanel.gridVisible()
            onToggled: if (drawingBottomPanel.controller) drawingBottomPanel.controller.setDrawingGridVisible(checked)
        }

        UiToggle {
            Layout.preferredWidth: 88
            Layout.preferredHeight: 24
            label: "Obj"
            checked: drawingBottomPanel.objectSnapEnabled()
            onToggled: if (drawingBottomPanel.controller) drawingBottomPanel.controller.setDrawingObjectSnapEnabled(checked)
        }

        Item { Layout.fillWidth: true }
    }
}
