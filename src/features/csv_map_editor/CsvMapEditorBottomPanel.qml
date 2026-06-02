import QtQuick
import QtQuick.Layouts
import "../../style"
import "../../components"

ColumnLayout {
    id: root

    property var controller: null
    property int controllerRevision: controller ? controller.revision : 0
    readonly property bool showingLog: controller && controller.selectedShelfTab === "Log"

    spacing: UiStyle.space0

    Repeater {
        model: root.controller
            ? (root.showingLog ? root.controller.mapLogRows(root.controllerRevision) : root.controller.mapValidationRows(root.controllerRevision))
            : []

        delegate: UiListRow {
            Layout.fillWidth: true
            Layout.preferredHeight: 20
            label: modelData.label
            meta: modelData.value
            metaMaxWidth: 560
        }
    }

    Item { Layout.fillHeight: true }
}
