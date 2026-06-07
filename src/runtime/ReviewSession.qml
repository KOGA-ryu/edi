import QtQuick

QtObject {
    id: reviewSession

    property bool hasReviewSubject: false
    property string selectedSubjectId: ""
    property string selectedSubjectLabel: ""
    property string selectedRouteId: ""
    property string rootRouteId: ""
    property var routes: []
    property var backStack: []
    property var forwardStack: []
    property var statusOverrides: ({})
    property var notes: []
    property var reviewSubjectDocument: ({})

    signal changed()

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
        return {
            id: String(route.route_id || route.id || ""),
            parent: String(route.parent || ""),
            label: String(route.label || route.route_id || route.id || ""),
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
            id: "missing_review_subject",
            parent: "",
            label: "Missing Review Subject",
            type: "root",
            status: "pending",
            summary: "Review subject data could not be loaded.",
            purpose: "Check the active project profile data_sources.review_subject path.",
            objects: [],
            children: [],
            codeRefs: [],
            prompts: ["Is the review subject JSON present and valid?"]
        }
    }

    function blankRoute() {
        return {
            id: "",
            parent: "",
            label: "",
            type: "",
            status: "",
            summary: "",
            purpose: "",
            objects: [],
            children: [],
            codeRefs: [],
            prompts: []
        }
    }

    function clearReviewSubject() {
        hasReviewSubject = false
        selectedSubjectId = ""
        selectedSubjectLabel = ""
        rootRouteId = ""
        selectedRouteId = ""
        routes = []
        backStack = []
        forwardStack = []
        statusOverrides = ({})
        notes = []
        reviewSubjectDocument = ({})
        changed()
    }

    function loadReviewSubject(document, reviewModeEnabled) {
        var sourceDocument = document || ({})
        var subject = sourceDocument && sourceDocument.subject ? sourceDocument.subject : ({})
        var sourceRoutes = asArray(sourceDocument && sourceDocument.routes ? sourceDocument.routes : [])

        reviewSubjectDocument = sourceDocument
        if (!reviewModeEnabled && !subject.subject_id && sourceRoutes.length === 0) {
            clearReviewSubject()
            return
        }

        selectedSubjectId = String(subject.subject_id || "")
        selectedSubjectLabel = String(subject.label || selectedSubjectId)
        rootRouteId = String(subject.root_route_id || "")

        var nextRoutes = []
        for (var index = 0; index < sourceRoutes.length; ++index) {
            var normalized = normalizeRoute(sourceRoutes[index])
            if (normalized.id.length > 0) {
                nextRoutes.push(normalized)
            }
        }

        routes = nextRoutes.length > 0 ? nextRoutes : [fallbackRootRoute()]
        hasReviewSubject = nextRoutes.length > 0
        if (!findRouteById(rootRouteId)) {
            rootRouteId = routes[0].id
        }
        selectedRouteId = rootRouteId
        backStack = []
        forwardStack = []
        statusOverrides = ({})
        notes = []
        changed()
    }

    function findRouteById(routeId) {
        for (var index = 0; index < routes.length; ++index) {
            if (routes[index].id === String(routeId || "")) {
                return routes[index]
            }
        }
        return null
    }

    function routeById(routeId) {
        if (!hasReviewSubject && routes.length === 0) {
            return blankRoute()
        }
        return findRouteById(routeId) || routes[0] || fallbackRootRoute()
    }

    function currentRoute() {
        return routeById(selectedRouteId)
    }

    function routeStatus(routeId) {
        var route = routeById(routeId)
        return statusOverrides[String(routeId || "")] || route.status || "pending"
    }

    function noteCount(routeId) {
        var count = 0
        var target = String(routeId || "")
        for (var index = 0; index < notes.length; ++index) {
            if (notes[index].routeId === target) {
                count += 1
            }
        }
        return count
    }

    function routeNotes(routeId, unusedRevision) {
        var target = String(routeId || "")
        var result = []
        for (var index = 0; index < notes.length; ++index) {
            if (notes[index].routeId === target) {
                result.push(notes[index])
            }
        }
        return result
    }

    function allNotes(unusedRevision) {
        return notes
    }

    function childRoutes(routeId, unusedRevision) {
        if (!hasReviewSubject) {
            return []
        }
        var route = routeById(routeId)
        var result = []
        for (var index = 0; index < route.children.length; ++index) {
            result.push(routeById(route.children[index]))
        }
        return result
    }

    function siblingRoutes(routeId) {
        if (!hasReviewSubject) {
            return []
        }
        var route = routeById(routeId)
        if (!route.parent) {
            return childRoutes(rootRouteId, 0)
        }
        return childRoutes(route.parent, 0)
    }

    function breadcrumb(routeId) {
        if (!hasReviewSubject) {
            return []
        }
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

    function selectRoute(routeId) {
        if (!hasReviewSubject) {
            return
        }
        var target = String(routeId || "")
        if (target === selectedRouteId || !findRouteById(target)) {
            return
        }
        var nextBack = backStack.slice()
        nextBack.push(selectedRouteId)
        backStack = nextBack
        forwardStack = []
        selectedRouteId = target
        changed()
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
        changed()
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
        changed()
    }

    function _setStatus(routeId, status, emitChanged) {
        var target = String(routeId || "")
        var next = Object.assign({}, statusOverrides)
        next[target] = String(status || "pending")
        statusOverrides = next
        if (emitChanged) {
            changed()
        }
    }

    function setStatus(routeId, status) {
        _setStatus(routeId, status, true)
    }

    function runInspectorAction(actionId, targetId) {
        var text = String(actionId || "")
        if (text.indexOf("status_") !== 0) {
            return
        }
        _setStatus(targetId || selectedRouteId, text.slice(7), true)
    }

    function addNote(routeId, status, body) {
        if (!hasReviewSubject) {
            return
        }
        var text = String(body || "").trim()
        if (!text.length) {
            return
        }
        var targetRouteId = String(routeId || "")
        var route = routeById(targetRouteId)
        var next = notes.slice()
        next.push({
            id: "note_" + String(next.length + 1).padStart(3, "0"),
            routeId: targetRouteId,
            routeLabel: route.label,
            status: status,
            body: text,
            createdAt: new Date().toLocaleString()
        })
        notes = next
        _setStatus(targetRouteId, status, false)
        changed()
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
