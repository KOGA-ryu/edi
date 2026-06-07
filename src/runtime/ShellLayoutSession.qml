import QtQuick
import "../style"

QtObject {
    id: shellLayoutSession

    property var targetRoot: null
    property bool leftPanelCollapsed: false
    property bool rightPanelCollapsed: true
    property bool bottomPanelCollapsed: true
    property int leftPanelWidth: UiStyle.leftPanelWidth
    property int rightPanelWidth: UiStyle.rightPanelWidth
    property int bottomPanelHeight: UiStyle.bottomPanelHeight
    property int leftPanelMinWidth: 180
    property int leftPanelMaxWidth: 520
    property int rightPanelMinWidth: 160
    property int rightPanelMaxWidth: 2400
    property int bottomPanelMinHeight: 96
    property int bottomPanelMaxHeight: 1000
    property int leftPanelAutoHideWidth: 640
    property int rightPanelAutoHideWidth: 0
    property int bottomPanelAutoHideHeight: 520
    property var rightInspectorSections: ({
        facts: true,
        selection: true,
        code_refs: true,
        notes: true,
        receipts: true,
        actions: true
    })
    property bool shellLayoutDirty: false
    property bool shellLayoutSaveOk: true
    property string shellLayoutPath: ""

    signal changed()

    function clamp(value, low, high) {
        return Math.max(low, Math.min(high, Math.round(Number(value))))
    }

    function policyInt(source, key, fallback, low, high) {
        var value = source && Number.isFinite(Number(source[key])) ? Number(source[key]) : fallback
        return clamp(value, low, high)
    }

    function markShellLayoutDirty() {
        shellLayoutDirty = true
        changed()
    }

    function loadShellLayout(document, inspectorDefaults) {
        var policy = document && document.policy ? document.policy : ({})
        var leftPolicy = policy.left || ({})
        var rightPolicy = policy.right || ({})
        var bottomPolicy = policy.bottom || ({})
        var rightPanel = document && document.right_panel ? document.right_panel : ({})
        var panels = document && document.panels ? document.panels : ({})
        var left = panels.left || ({})
        var right = panels.right || ({})
        var bottom = panels.bottom || ({})

        leftPanelMinWidth = policyInt(leftPolicy, "min_width", 180, 120, 900)
        leftPanelMaxWidth = policyInt(leftPolicy, "max_width", 520, leftPanelMinWidth, 1200)
        rightPanelMinWidth = policyInt(rightPolicy, "min_width", 160, 120, 900)
        rightPanelMaxWidth = policyInt(rightPolicy, "max_width", 2400, rightPanelMinWidth, 2400)
        bottomPanelMinHeight = policyInt(bottomPolicy, "min_height", 96, 60, 700)
        bottomPanelMaxHeight = policyInt(bottomPolicy, "max_height", 360, bottomPanelMinHeight, 1800)
        leftPanelAutoHideWidth = policyInt(leftPolicy, "auto_hide_below_width", 640, 0, 2400)
        rightPanelAutoHideWidth = policyInt(rightPolicy, "auto_hide_below_width", 0, 0, 2400)
        bottomPanelAutoHideHeight = policyInt(bottomPolicy, "auto_hide_below_height", 520, 0, 1800)
        leftPanelCollapsed = !!left.collapsed
        rightPanelCollapsed = typeof right.collapsed === "boolean" ? right.collapsed : true
        bottomPanelCollapsed = typeof bottom.collapsed === "boolean" ? bottom.collapsed : true
        rightInspectorSections = normalizedInspectorSections(rightPanel.sections, inspectorDefaults)
        leftPanelWidth = clamp(left.width || UiStyle.leftPanelWidth, leftPanelMinWidth, leftPanelMaxWidth)
        rightPanelWidth = clamp(right.width || UiStyle.rightPanelWidth, rightPanelMinWidth, rightPanelMaxWidth)
        bottomPanelHeight = clamp(bottom.height || UiStyle.bottomPanelHeight, bottomPanelMinHeight, bottomPanelMaxHeight)
        shellLayoutDirty = false
        shellLayoutSaveOk = true
    }

    function shellLayoutDocument() {
        return {
            window: {
                width: Math.round(windowWidth()),
                height: Math.round(windowHeight())
            },
            policy: {
                left: {
                    min_width: leftPanelMinWidth,
                    max_width: leftPanelMaxWidth,
                    auto_hide_below_width: leftPanelAutoHideWidth
                },
                right: {
                    min_width: rightPanelMinWidth,
                    max_width: rightPanelMaxWidth,
                    auto_hide_below_width: rightPanelAutoHideWidth
                },
                bottom: {
                    min_height: bottomPanelMinHeight,
                    max_height: bottomPanelMaxHeight,
                    auto_hide_below_height: bottomPanelAutoHideHeight
                }
            },
            right_panel: {
                sections: rightInspectorSections
            },
            panels: {
                left: {
                    collapsed: leftPanelCollapsed,
                    width: leftPanelWidth
                },
                right: {
                    collapsed: rightPanelCollapsed,
                    width: rightPanelWidth
                },
                bottom: {
                    collapsed: bottomPanelCollapsed,
                    height: bottomPanelHeight
                }
            }
        }
    }

    function normalizedInspectorSections(source, fallback) {
        var result
        if (fallback && typeof fallback === "object") {
            result = Object.assign({}, fallback)
        } else {
            result = {
                facts: true,
                selection: true,
                code_refs: true,
                notes: true,
                receipts: true,
                actions: true
            }
        }
        if (!source) {
            return result
        }
        var keys = Object.keys(result)
        for (var index = 0; index < keys.length; ++index) {
            var key = keys[index]
            if (typeof source[key] === "boolean") {
                result[key] = source[key]
            }
        }
        return result
    }

    function saveShellLayout() {
        if (typeof shellLayoutStore === "undefined" || !shellLayoutStore) {
            shellLayoutSaveOk = false
            changed()
            return false
        }
        shellLayoutSaveOk = shellLayoutStore.save(shellLayoutDocument())
        if (shellLayoutSaveOk) {
            shellLayoutDirty = false
        }
        changed()
        return shellLayoutSaveOk
    }

    function setInspectorSections(sections, inspectorDefaults) {
        rightInspectorSections = normalizedInspectorSections(sections, inspectorDefaults)
        changed()
    }

    function applyProjectPanelDefaults(defaults) {
        if (!defaults) {
            return
        }

        var didChange = false

        if (typeof defaults.left_collapsed === "boolean" && leftPanelCollapsed !== defaults.left_collapsed) {
            leftPanelCollapsed = defaults.left_collapsed
            didChange = true
        }
        if (typeof defaults.right_collapsed === "boolean" && rightPanelCollapsed !== defaults.right_collapsed) {
            rightPanelCollapsed = defaults.right_collapsed
            didChange = true
        }
        if (typeof defaults.bottom_collapsed === "boolean" && bottomPanelCollapsed !== defaults.bottom_collapsed) {
            bottomPanelCollapsed = defaults.bottom_collapsed
            didChange = true
        }

        if (didChange) {
            changed()
        }
    }

    function resetShellLayout() {
        leftPanelCollapsed = false
        rightPanelCollapsed = true
        bottomPanelCollapsed = true
        resetPanelSizes(true)
        markShellLayoutDirty()
    }

    function resetPanelSizes(skipNotify) {
        leftPanelWidth = clamp(UiStyle.leftPanelWidth, leftPanelMinWidth, leftPanelMaxWidth)
        rightPanelWidth = clamp(UiStyle.rightPanelWidth, rightPanelMinWidth, rightPanelMaxWidth)
        bottomPanelHeight = clamp(UiStyle.bottomPanelHeight, bottomPanelMinHeight, bottomPanelMaxHeight)
        if (!skipNotify) {
            markShellLayoutDirty()
        }
    }

    function windowWidth() {
        return targetRoot ? Number(targetRoot.width) : UiStyle.windowWidth
    }

    function windowHeight() {
        return targetRoot ? Number(targetRoot.height) : UiStyle.windowHeight
    }

    function panelManualCollapsed(panelId) {
        if (panelId === "left") {
            return leftPanelCollapsed
        }
        if (panelId === "right") {
            return rightPanelCollapsed
        }
        if (panelId === "bottom") {
            return bottomPanelCollapsed
        }
        return false
    }

    function panelAutoHidden(panelId) {
        if (panelManualCollapsed(panelId)) {
            return false
        }
        if (panelId === "left") {
            return windowWidth() < leftPanelAutoHideWidth
        }
        if (panelId === "right") {
            return windowWidth() < rightPanelAutoHideWidth
        }
        if (panelId === "bottom") {
            return windowHeight() < bottomPanelAutoHideHeight
        }
        return false
    }

    function panelVisible(panelId) {
        return !panelManualCollapsed(panelId) && !panelAutoHidden(panelId)
    }

    function panelState(panelId) {
        if (panelManualCollapsed(panelId)) {
            return "collapsed"
        }
        if (panelAutoHidden(panelId)) {
            return "auto_hidden"
        }
        return "visible"
    }

    function panelStateLabel(panelId) {
        var state = panelState(panelId)
        if (state === "collapsed") {
            return "manual collapsed"
        }
        if (state === "auto_hidden") {
            return "auto-hidden"
        }
        return "visible"
    }

    function panelStateDetail(panelId) {
        if (panelId === "left") {
            return panelAutoHidden(panelId) ? "auto-hidden below " + String(leftPanelAutoHideWidth) + "px width" : String(leftPanelWidth) + " px"
        }
        if (panelId === "right") {
            return panelAutoHidden(panelId) ? "auto-hidden below " + String(rightPanelAutoHideWidth) + "px width" : String(rightPanelWidth) + " px"
        }
        if (panelId === "bottom") {
            return panelAutoHidden(panelId) ? "auto-hidden below " + String(bottomPanelAutoHideHeight) + "px height" : String(bottomPanelHeight) + " px"
        }
        return ""
    }

    function applyLayoutPreset(presetId) {
        if (presetId === "full") {
            leftPanelCollapsed = false
            rightPanelCollapsed = false
            bottomPanelCollapsed = false
            resetPanelSizes(true)
        } else if (presetId === "focus") {
            leftPanelCollapsed = true
            rightPanelCollapsed = true
            bottomPanelCollapsed = true
        } else if (presetId === "review") {
            leftPanelCollapsed = false
            rightPanelCollapsed = true
            bottomPanelCollapsed = true
            resetPanelSizes(true)
        } else if (presetId === "tiny") {
            leftPanelCollapsed = true
            rightPanelCollapsed = true
            bottomPanelCollapsed = true
            resetPanelSizes(true)
        }
        markShellLayoutDirty()
    }

    function toggleLeftPanel() {
        leftPanelCollapsed = !leftPanelCollapsed
        markShellLayoutDirty()
    }

    function toggleRightPanel() {
        rightPanelCollapsed = !rightPanelCollapsed
        markShellLayoutDirty()
    }

    function toggleBottomPanel() {
        bottomPanelCollapsed = !bottomPanelCollapsed
        markShellLayoutDirty()
    }

    function setLeftPanelCollapsed(collapsed) {
        leftPanelCollapsed = !!collapsed
        markShellLayoutDirty()
    }

    function setRightPanelCollapsed(collapsed) {
        rightPanelCollapsed = !!collapsed
        markShellLayoutDirty()
    }

    function setBottomPanelCollapsed(collapsed) {
        bottomPanelCollapsed = !!collapsed
        markShellLayoutDirty()
    }

    function setLeftPanelWidth(width) {
        leftPanelWidth = clamp(width, leftPanelMinWidth, leftPanelMaxWidth)
        markShellLayoutDirty()
    }

    function setRightPanelWidth(width) {
        rightPanelWidth = clamp(width, rightPanelMinWidth, rightPanelMaxWidth)
        markShellLayoutDirty()
    }

    function setBottomPanelHeight(height) {
        bottomPanelHeight = clamp(height, bottomPanelMinHeight, bottomPanelMaxHeight)
        markShellLayoutDirty()
    }

    function setInspectorSectionVisible(sectionId, visible) {
        var next = normalizedInspectorSections(rightInspectorSections, {
            facts: true,
            selection: true,
            code_refs: true,
            notes: true,
            receipts: true,
            actions: true
        })
        next[String(sectionId)] = !!visible
        rightInspectorSections = next
        markShellLayoutDirty()
    }
}
