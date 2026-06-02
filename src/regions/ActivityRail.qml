import QtQuick
import QtQuick.Layouts
import "../style"
import "../components"

Rectangle {
    id: rail

    property string dataUi: "activity_rail"
    property string dataState: "idle"
    property string placementRole: "fixed_edge_rail"
    property string surfaceRecipeId: "activity_rail_surface"
    property var controller: null

    color: UiStyle.colorRail
    border.width: UiStyle.borderNone

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: UiStyle.space8
        spacing: UiStyle.space6

        Repeater {
            model: rail.controller ? rail.controller.activityModes : []
            delegate: UiIconButton {
                iconText: modelData.icon
                label: modelData.label
                tooltip: modelData.tooltip
                selected: rail.controller.activityMode === modelData.id
                Layout.alignment: Qt.AlignHCenter
                onClicked: rail.controller.setActivityMode(modelData.id)
            }
        }

        Item { Layout.fillHeight: true }
    }
}
