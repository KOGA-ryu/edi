#include <QGuiApplication>
#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QCryptographicHash>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QImage>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMetaObject>
#include <QObject>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickWindow>
#include <QSaveFile>
#include <QStringList>
#include <QTimer>
#include <QtGlobal>
#include <QUrl>
#include <QVariantList>
#include <QVariantMap>
#include <utility>

#include "core/DrawingCore.h"

class ShellLayoutStore final : public QObject {
    Q_OBJECT

public:
    explicit ShellLayoutStore(QString path, QObject *parent = nullptr)
        : QObject(parent),
          m_path(std::move(path)) {}

    Q_INVOKABLE bool save(const QVariantMap &layout) const {
        QSaveFile file(m_path);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            return false;
        }

        const QJsonObject object = QJsonObject::fromVariantMap(layout);
        const QJsonDocument document(object);
        file.write(document.toJson(QJsonDocument::Indented));
        return file.commit();
    }

    Q_INVOKABLE QString path() const {
        return m_path;
    }

private:
    QString m_path;
};

class DrawingDocumentStore final : public QObject {
    Q_OBJECT

public:
    explicit DrawingDocumentStore(QObject *parent = nullptr)
        : QObject(parent) {}

    Q_INVOKABLE QVariantMap save(const QUrl &url, const QVariantMap &model) const {
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

        const QJsonObject object = QJsonObject::fromVariantMap(model);
        if (object.isEmpty()) {
            result.insert(QStringLiteral("message"), QStringLiteral("drawing model empty"));
            return result;
        }

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

    Q_INVOKABLE QVariantMap open(const QUrl &url) const {
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
        if (object.value(QStringLiteral("export_kind")).toString() != QStringLiteral("pattern_lab_2d_native_model_v0")) {
            result.insert(QStringLiteral("message"), QStringLiteral("not a Draftsman drawing"));
            return result;
        }

        result.insert(QStringLiteral("ok"), true);
        result.insert(QStringLiteral("message"), QStringLiteral("opened drawing"));
        result.insert(QStringLiteral("path"), path);
        result.insert(QStringLiteral("model"), object.toVariantMap());
        return result;
    }

    Q_INVOKABLE QVariantMap exportSvg(const QUrl &url, const QString &svg) const {
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

    Q_INVOKABLE QVariantMap exportBlenderSvgBundle(const QUrl &url, const QString &svg, const QVariantMap &model) const {
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
        if (!writeTextFile(verifyPath, blenderSvgBundleVerifyScript())) {
            result.insert(QStringLiteral("message"), QStringLiteral("bundle verify script write failed"));
            return result;
        }
        if (!writeTextFile(scriptPath, blenderSvgImportScript())) {
            result.insert(QStringLiteral("message"), QStringLiteral("bundle script write failed"));
            return result;
        }
        if (!writeTextFile(readmePath, blenderSvgBundleReadme())) {
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

private:
    static QString localPath(const QUrl &url) {
        if (url.isLocalFile()) {
            return url.toLocalFile();
        }
        const QString text = url.toString().trimmed();
        if (text.startsWith(QStringLiteral("file://"))) {
            return QUrl(text).toLocalFile();
        }
        return text;
    }

    static QString bundleDirectoryPath(const QString &selectedPath) {
        const QFileInfo selectedInfo(selectedPath);
        if (selectedInfo.suffix().isEmpty()) {
            return selectedInfo.absoluteFilePath();
        }
        return selectedInfo.absoluteDir().filePath(selectedInfo.completeBaseName());
    }

    static bool writeTextFile(const QString &path, const QString &text) {
        QSaveFile file(path);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
            return false;
        }
        file.write(text.toUtf8());
        return file.commit();
    }

    static void copyIfPresent(QJsonObject &target, const QJsonObject &source, const QString &key) {
        if (source.contains(key) && !source.value(key).isUndefined()) {
            target.insert(key, source.value(key));
        }
    }

    static QJsonObject objectStyleManifest(const QJsonObject &object) {
        QJsonObject style;
        copyIfPresent(style, object, QStringLiteral("stroke_color"));
        copyIfPresent(style, object, QStringLiteral("fill_color"));
        copyIfPresent(style, object, QStringLiteral("line_thickness"));
        copyIfPresent(style, object, QStringLiteral("line_style"));
        copyIfPresent(style, object, QStringLiteral("stroke_opacity"));
        copyIfPresent(style, object, QStringLiteral("fill_opacity"));
        return style;
    }

    static QJsonObject objectCoordinateManifest(const QJsonObject &object) {
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

    static QJsonArray objectTagsManifest(const QJsonObject &object) {
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

    static QJsonObject objectMetadataManifest(const QJsonObject &object) {
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

    static QJsonObject objectManifestEntry(const QJsonObject &object) {
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

    static QJsonObject blenderSvgBundleManifest(const QString &bundlePath, const QJsonObject &model) {
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

    static bool hasNonEmptyString(const QJsonObject &object, const QString &key) {
        return object.value(key).toString().trimmed().length() > 0;
    }

    static bool hasNonEmptyTags(const QJsonObject &object) {
        return !object.value(QStringLiteral("tags")).toArray().isEmpty();
    }

    static bool isSupportedManifestObjectType(const QString &type) {
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

    static QJsonObject blenderSvgBundleExportReport(const QString &bundlePath,
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

    static QString blenderSvgBundleExportReportText(const QJsonObject &report) {
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

    #ifdef Q_MOC_RUN
    static QString blenderSvgImportScript();
    static QString blenderSvgBundleVerifyScript();
    static QString blenderSvgBundleReadme();
    #else
    static QString blenderSvgImportScript() {
        return QString::fromUtf8(R"PY(# Draftsman Blender SVG bundle importer.
# Run in Blender with: blender --python import_drawing_svg.py

import bpy
import json
import re
from pathlib import Path

BUNDLE_DIR = Path(__file__).resolve().parent
MANIFEST_PATH = BUNDLE_DIR / "manifest.json"

manifest = {}
if MANIFEST_PATH.exists():
    manifest = json.loads(MANIFEST_PATH.read_text(encoding="utf-8"))

import_hints = manifest.get("blender_import", {})
SVG_PATH = BUNDLE_DIR / import_hints.get("svg_file", "drawing.svg")
COLLECTION_NAME = import_hints.get("collection_name", "Draftsman SVG")
SCALE = float(import_hints.get("scale", 0.01))

if not SVG_PATH.exists():
    raise FileNotFoundError(f"Missing SVG file: {SVG_PATH}")

try:
    bpy.ops.preferences.addon_enable(module="io_curve_svg")
except Exception:
    pass

scene_collection_names = {collection.name for collection in bpy.context.scene.collection.children}
collection = bpy.data.collections.get(COLLECTION_NAME)
if collection is None:
    collection = bpy.data.collections.new(COLLECTION_NAME)
if collection.name not in scene_collection_names:
    bpy.context.scene.collection.children.link(collection)

def safe_name(value, fallback):
    raw = str(value or "").strip()
    if not raw:
        raw = fallback
    return re.sub(r"[^A-Za-z0-9_.-]+", "_", raw).strip("_") or fallback

def ensure_child_collection(parent, name):
    collection_name = safe_name(name, "ungrouped")
    if collection_name == parent.name:
        return parent
    child = bpy.data.collections.get(collection_name)
    if child is None:
        child = bpy.data.collections.new(collection_name)
    if child.name not in {existing.name for existing in parent.children}:
        parent.children.link(child)
    return child

def ensure_material(name, entry):
    material_name = safe_name(name, "")
    if not material_name:
        return None
    material = bpy.data.materials.get(material_name)
    if material is None:
        material = bpy.data.materials.new(material_name)
    color = color_from_entry(entry)
    if color:
        material.diffuse_color = color
    return material

def hex_to_rgba(value):
    raw = str(value or "").strip().lstrip("#")
    if len(raw) == 3:
        raw = "".join(ch * 2 for ch in raw)
    if len(raw) not in (6, 8):
        return None
    try:
        r = int(raw[0:2], 16) / 255.0
        g = int(raw[2:4], 16) / 255.0
        b = int(raw[4:6], 16) / 255.0
        a = int(raw[6:8], 16) / 255.0 if len(raw) == 8 else 1.0
        return (r, g, b, a)
    except ValueError:
        return None

def color_from_entry(entry):
    style = entry.get("style", {})
    return hex_to_rgba(style.get("fill_color")) or hex_to_rgba(style.get("stroke_color"))

before_names = set(bpy.data.objects.keys())

try:
    bpy.ops.import_curve.svg(filepath=str(SVG_PATH))
except Exception as exc:
    raise RuntimeError("Blender SVG import failed. Enable SVG import support, then rerun this script.") from exc

manifest_objects = manifest.get("objects", [])
manifest_by_id = {str(entry.get("id", "")): entry for entry in manifest_objects if entry.get("id")}

def clean_object_name(name):
    result = str(name)
    if result.startswith("draftsman_"):
        result = result[len("draftsman_"):]
    if len(result) > 4 and result[-4] == "." and result[-3:].isdigit():
        result = result[:-4]
    return result

def manifest_entry_for(obj, index):
    object_id = clean_object_name(obj.name)
    if object_id in manifest_by_id:
        return manifest_by_id[object_id]
    if index < len(manifest_objects):
        return manifest_objects[index]
    return {}

def metadata_value(entry, key, default=""):
    metadata = entry.get("metadata", {})
    value = entry.get(key, metadata.get(key, default))
    return "" if value is None else value

imported = [obj for obj in bpy.data.objects if obj.name not in before_names]
for index, obj in enumerate(imported):
    entry = manifest_entry_for(obj, index)
    tags = entry.get("tags", [])
    role = str(metadata_value(entry, "role"))
    material_name = str(metadata_value(entry, "material"))
    export_group = str(metadata_value(entry, "export_group"))
    target_collection = ensure_child_collection(collection, export_group) if export_group.strip() else collection
    material = ensure_material(material_name, entry)
    for existing_collection in list(obj.users_collection):
        existing_collection.objects.unlink(obj)
    target_collection.objects.link(obj)
    obj["draftsman_id"] = str(entry.get("id", clean_object_name(obj.name)))
    obj["draftsman_type"] = str(entry.get("type", "unknown"))
    obj["draftsman_tags"] = json.dumps(tags)
    obj["draftsman_role"] = role
    obj["draftsman_material"] = material_name
    obj["draftsman_intent"] = str(metadata_value(entry, "intent"))
    obj["draftsman_export_group"] = export_group
    obj["draftsman_manifest_index"] = index
    if material and hasattr(obj.data, "materials"):
        obj.data.materials.clear()
        obj.data.materials.append(material)
    role_prefix = safe_name(role, "")
    obj.name = "draftsman_" + (role_prefix + "_" if role_prefix else "") + obj.name
    obj.location.z = 0.0
    obj.scale = (SCALE, SCALE, SCALE)

manifest_count = manifest.get("object_count", len(imported))
print(f"Imported {len(imported)} SVG object(s) into collection '{COLLECTION_NAME}' from {SVG_PATH}")
print(f"Manifest object count: {manifest_count}")
)PY");
    }

    static QString blenderSvgBundleVerifyScript() {
        return QString::fromUtf8(R"PY(#!/usr/bin/env python3
"""Verify a Draftsman Blender SVG export bundle before opening Blender."""

import json
import sys
from collections import Counter
from pathlib import Path

BUNDLE_DIR = Path(__file__).resolve().parent
REQUIRED_FILES = [
    "drawing.svg",
    "manifest.json",
    "export_report.json",
    "import_drawing_svg.py",
]
SUPPORTED_TYPES = {
    "point",
    "line",
    "polyline",
    "circle",
    "arc",
    "rectangle",
    "polygon",
    "image_reference_frame",
    "ascii_crop_frame",
    "ascii_cell_region",
    "tone_probe",
    "glyph_baseline",
}

errors = []
warnings = []

def load_json(name):
    path = BUNDLE_DIR / name
    try:
        return json.loads(path.read_text(encoding="utf-8"))
    except Exception as exc:
        errors.append(f"{name} is not readable JSON: {exc}")
        return {}

for filename in REQUIRED_FILES:
    if not (BUNDLE_DIR / filename).exists():
        errors.append(f"missing required file: {filename}")

manifest = load_json("manifest.json") if (BUNDLE_DIR / "manifest.json").exists() else {}
report = load_json("export_report.json") if (BUNDLE_DIR / "export_report.json").exists() else {}

objects = manifest.get("objects", [])
manifest_count = int(manifest.get("object_count", len(objects)) or 0)
report_count = int(report.get("object_count", -1) or 0)
if manifest_count != len(objects):
    errors.append(f"manifest object_count {manifest_count} does not match objects length {len(objects)}")
if report_count != manifest_count:
    errors.append(f"report object_count {report_count} does not match manifest object_count {manifest_count}")

ids = [str(obj.get("id", "")).strip() for obj in objects]
missing_ids = sum(1 for object_id in ids if not object_id)
if missing_ids:
    errors.append(f"{missing_ids} object(s) missing id")

duplicates = sorted(object_id for object_id, count in Counter(ids).items() if object_id and count > 1)
if duplicates:
    errors.append("duplicate object id(s): " + ", ".join(duplicates))

type_counts = Counter(str(obj.get("type", "unknown") or "unknown") for obj in objects)
unknown_types = sorted(object_type for object_type in type_counts if object_type not in SUPPORTED_TYPES)
if unknown_types:
    warnings.append("unknown object type(s): " + ", ".join(unknown_types))

coverage = report.get("metadata_coverage", {})

print("Draftsman Blender SVG Bundle Check")
print(f"Bundle: {BUNDLE_DIR}")
print(f"Objects: {manifest_count}")
print("")
print("Object Types:")
if type_counts:
    for object_type, count in sorted(type_counts.items()):
        print(f"- {object_type}: {count}")
else:
    print("- none")

print("")
print("Metadata Coverage:")
print(f"- role: {coverage.get('with_role', 0)}")
print(f"- material: {coverage.get('with_material', 0)}")
print(f"- export_group: {coverage.get('with_export_group', 0)}")
print(f"- tags: {coverage.get('with_tags', 0)}")
print(f"- missing metadata: {coverage.get('missing_metadata', 0)}")
print(f"- missing ids: {coverage.get('missing_ids', missing_ids)}")

if warnings:
    print("")
    print("Warnings:")
    for warning in warnings:
        print(f"- {warning}")

if errors:
    print("")
    print("Errors:")
    for error in errors:
        print(f"- {error}")
    sys.exit(1)

print("")
print("Bundle structure ok.")
)PY");
    }

    static QString blenderSvgBundleReadme() {
        return QString::fromUtf8(R"TXT(Draftsman Blender SVG Bundle

Files:
- drawing.svg: exported Draftsman vector drawing.
- manifest.json: compact object metadata and Blender import hints.
- export_report.json / export_report.txt: export audit.
- import_drawing_svg.py: Blender script that imports drawing.svg as curves.
- verify_bundle.py: local bundle self-check before Blender import.

Imported Blender objects receive Draftsman custom properties:
draftsman_id, draftsman_type, draftsman_tags, draftsman_role, draftsman_material.

If object metadata is present, import_drawing_svg.py also:
- links objects into child collections using export_group.
- creates/assigns materials using material.
- prefixes object names using role.

Run:
python3 verify_bundle.py
blender --python import_drawing_svg.py

Or open Blender, load import_drawing_svg.py in the Text Editor, and run it.
)TXT");
    }
    #endif
};

class TextEditorStore final : public QObject {
    Q_OBJECT

public:
    explicit TextEditorStore(QString manifestPath, QObject *parent = nullptr)
        : QObject(parent),
          m_manifestPath(std::move(manifestPath)) {}

    Q_INVOKABLE QVariantMap save(const QVariantList &documents, const QString &activeId, bool saveAll,
                                const QVariantMap &editorState = QVariantMap()) const {
        QVariantMap result;
        result.insert(QStringLiteral("ok"), false);
        result.insert(QStringLiteral("message"), QStringLiteral("storage unavailable"));
        result.insert(QStringLiteral("documents"), documents);

        if (m_manifestPath.isEmpty()) {
            return result;
        }

        const QFileInfo manifestInfo(m_manifestPath);
        QDir manifestDir(manifestInfo.absoluteDir());
        if (!manifestDir.exists() && !QDir().mkpath(manifestDir.absolutePath())) {
            result.insert(QStringLiteral("message"), QStringLiteral("manifest directory unavailable"));
            return result;
        }

        QVariantList savedDocuments;
        QJsonArray manifestDocuments;
        const QVariantMap normalizedState = normalizeEditorState(editorState);
        for (const QVariant &entry : documents) {
            QVariantMap document = entry.toMap();
            const QString id = document.value(QStringLiteral("id")).toString().trimmed();
            if (id.isEmpty()) {
                result.insert(QStringLiteral("message"), QStringLiteral("document id missing"));
                return result;
            }

            QString relativePath = document.value(QStringLiteral("path")).toString().trimmed();
            if (relativePath.isEmpty()) {
                relativePath = QStringLiteral("docs/") + safeFileStem(id) + QStringLiteral(".txt");
                document.insert(QStringLiteral("path"), relativePath);
            }

            const QString cleanPath = cleanRelativePath(relativePath);
            if (cleanPath.isEmpty()) {
                result.insert(QStringLiteral("message"), QStringLiteral("invalid document path"));
                return result;
            }
            document.insert(QStringLiteral("path"), cleanPath);

            const bool shouldWriteText = saveAll || id == activeId;
            if (shouldWriteText) {
                const QString absoluteTextPath = manifestDir.absoluteFilePath(cleanPath);
                const QFileInfo textInfo(absoluteTextPath);
                if (!textInfo.absoluteDir().exists() && !QDir().mkpath(textInfo.absolutePath())) {
                    result.insert(QStringLiteral("message"), QStringLiteral("document directory unavailable"));
                    return result;
                }

                QSaveFile textFile(absoluteTextPath);
                if (!textFile.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
                    result.insert(QStringLiteral("message"), QStringLiteral("document write failed"));
                    return result;
                }
                textFile.write(document.value(QStringLiteral("text")).toString().toUtf8());
                if (!textFile.commit()) {
                    result.insert(QStringLiteral("message"), QStringLiteral("document commit failed"));
                    return result;
                }
                document.insert(QStringLiteral("initialText"), document.value(QStringLiteral("text")).toString());
            }

            const QString name = document.value(QStringLiteral("name")).toString().isEmpty()
                ? QStringLiteral("untitled.txt")
                : document.value(QStringLiteral("name")).toString();
            const QString language = document.value(QStringLiteral("language")).toString().isEmpty()
                ? QStringLiteral("text")
                : document.value(QStringLiteral("language")).toString();
            const QString role = normalizedDocumentRole(document.value(QStringLiteral("role")).toString());
            document.insert(QStringLiteral("role"), role);

            QJsonObject manifestDocument;
            manifestDocument.insert(QStringLiteral("id"), id);
            manifestDocument.insert(QStringLiteral("name"), name);
            manifestDocument.insert(QStringLiteral("language"), language);
            manifestDocument.insert(QStringLiteral("path"), cleanPath);
            manifestDocument.insert(QStringLiteral("role"), role);
            manifestDocuments.append(manifestDocument);
            savedDocuments.append(document);
        }

        QJsonObject manifest;
        manifest.insert(QStringLiteral("schema_version"), 1);
        manifest.insert(QStringLiteral("documents"), manifestDocuments);
        manifest.insert(QStringLiteral("editor_state"), QJsonObject::fromVariantMap(normalizedState));

        QSaveFile manifestFile(m_manifestPath);
        if (!manifestFile.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
            result.insert(QStringLiteral("message"), QStringLiteral("manifest write failed"));
            return result;
        }
        manifestFile.write(QJsonDocument(manifest).toJson(QJsonDocument::Indented));
        if (!manifestFile.commit()) {
            result.insert(QStringLiteral("message"), QStringLiteral("manifest commit failed"));
            return result;
        }

        result.insert(QStringLiteral("ok"), true);
        result.insert(QStringLiteral("message"), saveAll ? QStringLiteral("saved all") : QStringLiteral("saved active"));
        result.insert(QStringLiteral("documents"), savedDocuments);
        return result;
    }

    Q_INVOKABLE QString path() const {
        return m_manifestPath;
    }

    Q_INVOKABLE QVariantMap exportBundle(const QVariantList &documents, const QString &activeId, const QVariantMap &metadata) const {
        QVariantMap result;
        result.insert(QStringLiteral("ok"), false);
        result.insert(QStringLiteral("message"), QStringLiteral("storage unavailable"));

        if (m_manifestPath.isEmpty()) {
            return result;
        }

        const QFileInfo manifestInfo(m_manifestPath);
        QDir manifestDir(manifestInfo.absoluteDir());
        if (!manifestDir.exists() && !QDir().mkpath(manifestDir.absolutePath())) {
            result.insert(QStringLiteral("message"), QStringLiteral("manifest directory unavailable"));
            return result;
        }

        const QString packetType = safeFileStem(metadata.value(QStringLiteral("packet_type")).toString().trimmed().isEmpty()
            ? QStringLiteral("text_editor_bundle")
            : metadata.value(QStringLiteral("packet_type")).toString());
        const bool dexHandoff = packetType == QStringLiteral("dex_handoff");
        const QString timestamp = QDateTime::currentDateTimeUtc().toString(QStringLiteral("yyyyMMdd_HHmmss"));
        const QString exportRoot = manifestDir.absoluteFilePath(QStringLiteral("exports"));
        if (!QDir().mkpath(exportRoot)) {
            result.insert(QStringLiteral("message"), QStringLiteral("export directory unavailable"));
            return result;
        }

        QString packetName = packetType + QStringLiteral("_") + timestamp;
        QString packetPath = QDir(exportRoot).absoluteFilePath(packetName);
        int suffix = 2;
        while (QFileInfo::exists(packetPath)) {
            packetName = packetType + QStringLiteral("_") + timestamp + QStringLiteral("_") + QString::number(suffix);
            packetPath = QDir(exportRoot).absoluteFilePath(packetName);
            suffix += 1;
        }

        QDir packetDir(packetPath);
        if (!QDir().mkpath(packetPath) || !packetDir.mkpath(QStringLiteral("documents"))) {
            result.insert(QStringLiteral("message"), QStringLiteral("packet directory unavailable"));
            return result;
        }

        QJsonArray manifestDocuments;
        QJsonArray packetFiles;
        QString combined;
        QString promptText;
        QString contextText;
        QString promptDocumentId;
        QString promptDocumentName;
        int contextDocumentCount = 0;
        int exportedCount = 0;
        QVariantList pendingPromptDocuments;
        QVariantList exportedHandoffDocuments;
        for (const QVariant &entry : documents) {
            const QVariantMap document = entry.toMap();
            const QString id = document.value(QStringLiteral("id")).toString().trimmed();
            if (id.isEmpty()) {
                continue;
            }
            const QString role = normalizedDocumentRole(document.value(QStringLiteral("role")).toString());
            if (dexHandoff && role == QStringLiteral("scratch")) {
                continue;
            }

            const QString name = document.value(QStringLiteral("name")).toString().trimmed().isEmpty()
                ? id + QStringLiteral(".txt")
                : document.value(QStringLiteral("name")).toString().trimmed();
            const QString fileName = safeExportFileName(name, id);
            const QString relativePath = QStringLiteral("documents/") + fileName;
            const QString text = document.value(QStringLiteral("text")).toString();

            QSaveFile textFile(packetDir.absoluteFilePath(relativePath));
            if (!textFile.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
                result.insert(QStringLiteral("message"), QStringLiteral("document export failed"));
                return result;
            }
            textFile.write(text.toUtf8());
            if (!textFile.commit()) {
                result.insert(QStringLiteral("message"), QStringLiteral("document export commit failed"));
                return result;
            }
            const QString documentHash = sha256Hex(text.toUtf8());
            packetFiles.append(packetFileRecord(relativePath, documentHash, text.toUtf8().size()));

            combined += QStringLiteral("===== ") + name + QStringLiteral(" [") + id + QStringLiteral("] =====\n");
            combined += text;
            if (!combined.endsWith(QLatin1Char('\n'))) {
                combined += QLatin1Char('\n');
            }
            combined += QLatin1Char('\n');

            if (dexHandoff) {
                QVariantMap handoffDocument;
                handoffDocument.insert(QStringLiteral("id"), id);
                handoffDocument.insert(QStringLiteral("name"), name);
                handoffDocument.insert(QStringLiteral("text"), text);
                handoffDocument.insert(QStringLiteral("role"), role);
                exportedHandoffDocuments.append(handoffDocument);
                if (role == QStringLiteral("prompt")) {
                    pendingPromptDocuments.append(handoffDocument);
                }
                if (role == QStringLiteral("context") || role == QStringLiteral("reference")) {
                    contextText += QStringLiteral("===== ") + name + QStringLiteral(" [") + id + QStringLiteral("] =====\n");
                    contextText += text;
                    if (!contextText.endsWith(QLatin1Char('\n'))) {
                        contextText += QLatin1Char('\n');
                    }
                    contextText += QLatin1Char('\n');
                    contextDocumentCount += 1;
                }
            }

            QJsonObject manifestDocument;
            manifestDocument.insert(QStringLiteral("id"), id);
            manifestDocument.insert(QStringLiteral("name"), name);
            const QString language = document.value(QStringLiteral("language")).toString().trimmed().isEmpty()
                ? QStringLiteral("text")
                : document.value(QStringLiteral("language")).toString().trimmed();
            manifestDocument.insert(QStringLiteral("language"), language);
            manifestDocument.insert(QStringLiteral("role"), role);
            manifestDocument.insert(QStringLiteral("source_path"), document.value(QStringLiteral("path")).toString());
            manifestDocument.insert(QStringLiteral("exported_path"), relativePath);
            manifestDocument.insert(QStringLiteral("active"), id == activeId);
            manifestDocument.insert(QStringLiteral("chars"), text.length());
            manifestDocument.insert(QStringLiteral("lines"), text.isEmpty() ? 1 : text.count(QLatin1Char('\n')) + 1);
            manifestDocument.insert(QStringLiteral("sha256"), documentHash);
            manifestDocuments.append(manifestDocument);
            exportedCount += 1;
        }

        if (exportedCount <= 0) {
            result.insert(QStringLiteral("message"), QStringLiteral("no documents to export"));
            return result;
        }

        if (!writeUtf8File(packetDir.absoluteFilePath(QStringLiteral("all_documents.txt")), combined)) {
            result.insert(QStringLiteral("message"), QStringLiteral("combined export failed"));
            return result;
        }
        packetFiles.append(packetFileRecord(QStringLiteral("all_documents.txt"), sha256Hex(combined.toUtf8()), combined.toUtf8().size()));

        if (dexHandoff) {
            QVariantMap promptDocument;
            if (!pendingPromptDocuments.isEmpty()) {
                promptDocument = pendingPromptDocuments.first().toMap();
            } else {
                for (const QVariant &entry : documents) {
                    QVariantMap candidate = entry.toMap();
                    if (candidate.value(QStringLiteral("id")).toString() == activeId
                            && normalizedDocumentRole(candidate.value(QStringLiteral("role")).toString()) != QStringLiteral("scratch")) {
                        promptDocument = candidate;
                        break;
                    }
                }
            }
            if (promptDocument.isEmpty() && !exportedHandoffDocuments.isEmpty()) {
                promptDocument = exportedHandoffDocuments.first().toMap();
            }
            if (promptDocument.isEmpty()) {
                promptDocumentId = QStringLiteral("none");
                promptDocumentName = QStringLiteral("No prompt document");
                promptText = QStringLiteral("No non-scratch prompt document was exported.");
            } else {
                promptDocumentId = promptDocument.value(QStringLiteral("id")).toString();
                promptDocumentName = promptDocument.value(QStringLiteral("name")).toString();
                promptText = promptDocument.value(QStringLiteral("text")).toString();
            }

            QString promptFile;
            promptFile += QStringLiteral("# Active Draftsman Prompt\n\n");
            promptFile += QStringLiteral("Document: ") + promptDocumentName + QStringLiteral("\n");
            promptFile += QStringLiteral("Document id: ") + promptDocumentId + QStringLiteral("\n\n");
            promptFile += promptText;
            if (!promptFile.endsWith(QLatin1Char('\n'))) {
                promptFile += QLatin1Char('\n');
            }

            QString contextFile;
            contextFile += QStringLiteral("# Draftsman Context Documents\n\n");
            contextFile += QStringLiteral("Context documents: ") + QString::number(contextDocumentCount) + QStringLiteral("\n\n");
            contextFile += contextText.isEmpty() ? QStringLiteral("No additional context documents were exported.\n") : contextText;

            QString agentReadme;
            agentReadme += QStringLiteral("Draftsman Dex handoff packet\n");
            agentReadme += QStringLiteral("Created: ") + QDateTime::currentDateTimeUtc().toString(Qt::ISODate) + QStringLiteral("\n\n");
            agentReadme += QStringLiteral("Read order:\n");
            agentReadme += QStringLiteral("1. manifest.json\n");
            agentReadme += QStringLiteral("2. prompt.txt\n");
            agentReadme += QStringLiteral("3. context.txt\n");
            agentReadme += QStringLiteral("4. documents/ for exact source copies\n\n");
            agentReadme += QStringLiteral("Do not assume this packet syncs back to Draftsman. Return changes as a new packet, patch, or explicit instructions.\n");

            if (!writeUtf8File(packetDir.absoluteFilePath(QStringLiteral("prompt.txt")), promptFile)
                    || !writeUtf8File(packetDir.absoluteFilePath(QStringLiteral("context.txt")), contextFile)
                    || !writeUtf8File(packetDir.absoluteFilePath(QStringLiteral("AGENT_README.txt")), agentReadme)) {
                result.insert(QStringLiteral("message"), QStringLiteral("handoff export failed"));
                return result;
            }
            packetFiles.append(packetFileRecord(QStringLiteral("prompt.txt"), sha256Hex(promptFile.toUtf8()), promptFile.toUtf8().size()));
            packetFiles.append(packetFileRecord(QStringLiteral("context.txt"), sha256Hex(contextFile.toUtf8()), contextFile.toUtf8().size()));
            packetFiles.append(packetFileRecord(QStringLiteral("AGENT_README.txt"), sha256Hex(agentReadme.toUtf8()), agentReadme.toUtf8().size()));
        }

        QJsonObject manifest;
        manifest.insert(QStringLiteral("schema_version"), 1);
        manifest.insert(QStringLiteral("packet_type"), packetType);
        manifest.insert(QStringLiteral("created_at_utc"), QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
        manifest.insert(QStringLiteral("app"), QStringLiteral("Draftsman"));
        manifest.insert(QStringLiteral("profile_id"), metadata.value(QStringLiteral("profile_id")).toString());
        manifest.insert(QStringLiteral("profile_label"), metadata.value(QStringLiteral("profile_label")).toString());
        manifest.insert(QStringLiteral("active_document_id"), activeId);
        manifest.insert(QStringLiteral("documents"), manifestDocuments);
        manifest.insert(QStringLiteral("files"), packetFiles);
        if (dexHandoff) {
            QJsonObject handoffFiles;
            handoffFiles.insert(QStringLiteral("agent_readme"), QStringLiteral("AGENT_README.txt"));
            handoffFiles.insert(QStringLiteral("prompt"), QStringLiteral("prompt.txt"));
            handoffFiles.insert(QStringLiteral("context"), QStringLiteral("context.txt"));
            handoffFiles.insert(QStringLiteral("combined"), QStringLiteral("all_documents.txt"));
            manifest.insert(QStringLiteral("handoff_files"), handoffFiles);
        }

        QSaveFile manifestFile(packetDir.absoluteFilePath(QStringLiteral("manifest.json")));
        if (!manifestFile.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
            result.insert(QStringLiteral("message"), QStringLiteral("manifest export failed"));
            return result;
        }
        manifestFile.write(QJsonDocument(manifest).toJson(QJsonDocument::Indented));
        if (!manifestFile.commit()) {
            result.insert(QStringLiteral("message"), QStringLiteral("manifest export commit failed"));
            return result;
        }
        const QByteArray manifestBytes = QJsonDocument(manifest).toJson(QJsonDocument::Indented);
        packetFiles.append(packetFileRecord(QStringLiteral("manifest.json"), sha256Hex(manifestBytes), manifestBytes.size()));

        QString readme;
        readme += QStringLiteral("Draftsman text editor export bundle\n");
        readme += QStringLiteral("Created: ") + manifest.value(QStringLiteral("created_at_utc")).toString() + QStringLiteral("\n");
        readme += QStringLiteral("Documents: ") + QString::number(exportedCount) + QStringLiteral("\n\n");
        readme += QStringLiteral("Read manifest.json first. Document bodies are in documents/. all_documents.txt is a combined plain-text copy.\n");
        readme += QStringLiteral("Use index.txt for a quick packet file list with SHA-256 checksums.\n");
        if (writeUtf8File(packetDir.absoluteFilePath(QStringLiteral("README.txt")), readme)) {
            packetFiles.append(packetFileRecord(QStringLiteral("README.txt"), sha256Hex(readme.toUtf8()), readme.toUtf8().size()));
        }

        QString indexText;
        indexText += QStringLiteral("Draftsman packet index\n");
        indexText += QStringLiteral("Packet: ") + packetName + QStringLiteral("\n");
        indexText += QStringLiteral("Created: ") + manifest.value(QStringLiteral("created_at_utc")).toString() + QStringLiteral("\n\n");
        for (const QJsonValue &value : packetFiles) {
            const QJsonObject file = value.toObject();
            indexText += file.value(QStringLiteral("sha256")).toString()
                + QStringLiteral("  ")
                + file.value(QStringLiteral("path")).toString()
                + QStringLiteral("\n");
        }
        if (!writeUtf8File(packetDir.absoluteFilePath(QStringLiteral("index.txt")), indexText)) {
            result.insert(QStringLiteral("message"), QStringLiteral("index export failed"));
            return result;
        }

        result.insert(QStringLiteral("ok"), true);
        result.insert(QStringLiteral("message"), dexHandoff ? QStringLiteral("exported dex handoff") : QStringLiteral("exported bundle"));
        result.insert(QStringLiteral("path"), packetPath);
        result.insert(QStringLiteral("documents"), exportedCount);
        return result;
    }

    QVariantList load() const {
        QVariantList documents;
        if (m_manifestPath.isEmpty()) {
            return documents;
        }

        QFile manifestFile(m_manifestPath);
        if (!manifestFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            return documents;
        }

        const QJsonDocument manifest = QJsonDocument::fromJson(manifestFile.readAll());
        if (!manifest.isObject()) {
            return documents;
        }

        const QJsonArray entries = manifest.object().value(QStringLiteral("documents")).toArray();
        const QFileInfo manifestInfo(m_manifestPath);
        const QDir manifestDir(manifestInfo.absoluteDir());
        for (const QJsonValue &value : entries) {
            const QJsonObject item = value.toObject();
            const QString cleanPath = cleanRelativePath(item.value(QStringLiteral("path")).toString());
            if (cleanPath.isEmpty()) {
                continue;
            }

            QString text;
            QFile textFile(manifestDir.absoluteFilePath(cleanPath));
            if (textFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
                text = QString::fromUtf8(textFile.readAll());
            }

            QVariantMap document;
            document.insert(QStringLiteral("id"), item.value(QStringLiteral("id")).toString());
            document.insert(QStringLiteral("name"), item.value(QStringLiteral("name")).toString(QStringLiteral("untitled.txt")));
            document.insert(QStringLiteral("language"), item.value(QStringLiteral("language")).toString(QStringLiteral("text")));
            document.insert(QStringLiteral("path"), cleanPath);
            document.insert(QStringLiteral("role"), item.value(QStringLiteral("role")).toString());
            document.insert(QStringLiteral("initialText"), text);
            document.insert(QStringLiteral("text"), text);
            document.insert(QStringLiteral("cursorPosition"), 0);
            document.insert(QStringLiteral("selectionStart"), 0);
            document.insert(QStringLiteral("selectionEnd"), 0);
            if (!document.value(QStringLiteral("id")).toString().isEmpty()) {
                documents.append(document);
            }
        }
        return documents;
    }

    Q_INVOKABLE QVariantMap loadState() const {
        QVariantMap state;
        if (m_manifestPath.isEmpty()) {
            return state;
        }

        QFile manifestFile(m_manifestPath);
        if (!manifestFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            return state;
        }

        const QJsonDocument manifest = QJsonDocument::fromJson(manifestFile.readAll());
        if (!manifest.isObject()) {
            return state;
        }

        const QJsonObject editorState = manifest.object().value(QStringLiteral("editor_state")).toObject();
        if (editorState.isEmpty()) {
            return state;
        }

        state.insert(QStringLiteral("active_document_id"), editorState.value(QStringLiteral("active_document_id")).toString());
        state.insert(QStringLiteral("split_enabled"), editorState.value(QStringLiteral("split_enabled")).toBool());
        state.insert(QStringLiteral("secondary_document_id"), editorState.value(QStringLiteral("secondary_document_id")).toString());
        state.insert(QStringLiteral("wrap_enabled"), editorState.value(QStringLiteral("wrap_enabled")).toBool());
        state.insert(QStringLiteral("line_numbers_visible"), editorState.value(QStringLiteral("line_numbers_visible")).toBool());

        return state;
    }

    Q_INVOKABLE bool saveState(const QVariantMap &editorState) const {
        if (m_manifestPath.isEmpty()) {
            return false;
        }

        const QFileInfo manifestInfo(m_manifestPath);
        QDir manifestDir(manifestInfo.absoluteDir());
        if (!manifestDir.exists() && !QDir().mkpath(manifestDir.absolutePath())) {
            return false;
        }

        QJsonObject manifest;
        QFile manifestFileRead(m_manifestPath);
        if (manifestFileRead.open(QIODevice::ReadOnly | QIODevice::Text)) {
            const QJsonDocument existing = QJsonDocument::fromJson(manifestFileRead.readAll());
            if (existing.isObject()) {
                manifest = existing.object();
            }
        }

        manifest.insert(QStringLiteral("schema_version"), 1);
        manifest.insert(QStringLiteral("editor_state"), QJsonObject::fromVariantMap(normalizeEditorState(editorState)));

        QSaveFile manifestFile(m_manifestPath);
        if (!manifestFile.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
            return false;
        }
        manifestFile.write(QJsonDocument(manifest).toJson(QJsonDocument::Indented));
        return manifestFile.commit();
    }

private:
    static QString cleanRelativePath(const QString &path) {
        if (path.trimmed().isEmpty() || QFileInfo(path).isAbsolute()) {
            return QString();
        }
        const QString clean = QDir::cleanPath(path);
        if (clean == QStringLiteral(".") || clean == QStringLiteral("..") || clean.startsWith(QStringLiteral("../"))) {
            return QString();
        }
        return clean;
    }

    static QString safeFileStem(const QString &text) {
        QString result;
        for (const QChar character : text) {
            if (character.isLetterOrNumber() || character == QLatin1Char('-') || character == QLatin1Char('_')) {
                result.append(character);
            }
        }
        return result.isEmpty() ? QStringLiteral("untitled") : result;
    }

    static QString safeExportFileName(const QString &name, const QString &fallbackId) {
        QString base = QFileInfo(name).completeBaseName();
        QString suffix = QFileInfo(name).suffix();
        if (base.trimmed().isEmpty()) {
            base = fallbackId;
        }
        if (suffix.trimmed().isEmpty()) {
            suffix = QStringLiteral("txt");
        }
        const QString safeId = safeFileStem(fallbackId);
        return safeFileStem(base) + QStringLiteral("_") + safeId + QStringLiteral(".") + safeFileStem(suffix);
    }

    static QString normalizedDocumentRole(const QString &role) {
        const QString clean = role.trimmed().toLower();
        if (clean == QStringLiteral("prompt")
                || clean == QStringLiteral("context")
                || clean == QStringLiteral("reference")
                || clean == QStringLiteral("scratch")
                || clean == QStringLiteral("output")) {
            return clean;
        }
        return QStringLiteral("context");
    }

    static bool writeUtf8File(const QString &path, const QString &text) {
        QSaveFile file(path);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
            return false;
        }
        file.write(text.toUtf8());
        return file.commit();
    }

    static QString sha256Hex(const QByteArray &data) {
        return QString::fromLatin1(QCryptographicHash::hash(data, QCryptographicHash::Sha256).toHex());
    }

    static QJsonObject packetFileRecord(const QString &path, const QString &sha256, qsizetype bytes) {
        QJsonObject record;
        record.insert(QStringLiteral("path"), path);
        record.insert(QStringLiteral("sha256"), sha256);
        record.insert(QStringLiteral("bytes"), static_cast<double>(bytes));
        return record;
    }

    static QVariantMap normalizeEditorState(const QVariantMap &source) {
        QVariantMap state;
        state.insert(QStringLiteral("active_document_id"), source.value(QStringLiteral("active_document_id")).toString().trimmed());
        state.insert(QStringLiteral("split_enabled"), source.value(QStringLiteral("split_enabled")).toBool());
        state.insert(QStringLiteral("secondary_document_id"), source.value(QStringLiteral("secondary_document_id")).toString().trimmed());
        state.insert(QStringLiteral("wrap_enabled"), source.value(QStringLiteral("wrap_enabled")).toBool());
        state.insert(QStringLiteral("line_numbers_visible"), source.value(QStringLiteral("line_numbers_visible")).toBool());
        return state;
    }

    QString m_manifestPath;
};

int main(int argc, char *argv[]) {
    if (qEnvironmentVariableIsEmpty("QT_QUICK_CONTROLS_STYLE")) {
        qputenv("QT_QUICK_CONTROLS_STYLE", "Basic");
    }

    QGuiApplication app(argc, argv);
    app.setOrganizationName("KOGA-ryu");
    app.setApplicationName("Qt QML Draftsman Shell");

    QCommandLineParser parser;
    parser.setApplicationDescription("Editable Draftsman Qt/QML shell");
    parser.addHelpOption();
    const QCommandLineOption screenshotOption(
        QStringList() << "s" << "screenshot",
        "Save a screenshot of the app window to <path>, then quit.",
        "path");
    const QCommandLineOption routeOption(
        QStringList() << "route",
        "Select a review route before screenshot capture.",
        "route_id");
    const QCommandLineOption widthOption(
        QStringList() << "width",
        "Set the app window width before capture.",
        "pixels");
    const QCommandLineOption heightOption(
        QStringList() << "height",
        "Set the app window height before capture.",
        "pixels");
    const QCommandLineOption tabOption(
        QStringList() << "tab",
        "Select a local review tab before screenshot capture.",
        "tab_name");
    const QCommandLineOption noteOption(
        QStringList() << "note",
        "Add one in-memory note before screenshot capture.",
        "text");
    const QCommandLineOption noteStatusOption(
        QStringList() << "note-status",
        "Status to use with --note.",
        "status");
    const QCommandLineOption reviewSubjectOption(
        QStringList() << "review-subject",
        "Load review subject JSON from <path>.",
        "path");
    const QCommandLineOption themeOption(
        QStringList() << "theme",
        "Load UI theme JSON from <path>.",
        "path");
    const QCommandLineOption projectProfileOption(
        QStringList() << "project-profile",
        "Load project profile JSON from <path>.",
        "path");
    const QCommandLineOption shellLayoutOption(
        QStringList() << "shell-layout",
        "Load shell layout JSON from <path>.",
        "path");
    const QCommandLineOption activityOption(
        QStringList() << "activity",
        "Select an activity mode before screenshot capture.",
        "mode_id");
    const QCommandLineOption settingsPageOption(
        QStringList() << "settings-page",
        "Select a settings page before screenshot capture.",
        "page_id");
    const QCommandLineOption actionOption(
        QStringList() << "action",
        "Run a profile custom action after startup.",
        "action_id");
    parser.addOption(screenshotOption);
    parser.addOption(routeOption);
    parser.addOption(widthOption);
    parser.addOption(heightOption);
    parser.addOption(tabOption);
    parser.addOption(noteOption);
    parser.addOption(noteStatusOption);
    parser.addOption(reviewSubjectOption);
    parser.addOption(themeOption);
    parser.addOption(projectProfileOption);
    parser.addOption(shellLayoutOption);
    parser.addOption(activityOption);
    parser.addOption(settingsPageOption);
    parser.addOption(actionOption);
    parser.process(app);

    auto absolutePath = [](const QString &path) {
        if (path.isEmpty()) {
            return QString();
        }
        if (QFileInfo(path).isRelative()) {
            return QDir::current().absoluteFilePath(path);
        }
        return path;
    };

    auto projectSourcePath = [](const QString &path) {
        if (path.isEmpty()) {
            return QString();
        }
        if (QFileInfo(path).isRelative()) {
            return QDir(QStringLiteral(PROJECT_SOURCE_DIR)).absoluteFilePath(path);
        }
        return path;
    };

    auto loadJsonObject = [](const QString &path) {
        QVariant result = QVariantMap();
        QFile file(path);
        if (file.open(QIODevice::ReadOnly)) {
            const QJsonDocument document = QJsonDocument::fromJson(file.readAll());
            if (document.isObject()) {
                result = document.object().toVariantMap();
            }
        }
        return result;
    };

    auto loadTextFile = [](const QString &path) {
        QFile file(path);
        if (!path.isEmpty() && file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            return QString::fromUtf8(file.readAll());
        }
        return QString();
    };

    QString projectProfilePath = parser.isSet(projectProfileOption)
        ? parser.value(projectProfileOption)
        : QStringLiteral(PROJECT_SOURCE_DIR) + QStringLiteral("/data/project_profiles/draftsman_blank.json");
    projectProfilePath = absolutePath(projectProfilePath);
    const QVariant projectProfile = loadJsonObject(projectProfilePath);
    const QVariantMap projectProfileMap = projectProfile.toMap();
    const QVariantMap dataSources = projectProfileMap.value(QStringLiteral("data_sources")).toMap();
    const QString profileReviewSubjectPath = dataSources.value(QStringLiteral("review_subject")).toString().trimmed();

    QString reviewSubjectPath;
    QVariant reviewSubject = QVariantMap();
    if (parser.isSet(reviewSubjectOption)) {
        reviewSubjectPath = absolutePath(parser.value(reviewSubjectOption));
        reviewSubject = loadJsonObject(reviewSubjectPath);
    } else if (!profileReviewSubjectPath.isEmpty()) {
        reviewSubjectPath = projectSourcePath(profileReviewSubjectPath);
        reviewSubject = loadJsonObject(reviewSubjectPath);
    }

    const QString mapCsvPath = projectSourcePath(dataSources.value(QStringLiteral("map_csv")).toString().trimmed());
    const QString cellMetadataPath = projectSourcePath(dataSources.value(QStringLiteral("cell_metadata")).toString().trimmed());
    const QString mapCsvText = loadTextFile(mapCsvPath);
    const QString cellMetadataText = loadTextFile(cellMetadataPath);
    const QString textEditorManifestPath = projectSourcePath(dataSources.value(QStringLiteral("text_documents")).toString().trimmed());
    TextEditorStore textEditorStore(textEditorManifestPath);
    const QVariantList textEditorDocuments = textEditorStore.load();
    const QVariantMap textEditorState = textEditorStore.loadState();
    const QString drawingToolRegistryPath = QStringLiteral(PROJECT_SOURCE_DIR)
        + QStringLiteral("/data/features/drawing_tool/tool_registry.json");
    const QVariant drawingToolRegistry = loadJsonObject(drawingToolRegistryPath);
    QString drawingMetadataPresetsPath = projectSourcePath(dataSources.value(QStringLiteral("drawing_metadata_presets")).toString().trimmed());
    if (drawingMetadataPresetsPath.trimmed().isEmpty()) {
        drawingMetadataPresetsPath = QStringLiteral(PROJECT_SOURCE_DIR)
            + QStringLiteral("/data/features/drawing_tool/metadata_presets.json");
    }
    const QVariant drawingMetadataPresets = loadJsonObject(drawingMetadataPresetsPath);
    DrawingDocumentController drawingController;

    QString themePath = parser.isSet(themeOption)
        ? parser.value(themeOption)
        : QStringLiteral(PROJECT_SOURCE_DIR) + QStringLiteral("/data/ui_theme.json");
    themePath = absolutePath(themePath);
    const QVariant uiTheme = loadJsonObject(themePath);

    QString shellLayoutPath = parser.isSet(shellLayoutOption)
        ? parser.value(shellLayoutOption)
        : QStringLiteral(PROJECT_SOURCE_DIR) + QStringLiteral("/data/shell_layout.json");
    shellLayoutPath = absolutePath(shellLayoutPath);
    const QVariant shellLayout = loadJsonObject(shellLayoutPath);
    ShellLayoutStore shellLayoutStore(shellLayoutPath);
    DrawingDocumentStore drawingDocumentStore;

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty(QStringLiteral("initialReviewSubject"), reviewSubject);
    engine.rootContext()->setContextProperty(QStringLiteral("initialReviewSubjectPath"), reviewSubjectPath);
    engine.rootContext()->setContextProperty(QStringLiteral("initialUiTheme"), uiTheme);
    engine.rootContext()->setContextProperty(QStringLiteral("initialUiThemePath"), themePath);
    engine.rootContext()->setContextProperty(QStringLiteral("initialProjectProfile"), projectProfile);
    engine.rootContext()->setContextProperty(QStringLiteral("initialProjectProfilePath"), projectProfilePath);
    engine.rootContext()->setContextProperty(QStringLiteral("initialMapCsvPath"), mapCsvPath);
    engine.rootContext()->setContextProperty(QStringLiteral("initialMapCsvText"), mapCsvText);
    engine.rootContext()->setContextProperty(QStringLiteral("initialCellMetadataPath"), cellMetadataPath);
    engine.rootContext()->setContextProperty(QStringLiteral("initialCellMetadataText"), cellMetadataText);
    engine.rootContext()->setContextProperty(QStringLiteral("initialTextEditorDocuments"), textEditorDocuments);
    engine.rootContext()->setContextProperty(QStringLiteral("initialTextEditorState"), textEditorState);
    engine.rootContext()->setContextProperty(QStringLiteral("initialTextEditorManifestPath"), textEditorManifestPath);
    engine.rootContext()->setContextProperty(QStringLiteral("initialDrawingToolRegistry"), drawingToolRegistry);
    engine.rootContext()->setContextProperty(QStringLiteral("initialDrawingToolRegistryPath"), drawingToolRegistryPath);
    engine.rootContext()->setContextProperty(QStringLiteral("initialDrawingMetadataPresets"), drawingMetadataPresets);
    engine.rootContext()->setContextProperty(QStringLiteral("initialDrawingMetadataPresetsPath"), drawingMetadataPresetsPath);
    engine.rootContext()->setContextProperty(QStringLiteral("initialDrawingModel"), drawingController.modelDocument());
    engine.rootContext()->setContextProperty(QStringLiteral("nativeDrawingController"), &drawingController);
    engine.rootContext()->setContextProperty(QStringLiteral("initialShellLayout"), shellLayout);
    engine.rootContext()->setContextProperty(QStringLiteral("initialShellLayoutPath"), shellLayoutPath);
    engine.rootContext()->setContextProperty(QStringLiteral("shellLayoutStore"), &shellLayoutStore);
    engine.rootContext()->setContextProperty(QStringLiteral("textEditorStore"), &textEditorStore);
    engine.rootContext()->setContextProperty(QStringLiteral("drawingDocumentStore"), &drawingDocumentStore);
    const QUrl mainUrl = QUrl::fromLocalFile(QStringLiteral(QML_SOURCE_DIR) + QStringLiteral("/Main.qml"));
    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);
    engine.load(mainUrl);

    if (engine.rootObjects().isEmpty()) {
        return -1;
    }

    auto *window = qobject_cast<QQuickWindow *>(engine.rootObjects().constFirst());
    if (window == nullptr) {
        return -1;
    }

    bool widthOk = false;
    bool heightOk = false;
    const int requestedWidth = parser.value(widthOption).toInt(&widthOk);
    const int requestedHeight = parser.value(heightOption).toInt(&heightOk);
    if (widthOk && requestedWidth > 0) {
        window->setWidth(requestedWidth);
    }
    if (heightOk && requestedHeight > 0) {
        window->setHeight(requestedHeight);
    }

    if (parser.isSet(routeOption)) {
        if (auto *runtime = window->findChild<QObject *>(QStringLiteral("runtimeController"))) {
            QMetaObject::invokeMethod(
                runtime,
                "selectRoute",
                Q_ARG(QVariant, QVariant(parser.value(routeOption))));
        }
    }

    if (auto *runtime = window->findChild<QObject *>(QStringLiteral("runtimeController"))) {
        if (parser.isSet(activityOption)) {
            QMetaObject::invokeMethod(
                runtime,
                "setActivityMode",
                Q_ARG(QVariant, QVariant(parser.value(activityOption))));
        }

        if (parser.isSet(settingsPageOption)) {
            QMetaObject::invokeMethod(
                runtime,
                "setSettingsPage",
                Q_ARG(QVariant, QVariant(parser.value(settingsPageOption))));
        }

        if (parser.isSet(noteOption)) {
            const QString routeId = parser.isSet(routeOption)
                ? parser.value(routeOption)
                : runtime->property("rootRouteId").toString();
            const QString status = parser.isSet(noteStatusOption)
                ? parser.value(noteStatusOption)
                : QStringLiteral("pending");
            QMetaObject::invokeMethod(
                runtime,
                "addNote",
                Q_ARG(QVariant, QVariant(routeId)),
                Q_ARG(QVariant, QVariant(status)),
                Q_ARG(QVariant, QVariant(parser.value(noteOption))));
        }

        if (parser.isSet(tabOption)) {
            QMetaObject::invokeMethod(
                runtime,
                "setLocalTab",
                Q_ARG(QVariant, QVariant(parser.value(tabOption))));
        }

        if (parser.isSet(actionOption)) {
            QMetaObject::invokeMethod(
                runtime,
                "runCustomAction",
                Q_ARG(QVariant, QVariant(parser.value(actionOption))));
        }
    }

    if (parser.isSet(screenshotOption)) {
        const QString path = parser.value(screenshotOption);
        QTimer::singleShot(700, window, [window, path]() {
            const QFileInfo info(path);
            if (!info.absoluteDir().exists()) {
                QDir().mkpath(info.absolutePath());
            }
            const QImage image = window->grabWindow();
            const bool saved = image.save(path);
            QCoreApplication::exit(saved ? 0 : 2);
        });
    }

    return app.exec();
}

#include "main.moc"
