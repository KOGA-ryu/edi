import QtQuick
import QtQuick.Controls
import "../style"

UiButton {
    compact: true
    property string tooltip: label
    ToolTip.visible: hovered && tooltip.length > 0
    ToolTip.text: tooltip
}
