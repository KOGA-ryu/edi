#include <QGuiApplication>
#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QCryptographicHash>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMetaObject>
#include <QObject>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QSaveFile>
#include <QStandardPaths>
#include <QStringList>
#include <QtGlobal>
#include <QUrl>
#include <QVariantList>
#include <QVariantMap>
#include <utility>

#include "core/DrawingCore.h"
#include "io/DrawingRecentFilesStore.h"
#include "io/DrawingDocumentStore.h"
#include "io/ShellLayoutStore.h"
#include "io/TextEditorStore.h"

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
    const QCommandLineOption actionOption(
        QStringList() << "action",
        "Run a profile custom action after startup.",
        "action_id");
    parser.addOption(reviewSubjectOption);
    parser.addOption(themeOption);
    parser.addOption(projectProfileOption);
    parser.addOption(shellLayoutOption);
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
    QString drawingRecentFilesPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (drawingRecentFilesPath.trimmed().isEmpty()) {
        drawingRecentFilesPath = QStringLiteral(PROJECT_SOURCE_DIR) + QStringLiteral("/.draftsman_runtime");
    }
    drawingRecentFilesPath = QDir(drawingRecentFilesPath).filePath(QStringLiteral("drawing_recent_files.json"));
    DrawingRecentFilesStore drawingRecentFilesStore(drawingRecentFilesPath);
    const QVariantList drawingRecentFiles = drawingRecentFilesStore.load();

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
    engine.rootContext()->setContextProperty(QStringLiteral("initialDrawingRecentFiles"), drawingRecentFiles);
    engine.rootContext()->setContextProperty(QStringLiteral("initialDrawingRecentFilesPath"), drawingRecentFilesPath);
    engine.rootContext()->setContextProperty(QStringLiteral("initialShellLayout"), shellLayout);
    engine.rootContext()->setContextProperty(QStringLiteral("initialShellLayoutPath"), shellLayoutPath);
    engine.rootContext()->setContextProperty(QStringLiteral("shellLayoutStore"), &shellLayoutStore);
    engine.rootContext()->setContextProperty(QStringLiteral("textEditorStore"), &textEditorStore);
    engine.rootContext()->setContextProperty(QStringLiteral("drawingDocumentStore"), &drawingDocumentStore);
    engine.rootContext()->setContextProperty(QStringLiteral("drawingRecentFilesStore"), &drawingRecentFilesStore);
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

    QObject *rootObject = engine.rootObjects().constFirst();
    if (rootObject == nullptr) {
        return -1;
    }

    if (auto *runtime = rootObject->findChild<QObject *>(QStringLiteral("runtimeController"))) {
        if (parser.isSet(actionOption)) {
            QMetaObject::invokeMethod(
                runtime,
                "runCustomAction",
                Q_ARG(QVariant, QVariant(parser.value(actionOption))));
        }
    }

    return app.exec();
}
