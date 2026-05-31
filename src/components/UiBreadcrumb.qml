import QtQuick
import QtQuick.Layouts
import "../style"

RowLayout {
    id: breadcrumb
    property var items: []
    property var controller: null
    spacing: UiStyle.space4

    Repeater {
        model: breadcrumb.items
        delegate: RowLayout {
            spacing: UiStyle.space4
            Text {
                visible: index > 0
                text: "/"
                color: UiStyle.colorTextFaint
                font.family: UiStyle.fontSans
                font.pixelSize: UiStyle.fontSizeSm
            }
            Text {
                text: modelData.label
                color: index === breadcrumb.items.length - 1 ? UiStyle.colorText : UiStyle.colorAccent
                font.family: UiStyle.fontSans
                font.pixelSize: UiStyle.fontSizeSm
                font.weight: index === breadcrumb.items.length - 1 ? UiStyle.fontWeightSemiBold : UiStyle.fontWeightMedium
                elide: Text.ElideRight
                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: if (breadcrumb.controller) breadcrumb.controller.selectRoute(modelData.id)
                }
            }
        }
    }
}

