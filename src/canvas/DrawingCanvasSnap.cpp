#include "DrawingCanvasSnap.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace drawing_canvas {
namespace {

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
    if (!std::isfinite(finiteNumber(x, std::numeric_limits<double>::quiet_NaN()))
        || !std::isfinite(finiteNumber(y, std::numeric_limits<double>::quiet_NaN()))) {
        return;
    }
    candidates.push_back({
        clamp01(x),
        clamp01(y),
        sourceKind,
        label,
        object.id()
    });
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
    for (const SnapCandidate &candidate : snapCandidates(objects, settings)) {
        const double dx = (candidate.x - point.x) * boardPx;
        const double dy = (candidate.y - point.y) * boardPx;
        const double distance = std::sqrt(dx * dx + dy * dy);
        if (distance <= bestDistance) {
            found = true;
            bestDistance = distance;
            best = candidate;
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

std::vector<SnapCandidate> snapCandidatesForObject(const CanvasObjectView &object, const QVariantMap &settings) {
    std::vector<SnapCandidate> candidates;
    const QString kind = object.kind();
    if (kind == QStringLiteral("grid") || kind == QStringLiteral("rect") || kind == QStringLiteral("metadata")) {
        return candidates;
    }
    if (kind == QStringLiteral("line") || kind == QStringLiteral("glyph_baseline")) {
        const double x1 = object.number(QStringLiteral("x1"));
        const double y1 = object.number(QStringLiteral("y1"));
        const double x2 = object.number(QStringLiteral("x2"));
        const double y2 = object.number(QStringLiteral("y2"));
        if (boolSetting(settings, QStringLiteral("endpointEnabled"), true)) {
            addCandidate(candidates, object, x1, y1, QStringLiteral("endpoint"), QStringLiteral("endpoint"));
            addCandidate(candidates, object, x2, y2, QStringLiteral("endpoint"), QStringLiteral("endpoint"));
        }
        if (boolSetting(settings, QStringLiteral("midpointEnabled"), true)) {
            addCandidate(candidates, object, (x1 + x2) / 2.0, (y1 + y2) / 2.0, QStringLiteral("midpoint"), QStringLiteral("midpoint"));
        }
    } else if (kind == QStringLiteral("point") || kind == QStringLiteral("tone_probe") || kind == QStringLiteral("anchor")) {
        if (boolSetting(settings, QStringLiteral("endpointEnabled"), true)) {
            addCandidate(candidates, object, object.number(QStringLiteral("x")), object.number(QStringLiteral("y")), QStringLiteral("endpoint"), QStringLiteral("endpoint"));
        }
    } else if (kind == QStringLiteral("polyline")) {
        const std::vector<CanvasPoint> points = object.points();
        for (std::size_t index = 0; index < points.size(); ++index) {
            const bool isEndpoint = index == 0 || index == points.size() - 1;
            if (isEndpoint && boolSetting(settings, QStringLiteral("endpointEnabled"), true)) {
                addCandidate(candidates, object, points[index].x, points[index].y, QStringLiteral("endpoint"), QStringLiteral("endpoint"));
            } else if (!isEndpoint && boolSetting(settings, QStringLiteral("vertexEnabled"), true)) {
                addCandidate(candidates, object, points[index].x, points[index].y, QStringLiteral("vertex"), QStringLiteral("vertex"));
            }
            if (index > 0 && boolSetting(settings, QStringLiteral("midpointEnabled"), true)) {
                addCandidate(candidates, object, (points[index - 1].x + points[index].x) / 2.0, (points[index - 1].y + points[index].y) / 2.0, QStringLiteral("midpoint"), QStringLiteral("midpoint"));
            }
        }
    } else if (kind == QStringLiteral("circle") || kind == QStringLiteral("arc")) {
        if (boolSetting(settings, QStringLiteral("centerEnabled"), true)) {
            addCandidate(candidates, object, object.number(QStringLiteral("cx")), object.number(QStringLiteral("cy")), QStringLiteral("center"), QStringLiteral("center"));
        }
    } else if (isRectangleLike(kind)) {
        const double x = object.number(QStringLiteral("x"));
        const double y = object.number(QStringLiteral("y"));
        const double width = object.number(QStringLiteral("width"));
        const double height = object.number(QStringLiteral("height"));
        if (width <= 0.0 || height <= 0.0) {
            return candidates;
        }
        const double left = x;
        const double right = x + width;
        const double top = y;
        const double bottom = y + height;
        const double centerX = (left + right) / 2.0;
        const double centerY = (top + bottom) / 2.0;
        if (boolSetting(settings, QStringLiteral("vertexEnabled"), true)) {
            addCandidate(candidates, object, left, top, QStringLiteral("vertex"), QStringLiteral("vertex"));
            addCandidate(candidates, object, right, top, QStringLiteral("vertex"), QStringLiteral("vertex"));
            addCandidate(candidates, object, right, bottom, QStringLiteral("vertex"), QStringLiteral("vertex"));
            addCandidate(candidates, object, left, bottom, QStringLiteral("vertex"), QStringLiteral("vertex"));
        }
        if (boolSetting(settings, QStringLiteral("midpointEnabled"), true)) {
            addCandidate(candidates, object, centerX, top, QStringLiteral("midpoint"), QStringLiteral("midpoint"));
            addCandidate(candidates, object, right, centerY, QStringLiteral("midpoint"), QStringLiteral("midpoint"));
            addCandidate(candidates, object, centerX, bottom, QStringLiteral("midpoint"), QStringLiteral("midpoint"));
            addCandidate(candidates, object, left, centerY, QStringLiteral("midpoint"), QStringLiteral("midpoint"));
        }
        if (boolSetting(settings, QStringLiteral("centerEnabled"), true)) {
            addCandidate(candidates, object, centerX, centerY, QStringLiteral("center"), QStringLiteral("center"));
        }
    } else if (kind == QStringLiteral("polygon")) {
        const std::vector<CanvasPoint> points = object.points();
        if (points.size() < 3) {
            return candidates;
        }
        double sumX = 0.0;
        double sumY = 0.0;
        for (std::size_t index = 0; index < points.size(); ++index) {
            sumX += points[index].x;
            sumY += points[index].y;
            if (boolSetting(settings, QStringLiteral("vertexEnabled"), true)) {
                addCandidate(candidates, object, points[index].x, points[index].y, QStringLiteral("vertex"), QStringLiteral("vertex"));
            }
            if (boolSetting(settings, QStringLiteral("midpointEnabled"), true)) {
                const CanvasPoint next = points[(index + 1) % points.size()];
                addCandidate(candidates, object, (points[index].x + next.x) / 2.0, (points[index].y + next.y) / 2.0, QStringLiteral("midpoint"), QStringLiteral("midpoint"));
            }
        }
        if (boolSetting(settings, QStringLiteral("centerEnabled"), true)) {
            addCandidate(candidates, object, sumX / static_cast<double>(points.size()), sumY / static_cast<double>(points.size()), QStringLiteral("center"), QStringLiteral("center"));
        }
    }
    return candidates;
}

std::vector<SnapCandidate> snapCandidates(const std::vector<CanvasObjectView> &objects, const QVariantMap &settings) {
    std::vector<SnapCandidate> result;
    for (const CanvasObjectView &object : objects) {
        const std::vector<SnapCandidate> objectCandidates = snapCandidatesForObject(object, settings);
        result.insert(result.end(), objectCandidates.begin(), objectCandidates.end());
    }
    return result;
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
