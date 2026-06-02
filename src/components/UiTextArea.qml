import QtQuick
import QtQuick.Controls
import "../style"

TextArea {
    id: area
    color: UiStyle.colorText
    placeholderTextColor: UiStyle.colorTextFaint
    font.family: UiStyle.fontSans
    font.pixelSize: UiStyle.fontSizeBody
    selectedTextColor: UiStyle.colorWindow
    selectionColor: UiStyle.colorAccent
    wrapMode: TextArea.Wrap
    background: Rectangle {
        color: area.activeFocus ? UiStyle.mix(UiStyle.colorControl, UiStyle.colorAccent, 0.12) : UiStyle.colorControl
        border.width: area.activeFocus ? UiStyle.borderThin : UiStyle.borderNone
        border.color: area.activeFocus ? UiStyle.colorBorderFocus : UiStyle.colorTransparent
        radius: UiStyle.radiusSm
    }
}
