import QtQuick
import QtQuick.Controls
import "../style"

Rectangle {
    id: menuButton

    property string label: "Menu"
    default property alias menuItems: popup.contentData

    implicitWidth: labelText.implicitWidth + UiStyle.space16
    implicitHeight: 28
    radius: UiStyle.radiusSm
    color: hovered || popup.visible ? UiStyle.colorControlHover : UiStyle.colorTransparent
    border.width: UiStyle.borderNone

    property bool hovered: false

    Text {
        id: labelText
        anchors.centerIn: parent
        text: menuButton.label
        color: menuButton.hovered || popup.visible ? UiStyle.colorText : UiStyle.colorTextMuted
        font.family: UiStyle.fontSans
        font.pixelSize: UiStyle.fontSizeSm
        font.weight: menuButton.hovered || popup.visible ? UiStyle.fontWeightSemiBold : UiStyle.fontWeightMedium
    }

    MouseArea {
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onEntered: menuButton.hovered = true
        onExited: menuButton.hovered = false
        onClicked: popup.open()
    }

    Menu {
        id: popup
        y: menuButton.height + UiStyle.space2
        width: Math.max(190, implicitWidth)
        modal: false
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutsideParent

        background: Rectangle {
            color: UiStyle.colorPanelAlt
            radius: UiStyle.radiusSm
            border.width: UiStyle.borderThin
            border.color: UiStyle.colorBorderMinor
        }

        delegate: MenuItem {
            id: item
            implicitWidth: 190
            implicitHeight: 28

            contentItem: Text {
                text: item.text
                color: item.enabled ? (item.highlighted ? UiStyle.colorText : UiStyle.colorTextMuted) : UiStyle.colorDisabled
                font.family: UiStyle.fontSans
                font.pixelSize: UiStyle.fontSizeSm
                font.weight: item.highlighted ? UiStyle.fontWeightSemiBold : UiStyle.fontWeightMedium
                elide: Text.ElideRight
                verticalAlignment: Text.AlignVCenter
            }

            background: Rectangle {
                color: item.highlighted && item.enabled ? UiStyle.colorControlHover : UiStyle.colorTransparent
                radius: UiStyle.radiusSm
            }
        }
    }
}
