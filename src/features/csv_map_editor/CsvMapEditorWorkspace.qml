import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../../style"

Rectangle {
    id: root

    property var controller: null
    property int controllerRevision: controller ? controller.revision : 0
    readonly property int rows: controller ? controller.mapRowCount() : 0
    readonly property int cols: controller ? controller.mapColCount() : 0
    readonly property int cellSize: Math.max(36, Math.min(64, Math.floor((width - UiStyle.space16) / Math.max(1, cols))))

    color: UiStyle.colorWorkspace
    border.width: UiStyle.borderNone

    function tokenColor(token) {
        if (token === "wall") return UiStyle.mix(UiStyle.colorSurface, UiStyle.colorText, 0.04)
        if (token === "floor") return UiStyle.mix(UiStyle.colorSurface, UiStyle.colorText, 0.10)
        if (token === "door") return UiStyle.mix(UiStyle.colorSurface, UiStyle.colorWarning, 0.24)
        if (token === "start") return UiStyle.mix(UiStyle.colorSurface, UiStyle.colorSuccess, 0.24)
        if (token === "secret") return UiStyle.mix(UiStyle.colorSurface, UiStyle.colorAccent, 0.26)
        if (token === "encounter") return UiStyle.mix(UiStyle.colorSurface, UiStyle.colorDanger, 0.22)
        return UiStyle.colorControl
    }

    function tokenShort(token) {
        if (token === "encounter") return "enc"
        return token
    }

    Flickable {
        anchors.fill: parent
        anchors.margins: UiStyle.space8
        clip: true
        boundsBehavior: Flickable.StopAtBounds
        contentWidth: grid.implicitWidth
        contentHeight: grid.implicitHeight

        GridLayout {
            id: grid
            columns: Math.max(1, root.cols)
            columnSpacing: UiStyle.space2
            rowSpacing: UiStyle.space2

            Repeater {
                model: root.rows * root.cols

                delegate: Rectangle {
                    id: cell

                    readonly property int rowIndex: Math.floor(index / Math.max(1, root.cols))
                    readonly property int colIndex: index % Math.max(1, root.cols)
                    readonly property string token: root.controller ? root.controller.mapTokenAt(rowIndex, colIndex) : ""
                    readonly property bool selected: root.controller && root.controller.selectedMapRow === rowIndex && root.controller.selectedMapCol === colIndex

                    Layout.preferredWidth: root.cellSize
                    Layout.preferredHeight: root.cellSize
                    color: cell.selected ? UiStyle.mix(root.tokenColor(cell.token), UiStyle.colorAccent, 0.34) : root.tokenColor(cell.token)
                    border.width: cell.selected ? UiStyle.borderThin : UiStyle.borderNone
                    border.color: UiStyle.colorAccent
                    radius: UiStyle.radiusSm

                    Text {
                        anchors.centerIn: parent
                        width: parent.width - UiStyle.space4
                        text: root.tokenShort(cell.token)
                        color: cell.selected ? UiStyle.colorText : UiStyle.colorTextMuted
                        font.family: UiStyle.fontMono
                        font.pixelSize: UiStyle.fontSizeXs
                        horizontalAlignment: Text.AlignHCenter
                        elide: Text.ElideRight
                    }

                    ToolTip.visible: mouseArea.containsMouse
                    ToolTip.text: String(cell.rowIndex) + "," + String(cell.colIndex) + " " + cell.token
                    ToolTip.delay: 1200
                    ToolTip.timeout: 6000

                    MouseArea {
                        id: mouseArea
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: if (root.controller) root.controller.selectMapCell(cell.rowIndex, cell.colIndex)
                    }
                }
            }
        }
    }
}
