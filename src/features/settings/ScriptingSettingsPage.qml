import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../../style"
import "../../components"

ScrollView {
    id: root

    property var controller: null

    clip: true

    function rowHeight(count) {
        return UiStyle.space12 * 2 + UiStyle.sectionHeaderHeight + count * 24 + Math.max(0, count - 1) * UiStyle.space4
    }

    ColumnLayout {
        width: Math.max(parent.width - 18, 540)
        spacing: UiStyle.space10

        UiPanel {
            Layout.fillWidth: true
            implicitHeight: root.rowHeight(4)

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: UiStyle.space12
                spacing: UiStyle.space4

                UiSectionHeader {
                    title: "Runtime Boundary"
                    Layout.fillWidth: true
                }

                UiListRow { Layout.fillWidth: true; label: "Canvas contracts"; meta: "C++ active"; metaMaxWidth: 180 }
                UiListRow { Layout.fillWidth: true; label: "QML role"; meta: "view adapter"; metaMaxWidth: 180 }
                UiListRow { Layout.fillWidth: true; label: "JS canvas modules"; meta: "retiring"; metaMaxWidth: 180 }
                UiListRow { Layout.fillWidth: true; label: "Lua runtime"; meta: "not enabled"; metaMaxWidth: 180 }
            }
        }

        UiPanel {
            Layout.fillWidth: true
            implicitHeight: root.rowHeight(5)

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: UiStyle.space12
                spacing: UiStyle.space4

                UiSectionHeader {
                    title: "Harness Policy"
                    Layout.fillWidth: true
                }

                UiListRow { Layout.fillWidth: true; label: "Legacy JS/QML tests"; meta: "off by default"; metaMaxWidth: 180 }
                UiListRow { Layout.fillWidth: true; label: "Telemetry capture"; meta: "off"; metaMaxWidth: 180 }
                UiListRow { Layout.fillWidth: true; label: "Workflow runner"; meta: "guarded"; metaMaxWidth: 180 }
                UiListRow { Layout.fillWidth: true; label: "Raw metrics"; meta: "do not read"; metaMaxWidth: 180 }
                UiListRow { Layout.fillWidth: true; label: "Compact reports"; meta: "allowed"; metaMaxWidth: 180 }
            }
        }

        UiPanel {
            Layout.fillWidth: true
            implicitHeight: root.rowHeight(4)

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: UiStyle.space12
                spacing: UiStyle.space4

                UiSectionHeader {
                    title: "Scripting Surface"
                    Layout.fillWidth: true
                }

                UiListRow { Layout.fillWidth: true; label: "Product toggles"; meta: "none"; metaMaxWidth: 180 }
                UiListRow { Layout.fillWidth: true; label: "Fixture scripts"; meta: "read-only"; metaMaxWidth: 180 }
                UiListRow { Layout.fillWidth: true; label: "Control runner"; meta: "test bridge"; metaMaxWidth: 180 }
                UiListRow { Layout.fillWidth: true; label: "Persistence"; meta: "planned"; metaMaxWidth: 180 }
            }
        }

        UiPanel {
            Layout.fillWidth: true
            implicitHeight: 160

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: UiStyle.space12
                spacing: UiStyle.space6

                UiSectionHeader {
                    title: "Boundary Files"
                    Layout.fillWidth: true
                }

                UiCodeRefRow {
                    Layout.fillWidth: true
                    path: "CMakeLists.txt"
                    role: "test registration"
                }

                UiCodeRefRow {
                    Layout.fillWidth: true
                    path: "AGENTS.md"
                    role: "agent policy"
                }

                Text {
                    Layout.fillWidth: true
                    text: "This page is read-only until scripting becomes a product feature with durable settings and explicit ownership."
                    color: UiStyle.colorTextFaint
                    font.family: UiStyle.fontSans
                    font.pixelSize: UiStyle.fontSizeSm
                    wrapMode: Text.WordWrap
                }
            }
        }

        Item { Layout.fillHeight: true }
    }
}
