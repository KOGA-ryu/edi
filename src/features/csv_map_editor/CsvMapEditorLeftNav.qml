import QtQuick
import QtQuick.Layouts
import "../../style"
import "../../components"

ColumnLayout {
    id: root

    property var controller: null
    property int controllerRevision: controller ? controller.revision : 0

    spacing: UiStyle.space6

    UiSectionHeader { title: "Map"; Layout.fillWidth: true }

    UiListRow {
        Layout.fillWidth: true
        label: "Starter grid"
        meta: root.controller ? String(root.controller.mapColCount()) + "x" + String(root.controller.mapRowCount()) : ""
    }

    UiListRow {
        Layout.fillWidth: true
        label: "Layer"
        meta: "schematic"
    }

    UiListRow {
        Layout.fillWidth: true
        label: "Selected"
        meta: root.controller ? String(root.controller.selectedMapRow) + "," + String(root.controller.selectedMapCol) : ""
    }

    UiSectionHeader { title: "Palette"; Layout.fillWidth: true }

    Repeater {
        model: root.controller ? root.controller.mapPaletteRows(root.controllerRevision) : []
        delegate: UiListRow {
            Layout.fillWidth: true
            label: modelData.label
            meta: modelData.meta
            tooltip: modelData.label + " token count"
        }
    }

    Item { Layout.fillHeight: true }
}
