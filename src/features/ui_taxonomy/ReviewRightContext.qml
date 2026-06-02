import QtQuick
import QtQuick.Layouts

import "../inspector"

RightInspector {
    id: root

    property int controllerRevision: controller ? controller.revision : 0

    document: controller ? controller.rightInspectorDocument(controllerRevision) : ({})
}
