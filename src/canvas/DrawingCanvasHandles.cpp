#include "DrawingCanvasHandles.h"

#include <algorithm>
#include <cmath>

namespace drawing_canvas {
namespace {

constexpr double pi = 3.14159265358979323846;

HandleUpdatePlan updatePlan(std::initializer_list<FieldUpdate> updates) {
    return {true, std::vector<FieldUpdate>(updates)};
}

HandleUpdatePlan emptyUpdatePlan() {
    return {};
}

std::vector<HandleDescriptor> pointHandles(const CanvasObjectView &object) {
    return {{
        QStringLiteral("point_position"),
        QStringLiteral("point"),
        QStringLiteral("move"),
        QStringLiteral("x_y"),
        {QStringLiteral("x_px"), QStringLiteral("y_px")},
        clamp01(object.number(QStringLiteral("x"))),
        clamp01(object.number(QStringLiteral("y")))
    }};
}

std::vector<HandleDescriptor> lineHandles(const CanvasObjectView &object) {
    return {
        {
            QStringLiteral("line_start"),
            QStringLiteral("endpoint"),
            QStringLiteral("resize"),
            QStringLiteral("x1_y1"),
            {QStringLiteral("x1_px"), QStringLiteral("y1_px")},
            clamp01(object.number(QStringLiteral("x1"))),
            clamp01(object.number(QStringLiteral("y1")))
        },
        {
            QStringLiteral("line_end"),
            QStringLiteral("endpoint"),
            QStringLiteral("resize"),
            QStringLiteral("x2_y2"),
            {QStringLiteral("x2_px"), QStringLiteral("y2_px")},
            clamp01(object.number(QStringLiteral("x2"))),
            clamp01(object.number(QStringLiteral("y2")))
        }
    };
}

std::vector<HandleDescriptor> circleHandles(const CanvasObjectView &object) {
    const double cx = clamp01(object.number(QStringLiteral("cx")));
    const double cy = clamp01(object.number(QStringLiteral("cy")));
    const double radius = std::max(0.0, object.number(QStringLiteral("radius")));
    return {
        {
            QStringLiteral("circle_center"),
            QStringLiteral("center"),
            QStringLiteral("move"),
            QStringLiteral("cx_cy"),
            {QStringLiteral("cx_px"), QStringLiteral("cy_px")},
            cx,
            cy
        },
        {
            QStringLiteral("circle_radius"),
            QStringLiteral("radius"),
            QStringLiteral("resize"),
            QStringLiteral("radius"),
            {QStringLiteral("radius_px")},
            clamp01(cx + radius),
            cy
        }
    };
}

std::vector<HandleDescriptor> dimensionHandles(const CanvasObjectView &object) {
    const double x1 = clamp01(object.number(QStringLiteral("x1")));
    const double y1 = clamp01(object.number(QStringLiteral("y1")));
    const double x2 = clamp01(object.number(QStringLiteral("x2")));
    const double y2 = clamp01(object.number(QStringLiteral("y2")));
    const double labelX = clamp01(object.number(QStringLiteral("label_x"), (x1 + x2) / 2.0));
    const double labelY = clamp01(object.number(QStringLiteral("label_y"), (y1 + y2) / 2.0));
    const double midpointX = clamp01((x1 + x2) / 2.0);
    const double midpointY = clamp01((y1 + y2) / 2.0);
    HandleDescriptor offset {
        QStringLiteral("dimension_offset"),
        QStringLiteral("offset"),
        QStringLiteral("move"),
        QStringLiteral("offset"),
        {QStringLiteral("offset_px")},
        labelX,
        labelY
    };
    offset.anchorX = midpointX;
    offset.anchorY = midpointY;
    offset.hasAnchor = true;
    return {
        {
            QStringLiteral("dimension_start"),
            QStringLiteral("endpoint"),
            QStringLiteral("resize"),
            QStringLiteral("x1_y1"),
            {QStringLiteral("x1_px"), QStringLiteral("y1_px")},
            x1,
            y1
        },
        {
            QStringLiteral("dimension_end"),
            QStringLiteral("endpoint"),
            QStringLiteral("resize"),
            QStringLiteral("x2_y2"),
            {QStringLiteral("x2_px"), QStringLiteral("y2_px")},
            x2,
            y2
        },
        offset
    };
}

std::vector<HandleDescriptor> readOnlyVertexHandles(const CanvasObjectView &object) {
    std::vector<HandleDescriptor> handles;
    const std::vector<CanvasPoint> points = object.points();
    handles.reserve(points.size());
    for (std::size_t index = 0; index < points.size(); ++index) {
        handles.push_back({
            QStringLiteral("vertex_%1").arg(index),
            QStringLiteral("vertex"),
            QStringLiteral("default"),
            QString(),
            {},
            clamp01(points[index].x),
            clamp01(points[index].y),
            true
        });
    }
    return handles;
}

double angleSnappedDegrees(double degrees, double increment) {
    const double step = std::max(1.0, finiteNumber(increment, 15.0));
    return std::round(finiteNumber(degrees, 0.0) / step) * step;
}

HandleUpdatePlan rectangleCornerUpdatePlan(const CanvasObjectView &object, const QString &handleId, const CanvasPoint &point, const QVariantMap &settings) {
    const double left = object.number(QStringLiteral("x"));
    const double top = object.number(QStringLiteral("y"));
    const double right = left + object.number(QStringLiteral("width"));
    const double bottom = top + object.number(QStringLiteral("height"));
    const CanvasPoint localPoint = unrotatePointForRect(object, point.x, point.y);
    const double fixedX = handleId == QStringLiteral("rect_nw") || handleId == QStringLiteral("rect_sw") ? right : left;
    const double fixedY = handleId == QStringLiteral("rect_nw") || handleId == QStringLiteral("rect_ne") ? bottom : top;
    double nextLeft = std::min(fixedX, localPoint.x);
    double nextTop = std::min(fixedY, localPoint.y);
    double nextWidth = std::max(1.0 / canvasSizePx(settings), std::abs(fixedX - localPoint.x));
    double nextHeight = std::max(1.0 / canvasSizePx(settings), std::abs(fixedY - localPoint.y));
    if (settings.value(QStringLiteral("shiftConstrain")).toBool()) {
        const double aspect = std::max(0.000001, object.number(QStringLiteral("width")))
            / std::max(0.000001, object.number(QStringLiteral("height")));
        if (nextWidth / std::max(0.000001, nextHeight) > aspect) {
            nextHeight = nextWidth / aspect;
        } else {
            nextWidth = nextHeight * aspect;
        }
        nextLeft = fixedX < localPoint.x ? fixedX : fixedX - nextWidth;
        nextTop = fixedY < localPoint.y ? fixedY : fixedY - nextHeight;
    }
    return updatePlan({
        {QStringLiteral("x_px"), rawNormalizedToPx(nextLeft, settings)},
        {QStringLiteral("y_px"), rawNormalizedToPx(nextTop, settings)},
        {QStringLiteral("width_px"), rawNormalizedToPx(nextWidth, settings)},
        {QStringLiteral("height_px"), rawNormalizedToPx(nextHeight, settings)}
    });
}

HandleUpdatePlan rectangleRotateUpdatePlan(const CanvasObjectView &object, const CanvasPoint &point, const QVariantMap &settings) {
    const CanvasPoint center = rotatedRectCenter(object);
    const double rotation = std::atan2(finiteNumber(point.y, 0.0) - center.y, finiteNumber(point.x, 0.0) - center.x) * 180.0 / pi + 90.0;
    double normalizedRotation = std::fmod(std::fmod(rotation, 360.0) + 360.0, 360.0);
    if (settings.value(QStringLiteral("shiftConstrain")).toBool()) {
        normalizedRotation = std::fmod(std::fmod(angleSnappedDegrees(normalizedRotation, finiteNumber(settings.value(QStringLiteral("angleSnapDeg")), 15.0)), 360.0) + 360.0, 360.0);
    }
    return updatePlan({{QStringLiteral("rotation_deg"), roundDegrees(normalizedRotation)}});
}

} // namespace

double canvasSizePx(const QVariantMap &settings) {
    return std::max(1.0, finiteNumber(settings.value(QStringLiteral("canvasSizePx")), 512.0));
}

double rotateHandleOffsetPx(const QVariantMap &settings) {
    return std::max(1.0, finiteNumber(settings.value(QStringLiteral("rotateHandleOffsetPx")), 28.0));
}

double normalizedToPx(double value, const QVariantMap &settings) {
    return std::round(clamp01(value) * canvasSizePx(settings) * 1000.0) / 1000.0;
}

double rawNormalizedToPx(double value, const QVariantMap &settings) {
    return std::round(finiteNumber(value, 0.0) * canvasSizePx(settings) * 1000.0) / 1000.0;
}

double roundDegrees(double value) {
    return std::round(finiteNumber(value, 0.0) * 1000.0) / 1000.0;
}

CanvasPoint rotatedRectCenter(const CanvasObjectView &object) {
    return {
        object.number(QStringLiteral("x")) + object.number(QStringLiteral("width")) / 2.0,
        object.number(QStringLiteral("y")) + object.number(QStringLiteral("height")) / 2.0
    };
}

std::vector<HandleDescriptor> rotatedRectCorners(const CanvasObjectView &object) {
    const double x = object.number(QStringLiteral("x"));
    const double y = object.number(QStringLiteral("y"));
    const double width = object.number(QStringLiteral("width"));
    const double height = object.number(QStringLiteral("height"));
    const double cx = x + width / 2.0;
    const double cy = y + height / 2.0;
    const double angle = object.number(QStringLiteral("rotation_deg")) * pi / 180.0;
    const double cosA = std::cos(angle);
    const double sinA = std::sin(angle);
    std::vector<HandleDescriptor> source = {
        {QStringLiteral("rect_nw"), QStringLiteral("corner"), QStringLiteral("resize"), QStringLiteral("x_y_width_height"), {QStringLiteral("x_px"), QStringLiteral("y_px"), QStringLiteral("width_px"), QStringLiteral("height_px")}, x, y},
        {QStringLiteral("rect_ne"), QStringLiteral("corner"), QStringLiteral("resize"), QStringLiteral("x_y_width_height"), {QStringLiteral("x_px"), QStringLiteral("y_px"), QStringLiteral("width_px"), QStringLiteral("height_px")}, x + width, y},
        {QStringLiteral("rect_sw"), QStringLiteral("corner"), QStringLiteral("resize"), QStringLiteral("x_y_width_height"), {QStringLiteral("x_px"), QStringLiteral("y_px"), QStringLiteral("width_px"), QStringLiteral("height_px")}, x, y + height},
        {QStringLiteral("rect_se"), QStringLiteral("corner"), QStringLiteral("resize"), QStringLiteral("x_y_width_height"), {QStringLiteral("x_px"), QStringLiteral("y_px"), QStringLiteral("width_px"), QStringLiteral("height_px")}, x + width, y + height}
    };
    for (HandleDescriptor &handle : source) {
        const double dx = handle.x - cx;
        const double dy = handle.y - cy;
        handle.x = cx + dx * cosA - dy * sinA;
        handle.y = cy + dx * sinA + dy * cosA;
    }
    return source;
}

CanvasPoint rotatedRectTopMidpoint(const CanvasObjectView &object) {
    const std::vector<HandleDescriptor> corners = rotatedRectCorners(object);
    if (corners.size() < 2) {
        return rotatedRectCenter(object);
    }
    return {
        (finiteNumber(corners[0].x, 0.0) + finiteNumber(corners[1].x, 0.0)) / 2.0,
        (finiteNumber(corners[0].y, 0.0) + finiteNumber(corners[1].y, 0.0)) / 2.0
    };
}

HandleDescriptor rotatedRectRotationHandle(const CanvasObjectView &object, const QVariantMap &settings) {
    const CanvasPoint center = rotatedRectCenter(object);
    const CanvasPoint top = rotatedRectTopMidpoint(object);
    const double dx = top.x - center.x;
    const double dy = top.y - center.y;
    const double length = std::max(0.000001, std::sqrt(dx * dx + dy * dy));
    const double offset = rotateHandleOffsetPx(settings) / canvasSizePx(settings);
    HandleDescriptor result {
        QStringLiteral("rect_rotate"),
        QStringLiteral("rotate"),
        QStringLiteral("rotate"),
        QStringLiteral("rotation_deg"),
        {QStringLiteral("rotation_deg")},
        top.x + dx / length * offset,
        top.y + dy / length * offset
    };
    result.anchorX = top.x;
    result.anchorY = top.y;
    result.hasAnchor = true;
    return result;
}

CanvasPoint unrotatePointForRect(const CanvasObjectView &object, double x, double y) {
    const CanvasPoint center = rotatedRectCenter(object);
    const double angle = -object.number(QStringLiteral("rotation_deg")) * pi / 180.0;
    const double dx = finiteNumber(x, 0.0) - center.x;
    const double dy = finiteNumber(y, 0.0) - center.y;
    const double cosA = std::cos(angle);
    const double sinA = std::sin(angle);
    return {
        center.x + dx * cosA - dy * sinA,
        center.y + dx * sinA + dy * cosA
    };
}

std::vector<HandleDescriptor> handlesForObject(const CanvasObjectView &object, const QVariantMap &settings) {
    const QString kind = object.kind();
    if (kind == QStringLiteral("point") || kind == QStringLiteral("tone_probe")) {
        return pointHandles(object);
    }
    if (kind == QStringLiteral("line") || kind == QStringLiteral("glyph_baseline")) {
        return lineHandles(object);
    }
    if (kind == QStringLiteral("circle") || kind == QStringLiteral("arc")) {
        return circleHandles(object);
    }
    if (kind == QStringLiteral("dimension")) {
        return dimensionHandles(object);
    }
    if (isRectangleLike(kind)) {
        std::vector<HandleDescriptor> handles = rotatedRectCorners(object);
        handles.push_back(rotatedRectRotationHandle(object, settings));
        return handles;
    }
    if (kind == QStringLiteral("polyline") || kind == QStringLiteral("polygon")) {
        return readOnlyVertexHandles(object);
    }
    return {};
}

std::vector<HandleDescriptor> visibleHandlesForObject(const CanvasObjectView &object, const QVariantMap &settings) {
    std::vector<HandleDescriptor> result;
    for (const HandleDescriptor &handle : handlesForObject(object, settings)) {
        if (handle.visible) {
            result.push_back(handle);
        }
    }
    return result;
}

HandleDescriptor handleById(const CanvasObjectView &object, const QString &handleId, const QVariantMap &settings) {
    for (const HandleDescriptor &handle : handlesForObject(object, settings)) {
        if (handle.id == handleId) {
            return handle;
        }
    }
    return {};
}

HitResult hitHandleAt(const CanvasObjectView &object, double screenX, double screenY, const BoardBounds &bounds, const QVariantMap &settings) {
    HitResult best;
    for (const HandleDescriptor &handle : visibleHandlesForObject(object, settings)) {
        const double px = finiteNumber(bounds.x, 0.0) + finiteNumber(handle.x, 0.0) * std::max(0.000001, finiteNumber(bounds.size, 1.0));
        const double py = finiteNumber(bounds.y, 0.0) + finiteNumber(handle.y, 0.0) * std::max(0.000001, finiteNumber(bounds.size, 1.0));
        const double dx = finiteNumber(screenX, 0.0) - px;
        const double dy = finiteNumber(screenY, 0.0) - py;
        const double distance = std::sqrt(dx * dx + dy * dy);
        const double threshold = handle.role == QStringLiteral("rotate")
            ? std::max(0.0, finiteNumber(settings.value(QStringLiteral("rotateHandleHitTolerancePx")), 18.0))
            : std::max(0.0, finiteNumber(settings.value(QStringLiteral("handleHitTolerancePx")), 14.0));
        if (distance <= threshold && distance <= best.distance) {
            best.ok = true;
            best.objectId = handle.id;
            best.kind = QStringLiteral("handle");
            best.distance = distance;
        }
    }
    return best;
}

HandleUpdatePlan handleUpdatePlan(const CanvasObjectView &object, const QString &handleId, const CanvasPoint &point, const QVariantMap &settings) {
    const HandleDescriptor handle = handleById(object, handleId, settings);
    if (handle.id.isEmpty() || handle.readOnly) {
        return emptyUpdatePlan();
    }
    const QString kind = object.kind();
    const double x = clamp01(point.x);
    const double y = clamp01(point.y);
    if ((kind == QStringLiteral("point") || kind == QStringLiteral("tone_probe")) && handleId == QStringLiteral("point_position")) {
        return updatePlan({{QStringLiteral("x_px"), normalizedToPx(x, settings)}, {QStringLiteral("y_px"), normalizedToPx(y, settings)}});
    }
    if ((kind == QStringLiteral("line") || kind == QStringLiteral("glyph_baseline")) && handleId == QStringLiteral("line_start")) {
        return updatePlan({{QStringLiteral("x1_px"), normalizedToPx(x, settings)}, {QStringLiteral("y1_px"), normalizedToPx(y, settings)}});
    }
    if ((kind == QStringLiteral("line") || kind == QStringLiteral("glyph_baseline")) && handleId == QStringLiteral("line_end")) {
        return updatePlan({{QStringLiteral("x2_px"), normalizedToPx(x, settings)}, {QStringLiteral("y2_px"), normalizedToPx(y, settings)}});
    }
    if ((kind == QStringLiteral("circle") || kind == QStringLiteral("arc")) && handleId == QStringLiteral("circle_center")) {
        return updatePlan({{QStringLiteral("cx_px"), normalizedToPx(x, settings)}, {QStringLiteral("cy_px"), normalizedToPx(y, settings)}});
    }
    if ((kind == QStringLiteral("circle") || kind == QStringLiteral("arc")) && handleId == QStringLiteral("circle_radius")) {
        const double dx = x - object.number(QStringLiteral("cx"));
        const double dy = y - object.number(QStringLiteral("cy"));
        return updatePlan({{QStringLiteral("radius_px"), rawNormalizedToPx(std::sqrt(dx * dx + dy * dy), settings)}});
    }
    if (kind == QStringLiteral("dimension") && handleId == QStringLiteral("dimension_start")) {
        return updatePlan({{QStringLiteral("x1_px"), normalizedToPx(x, settings)}, {QStringLiteral("y1_px"), normalizedToPx(y, settings)}});
    }
    if (kind == QStringLiteral("dimension") && handleId == QStringLiteral("dimension_end")) {
        return updatePlan({{QStringLiteral("x2_px"), normalizedToPx(x, settings)}, {QStringLiteral("y2_px"), normalizedToPx(y, settings)}});
    }
    if (kind == QStringLiteral("dimension") && handleId == QStringLiteral("dimension_offset")) {
        const double x1 = object.number(QStringLiteral("x1"));
        const double y1 = object.number(QStringLiteral("y1"));
        const double x2 = object.number(QStringLiteral("x2"));
        const double y2 = object.number(QStringLiteral("y2"));
        const double dx = x2 - x1;
        const double dy = y2 - y1;
        const double length = std::max(0.000001, std::sqrt(dx * dx + dy * dy));
        const double midpointX = (x1 + x2) / 2.0;
        const double midpointY = (y1 + y2) / 2.0;
        const double nx = -dy / length;
        const double ny = dx / length;
        const double offset = (x - midpointX) * nx + (y - midpointY) * ny;
        return updatePlan({{QStringLiteral("offset_px"), rawNormalizedToPx(offset, settings)}});
    }
    if (isRectangleLike(kind) && handleId == QStringLiteral("rect_rotate")) {
        return rectangleRotateUpdatePlan(object, point, settings);
    }
    if (isRectangleLike(kind)
        && (handleId == QStringLiteral("rect_nw") || handleId == QStringLiteral("rect_ne")
            || handleId == QStringLiteral("rect_sw") || handleId == QStringLiteral("rect_se"))) {
        return rectangleCornerUpdatePlan(object, handleId, point, settings);
    }
    return emptyUpdatePlan();
}

} // namespace drawing_canvas
