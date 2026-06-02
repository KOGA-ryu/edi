#include <QGuiApplication>
#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QImage>
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
    parser.addOption(activityOption);
    parser.addOption(settingsPageOption);
    parser.process(app);

    auto absolutePath = [](const QString &path) {
        if (QFileInfo(path).isRelative()) {
            return QDir::current().absoluteFilePath(path);
        }
        return path;
    };

    auto projectSourcePath = [](const QString &path) {
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

    QString projectProfilePath = parser.isSet(projectProfileOption)
        ? parser.value(projectProfileOption)
        : QStringLiteral(PROJECT_SOURCE_DIR) + QStringLiteral("/data/project_profiles/draftsman_blank.json");
    projectProfilePath = absolutePath(projectProfilePath);
    const QVariant projectProfile = loadJsonObject(projectProfilePath);
    const QVariantMap projectProfileMap = projectProfile.toMap();
    const QVariantMap dataSources = projectProfileMap.value(QStringLiteral("data_sources")).toMap();
    const QString profileReviewSubjectPath = dataSources.value(
        QStringLiteral("review_subject"),
        QStringLiteral("data/review_subjects/draftsman_ui_taxonomy.json")).toString();

    QString reviewSubjectPath = parser.isSet(reviewSubjectOption)
        ? parser.value(reviewSubjectOption)
        : profileReviewSubjectPath;
    reviewSubjectPath = parser.isSet(reviewSubjectOption)
        ? absolutePath(reviewSubjectPath)
        : projectSourcePath(reviewSubjectPath);
    const QVariant reviewSubject = loadJsonObject(reviewSubjectPath);

    QString themePath = parser.isSet(themeOption)
        ? parser.value(themeOption)
        : QStringLiteral(PROJECT_SOURCE_DIR) + QStringLiteral("/data/ui_theme.json");
    themePath = absolutePath(themePath);
    const QVariant uiTheme = loadJsonObject(themePath);

    QString shellLayoutPath = QStringLiteral(PROJECT_SOURCE_DIR) + QStringLiteral("/data/shell_layout.json");
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
    engine.rootContext()->setContextProperty(QStringLiteral("initialShellLayout"), shellLayout);
    engine.rootContext()->setContextProperty(QStringLiteral("initialShellLayoutPath"), shellLayoutPath);
    engine.rootContext()->setContextProperty(QStringLiteral("shellLayoutStore"), &shellLayoutStore);
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
                : QStringLiteral("draftsman_ui");
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
