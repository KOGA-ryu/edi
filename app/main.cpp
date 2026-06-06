#include <QGuiApplication>
#include <QCommandLineOption>
#include <QCommandLineParser>
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
#include <QTimer>
#include <QtGlobal>
#include <QUrl>
#include <QVariantList>
#include <QVariantMap>
#include <utility>

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

class TextEditorStore final : public QObject {
    Q_OBJECT

public:
    explicit TextEditorStore(QString manifestPath, QObject *parent = nullptr)
        : QObject(parent),
          m_manifestPath(std::move(manifestPath)) {}

    Q_INVOKABLE QVariantMap save(const QVariantList &documents, const QString &activeId, bool saveAll) const {
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

            QJsonObject manifestDocument;
            manifestDocument.insert(QStringLiteral("id"), id);
            manifestDocument.insert(QStringLiteral("name"), name);
            manifestDocument.insert(QStringLiteral("language"), language);
            manifestDocument.insert(QStringLiteral("path"), cleanPath);
            manifestDocuments.append(manifestDocument);
            savedDocuments.append(document);
        }

        QJsonObject manifest;
        manifest.insert(QStringLiteral("schema_version"), 1);
        manifest.insert(QStringLiteral("documents"), manifestDocuments);

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
    engine.rootContext()->setContextProperty(QStringLiteral("initialTextEditorManifestPath"), textEditorManifestPath);
    engine.rootContext()->setContextProperty(QStringLiteral("initialShellLayout"), shellLayout);
    engine.rootContext()->setContextProperty(QStringLiteral("initialShellLayoutPath"), shellLayoutPath);
    engine.rootContext()->setContextProperty(QStringLiteral("shellLayoutStore"), &shellLayoutStore);
    engine.rootContext()->setContextProperty(QStringLiteral("textEditorStore"), &textEditorStore);
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
