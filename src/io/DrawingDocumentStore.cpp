#include "DrawingDocumentStore.h"

#include <QCryptographicHash>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>
#include <QUrl>
#include <QStringList>

#include "export/BlenderSvgBundleTemplates.h"

DrawingDocumentStore::DrawingDocumentStore(QObject *parent)
    : QObject(parent) {}

QVariantMap DrawingDocumentStore::save(const QUrl &url, const QVariantMap &model) const {
    QVariantMap result;
    result.insert(QStringLiteral("ok"), false);
    result.insert(QStringLiteral("message"), QStringLiteral("save unavailable"));

    QString path = localPath(url);
    if (path.trimmed().isEmpty()) {
        result.insert(QStringLiteral("message"), QStringLiteral("drawing path missing"));
        return result;
    }
    if (QFileInfo(path).suffix().isEmpty()) {
        path += QStringLiteral(".json");
    }

    const QFileInfo info(path);
    if (!info.absoluteDir().exists() && !QDir().mkpath(info.absolutePath())) {
        result.insert(QStringLiteral("message"), QStringLiteral("drawing directory unavailable"));
        return result;
    }

    const QJsonObject modelObject = QJsonObject::fromVariantMap(model);
    if (modelObject.isEmpty()) {
        result.insert(QStringLiteral("message"), QStringLiteral("drawing model empty"));
        return result;
    }
    if (modelObject.value(QStringLiteral("export_kind")).toString() != QStringLiteral("pattern_lab_2d_native_model_v0")) {
        result.insert(QStringLiteral("message"), QStringLiteral("drawing model rejected"));
        return result;
    }

    QJsonObject object;
    object.insert(QStringLiteral("document_kind"), QStringLiteral("draftsman_drawing_document_v1"));
    object.insert(QStringLiteral("model_kind"), QStringLiteral("pattern_lab_2d_native_model_v0"));
    object.insert(QStringLiteral("model"), modelObject);

    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        result.insert(QStringLiteral("message"), QStringLiteral("drawing write failed"));
        return result;
    }
    file.write(QJsonDocument(object).toJson(QJsonDocument::Indented));
    if (!file.commit()) {
        result.insert(QStringLiteral("message"), QStringLiteral("drawing commit failed"));
        return result;
    }

    result.insert(QStringLiteral("ok"), true);
    result.insert(QStringLiteral("message"), QStringLiteral("saved drawing"));
    result.insert(QStringLiteral("path"), path);
    return result;
}

QVariantMap DrawingDocumentStore::open(const QUrl &url) const {
    QVariantMap result;
    result.insert(QStringLiteral("ok"), false);
    result.insert(QStringLiteral("message"), QStringLiteral("open unavailable"));

    const QString path = localPath(url);
    if (path.trimmed().isEmpty()) {
        result.insert(QStringLiteral("message"), QStringLiteral("drawing path missing"));
        return result;
    }

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        result.insert(QStringLiteral("message"), QStringLiteral("drawing read failed"));
        return result;
    }

    const QJsonDocument document = QJsonDocument::fromJson(file.readAll());
    if (!document.isObject()) {
        result.insert(QStringLiteral("message"), QStringLiteral("drawing json invalid"));
        return result;
    }

    const QJsonObject object = document.object();
    QJsonObject model = object;
    if (object.value(QStringLiteral("document_kind")).toString() == QStringLiteral("draftsman_drawing_document_v1")) {
        model = object.value(QStringLiteral("model")).toObject();
    }
    if (model.value(QStringLiteral("export_kind")).toString() != QStringLiteral("pattern_lab_2d_native_model_v0")) {
        result.insert(QStringLiteral("message"), QStringLiteral("not a Draftsman drawing"));
        return result;
    }

    result.insert(QStringLiteral("ok"), true);
    result.insert(QStringLiteral("message"), QStringLiteral("opened drawing"));
    result.insert(QStringLiteral("path"), path);
    result.insert(QStringLiteral("model"), model.toVariantMap());
    return result;
}

