#include "BlenderSvgBundleExport.h"

#include <QDateTime>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QStringList>

namespace {

void copyIfPresent(QJsonObject &target, const QJsonObject &source, const QString &key) {
    if (source.contains(key) && !source.value(key).isUndefined()) {
        target.insert(key, source.value(key));
    }
}

QJsonObject objectStyleManifest(const QJsonObject &object) {
    QJsonObject style;
    copyIfPresent(style, object, QStringLiteral("stroke_color"));
    copyIfPresent(style, object, QStringLiteral("fill_color"));
    copyIfPresent(style, object, QStringLiteral("line_thickness"));
    copyIfPresent(style, object, QStringLiteral("line_style"));
    copyIfPresent(style, object, QStringLiteral("stroke_opacity"));
    copyIfPresent(style, object, QStringLiteral("fill_opacity"));
    return style;
}

QJsonObject objectCoordinateManifest(const QJsonObject &object) {
    QJsonObject coordinates;
    const QStringList keys = {
        QStringLiteral("point_px"),
        QStringLiteral("points"),
        QStringLiteral("x"),
        QStringLiteral("y"),
        QStringLiteral("width"),
        QStringLiteral("height"),
        QStringLiteral("x1"),
        QStringLiteral("y1"),
        QStringLiteral("x2"),
        QStringLiteral("y2"),
        QStringLiteral("cx"),
        QStringLiteral("cy"),
        QStringLiteral("radius"),
        QStringLiteral("start_angle_deg"),
        QStringLiteral("end_angle_deg"),
        QStringLiteral("rotation_deg"),
        QStringLiteral("sides"),
    };
    for (const QString &key : keys) {
        copyIfPresent(coordinates, object, key);
    }
    return coordinates;
}

QJsonArray objectTagsManifest(const QJsonObject &object) {
    const QJsonValue tagsValue = object.value(QStringLiteral("tags"));
    if (tagsValue.isArray()) {
        return tagsValue.toArray();
    }
    if (tagsValue.isString() && !tagsValue.toString().trimmed().isEmpty()) {
        QJsonArray tags;
        tags.append(tagsValue.toString().trimmed());
        return tags;
    }
    return {};
}

QJsonObject objectMetadataManifest(const QJsonObject &object) {
    QJsonObject metadata;
    const QStringList keys = {
        QStringLiteral("intent"),
        QStringLiteral("role"),
        QStringLiteral("material"),
        QStringLiteral("export_group"),
    };
    for (const QString &key : keys) {
        copyIfPresent(metadata, object, key);
    }
    return metadata;
}

QJsonObject objectManifestEntry(const QJsonObject &object) {
    QJsonObject entry;
    entry.insert(QStringLiteral("id"), object.value(QStringLiteral("id")).toString());
    entry.insert(QStringLiteral("type"), object.value(QStringLiteral("kind")).toString(QStringLiteral("unknown")));
    entry.insert(QStringLiteral("tags"), objectTagsManifest(object));
    const QJsonObject metadata = objectMetadataManifest(object);
    for (const QString &key : metadata.keys()) {
        entry.insert(key, metadata.value(key));
    }
    copyIfPresent(entry, object, QStringLiteral("layer_id"));
    copyIfPresent(entry, object, QStringLiteral("line_variant"));
    copyIfPresent(entry, object, QStringLiteral("circle_arc_mode"));
    entry.insert(QStringLiteral("metadata"), metadata);
    entry.insert(QStringLiteral("style"), objectStyleManifest(object));
    entry.insert(QStringLiteral("source_coordinates"), objectCoordinateManifest(object));
    return entry;
}

bool hasNonEmptyString(const QJsonObject &object, const QString &key) {
    return object.value(key).toString().trimmed().length() > 0;
}

bool hasNonEmptyTags(const QJsonObject &object) {
    return !object.value(QStringLiteral("tags")).toArray().isEmpty();
}

bool isSupportedManifestObjectType(const QString &type) {
    static const QStringList supportedTypes = {
        QStringLiteral("point"),
        QStringLiteral("line"),
        QStringLiteral("polyline"),
        QStringLiteral("circle"),
        QStringLiteral("arc"),
        QStringLiteral("rectangle"),
        QStringLiteral("polygon"),
        QStringLiteral("image_reference_frame"),
        QStringLiteral("ascii_crop_frame"),
        QStringLiteral("ascii_cell_region"),
        QStringLiteral("tone_probe"),
        QStringLiteral("glyph_baseline"),
    };
    return supportedTypes.contains(type);
}

}  // namespace

