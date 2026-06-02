import QtQuick
import QtQuick.Layouts
import "../../style"
import "../../components"

ColumnLayout {
    id: root
    property var controller: null
    property var route: controller ? controller.currentRoute() : ({})
    spacing: UiStyle.space0

    Repeater {
        model: root.route.codeRefs || []
        delegate: UiCodeRefRow {
            Layout.fillWidth: true
            path: modelData
            role: "source"
        }
    }
}
