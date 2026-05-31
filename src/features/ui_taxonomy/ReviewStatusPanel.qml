import QtQuick
import QtQuick.Layouts
import "../../style"
import "../../components"

UiPanel {
    id: panel
    property var controller: null
    property var counts: controller ? controller.statusCounts(controller.revision) : ({})

    panelColor: UiStyle.colorPanel
    panelBorder: UiStyle.colorBorderMinor

    RowLayout {
        anchors.fill: parent
        spacing: UiStyle.space8
        UiStatusChip { status: "pending"; label: "pending " + (panel.counts.pending || 0) }
        UiStatusChip { status: "accepted"; label: "accepted " + (panel.counts.accepted || 0) }
        UiStatusChip { status: "needs_rework"; label: "rework " + (panel.counts.needs_rework || 0) }
        UiStatusChip { status: "rejected"; label: "rejected " + (panel.counts.rejected || 0) }
        Item { Layout.fillWidth: true }
    }
}

