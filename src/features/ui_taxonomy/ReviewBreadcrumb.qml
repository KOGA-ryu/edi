import QtQuick
import QtQuick.Layouts
import "../../style"
import "../../components"

Rectangle {
    id: root
    property var controller: null
    implicitHeight: 34
    color: UiStyle.colorPanelAlt
    radius: UiStyle.radiusSm
    border.width: UiStyle.borderNone

    UiBreadcrumb {
        anchors.fill: parent
        anchors.leftMargin: UiStyle.space10
        anchors.rightMargin: UiStyle.space10
        items: root.controller ? root.controller.breadcrumb(root.controller.selectedRouteId) : []
        controller: root.controller
    }
}