QJsonObject BlenderSvgBundleExport::manifest(const QString &bundlePath, const QJsonObject &model) {
    const QFileInfo bundleInfo(bundlePath);
    const QJsonArray canvas = model.value(QStringLiteral("canvas_px")).toArray();
    const QJsonArray objects = model.value(QStringLiteral("generated_objects")).toArray();

    QJsonArray manifestObjects;
    for (const QJsonValue &value : objects) {
        const QJsonObject object = value.toObject();
        if (!object.isEmpty()) {
            manifestObjects.append(objectManifestEntry(object));
        }
    }

    QJsonObject importHints;
    importHints.insert(QStringLiteral("svg_file"), QStringLiteral("drawing.svg"));
    importHints.insert(QStringLiteral("scale"), 0.01);
    importHints.insert(QStringLiteral("collection_name"), QStringLiteral("Draftsman SVG"));
    importHints.insert(QStringLiteral("origin_mode"), QStringLiteral("svg_top_left_canvas_space"));

    QJsonObject manifest;
    manifest.insert(QStringLiteral("schema"), QStringLiteral("draftsman_blender_svg_bundle_manifest_v1"));
    manifest.insert(QStringLiteral("document_name"),
                   bundleInfo.completeBaseName().isEmpty() ? bundleInfo.fileName() : bundleInfo.completeBaseName());
    manifest.insert(QStringLiteral("exported_at_utc"), QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs));
    manifest.insert(QStringLiteral("canvas_px"), canvas);
    manifest.insert(QStringLiteral("object_count"), manifestObjects.size());
    manifest.insert(QStringLiteral("objects"), manifestObjects);
    manifest.insert(QStringLiteral("blender_import"), importHints);
    return manifest;
}

QJsonObject BlenderSvgBundleExport::exportReport(const QString &bundlePath,
                                                 const QString &svgPath,
                                                 const QString &manifestPath,
                                                 const QString &scriptPath,
                                                 const QString &readmePath,
                                                 const QString &reportPath,
                                                 const QString &reportTextPath,
                                                 const QString &verifyPath,
                                                 const QJsonObject &manifest) {
    const QJsonArray objects = manifest.value(QStringLiteral("objects")).toArray();
    QJsonObject typeCounts;
    QJsonArray unknownTypes;
    int missingIdCount = 0;
    int roleCount = 0;
    int materialCount = 0;
    int exportGroupCount = 0;
    int tagsCount = 0;
    int missingMetadataCount = 0;

    for (const QJsonValue &value : objects) {
        const QJsonObject object = value.toObject();
        const QString id = object.value(QStringLiteral("id")).toString().trimmed();
        const QString type = object.value(QStringLiteral("type")).toString(QStringLiteral("unknown")).trimmed();
        const QString countKey = type.isEmpty() ? QStringLiteral("unknown") : type;
        typeCounts.insert(countKey, typeCounts.value(countKey).toInt() + 1);

        if (id.isEmpty()) {
            ++missingIdCount;
        }
        if (hasNonEmptyString(object, QStringLiteral("role"))) {
            ++roleCount;
        }
        if (hasNonEmptyString(object, QStringLiteral("material"))) {
            ++materialCount;
        }
        if (hasNonEmptyString(object, QStringLiteral("export_group"))) {
            ++exportGroupCount;
        }
        if (hasNonEmptyTags(object)) {
            ++tagsCount;
        }
        if (!hasNonEmptyString(object, QStringLiteral("role"))
            && !hasNonEmptyString(object, QStringLiteral("material"))
            && !hasNonEmptyString(object, QStringLiteral("export_group"))
            && !hasNonEmptyString(object, QStringLiteral("intent"))
            && !hasNonEmptyTags(object)) {
            ++missingMetadataCount;
        }
        if (!isSupportedManifestObjectType(countKey) && !unknownTypes.contains(countKey)) {
            unknownTypes.append(countKey);
        }
    }

    QJsonArray warnings;
    if (objects.isEmpty()) {
        warnings.append(QStringLiteral("no objects exported"));
    }
    if (missingIdCount > 0) {
        warnings.append(QStringLiteral("objects missing IDs"));
    }
    if (missingMetadataCount > 0) {
        warnings.append(QStringLiteral("objects missing metadata"));
    }
    if (!unknownTypes.isEmpty()) {
        warnings.append(QStringLiteral("unsupported or unknown object type"));
    }

    QJsonObject paths;
    paths.insert(QStringLiteral("bundle"), bundlePath);
    paths.insert(QStringLiteral("svg"), svgPath);
    paths.insert(QStringLiteral("manifest"), manifestPath);
    paths.insert(QStringLiteral("script"), scriptPath);
    paths.insert(QStringLiteral("readme"), readmePath);
    paths.insert(QStringLiteral("report_json"), reportPath);
    paths.insert(QStringLiteral("report_text"), reportTextPath);
    paths.insert(QStringLiteral("verify_script"), verifyPath);

    QJsonObject metadataCoverage;
    metadataCoverage.insert(QStringLiteral("with_role"), roleCount);
    metadataCoverage.insert(QStringLiteral("with_material"), materialCount);
    metadataCoverage.insert(QStringLiteral("with_export_group"), exportGroupCount);
    metadataCoverage.insert(QStringLiteral("with_tags"), tagsCount);
    metadataCoverage.insert(QStringLiteral("missing_metadata"), missingMetadataCount);
    metadataCoverage.insert(QStringLiteral("missing_ids"), missingIdCount);

    QJsonObject report;
    report.insert(QStringLiteral("schema"), QStringLiteral("draftsman_blender_svg_export_report_v1"));
    report.insert(QStringLiteral("exported_at_utc"), QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs));
    report.insert(QStringLiteral("bundle_path"), bundlePath);
    report.insert(QStringLiteral("paths"), paths);
    report.insert(QStringLiteral("object_count"), objects.size());
    report.insert(QStringLiteral("object_type_counts"), typeCounts);
    report.insert(QStringLiteral("metadata_coverage"), metadataCoverage);
    report.insert(QStringLiteral("unknown_types"), unknownTypes);
    report.insert(QStringLiteral("warnings"), warnings);
    return report;
}

