import QtQuick
import "../style"

QtObject {
    id: runtimeController

    property var targetRoot: null
    property string activityMode: "review"
    property string selectedSubjectId: "draftsman_ui_taxonomy"
    property string selectedSubjectLabel: "Draftsman UI Taxonomy"
    property string selectedRouteId: "draftsman_ui"
    property string selectedLocalTab: "overview"
    property string selectedShelfTab: "Output"
    property string selectedSettingsPage: "theme"
    property bool writeDisabled: true
    property string projectProfilePath: ""
    property var projectProfileDocument: ({})
    property string projectId: "draftsman_blank"
    property string projectTitle: "Draftsman"
    property string projectType: "blank_shell"
    property string projectRootPath: ""
    property string projectSummary: "Reusable blank Draftsman shell."
    property string settingsNavLabel: "Theme and layout"
    property string mainWorkspaceFeature: "ui_taxonomy_review"
    property string rightInspectorSource: "selected_route"
    property var leftProjectRows: [
        { label: "Project slot", meta: "blank" },
        { label: "Scratch", meta: "workflow" },
        { label: "Final", meta: "workflow" }
    ]
    property bool leftPanelCollapsed: false
    property bool rightPanelCollapsed: true
    property bool bottomPanelCollapsed: true
    property int leftPanelWidth: UiStyle.leftPanelWidth
    property int rightPanelWidth: UiStyle.rightPanelWidth
    property int bottomPanelHeight: UiStyle.bottomPanelHeight
    property int leftPanelMinWidth: 180
    property int leftPanelMaxWidth: 520
    property int rightPanelMinWidth: 240
    property int rightPanelMaxWidth: 460
    property int bottomPanelMinHeight: 96
    property int bottomPanelMaxHeight: 360
    property int leftPanelAutoHideWidth: 640
    property int rightPanelAutoHideWidth: 520
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
    property int revision: 0

    property var backStack: []
    property var forwardStack: []
    property var statusOverrides: ({})
    property var notes: []
    property var reviewSubjectDocument: ({})
    property var uiThemeDocument: ({})
    property string uiThemePath: ""

    property var activityModes: [
        { id: "binder", label: "Binder", icon: "B", tooltip: "Binder workspace" },
        { id: "review", label: "Review", icon: "R", tooltip: "UI taxonomy review gate" },
        { id: "settings", label: "Settings", icon: "S", tooltip: "Settings surface" },
        { id: "proof", label: "Proof", icon: "P", tooltip: "Proof and receipts" }
    ]

    readonly property var localTabs: [
        { id: "overview", label: "Overview", tooltip: "Summary of the selected review route, including purpose, status counts, and nearby route cards." },
        { id: "objects", label: "Objects", tooltip: "Review the child routes, UI regions, panels, and objects attached to the selected route." },
        { id: "code", label: "Code", tooltip: "Show the source references and implementation files tied to the selected route." },
        { id: "prompts", label: "Prompts", tooltip: "Show the human and agent prompts that define what should be reviewed or changed." },
        { id: "notes", label: "Notes", tooltip: "Show review notes, decisions, and comments attached to the selected route." }
    ]
    property var shelfTabs: ["Output", "Proof", "Receipts", "Log"]

    property var routes: []
    property string rootRouteId: "draftsman_ui"

    Component.onCompleted: {
        projectProfilePath = typeof initialProjectProfilePath === "undefined" ? "" : String(initialProjectProfilePath)
        projectProfileDocument = typeof initialProjectProfile === "undefined" ? ({}) : initialProjectProfile
        loadProjectProfile(projectProfileDocument)

        uiThemeDocument = typeof initialUiTheme === "undefined" ? ({}) : initialUiTheme
        uiThemePath = typeof initialUiThemePath === "undefined" ? "" : String(initialUiThemePath)
        UiStyle.applyTheme(uiThemeDocument)
        shellLayoutPath = typeof initialShellLayoutPath === "undefined" ? "" : String(initialShellLayoutPath)
        loadShellLayout(typeof initialShellLayout === "undefined" ? ({}) : initialShellLayout)

        var document = typeof initialReviewSubject === "undefined" ? ({}) : initialReviewSubject
        reviewSubjectDocument = document
        loadReviewSubject(reviewSubjectDocument)
    }

    function clamp(value, low, high) {
        return Math.max(low, Math.min(high, Math.round(Number(value))))
    }

    function policyInt(source, key, fallback, low, high) {
        var value = source && Number.isFinite(Number(source[key])) ? Number(source[key]) : fallback
        return clamp(value, low, high)
    }

    function markShellLayoutDirty() {
        shellLayoutDirty = true
        revision += 1
    }

    function loadShellLayout(document) {
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
        rightPanelMinWidth = policyInt(rightPolicy, "min_width", 240, 120, 900)
        rightPanelMaxWidth = policyInt(rightPolicy, "max_width", 460, rightPanelMinWidth, 1200)
        bottomPanelMinHeight = policyInt(bottomPolicy, "min_height", 96, 60, 700)
        bottomPanelMaxHeight = policyInt(bottomPolicy, "max_height", 360, bottomPanelMinHeight, 1000)
        leftPanelAutoHideWidth = policyInt(leftPolicy, "auto_hide_below_width", 640, 0, 2400)
        rightPanelAutoHideWidth = policyInt(rightPolicy, "auto_hide_below_width", 520, 0, 2400)
        bottomPanelAutoHideHeight = policyInt(bottomPolicy, "auto_hide_below_height", 520, 0, 1800)
        leftPanelCollapsed = !!left.collapsed
        rightPanelCollapsed = typeof right.collapsed === "boolean" ? right.collapsed : true
        bottomPanelCollapsed = typeof bottom.collapsed === "boolean" ? bottom.collapsed : true
        rightInspectorSections = normalizedInspectorSections(rightPanel.sections)
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

    function normalizedInspectorSections(source) {
        var defaults = defaultInspectorSections()
        if (!source) {
            return defaults
        }
        var result = Object.assign({}, defaults)
        var keys = Object.keys(defaults)
        for (var index = 0; index < keys.length; ++index) {
            var key = keys[index]
            if (typeof source[key] === "boolean") {
                result[key] = source[key]
            }
        }
        return result
    }

    function defaultInspectorSections() {
        var inspector = projectProfileDocument && projectProfileDocument.right_inspector ? projectProfileDocument.right_inspector : ({})
        var source = inspector.sections || ({})
        var result = {
            facts: true,
            selection: true,
            code_refs: true,
            notes: true,
            receipts: true,
            actions: true
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

    function defaultActivityModes() {
        return [
            { id: "binder", label: "Binder", icon: "B", tooltip: "Binder workspace" },
            { id: "review", label: "Review", icon: "R", tooltip: "Review gate workspace" },
            { id: "settings", label: "Settings", icon: "S", tooltip: "Settings surface" },
            { id: "proof", label: "Proof", icon: "P", tooltip: "Proof and receipts" }
        ]
    }

    function normalizeActivityModes(source) {
        var modes = []
        var sourceModes = asArray(source)
        for (var index = 0; index < sourceModes.length; ++index) {
            var mode = sourceModes[index]
            if (mode && mode.enabled !== false && String(mode.id || "").length > 0) {
                modes.push({
                    id: String(mode.id),
                    label: String(mode.label || mode.id),
                    icon: String(mode.icon || String(mode.label || mode.id).charAt(0).toUpperCase()),
                    tooltip: String(mode.tooltip || mode.label || mode.id)
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
        var tabs = asArray(source)
        for (var index = 0; index < tabs.length; ++index) {
            var tab = String(tabs[index] || "").trim()
            if (tab.length > 0) {
                result.push(tab)
            }
        }
        return result.length > 0 ? result : ["Output", "Proof", "Receipts", "Log"]
    }

    function activityModeAvailable(modeId) {
        for (var index = 0; index < activityModes.length; ++index) {
            if (activityModes[index].id === modeId) {
                return true
            }
        }
        return false
    }

    function loadProjectProfile(document) {
        var profile = document && document.profile ? document.profile : ({})
        var leftPanel = document && document.left_panel ? document.left_panel : ({})
        var mainWorkspace = document && document.main_workspace ? document.main_workspace : ({})
        var rightInspector = document && document.right_inspector ? document.right_inspector : ({})
        var bottomPanel = document && document.bottom_panel ? document.bottom_panel : ({})
        var writePolicy = document && document.write_policy ? document.write_policy : ({})

        projectId = String(profile.profile_id || "draftsman_blank")
        projectTitle = String(profile.label || "Draftsman")
        projectType = String(profile.type || "blank_shell")
        projectRootPath = String(profile.root_path || "")
        projectSummary = String(profile.summary || "Reusable blank Draftsman shell.")
        settingsNavLabel = String(leftPanel.settings_label || "Theme and layout")
        mainWorkspaceFeature = String(mainWorkspace.feature || "ui_taxonomy_review")
        rightInspectorSource = String(rightInspector.source || "selected_route")
        writeDisabled = typeof writePolicy.writes_enabled === "boolean" ? !writePolicy.writes_enabled : true
        activityModes = normalizeActivityModes(document && document.activity_modes)
        leftProjectRows = normalizeProjectRows(leftPanel.project_rows)
        shelfTabs = normalizeShelfTabs(bottomPanel.tabs)
        selectedShelfTab = shelfTabs.indexOf(selectedShelfTab) >= 0 ? selectedShelfTab : shelfTabs[0]
        rightInspectorSections = normalizedInspectorSections(rightInspector.sections)

        var defaultActivity = String(profile.default_activity || activityMode)
        activityMode = activityModeAvailable(defaultActivity) ? defaultActivity : activityModes[0].id
    }

    function saveShellLayout() {
        if (typeof shellLayoutStore === "undefined" || !shellLayoutStore) {
            shellLayoutSaveOk = false
            return false
        }
        shellLayoutSaveOk = shellLayoutStore.save(shellLayoutDocument())
        if (shellLayoutSaveOk) {
            shellLayoutDirty = false
        }
        revision += 1
        return shellLayoutSaveOk
    }

    function resetShellLayout() {
        leftPanelCollapsed = false
        rightPanelCollapsed = true
        bottomPanelCollapsed = true
        resetPanelSizes()
        markShellLayoutDirty()
    }

    function resetPanelSizes() {
        leftPanelWidth = clamp(UiStyle.leftPanelWidth, leftPanelMinWidth, leftPanelMaxWidth)
        rightPanelWidth = clamp(UiStyle.rightPanelWidth, rightPanelMinWidth, rightPanelMaxWidth)
        bottomPanelHeight = clamp(UiStyle.bottomPanelHeight, bottomPanelMinHeight, bottomPanelMaxHeight)
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
            resetPanelSizes()
        } else if (presetId === "focus") {
            leftPanelCollapsed = true
            rightPanelCollapsed = true
            bottomPanelCollapsed = true
        } else if (presetId === "review") {
            leftPanelCollapsed = false
            rightPanelCollapsed = true
            bottomPanelCollapsed = true
            resetPanelSizes()
        } else if (presetId === "tiny") {
            leftPanelCollapsed = true
            rightPanelCollapsed = true
            bottomPanelCollapsed = true
            resetPanelSizes()
        }
        markShellLayoutDirty()
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

    function normalizeRoute(route) {
        var routeId = String(route.route_id || route.id || "")
        return {
            id: routeId,
            parent: String(route.parent || ""),
            label: String(route.label || routeId),
            type: String(route.type || "route"),
            status: String(route.status || "pending"),
            summary: String(route.summary || ""),
            purpose: String(route.purpose || route.summary || ""),
            objects: asArray(route.objects),
            children: asArray(route.children),
            codeRefs: asArray(route.code_refs || route.codeRefs),
            prompts: asArray(route.prompts)
        }
    }

    function fallbackRootRoute() {
        return {
            id: "draftsman_ui",
            parent: "",
            label: "Draftsman UI",
            type: "root",
            status: "pending",
            summary: "Review subject data could not be loaded.",
            purpose: "Check data/review_subjects/draftsman_ui_taxonomy.json.",
            objects: [],
            children: [],
            codeRefs: ["data/review_subjects/draftsman_ui_taxonomy.json"],
            prompts: ["Is the review subject JSON present and valid?"]
        }
    }

    function loadReviewSubject(document) {
        var subject = document && document.subject ? document.subject : ({})
        selectedSubjectId = String(subject.subject_id || "draftsman_ui_taxonomy")
        selectedSubjectLabel = String(subject.label || "Draftsman UI Taxonomy")
        rootRouteId = String(subject.root_route_id || "draftsman_ui")

        var nextRoutes = []
        var sourceRoutes = asArray(document && document.routes ? document.routes : [])
        for (var index = 0; index < sourceRoutes.length; ++index) {
            var normalized = normalizeRoute(sourceRoutes[index])
            if (normalized.id.length > 0) {
                nextRoutes.push(normalized)
            }
        }

        routes = nextRoutes.length > 0 ? nextRoutes : [fallbackRootRoute()]
        if (!findRouteById(rootRouteId)) {
            rootRouteId = routes[0].id
        }
        selectedRouteId = rootRouteId
        backStack = []
        forwardStack = []
        revision += 1
    }

    function findRouteById(routeId) {
        for (var index = 0; index < routes.length; ++index) {
            if (routes[index].id === routeId) {
                return routes[index]
            }
        }
        return null
    }

    function routeById(routeId) {
        return findRouteById(routeId) || routes[0] || fallbackRootRoute()
    }

    function currentRoute() {
        return routeById(selectedRouteId)
    }

    function routeStatus(routeId) {
        var route = routeById(routeId)
        return statusOverrides[routeId] || route.status || "pending"
    }

    function noteCount(routeId) {
        var count = 0
        for (var index = 0; index < notes.length; ++index) {
            if (notes[index].routeId === routeId) {
                count += 1
            }
        }
        return count
    }

    function routeNotes(routeId, unusedRevision) {
        var result = []
        for (var index = 0; index < notes.length; ++index) {
            if (notes[index].routeId === routeId) {
                result.push(notes[index])
            }
        }
        return result
    }

    function allNotes(unusedRevision) {
        return notes
    }

    function inspectorSectionVisible(sectionId) {
        var sections = normalizedInspectorSections(rightInspectorSections)
        return sections[String(sectionId)] !== false
    }

    function setInspectorSectionVisible(sectionId, visible) {
        var next = normalizedInspectorSections(rightInspectorSections)
        next[String(sectionId)] = !!visible
        rightInspectorSections = next
        markShellLayoutDirty()
    }

    function rightInspectorDocument(unusedRevision) {
        var route = currentRoute()
        var notesForRoute = routeNotes(route.id, revision)
        return {
            id: "route_inspector",
            targetId: route.id,
            targetLabel: route.label,
            targetType: route.type,
            status: routeStatus(route.id),
            sections: [
                {
                    id: "facts",
                    label: "Facts",
                    type: "rows",
                    visible: inspectorSectionVisible("facts"),
                    rows: [
                        { label: "Route", value: route.label },
                        { label: "Type", value: route.type },
                        { label: "Status", value: routeStatus(route.id) },
                        { label: "Children", value: String((route.children || []).length) },
                        { label: "Notes", value: String(noteCount(route.id)) }
                    ]
                },
                {
                    id: "selection",
                    label: "Selection",
                    type: "text",
                    visible: inspectorSectionVisible("selection"),
                    items: [
                        { label: "Summary", value: route.summary || "" },
                        { label: "Purpose", value: route.purpose || "" },
                        { label: "Path", value: breadcrumbText(route.id) }
                    ]
                },
                {
                    id: "code_refs",
                    label: "Code Refs",
                    type: "code_refs",
                    visible: inspectorSectionVisible("code_refs"),
                    items: route.codeRefs || []
                },
                {
                    id: "notes",
                    label: "Notes",
                    type: "notes",
                    visible: inspectorSectionVisible("notes"),
                    items: notesForRoute.slice(-2)
                },
                {
                    id: "receipts",
                    label: "Receipts",
                    type: "rows",
                    visible: inspectorSectionVisible("receipts"),
                    rows: [
                        { label: "Writes", value: writeDisabled ? "disabled" : "enabled" },
                        { label: "Storage", value: "in memory" },
                        { label: "Subject", value: selectedSubjectId }
                    ]
                },
                {
                    id: "actions",
                    label: "Actions",
                    type: "actions",
                    visible: inspectorSectionVisible("actions"),
                    actions: [
                        { id: "status_pending", label: "Pending", kind: "status", value: "pending" },
                        { id: "status_accepted", label: "Accept", kind: "status", value: "accepted" },
                        { id: "status_needs_rework", label: "Rework", kind: "status", value: "needs_rework" },
                        { id: "status_rejected", label: "Reject", kind: "status", value: "rejected" }
                    ]
                }
            ]
        }
    }

    function childRoutes(routeId, unusedRevision) {
        var route = routeById(routeId)
        var result = []
        for (var index = 0; index < route.children.length; ++index) {
            result.push(routeById(route.children[index]))
        }
        return result
    }

    function siblingRoutes(routeId) {
        var route = routeById(routeId)
        if (!route.parent) {
            return childRoutes(rootRouteId, revision)
        }
        return childRoutes(route.parent, revision)
    }

    function breadcrumb(routeId) {
        var result = []
        var guard = 0
        var route = routeById(routeId)
        while (route && guard < 20) {
            result.unshift(route)
            if (!route.parent) {
                break
            }
            route = routeById(route.parent)
            guard += 1
        }
        return result
    }

    function breadcrumbText(routeId) {
        var crumbs = breadcrumb(routeId)
        var labels = []
        for (var index = 0; index < crumbs.length; ++index) {
            labels.push(crumbs[index].label)
        }
        return labels.join(" / ")
    }

    function setActivityMode(modeId) {
        if (!activityModeAvailable(modeId)) {
            return
        }
        activityMode = modeId
        if (modeId === "settings" && !selectedSettingsPage.length) {
            selectedSettingsPage = "theme"
        }
        revision += 1
    }

    function setSettingsPage(pageId) {
        selectedSettingsPage = pageId
        activityMode = "settings"
        revision += 1
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

    function applyTheme(theme) {
        uiThemeDocument = theme
        UiStyle.applyTheme(theme)
        revision += 1
    }

    function selectRoute(routeId) {
        if (routeId === selectedRouteId || !findRouteById(routeId)) {
            return
        }
        var nextBack = backStack.slice()
        nextBack.push(selectedRouteId)
        backStack = nextBack
        forwardStack = []
        selectedRouteId = routeId
        revision += 1
    }

    function goHome() {
        selectRoute(rootRouteId)
    }

    function goBack() {
        if (backStack.length === 0) {
            return
        }
        var nextBack = backStack.slice()
        var prior = nextBack.pop()
        var nextForward = forwardStack.slice()
        nextForward.push(selectedRouteId)
        backStack = nextBack
        forwardStack = nextForward
        selectedRouteId = prior
        revision += 1
    }

    function goForward() {
        if (forwardStack.length === 0) {
            return
        }
        var nextForward = forwardStack.slice()
        var next = nextForward.pop()
        var nextBack = backStack.slice()
        nextBack.push(selectedRouteId)
        backStack = nextBack
        forwardStack = nextForward
        selectedRouteId = next
        revision += 1
    }

    function setLocalTab(tabName) {
        selectedLocalTab = tabName
        revision += 1
    }

    function setShelfTab(tabName) {
        selectedShelfTab = tabName
        revision += 1
    }

    function setStatus(routeId, status) {
        var next = Object.assign({}, statusOverrides)
        next[routeId] = status
        statusOverrides = next
        revision += 1
    }

    function runInspectorAction(actionId, targetId) {
        var text = String(actionId || "")
        if (text.indexOf("status_") !== 0) {
            return
        }
        setStatus(targetId || selectedRouteId, text.slice(7))
    }

    function addNote(routeId, status, body) {
        var text = String(body || "").trim()
        if (!text.length) {
            return
        }
        var next = notes.slice()
        next.push({
            id: "note_" + String(next.length + 1).padStart(3, "0"),
            routeId: routeId,
            routeLabel: routeById(routeId).label,
            status: status,
            body: text,
            createdAt: new Date().toLocaleString()
        })
        notes = next
        setStatus(routeId, status)
        revision += 1
    }

    function statusCounts(unusedRevision) {
        var counts = { pending: 0, accepted: 0, needs_rework: 0, rejected: 0 }
        for (var index = 0; index < routes.length; ++index) {
            var status = routeStatus(routes[index].id)
            if (counts[status] === undefined) {
                counts[status] = 0
            }
            counts[status] += 1
        }
        return counts
    }
}
