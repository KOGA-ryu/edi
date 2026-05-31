import QtQuick

QtObject {
    id: runtimeController

    property var targetRoot: null
    property string activityMode: "review"
    property string selectedSubjectId: "draftsman_ui_taxonomy"
    property string selectedSubjectLabel: "Draftsman UI Taxonomy"
    property string selectedRouteId: "draftsman_ui"
    property string selectedLocalTab: "Overview"
    property string selectedShelfTab: "Output"
    property bool writeDisabled: true
    property int revision: 0

    property var backStack: []
    property var forwardStack: []
    property var statusOverrides: ({})
    property var notes: []
    property var reviewSubjectDocument: ({})

    readonly property var activityModes: [
        { id: "binder", label: "Binder", icon: "B", tooltip: "Binder workspace" },
        { id: "review", label: "Review", icon: "R", tooltip: "UI taxonomy review gate" },
        { id: "settings", label: "Settings", icon: "S", tooltip: "Settings surface" },
        { id: "proof", label: "Proof", icon: "P", tooltip: "Proof and receipts" }
    ]

    readonly property var localTabs: ["Overview", "Objects", "Code", "Prompts", "Notes"]
    readonly property var shelfTabs: ["Output", "Proof", "Receipts", "Log"]

    property var routes: []
    property string rootRouteId: "draftsman_ui"

    Component.onCompleted: {
        var document = typeof initialReviewSubject === "undefined" ? ({}) : initialReviewSubject
        reviewSubjectDocument = document
        loadReviewSubject(reviewSubjectDocument)
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
