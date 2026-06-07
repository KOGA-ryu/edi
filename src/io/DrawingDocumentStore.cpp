#include "DrawingDocumentStore.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>
#include <QUrl>

#include "export/BlenderSvgBundleTemplates.h"
#include "export/BlenderSvgBundleExport.h"

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
    const QJsonObject manifest = BlenderSvgBundleExport::manifest(bundlePath, QJsonObject::fromVariantMap(model));
    const QString reportPath = bundleDir.filePath(QStringLiteral("export_report.json"));
    const QString reportTextPath = bundleDir.filePath(QStringLiteral("export_report.txt"));
    const QString verifyPath = bundleDir.filePath(QStringLiteral("verify_bundle.py"));
    const QJsonObject exportReport = BlenderSvgBundleExport::exportReport(
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
    if (!writeTextFile(reportTextPath, BlenderSvgBundleExport::exportReportText(exportReport))) {
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