QVariantMap DrawingDocumentStore::exportSvg(const QUrl &url, const QString &svg) const {
    QVariantMap result;
    result.insert(QStringLiteral("ok"), false);
    result.insert(QStringLiteral("message"), QStringLiteral("svg export unavailable"));

    QString path = localPath(url);
    if (path.trimmed().isEmpty()) {
        result.insert(QStringLiteral("message"), QStringLiteral("svg path missing"));
        return result;
    }
    if (QFileInfo(path).suffix().isEmpty()) {
        path += QStringLiteral(".svg");
    }

    const QFileInfo info(path);
    if (!info.absoluteDir().exists() && !QDir().mkpath(info.absolutePath())) {
        result.insert(QStringLiteral("message"), QStringLiteral("svg directory unavailable"));
        return result;
    }
    if (svg.trimmed().isEmpty()) {
        result.insert(QStringLiteral("message"), QStringLiteral("svg output empty"));
        return result;
    }

    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        result.insert(QStringLiteral("message"), QStringLiteral("svg write failed"));
        return result;
    }
    file.write(svg.toUtf8());
    if (!file.commit()) {
        result.insert(QStringLiteral("message"), QStringLiteral("svg commit failed"));
        return result;
    }

    result.insert(QStringLiteral("ok"), true);
    result.insert(QStringLiteral("message"), QStringLiteral("exported svg"));
    result.insert(QStringLiteral("path"), path);
    return result;
}

QVariantMap DrawingDocumentStore::exportBlenderSvgBundle(const QUrl &url, const QString &svg, const QVariantMap &model) const {
    QVariantMap result;
    result.insert(QStringLiteral("ok"), false);
    result.insert(QStringLiteral("message"), QStringLiteral("Blender SVG bundle unavailable"));

    const QString selectedPath = localPath(url);
    if (selectedPath.trimmed().isEmpty()) {
        result.insert(QStringLiteral("message"), QStringLiteral("bundle path missing"));
        return result;
    }
    if (svg.trimmed().isEmpty()) {
        result.insert(QStringLiteral("message"), QStringLiteral("svg output empty"));
        return result;
    }

    const QString bundlePath = bundleDirectoryPath(selectedPath);
    if (!QDir().mkpath(bundlePath)) {
        result.insert(QStringLiteral("message"), QStringLiteral("bundle directory unavailable"));
        return result;
    }

    const QDir bundleDir(bundlePath);
    const QString svgPath = bundleDir.filePath(QStringLiteral("drawing.svg"));
    const QString scriptPath = bundleDir.filePath(QStringLiteral("import_drawing_svg.py"));
    const QString readmePath = bundleDir.filePath(QStringLiteral("README.txt"));
    const QString manifestPath = bundleDir.filePath(QStringLiteral("manifest.json"));
    const QJsonObject manifest = blenderSvgBundleManifest(bundlePath, QJsonObject::fromVariantMap(model));
    const QString reportPath = bundleDir.filePath(QStringLiteral("export_report.json"));
    const QString reportTextPath = bundleDir.filePath(QStringLiteral("export_report.txt"));
    const QString verifyPath = bundleDir.filePath(QStringLiteral("verify_bundle.py"));
    const QJsonObject exportReport = blenderSvgBundleExportReport(
        bundlePath,
        svgPath,
        manifestPath,
        scriptPath,
        readmePath,
        reportPath,
        reportTextPath,
        verifyPath,
        manifest);

    if (!writeTextFile(svgPath, svg)) {
        result.insert(QStringLiteral("message"), QStringLiteral("bundle svg write failed"));
        return result;
    }
    if (!writeTextFile(manifestPath, QString::fromUtf8(QJsonDocument(manifest).toJson(QJsonDocument::Indented)))) {
        result.insert(QStringLiteral("message"), QStringLiteral("bundle manifest write failed"));
        return result;
    }
    if (!writeTextFile(reportPath, QString::fromUtf8(QJsonDocument(exportReport).toJson(QJsonDocument::Indented)))) {
        result.insert(QStringLiteral("message"), QStringLiteral("bundle export report write failed"));
        return result;
    }
    if (!writeTextFile(reportTextPath, blenderSvgBundleExportReportText(exportReport))) {
        result.insert(QStringLiteral("message"), QStringLiteral("bundle export report text write failed"));
        return result;
    }
    if (!writeTextFile(verifyPath, BlenderSvgBundleTemplates::verifyScript())) {
        result.insert(QStringLiteral("message"), QStringLiteral("bundle verify script write failed"));
        return result;
    }
    if (!writeTextFile(scriptPath, BlenderSvgBundleTemplates::importScript())) {
        result.insert(QStringLiteral("message"), QStringLiteral("bundle script write failed"));
        return result;
    }
    if (!writeTextFile(readmePath, BlenderSvgBundleTemplates::readme())) {
        result.insert(QStringLiteral("message"), QStringLiteral("bundle readme write failed"));
        return result;
    }

    result.insert(QStringLiteral("ok"), true);
    result.insert(QStringLiteral("message"), QStringLiteral("exported Blender SVG bundle"));
    result.insert(QStringLiteral("path"), bundlePath);
    result.insert(QStringLiteral("svg_path"), svgPath);
    result.insert(QStringLiteral("manifest_path"), manifestPath);
    result.insert(QStringLiteral("report_path"), reportPath);
    result.insert(QStringLiteral("report_text_path"), reportTextPath);
    result.insert(QStringLiteral("verify_path"), verifyPath);
    result.insert(QStringLiteral("script_path"), scriptPath);
    result.insert(QStringLiteral("readme_path"), readmePath);
    return result;
}

