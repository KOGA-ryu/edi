import QtQuick
import QtQuick.Controls
import "../style"

TextArea {
    id: area
    property color fillColor: UiStyle.colorControl

    color: UiStyle.colorText
    placeholderTextColor: UiStyle.colorTextFaint
    font.family: UiStyle.fontSans
    font.pixelSize: UiStyle.fontSizeBody
    selectedTextColor: UiStyle.colorWindow
    selectionColor: UiStyle.colorAccent
    wrapMode: TextArea.Wrap
    background: Rectangle {
        color: area.fillColor
        border.width: UiStyle.borderNone
        border.color: UiStyle.colorTransparent
        radius: UiStyle.radiusSm
    }
}
