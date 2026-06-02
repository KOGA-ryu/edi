import QtQuick
import QtQuick.Controls
import "../style"

TextField {
    color: UiStyle.colorText
    placeholderTextColor: UiStyle.colorTextFaint
    font.family: UiStyle.fontSans
    font.pixelSize: UiStyle.fontSizeBody
    selectedTextColor: UiStyle.colorWindow
    selectionColor: UiStyle.colorAccent
    background: Rectangle {
        color: UiStyle.colorControl
        border.width: UiStyle.borderNone
        border.color: UiStyle.colorTransparent
        radius: UiStyle.radiusSm
    }
}
