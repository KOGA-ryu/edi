#include "DrawingCanvasSnap.h"

#include <algorithm>
#include <cmath>

namespace drawing_canvas {
namespace {

struct SnapCandidate {
    double x = 0.0;
    double y = 0.0;
    QString sourceKind;
    QString label;
    QString sourceObjectId;
};

bool boolSetting(const QVariantMap &settings, const QString &field, bool fallback) {
    return settings.contains(field) ? settings.value(field).toBool() : fallback;
}

CanvasPoint normalizedPoint(const CanvasPoint &point) {
    return {clamp01(point.x), clamp01(point.y)};
}

double boardSizePx(const QVariantMap &settings) {
    return std::max(1.0, finiteNumber(settings.value(QStringLiteral("boardSizePx")), finiteNumber(settings.value(QStringLiteral("canvasSizePx")), 512.0)));
}

double canvasSizeForSnap(const QVariantMap &settings) {
    return std::max(1.0, finiteNumber(settings.value(QStringLiteral("canvasSizePx")), 512.0));
}

SnapResult candidateToResult(const SnapCandidate &candidate, const QVariantMap &settings) {
    SnapResult result;
    result.x = clamp01(candidate.x);
    result.y = clamp01(candidate.y);
    result.kind = QStringLiteral("object");
    result.label = candidate.label;
    result.sourceObjectId = candidate.sourceObjectId;
    result.sourceKind = candidate.sourceKind;
    result.stepPx = effectiveGridStepPx(settings);
    return result;
}

void addCandidate(std::vector<SnapCandidate> &candidates, const CanvasObjectView &object, double x, double y, const QString &sourceKind, const QString &label) {
    candidates.push_back({
        finiteNumber(x, 0.0),
        finiteNumber(y, 0.0),
        sourceKind,
        label,
        object.id()
    });
}

std::vector<SnapCandidate> snapCandidatesForObject(const CanvasObjectView &object, const QVariantMap &settings) {
    std::vector<SnapCandidate> candidates;
    if (!object.visible()) {
        return candidates;
    }
    const QString kind = object.kind();
    if ((kind == QStringLiteral("line") || kind == QStringLiteral("glyph_baseline"))) {
        if (boolSetting(settings, QStringLiteral("endpointEnabled"), true)) {
            addCandidate(candidates, object, object.number(QStringLiteral("x1")), object.number(QStringLiteral("y1")), QStringLiteral("endpoint"), QStringLiteral("endpoint"));
            addCandidate(candidates, object, object.number(QStringLiteral("x2")), object.number(QStringLiteral("y2")), QStringLiteral("endpoint"), QStringLiteral("endpoint"));
        }
        if (boolSetting(settings, QStringLiteral("midpointEnabled"), true)) {
            addCandidate(candidates, object,
                         (object.number(QStringLiteral("x1")) + object.number(QStringLiteral("x2"))) / 2.0,
                         (object.number(QStringLiteral("y1")) + object.number(QStringLiteral("y2"))) / 2.0,
                         QStringLiteral("midpoint"),
                         QStringLiteral("midpoint"));
        }
        return candidates;
    }
    if (kind == QStringLiteral("point") || kind == QStringLiteral("tone_probe") || kind == QStringLiteral("anchor")) {
        if (boolSetting(settings, QStringLiteral("vertexEnabled"), true)) {
            addCandidate(candidates, object, object.number(QStringLiteral("x")), object.number(QStringLiteral("y")), QStringLiteral("vertex"), QStringLiteral("point"));
        }
        return candidates;
    }
    if (kind == QStringLiteral("polyline") || kind == QStringLiteral("polygon")) {
        const std::vector<CanvasPoint> points = object.points();
        if (boolSetting(settings, QStringLiteral("vertexEnabled"), true)) {
            for (const CanvasPoint &point : points) {
                addCandidate(candidates, object, point.x, point.y, QStringLiteral("vertex"), QStringLiteral("vertex"));
            }
        }
        if (boolSetting(settings, QStringLiteral("midpointEnabled"), true)) {
            const std::size_t limit = kind == QStringLiteral("polygon") ? points.size() : points.size() > 0 ? points.size() - 1 : 0;
            for (std::size_t i = 0; i < limit && points.size() > 1; ++i) {
                const CanvasPoint a = points[i];
                const CanvasPoint b = points[(i + 1) % points.size()];
                addCandidate(candidates, object, (a.x + b.x) / 2.0, (a.y + b.y) / 2.0, QStringLiteral("midpoint"), QStringLiteral("midpoint"));
            }
        }
        return candidates;
    }
    if (kind == QStringLiteral("circle") || kind == QStringLiteral("arc")) {
        if (boolSetting(settings, QStringLiteral("centerEnabled"), true)) {
            addCandidate(candidates, object, object.number(QStringLiteral("cx")), object.number(QStringLiteral("cy")), QStringLiteral("center"), QStringLiteral("center"));
        }
        return candidates;
    }
    if (isRectangleLike(kind)) {
        const double x = object.number(QStringLiteral("x"));
        const double y = object.number(QStringLiteral("y"));
        const double width = object.number(QStringLiteral("width"));
        const double height = object.number(QStringLiteral("height"));
        if (boolSetting(settings, QStringLiteral("vertexEnabled"), true)) {
            addCandidate(candidates, object, x, y, QStringLiteral("vertex"), QStringLiteral("corner"));
            addCandidate(candidates, object, x + width, y, QStringLiteral("vertex"), QStringLiteral("corner"));
            addCandidate(candidates, object, x, y + height, QStringLiteral("vertex"), QStringLiteral("corner"));
            addCandidate(candidates, object, x + width, y + height, QStringLiteral("vertex"), QStringLiteral("corner"));
        }
        if (boolSetting(settings, QStringLiteral("centerEnabled"), true)) {
            addCandidate(candidates, object, x + width / 2.0, y + height / 2.0, QStringLiteral("center"), QStringLiteral("center"));
        }
        return candidates;
    }
    return candidates;
}

SnapResult nearestObjectSnap(const CanvasPoint &point, const std::vector<CanvasObjectView> &objects, const QVariantMap &settings) {
    SnapResult result;
    if (!boolSetting(settings, QStringLiteral("objectSnapEnabled"), false)) {
        return result;
    }
    const double tolerancePx = std::max(0.0, finiteNumber(settings.value(QStringLiteral("objectSnapTolerancePx")), 14.0));
    const double boardPx = boardSizePx(settings);
    double bestDistance = tolerancePx;
    bool found = false;
    SnapCandidate best;
    for (const CanvasObjectView &object : objects) {
        for (const SnapCandidate &candidate : snapCandidatesForObject(object, settings)) {
            const double dx = (candidate.x - point.x) * boardPx;
            const double dy = (candidate.y - point.y) * boardPx;
            const double distance = std::sqrt(dx * dx + dy * dy);
            if (distance <= bestDistance) {
                found = true;
                bestDistance = distance;
                best = candidate;
            }
        }
    }
    return found ? candidateToResult(best, settings) : result;
}

} // namespace

double effectiveGridStepPx(const QVariantMap &settings) {
    const double base = std::max(1.0, finiteNumber(settings.value(QStringLiteral("gridStepPx")), 32.0));
    const double zoom = std::max(0.1, finiteNumber(settings.value(QStringLiteral("zoom")), 1.0));
    if (zoom >= 6.0) {
        return std::max(1.0, base / 8.0);
    }
    if (zoom >= 3.0) {
        return std::max(1.0, base / 4.0);
    }
    if (zoom >= 1.65) {
        return std::max(1.0, base / 2.0);
    }
    if (zoom < 0.62) {
        return base * 2.0;
    }
    return base;
}

SnapResult noneSnap(const CanvasPoint &point, const QVariantMap &settings) {
    const CanvasPoint safe = normalizedPoint(point);
    return {safe.x, safe.y, QStringLiteral("none"), QStringLiteral("none"), QString(), QString(), effectiveGridStepPx(settings)};
}

SnapResult gridSnap(const CanvasPoint &point, const QVariantMap &settings) {
    const CanvasPoint safe = normalizedPoint(point);
    const double stepPx = effectiveGridStepPx(settings);
    const double canvasPx = canvasSizeForSnap(settings);
    const double step = std::max(1.0 / canvasPx, stepPx / canvasPx);
    const double x = clamp01(std::round(safe.x / step) * step);
    const double y = clamp01(std::round(safe.y / step) * step);
    return {
        x,
        y,
        QStringLiteral("grid"),
        QStringLiteral("grid %1px").arg(std::round(stepPx)),
        QString(),
        QString(),
        stepPx
    };
}

SnapResult resolveSnap(const CanvasPoint &rawPoint, const std::vector<CanvasObjectView> &objects, const QVariantMap &settings) {
    const CanvasPoint point = normalizedPoint(rawPoint);
    const QString priority = settings.value(QStringLiteral("objectPriority"), QStringLiteral("before_grid")).toString();
    if (priority != QStringLiteral("after_grid")) {
        const SnapResult objectSnap = nearestObjectSnap(point, objects, settings);
        if (objectSnap.kind == QStringLiteral("object")) {
            return objectSnap;
        }
    }
    if (boolSetting(settings, QStringLiteral("gridEnabled"), false)) {
        return gridSnap(point, settings);
    }
    if (priority == QStringLiteral("after_grid")) {
        const SnapResult objectSnap = nearestObjectSnap(point, objects, settings);
        if (objectSnap.kind == QStringLiteral("object")) {
            return objectSnap;
        }
    }
    return noneSnap(point, settings);
}

} // namespace drawing_canvas