QString BlenderSvgBundleExport::exportReportText(const QJsonObject &report) {
    QStringList lines;
    lines << QStringLiteral("Draftsman Blender SVG Export Report");
    lines << QStringLiteral("Exported UTC: %1").arg(report.value(QStringLiteral("exported_at_utc")).toString());
    lines << QStringLiteral("Bundle: %1").arg(report.value(QStringLiteral("bundle_path")).toString());
    lines << QStringLiteral("Objects: %1").arg(report.value(QStringLiteral("object_count")).toInt());
    lines << QString();
    lines << QStringLiteral("Object Types:");
    const QJsonObject typeCounts = report.value(QStringLiteral("object_type_counts")).toObject();
    const QStringList typeKeys = typeCounts.keys();
    if (typeKeys.isEmpty()) {
        lines << QStringLiteral("- none");
    } else {
        for (const QString &key : typeKeys) {
            lines << QStringLiteral("- %1: %2").arg(key).arg(typeCounts.value(key).toInt());
        }
    }

    const QJsonObject coverage = report.value(QStringLiteral("metadata_coverage")).toObject();
    lines << QString();
    lines << QStringLiteral("Metadata Coverage:");
    lines << QStringLiteral("- role: %1").arg(coverage.value(QStringLiteral("with_role")).toInt());
    lines << QStringLiteral("- material: %1").arg(coverage.value(QStringLiteral("with_material")).toInt());
    lines << QStringLiteral("- export_group: %1").arg(coverage.value(QStringLiteral("with_export_group")).toInt());
    lines << QStringLiteral("- tags: %1").arg(coverage.value(QStringLiteral("with_tags")).toInt());
    lines << QStringLiteral("- missing metadata: %1").arg(coverage.value(QStringLiteral("missing_metadata")).toInt());
    lines << QStringLiteral("- missing IDs: %1").arg(coverage.value(QStringLiteral("missing_ids")).toInt());

    lines << QString();
    lines << QStringLiteral("Warnings:");
    const QJsonArray warnings = report.value(QStringLiteral("warnings")).toArray();
    if (warnings.isEmpty()) {
        lines << QStringLiteral("- none");
    } else {
        for (const QJsonValue &warning : warnings) {
            lines << QStringLiteral("- %1").arg(warning.toString());
        }
    }
    lines << QString();
    return lines.join(QLatin1Char('\n'));
}
