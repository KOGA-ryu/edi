#include "DrawingCanvasHitTest.h"
#include "DrawingCanvasHandles.h"

#include <algorithm>
#include <cmath>

namespace drawing_canvas {
namespace {

constexpr double pi = 3.14159265358979323846;

double distanceToSegment(double px, double py, double ax, double ay, double bx, double by) {
    const double dx = bx - ax;
    const double dy = by - ay;
    if (std::abs(dx) < 0.000001 && std::abs(dy) < 0.000001) {
        const double ox = px - ax;
        const double oy = py - ay;
        return std::sqrt(ox * ox + oy * oy);
    }
    const double t = std::max(0.0, std::min(1.0, ((px - ax) * dx + (py - ay) * dy) / (dx * dx + dy * dy)));
    const double x = ax + t * dx;
    const double y = ay + t * dy;
    const double ox = px - x;
    const double oy = py - y;
    return std::sqrt(ox * ox + oy * oy);
}

bool pointInPolygon(const std::vector<CanvasPoint> &points, double x, double y) {
    if (points.size() < 3) {
        return false;
    }
    bool inside = false;
    for (std::size_t i = 0, j = points.size() - 1; i < points.size(); j = i++) {
        const double xi = finiteNumber(points[i].x, 0.0);
        const double yi = finiteNumber(points[i].y, 0.0);
        const double xj = finiteNumber(points[j].x, 0.0);
        const double yj = finiteNumber(points[j].y, 0.0);
        const bool intersects = ((yi > y) != (yj > y))
            && (x < (xj - xi) * (y - yi) / std::max(0.000001, yj - yi) + xi);
        if (intersects) {
            inside = !inside;
        }
    }
    return inside;
}

double angleDegrees(double x, double y) {
    return std::fmod(std::atan2(y, x) * 180.0 / pi + 360.0, 360.0);
}

bool angleInArc(double angle, double start, double end) {
    const double normalizedAngle = std::fmod(std::fmod(angle, 360.0) + 360.0, 360.0);
    const double normalizedStart = std::fmod(std::fmod(start, 360.0) + 360.0, 360.0);
    const double normalizedEnd = std::fmod(std::fmod(end, 360.0) + 360.0, 360.0);
    if (normalizedStart <= normalizedEnd) {
        return normalizedAngle >= normalizedStart && normalizedAngle <= normalizedEnd;
    }
    return normalizedAngle >= normalizedStart || normalizedAngle <= normalizedEnd;
}

CanvasPoint unrotatePoint(const CanvasObjectView &object, double x, double y) {
    return unrotatePointForRect(object, x, y);
}

} // namespace

double objectHitScore(const CanvasObjectView &object, double x, double y) {
    const QString kind = object.kind();
    if (object.visible() == false) {
        return 999.0;
    }
    if (kind == QStringLiteral("point") || kind == QStringLiteral("tone_probe")) {
        const double dx = x - object.number(QStringLiteral("x"));
        const double dy = y - object.number(QStringLiteral("y"));
        return std::sqrt(dx * dx + dy * dy);
    }
    if (kind == QStringLiteral("line") || kind == QStringLiteral("glyph_baseline")) {
        return distanceToSegment(
            x, y,
            object.number(QStringLiteral("x1")),
            object.number(QStringLiteral("y1")),
            object.number(QStringLiteral("x2")),
            object.number(QStringLiteral("y2")));
    }
    if (kind == QStringLiteral("polyline")) {
        const std::vector<CanvasPoint> points = object.points();
        if (points.size() < 2) {
            return 999.0;
        }
        double best = 999.0;
        for (std::size_t i = 1; i < points.size(); ++i) {
            best = std::min(best, distanceToSegment(x, y, points[i - 1].x, points[i - 1].y, points[i].x, points[i].y));
        }
        return best;
    }
    if (kind == QStringLiteral("circle") || kind == QStringLiteral("arc")) {
        const double cx = object.number(QStringLiteral("cx"));
        const double cy = object.number(QStringLiteral("cy"));
        const double radius = std::max(0.0, object.number(QStringLiteral("radius")));
        const double dx = x - cx;
        const double dy = y - cy;
        const double distance = std::sqrt(dx * dx + dy * dy);
        if (kind == QStringLiteral("arc")
            && !angleInArc(angleDegrees(dx, dy), object.number(QStringLiteral("start_angle_deg")), object.number(QStringLiteral("end_angle_deg")))) {
            return 999.0;
        }
        return std::abs(distance - radius);
    }
    if (isRectangleLike(kind)) {
        const CanvasPoint local = unrotatePoint(object, x, y);
        const double left = object.number(QStringLiteral("x"));
        const double top = object.number(QStringLiteral("y"));
        const double width = object.number(QStringLiteral("width"));
        const double height = object.number(QStringLiteral("height"));
        const double right = left + width;
        const double bottom = top + height;
        if (local.x >= left && local.x <= right && local.y >= top && local.y <= bottom) {
            const double edgeDistance = std::min({local.x - left, right - local.x, local.y - top, bottom - local.y});
            return std::max(0.0, edgeDistance);
        }
        const double clampedX = std::max(left, std::min(right, local.x));
        const double clampedY = std::max(top, std::min(bottom, local.y));
        const double dx = local.x - clampedX;
        const double dy = local.y - clampedY;
        return std::sqrt(dx * dx + dy * dy);
    }
    if (kind == QStringLiteral("polygon")) {
        const std::vector<CanvasPoint> points = object.points();
        if (points.size() < 3) {
            return 999.0;
        }
        if (pointInPolygon(points, x, y)) {
            return 0.0;
        }
        double best = 999.0;
        for (std::size_t i = 0; i < points.size(); ++i) {
            const CanvasPoint a = points[i];
            const CanvasPoint b = points[(i + 1) % points.size()];
            best = std::min(best, distanceToSegment(x, y, a.x, a.y, b.x, b.y));
        }
        return best;
    }
    return 999.0;
}

HitResult hitObjectAt(const std::vector<CanvasObjectView> &objects, double x, double y, double tolerance) {
    HitResult best;
    best.distance = std::max(0.0, finiteNumber(tolerance, 0.02));
    for (auto it = objects.rbegin(); it != objects.rend(); ++it) {
        const double score = objectHitScore(*it, x, y);
        if (score < best.distance) {
            best.ok = true;
            best.objectId = it->id();
            best.kind = QStringLiteral("object");
            best.distance = score;
        }
    }
    if (!best.ok) {
        best.distance = 999.0;
    }
    return best;
}

} // namespace drawing_canvas