QString DrawingDocumentStore::localPath(const QUrl &url) {
    if (url.isLocalFile()) {
        return url.toLocalFile();
    }
    const QString text = url.toString(QUrl::PreferLocalFile).trimmed();
    if (text.startsWith(QStringLiteral("file://"))) {
        return QUrl(text).toLocalFile();
    }
    return QUrl::fromPercentEncoding(text.toUtf8());
}

QString DrawingDocumentStore::bundleDirectoryPath(const QString &selectedPath) {
    const QFileInfo selectedInfo(selectedPath);
    if (selectedInfo.suffix().isEmpty()) {
        return selectedInfo.absoluteFilePath();
    }
    return selectedInfo.absoluteDir().filePath(selectedInfo.completeBaseName());
}

bool DrawingDocumentStore::writeTextFile(const QString &path, const QString &text) {
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        return false;
    }
    file.write(text.toUtf8());
    return file.commit();
}

void DrawingDocumentStore::copyIfPresent(QJsonObject &target, const QJsonObject &source, const QString &key) {
    if (source.contains(key) && !source.value(key).isUndefined()) {
        target.insert(key, source.value(key));
    }
}

QJsonObject DrawingDocumentStore::objectStyleManifest(const QJsonObject &object) {
    QJsonObject style;
    copyIfPresent(style, object, QStringLiteral("stroke_color"));
    copyIfPresent(style, object, QStringLiteral("fill_color"));
    copyIfPresent(style, object, QStringLiteral("line_thickness"));
    copyIfPresent(style, object, QStringLiteral("line_style"));
    copyIfPresent(style, object, QStringLiteral("stroke_opacity"));
    copyIfPresent(style, object, QStringLiteral("fill_opacity"));
    return style;
}

QJsonObject DrawingDocumentStore::objectCoordinateManifest(const QJsonObject &object) {
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
        QStringLiteral("sides")
    };
    for (const QString &key : keys) {
        copyIfPresent(coordinates, object, key);
    }
    return coordinates;
}

QJsonArray DrawingDocumentStore::objectTagsManifest(const QJsonObject &object) {
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

QJsonObject DrawingDocumentStore::objectMetadataManifest(const QJsonObject &object) {
    QJsonObject metadata;
    const QStringList keys = {
        QStringLiteral("intent"),
        QStringLiteral("role"),
        QStringLiteral("material"),
        QStringLiteral("export_group")
    };
    for (const QString &key : keys) {
        copyIfPresent(metadata, object, key);
    }
    return metadata;
}

QJsonObject DrawingDocumentStore::objectManifestEntry(const QJsonObject &object) {
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

QJsonObject DrawingDocumentStore::blenderSvgBundleManifest(const QString &bundlePath, const QJsonObject &model) {
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
    manifest.insert(QStringLiteral("document_name"), bundleInfo.completeBaseName().isEmpty() ? bundleInfo.fileName() : bundleInfo.completeBaseName());
    manifest.insert(QStringLiteral("exported_at_utc"), QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs));
    manifest.insert(QStringLiteral("canvas_px"), canvas);
    manifest.insert(QStringLiteral("object_count"), manifestObjects.size());
    manifest.insert(QStringLiteral("objects"), manifestObjects);
    manifest.insert(QStringLiteral("blender_import"), importHints);
    return manifest;
}

bool DrawingDocumentStore::hasNonEmptyString(const QJsonObject &object, const QString &key) {
    return object.value(key).toString().trimmed().length() > 0;
}

bool DrawingDocumentStore::hasNonEmptyTags(const QJsonObject &object) {
    return !object.value(QStringLiteral("tags")).toArray().isEmpty();
}

bool DrawingDocumentStore::isSupportedManifestObjectType(const QString &type) {
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
        QStringLiteral("glyph_baseline")
    };
    return supportedTypes.contains(type);
}

QJsonObject DrawingDocumentStore::blenderSvgBundleExportReport(const QString &bundlePath,
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

QString DrawingDocumentStore::blenderSvgBundleExportReportText(const QJsonObject &report) {
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
