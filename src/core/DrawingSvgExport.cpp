#include "DrawingCore.h"
#include "DrawingCoreInternal.h"

#include <QRegularExpression>

#include <algorithm>
#include <cmath>

namespace drawing_core {

QString svgNumber(double value) {
    return QString::number(value, 'f', 3).replace(QRegularExpression("\\.?0+$"), "");
}

QString pointsToSvg(const QJsonArray &points) {
    QStringList values;
    for (const QJsonValue value : points) {
        const QJsonArray point = value.toArray();
        if (point.size() >= 2) {
            values.append(svgNumber(point.at(0).toDouble()) + "," + svgNumber(point.at(1).toDouble()));
        }
    }
    return values.join(" ");
}

QString svgEscaped(const QString &value) {
    return value.toHtmlEscaped();
}

QString svgDashArray(const QString &lineStyle, double strokeWidth) {
    const QString style = normalizedLineStyle(lineStyle);
    const double width = std::max(1.0, strokeWidth);
    if (style == QStringLiteral("dashed")) {
        return svgNumber(width * 3.2) + QStringLiteral(" ") + svgNumber(width * 2.1);
    }
    if (style == QStringLiteral("dotted")) {
        return svgNumber(width) + QStringLiteral(" ") + svgNumber(width * 2.0);
    }
    return {};
}

QString svgDataAttribute(const QJsonObject &object, const QString &field, const QString &attribute) {
    if (!object.contains(field)) {
        return {};
    }
    QString value;
    if (object.value(field).isArray()) {
        QStringList values;
        const QJsonArray items = object.value(field).toArray();
        for (const QJsonValue item : items) {
            const QString text = item.toString().trimmed();
            if (!text.isEmpty()) {
                values.append(text);
            }
        }
        value = values.join(QStringLiteral(","));
    } else {
        value = object.value(field).toString().trimmed();
    }
    if (value.isEmpty()) {
        return {};
    }
    return QStringLiteral(" data-%1=\"%2\"").arg(attribute, svgEscaped(value));
}

QString svgMetadataAttributes(const QJsonObject &object) {
    QString attributes;
    attributes += svgDataAttribute(object, QStringLiteral("role"), QStringLiteral("role"));
    attributes += svgDataAttribute(object, QStringLiteral("material"), QStringLiteral("material"));
    attributes += svgDataAttribute(object, QStringLiteral("intent"), QStringLiteral("intent"));
    attributes += svgDataAttribute(object, QStringLiteral("export_group"), QStringLiteral("export-group"));
    attributes += svgDataAttribute(object, QStringLiteral("tags"), QStringLiteral("tags"));
    return attributes;
}

QString svgCommonAttributes(const QJsonObject &object, bool fillAllowed, const QString &fallbackStroke) {
    const QString strokeColor = normalizedHexColor(stringAt(object, QStringLiteral("stroke_color"), fallbackStroke));
    const QString fillColor = fillAllowed ? normalizedHexColor(stringAt(object, QStringLiteral("fill_color"))) : QString();
    const double strokeWidth = std::clamp(numberAt(object, QStringLiteral("line_thickness"), 2.0), 0.25, 64.0);
    const double strokeOpacity = std::clamp(numberAt(object, QStringLiteral("stroke_opacity"), 1.0), 0.0, 1.0);
    const QString dash = svgDashArray(stringAt(object, QStringLiteral("line_style"), QStringLiteral("solid")), strokeWidth);

    QString attributes;
    attributes += QStringLiteral(" stroke=\"%1\"").arg(svgEscaped(strokeColor.isEmpty() ? fallbackStroke : strokeColor));
    attributes += QStringLiteral(" stroke-width=\"%1\"").arg(svgNumber(strokeWidth));
    attributes += QStringLiteral(" stroke-linecap=\"round\" stroke-linejoin=\"round\"");
    attributes += QStringLiteral(" stroke-opacity=\"%1\"").arg(svgNumber(strokeOpacity));
    attributes += QStringLiteral(" fill=\"%1\"").arg(fillColor.isEmpty() ? QStringLiteral("none") : svgEscaped(fillColor));
    if (!fillColor.isEmpty()) {
        attributes += QStringLiteral(" fill-opacity=\"%1\"").arg(svgNumber(strokeOpacity));
    }
    if (!dash.isEmpty()) {
        attributes += QStringLiteral(" stroke-dasharray=\"%1\"").arg(svgEscaped(dash));
    }
    attributes += svgMetadataAttributes(object);
    return attributes;
}

QString svgPointAttributes(const QJsonObject &object, const QString &fallbackStroke) {
    const QString strokeColor = normalizedHexColor(stringAt(object, QStringLiteral("stroke_color"), fallbackStroke));
    const QString fillColor = normalizedHexColor(stringAt(object, QStringLiteral("fill_color")));
    const double strokeWidth = std::clamp(numberAt(object, QStringLiteral("line_thickness"), 2.0), 0.25, 64.0);
    const double strokeOpacity = std::clamp(numberAt(object, QStringLiteral("stroke_opacity"), 1.0), 0.0, 1.0);
    const QString resolvedStroke = strokeColor.isEmpty() ? fallbackStroke : strokeColor;
    QString attributes;
    attributes += QStringLiteral(" fill=\"%1\"").arg(svgEscaped(fillColor.isEmpty() ? resolvedStroke : fillColor));
    attributes += QStringLiteral(" stroke=\"%1\"").arg(svgEscaped(resolvedStroke));
    attributes += QStringLiteral(" stroke-width=\"%1\"").arg(svgNumber(std::max(1.0, strokeWidth * 0.5)));
    attributes += QStringLiteral(" opacity=\"%1\"").arg(svgNumber(strokeOpacity));
    attributes += svgMetadataAttributes(object);
    return attributes;
}

void appendSerializedObjectToSvg(QString &svg, const QJsonObject &object) {
    const QString kind = object.value("kind").toString();
    const ShapeKind shapeKind = objectShapeKind(object);
    switch (shapeKind) {
    case ShapeKind::Point: {
        const QJsonArray point = object.value("point_px").toArray();
        if (kind == QStringLiteral("tone_probe")) {
            svg += QStringLiteral("    <circle id=\"%1\" cx=\"%2\" cy=\"%3\" r=\"7\"%4/>\n")
                .arg(svgEscaped(object.value("id").toString()), svgNumber(point.at(0).toDouble()), svgNumber(point.at(1).toDouble()), svgPointAttributes(object, QStringLiteral("#70d6ff")));
        } else {
            svg += QStringLiteral("    <circle id=\"%1\" cx=\"%2\" cy=\"%3\" r=\"5\"%4/>\n")
                .arg(svgEscaped(object.value("id").toString()), svgNumber(point.at(0).toDouble()), svgNumber(point.at(1).toDouble()), svgPointAttributes(object));
        }
        break;
    }
    case ShapeKind::Line: {
        const QJsonArray from = object.value("from_px").toArray();
        const QJsonArray to = object.value("to_px").toArray();
        if (kind == QStringLiteral("glyph_baseline")) {
            svg += QStringLiteral("    <line id=\"%1\" x1=\"%2\" y1=\"%3\" x2=\"%4\" y2=\"%5\"%6/>\n")
                .arg(svgEscaped(object.value("id").toString()), svgNumber(from.at(0).toDouble()), svgNumber(from.at(1).toDouble()), svgNumber(to.at(0).toDouble()), svgNumber(to.at(1).toDouble()), svgCommonAttributes(object, false, QStringLiteral("#70d6ff")));
        } else {
            svg += QStringLiteral("    <line id=\"%1\" x1=\"%2\" y1=\"%3\" x2=\"%4\" y2=\"%5\"%6/>\n")
                .arg(svgEscaped(object.value("id").toString()), svgNumber(from.at(0).toDouble()), svgNumber(from.at(1).toDouble()), svgNumber(to.at(0).toDouble()), svgNumber(to.at(1).toDouble()), svgCommonAttributes(object, false));
        }
        break;
    }
    case ShapeKind::Circle: {
        const QJsonArray center = object.value("center_px").toArray();
        if (kind == QStringLiteral("arc")) {
            const double radius = object.value("radius_px").toDouble();
            const double start = degreesToRadians(object.value("start_angle_deg").toDouble());
            const double end = degreesToRadians(object.value("end_angle_deg").toDouble());
            const double x1 = center.at(0).toDouble() + std::cos(start) * radius;
            const double y1 = center.at(1).toDouble() + std::sin(start) * radius;
            const double x2 = center.at(0).toDouble() + std::cos(end) * radius;
            const double y2 = center.at(1).toDouble() + std::sin(end) * radius;
            const int largeArc = std::abs(object.value("end_angle_deg").toDouble() - object.value("start_angle_deg").toDouble()) > 180.0 ? 1 : 0;
            svg += QStringLiteral("    <path id=\"%1\" d=\"M %2 %3 A %4 %4 0 %5 1 %6 %7\"%8/>\n")
                .arg(svgEscaped(object.value("id").toString()), svgNumber(x1), svgNumber(y1), svgNumber(radius), QString::number(largeArc), svgNumber(x2), svgNumber(y2), svgCommonAttributes(object, false));
        } else {
            svg += QStringLiteral("    <circle id=\"%1\" cx=\"%2\" cy=\"%3\" r=\"%4\"%5/>\n")
                .arg(svgEscaped(object.value("id").toString()), svgNumber(center.at(0).toDouble()), svgNumber(center.at(1).toDouble()), svgNumber(object.value("radius_px").toDouble()), svgCommonAttributes(object, true));
        }
        break;
    }
    case ShapeKind::Rectangle: {
        const QJsonArray rect = object.value("rect_px").toArray();
        if (kind == QStringLiteral("rectangle")) {
            svg += QStringLiteral("    <rect id=\"%1\" x=\"%2\" y=\"%3\" width=\"%4\" height=\"%5\"%6/>\n")
                .arg(svgEscaped(object.value("id").toString()), svgNumber(rect.at(0).toDouble()), svgNumber(rect.at(1).toDouble()), svgNumber(rect.at(2).toDouble()), svgNumber(rect.at(3).toDouble()), svgCommonAttributes(object, true));
        } else if (kind == QStringLiteral("image_reference_frame")) {
            svg += QStringLiteral("    <rect id=\"%1\" x=\"%2\" y=\"%3\" width=\"%4\" height=\"%5\"%6/>\n")
                .arg(svgEscaped(object.value("id").toString()), svgNumber(rect.at(0).toDouble()), svgNumber(rect.at(1).toDouble()), svgNumber(rect.at(2).toDouble()), svgNumber(rect.at(3).toDouble()), svgCommonAttributes(object, true, QStringLiteral("#70d6ff")));
        } else if (kind == QStringLiteral("ascii_crop_frame")) {
            svg += QStringLiteral("    <rect id=\"%1\" x=\"%2\" y=\"%3\" width=\"%4\" height=\"%5\"%6/>\n")
                .arg(svgEscaped(object.value("id").toString()), svgNumber(rect.at(0).toDouble()), svgNumber(rect.at(1).toDouble()), svgNumber(rect.at(2).toDouble()), svgNumber(rect.at(3).toDouble()), svgCommonAttributes(object, true, QStringLiteral("#ffcc66")));
        } else {
            svg += QStringLiteral("    <rect id=\"%1\" x=\"%2\" y=\"%3\" width=\"%4\" height=\"%5\"%6/>\n")
                .arg(svgEscaped(object.value("id").toString()), svgNumber(rect.at(0).toDouble()), svgNumber(rect.at(1).toDouble()), svgNumber(rect.at(2).toDouble()), svgNumber(rect.at(3).toDouble()), svgCommonAttributes(object, true, QStringLiteral("#c084fc")));
        }
        break;
    }
    case ShapeKind::Polyline:
        svg += QStringLiteral("    <polyline id=\"%1\" points=\"%2\"%3/>\n")
            .arg(svgEscaped(object.value("id").toString()), svgEscaped(pointsToSvg(object.value("points_px").toArray())), svgCommonAttributes(object, false));
        break;
    case ShapeKind::Polygon:
        svg += QStringLiteral("    <polygon id=\"%1\" points=\"%2\"%3/>\n")
            .arg(svgEscaped(object.value("id").toString()), svgEscaped(pointsToSvg(object.value("points_px").toArray())), svgCommonAttributes(object, true));
        break;
    case ShapeKind::Unknown:
        break;
    }
}

} // namespace drawing_core

QString DrawingCore::modelToSvg(const QJsonObject &model) {
    const int canvas = model.value("canvas_px").toArray().at(0).toInt(drawing_core::kDefaultCanvasPx);
    QString svg;
    svg += QStringLiteral("<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 %1 %1\" width=\"%1\" height=\"%1\">\n").arg(canvas);
    svg += QStringLiteral("  <g id=\"script_geometry\">\n");
    const QJsonArray objects = model.value("generated_objects").toArray();
    for (const QJsonValue value : objects) {
        drawing_core::appendSerializedObjectToSvg(svg, value.toObject());
    }
    svg += QStringLiteral("  </g>\n</svg>\n");
    return svg;
}
