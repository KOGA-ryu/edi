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
    property bool leftPanelCollapsed: false
    property bool rightPanelCollapsed: false
    property bool bottomPanelCollapsed: false
    property int leftPanelWidth: UiStyle.leftPanelWidth
    property int rightPanelWidth: UiStyle.rightPanelWidth
    property int bottomPanelHeight: UiStyle.bottomPanelHeight
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

    readonly property var activityModes: [
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
    readonly property var shelfTabs: ["Output", "Proof", "Receipts", "Log"]

    property var routes: []
    property string rootRouteId: "draftsman_ui"

    Component.onCompleted: {
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

    function markShellLayoutDirty() {
        shellLayoutDirty = true
        revision += 1
    }

    function loadShellLayout(document) {
        var panels = document && document.panels ? document.panels : ({})
        var left = panels.left || ({})
        var right = panels.right || ({})
        var bottom = panels.bottom || ({})

        leftPanelCollapsed = !!left.collapsed
        rightPanelCollapsed = !!right.collapsed
        bottomPanelCollapsed = !!bottom.collapsed
        leftPanelWidth = clamp(left.width || UiStyle.leftPanelWidth, 180, 520)
        rightPanelWidth = clamp(right.width || UiStyle.rightPanelWidth, 220, 560)
        bottomPanelHeight = clamp(bottom.height || UiStyle.bottomPanelHeight, 96, 360)
        shellLayoutDirty = false
        shellLayoutSaveOk = true
    }

    function shellLayoutDocument() {
        return {
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
        rightPanelCollapsed = false
        bottomPanelCollapsed = false
        leftPanelWidth = UiStyle.leftPanelWidth
        rightPanelWidth = UiStyle.rightPanelWidth
        bottomPanelHeight = UiStyle.bottomPanelHeight
        markShellLayoutDirty()
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
            return windowWidth() < 640
        }
        if (panelId === "right") {
            return windowWidth() < 1040
        }
        if (panelId === "bottom") {
            return windowHeight() < 520
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
            return panelAutoHidden(panelId) ? "auto-hidden below 640px width" : String(leftPanelWidth) + " px"
        }
        if (panelId === "right") {
            return panelAutoHidden(panelId) ? "auto-hidden below 1040px width" : String(rightPanelWidth) + " px"
        }
        if (panelId === "bottom") {
            return panelAutoHidden(panelId) ? "auto-hidden below 520px height" : String(bottomPanelHeight) + " px"
        }
        return ""
    }

    function applyLayoutPreset(presetId) {
        if (presetId === "full") {
            leftPanelCollapsed = false
            rightPanelCollapsed = false
            bottomPanelCollapsed = false
            leftPanelWidth = UiStyle.leftPanelWidth
            rightPanelWidth = UiStyle.rightPanelWidth
            bottomPanelHeight = UiStyle.bottomPanelHeight
        } else if (presetId === "focus") {
            leftPanelCollapsed = true
            rightPanelCollapsed = true
            bottomPanelCollapsed = true
        } else if (presetId === "review") {
            leftPanelCollapsed = false
            rightPanelCollapsed = false
            bottomPanelCollapsed = true
            leftPanelWidth = UiStyle.leftPanelWidth
            rightPanelWidth = UiStyle.rightPanelWidth
        } else if (presetId === "tiny") {
            leftPanelCollapsed = true
            rightPanelCollapsed = true
            bottomPanelCollapsed = true
            leftPanelWidth = UiStyle.leftPanelWidth
            rightPanelWidth = UiStyle.rightPanelWidth
            bottomPanelHeight = UiStyle.bottomPanelHeight
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
        leftPanelWidth = clamp(width, 180, 520)
        markShellLayoutDirty()
    }

    function setRightPanelWidth(width) {
        rightPanelWidth = clamp(width, 220, 560)
        markShellLayoutDirty()
    }

    function setBottomPanelHeight(height) {
        bottomPanelHeight = clamp(height, 96, 360)
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
