import QtQuick
import QtQuick.Layouts
import "../../style"
import "../../components"

UiPanel {
    id: panel
    property var controller: null
    property var counts: controller ? controller.statusCounts(controller.revision) : ({})

    panelColor: UiStyle.colorPanel
    panelPadding: UiStyle.space2

    RowLayout {
        anchors.fill: parent
        spacing: UiStyle.space4
        UiStatusChip { status: "pending"; label: "pending " + (panel.counts.pending || 0); compact: true }
        UiStatusChip { status: "accepted"; label: "accepted " + (panel.counts.accepted || 0); compact: true }
        UiStatusChip { status: "needs_rework"; label: "rework " + (panel.counts.needs_rework || 0); compact: true }
        UiStatusChip { status: "rejected"; label: "rejected " + (panel.counts.rejected || 0); compact: true }
        Item { Layout.fillWidth: true }
    }
}
