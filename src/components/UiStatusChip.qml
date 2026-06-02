import QtQuick
import "../style"

Rectangle {
    id: chip
    property string status: "pending"
    property string label: status === "needs_rework" ? "needs rework" : status

    implicitHeight: 20
    implicitWidth: Math.max(56, text.implicitWidth + UiStyle.space12)
    radius: UiStyle.radiusSm
    color: {
        if (status === "accepted") return Qt.rgba(0.36, 0.61, 0.42, 0.24)
        if (status === "needs_rework") return Qt.rgba(0.76, 0.60, 0.28, 0.24)
        if (status === "rejected") return Qt.rgba(0.74, 0.33, 0.33, 0.24)
        return Qt.rgba(0.42, 0.55, 0.70, 0.20)
    }
    border.width: UiStyle.borderThin
    border.color: {
        if (status === "accepted") return UiStyle.colorSuccess
        if (status === "needs_rework") return UiStyle.colorWarning
        if (status === "rejected") return UiStyle.colorDanger
        return UiStyle.colorPending
    }

    Text {
        id: text
        anchors.centerIn: parent
        text: chip.label
        color: chip.border.color
        font.family: UiStyle.fontSans
        font.pixelSize: UiStyle.fontSizeXs
        font.weight: UiStyle.fontWeightSemiBold
        elide: Text.ElideRight
        width: parent.width - UiStyle.space6
        horizontalAlignment: Text.AlignHCenter
    }
}
