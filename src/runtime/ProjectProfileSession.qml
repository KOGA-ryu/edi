import QtQuick

QtObject {
    id: projectProfileSession

    property string projectProfilePath: ""
    property var projectProfileDocument: ({})
    property string projectId: "draftsman_blank"
    property string projectTitle: "Draftsman"
    property string projectType: "blank_shell"
    property string projectRootPath: ""
    property string projectSummary: "Reusable blank Draftsman shell."
    property string settingsNavLabel: "Theme and layout"
    property string mainWorkspaceFeature: "blank_canvas"
    property string rightInspectorSource: "none"
    property var projectPanelDefaults: ({})
    property bool writeDisabled: true
    property var activityModes: [
        { id: "blank", label: "Blank", icon: "B", tooltip: "Blank workspace", exclusiveGroup: "" },
        { id: "review", label: "Review", icon: "R", tooltip: "Review gate workspace", exclusiveGroup: "" },
        { id: "settings", label: "Settings", icon: "S", tooltip: "Settings surface", exclusiveGroup: "system" },
        { id: "proof", label: "Proof", icon: "P", tooltip: "Proof and receipts", exclusiveGroup: "" }
    ]
    property var leftProjectRows: [
        { label: "Project slot", meta: "blank" },
        { label: "Scratch", meta: "workflow" },
        { label: "Final", meta: "workflow" }
    ]
    property var shelfTabs: []
    property string selectedShelfTab: ""
    property var customActions: []
    property string customActionStatus: ""
    property string customActionOutputPath: ""
    property string defaultActivityMode: ""

    signal changed()

    function setProjectProfileState(profilePath, document) {
        projectProfilePath = String(profilePath || "")
        projectProfileDocument = document || ({})
    }

    function asArray(value) {
        if (!value) {
            return []
        }
        if (Array.isArray(value)) {
            return value
        }
        if (typeof value.length === "number") {
            var result = []
            for (var index = 0; index < value.length; ++index) {
                result.push(value[index])
            }
            return result
        }
        return []
    }

    function defaultActivityModes() {
        return [
            { id: "blank", label: "Blank", icon: "B", tooltip: "Blank workspace", exclusiveGroup: "" },
            { id: "review", label: "Review", icon: "R", tooltip: "Review gate workspace", exclusiveGroup: "" },
            { id: "settings", label: "Settings", icon: "S", tooltip: "Settings surface", exclusiveGroup: "system" },
            { id: "proof", label: "Proof", icon: "P", tooltip: "Proof and receipts", exclusiveGroup: "" }
        ]
    }

    function defaultActivityExclusiveGroup(modeId) {
        var id = String(modeId || "")
        if (id === "map_generator"
                || id === "map_editor"
                || id === "drawing_tool"
                || id === "drawing_drafting"
                || id === "blender_scripts"
                || id === "tool_workspace") {
            return "tool_type"
        }
        if (id === "settings") {
            return "system"
        }
        return ""
    }

    function normalizeActivityModes(source) {
        var modes = []
        var sourceModes = asArray(source)
        for (var index = 0; index < sourceModes.length; ++index) {
            var mode = sourceModes[index]
            if (mode && mode.enabled !== false && String(mode.id || "").length > 0) {
                var id = String(mode.id)
                modes.push({
                    id: id,
                    label: String(mode.label || mode.id),
                    icon: String(mode.icon || String(mode.label || mode.id).charAt(0).toUpperCase()),
                    tooltip: String(mode.tooltip || mode.label || mode.id),
                    exclusiveGroup: String(mode.exclusive_group || mode.exclusiveGroup || defaultActivityExclusiveGroup(id))
                })
            }
        }
        return modes.length > 0 ? modes : defaultActivityModes()
    }

    function normalizeProjectRows(source) {
        var rows = []
        var sourceRows = asArray(source)
        for (var index = 0; index < sourceRows.length; ++index) {
            var row = sourceRows[index]
            if (row && String(row.label || "").length > 0) {
                rows.push({
                    label: String(row.label),
                    meta: String(row.meta || "")
                })
            }
        }
        return rows.length > 0 ? rows : [
            { label: "Project slot", meta: "blank" },
            { label: "Scratch", meta: "workflow" },
            { label: "Final", meta: "workflow" }
        ]
    }

    function normalizeShelfTabs(source) {
        var result = []
        if (Array.isArray(source) && source.length === 0) {
            return result
        }
        var tabs = asArray(source)
        for (var index = 0; index < tabs.length; ++index) {
            var tab = String(tabs[index] || "").trim()
            if (tab.length > 0) {
                result.push(tab)
            }
        }
        return result.length > 0 ? result : ["Output", "Proof", "Receipts", "Log"]
    }

    function normalizeCustomActions(source) {
        var actions = asArray(source)
        var result = []
        var seen = ({})
        for (var index = 0; index < actions.length; ++index) {
            var action = actions[index] || ({})
            var id = String(action.id || "").trim()
            var label = String(action.label || "").trim()
            var handler = String(action.handler || "").trim()
            if (!id.length || !label.length || !handler.length || seen[id]) {
                continue
            }
            seen[id] = true
            result.push({
                id: id,
                label: label,
                menu: String(action.menu || "Tools").trim() || "Tools",
                activity: String(action.activity || "").trim(),
                handler: handler,
                enabled: action.enabled !== false,
                args: action.args || ({})
            })
        }
        return result
    }

    function normalizeReviewModeTabs(source) {
        return normalizeShelfTabs(source)
    }

    function activityModeAvailable(modeId) {
        for (var index = 0; index < activityModes.length; ++index) {
            if (activityModes[index].id === modeId) {
                return true
            }
        }
        return false
    }

    function hasActivityMode(modeId) {
        return activityModeAvailable(modeId)
    }

    function customActionVisible(action, activeMode) {
        if (!action || action.enabled === false) {
            return false
        }
        var mode = String(activeMode || "")
        if (action.activity && action.activity !== mode) {
            return false
        }
        return true
    }

    function menuCustomActions(menuName, unusedRevision, activeMode) {
        var menu = String(menuName || "")
        var result = []
        for (var index = 0; index < customActions.length; ++index) {
            var action = customActions[index]
            if (String(action.menu || "") === menu && customActionVisible(action, activeMode)) {
                result.push(action)
            }
        }
        return result
    }

    function customActionById(actionId) {
        var id = String(actionId || "")
        for (var index = 0; index < customActions.length; ++index) {
            if (customActions[index].id === id) {
                return customActions[index]
            }
        }
        return null
    }

    function setCustomActionResult(status, outputPath) {
        customActionStatus = String(status || "")
        customActionOutputPath = String(outputPath || "")
        changed()
    }

    function selectShelfTab(tabName) {
        var next = String(tabName || "")
        if (selectedShelfTab === next) {
            return
        }
        selectedShelfTab = next
        changed()
    }

    function loadProjectProfile(document) {
        var profileDocument = document ? document : ({})
        var profile = profileDocument && profileDocument.profile ? profileDocument.profile : ({})
        var leftPanel = profileDocument && profileDocument.left_panel ? profileDocument.left_panel : ({})
        var mainWorkspace = profileDocument && profileDocument.main_workspace ? profileDocument.main_workspace : ({})
        var rightInspector = profileDocument && profileDocument.right_inspector ? profileDocument.right_inspector : ({})
        var panelDefaults = profileDocument && profileDocument.panel_defaults ? profileDocument.panel_defaults : ({})
        var writePolicy = profileDocument && profileDocument.write_policy ? profileDocument.write_policy : ({})
        var activitySections = profileDocument && profileDocument.activity_modes
        var rowRows = leftPanel.project_rows
        var projectTabs = profileDocument && profileDocument.bottom_panel ? (profileDocument.bottom_panel.tabs || ({}) ) : ({})

        projectProfileDocument = profileDocument
        projectId = String(profile.profile_id || "draftsman_blank")
        projectTitle = String(profile.label || "Draftsman")
        projectType = String(profile.type || "blank_shell")
        projectRootPath = String(profile.root_path || "")
        projectSummary = String(profile.summary || "Reusable blank Draftsman shell.")
        settingsNavLabel = String(leftPanel.settings_label || "Theme and layout")
        mainWorkspaceFeature = String(mainWorkspace.feature || "blank_canvas")
        rightInspectorSource = String(rightInspector.source || "none")
        projectPanelDefaults = panelDefaults
        writeDisabled = typeof writePolicy.writes_enabled === "boolean" ? !writePolicy.writes_enabled : true

        activityModes = normalizeActivityModes(activitySections)
        leftProjectRows = normalizeProjectRows(rowRows)
        shelfTabs = normalizeShelfTabs(projectTabs)
        if (shelfTabs.length > 0 && shelfTabs.indexOf(selectedShelfTab) >= 0) {
            selectedShelfTab = shelfTabs[shelfTabs.indexOf(selectedShelfTab)]
        } else {
            selectedShelfTab = shelfTabs[0] || ""
        }

        customActions = normalizeCustomActions(profileDocument && profileDocument.custom_actions)
        defaultActivityMode = String(profile.default_activity || "")
        changed()
    }
}
