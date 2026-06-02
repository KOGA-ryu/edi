import QtQuick
import QtQuick.Layouts
import "../../style"
import "../../components"

ColumnLayout {
    id: root
    property var controller: null
    spacing: UiStyle.space2

    GridLayout {
        Layout.fillWidth: true
        columns: width > 820 ? 2 : 1
        columnSpacing: UiStyle.space4
        rowSpacing: UiStyle.space2

        Repeater {
            model: root.controller ? root.controller.childRoutes(root.controller.selectedRouteId, root.controller.revision) : []
            delegate: UiRouteCard {
                Layout.fillWidth: true
                title: modelData.label
                summary: modelData.summary
                status: root.controller.routeStatus(modelData.id)
                meta: modelData.type + " / " + modelData.children.length + " child routes"
                noteCount: root.controller.noteCount(modelData.id)
                onClicked: root.controller.selectRoute(modelData.id)
            }
        }
    }

    Text {
        visible: root.controller && root.controller.childRoutes(root.controller.selectedRouteId, root.controller.revision).length === 0
        Layout.fillWidth: true
        text: "No deeper routes. Use the local tabs, add a note, or navigate back/home."
        color: UiStyle.colorTextMuted
        font.family: UiStyle.fontSans
        font.pixelSize: UiStyle.fontSizeSm
        wrapMode: Text.WordWrap
    }
}
