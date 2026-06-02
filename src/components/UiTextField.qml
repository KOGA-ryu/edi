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
        color: parent.activeFocus ? UiStyle.mix(UiStyle.colorControl, UiStyle.colorAccent, 0.12) : UiStyle.colorControl
        border.width: parent.activeFocus ? UiStyle.borderThin : UiStyle.borderNone
        border.color: parent.activeFocus ? UiStyle.colorBorderFocus : UiStyle.colorTransparent
        radius: UiStyle.radiusSm
    }
}
