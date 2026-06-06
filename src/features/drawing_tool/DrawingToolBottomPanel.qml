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

    function swatchColor(value, fallback) {
        var raw = String(value || "").trim()
        return raw.length > 0 ? raw : fallback
    }

    component ColorField: RowLayout {
        property string fieldId: ""
        property string valueText: ""
        property string fallbackColor: UiStyle.colorControl
        spacing: UiStyle.space2

        Rectangle {
            Layout.preferredWidth: 20
            Layout.preferredHeight: 20
            radius: UiStyle.radiusSm
            color: drawingBottomPanel.swatchColor(parent.valueText, parent.fallbackColor)
            border.width: UiStyle.borderThin
            border.color: UiStyle.colorPanelRaised
        }

        UiTextField {
            Layout.preferredWidth: 72
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

    component NumericField: UiTextField {
        property string fieldId: ""
        property string valueText: ""

        Layout.preferredWidth: 42
        Layout.preferredHeight: 24
        horizontalAlignment: TextInput.AlignHCenter
        text: valueText
        property string lastCommittedText: valueText

        function commit() {
            if (text === lastCommittedText) {
                return
            }
            lastCommittedText = text
            drawingBottomPanel.setFieldValue(fieldId, text)
        }

        onAccepted: commit()
        onEditingFinished: commit()
    }

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: UiStyle.space6
        anchors.rightMargin: UiStyle.space6
        spacing: UiStyle.space4

        ColorField {
            fieldId: "stroke"
            valueText: String(drawingBottomPanel.controller ? (drawingBottomPanel.controller.drawingStrokeColor || "") : "")
            fallbackColor: UiStyle.colorAccent
        }

        ColorField {
            fieldId: "fill"
            valueText: String(drawingBottomPanel.controller ? (drawingBottomPanel.controller.drawingFillColor || "") : "")
            fallbackColor: UiStyle.colorControl
        }

        NumericField {
            fieldId: "width"
            valueText: String(Number(drawingBottomPanel.controller ? drawingBottomPanel.controller.drawingLineThickness : 2))
        }

        NumericField {
            fieldId: "opacity"
            valueText: String(Number(drawingBottomPanel.controller ? drawingBottomPanel.controller.drawingStrokeOpacity : 1))
        }

        Rectangle {
            Layout.preferredWidth: 1
            Layout.fillHeight: true
            color: UiStyle.colorPanelRaised
            opacity: 0.7
        }

        UiButton {
            Layout.preferredWidth: 48
            Layout.preferredHeight: 24
            label: "Line"
            selected: drawingBottomPanel.styleLineStyle() === "solid"
            onClicked: if (drawingBottomPanel.controller) drawingBottomPanel.controller.setDrawingLineStyle("solid")
        }

        UiButton {
            Layout.preferredWidth: 48
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

        Item { Layout.fillWidth: true }

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
    }
}
