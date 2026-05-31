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
        color: UiStyle.colorControl
        border.width: UiStyle.borderThin
        border.color: area.activeFocus ? UiStyle.colorBorderFocus : UiStyle.colorBorderMinor
        radius: UiStyle.radiusSm
    }
}

