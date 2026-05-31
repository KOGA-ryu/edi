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
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickWindow>
#include <QTimer>
#include <QtGlobal>
#include <QUrl>

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
    parser.addOption(screenshotOption);
    parser.addOption(routeOption);
    parser.addOption(widthOption);
    parser.addOption(heightOption);
    parser.addOption(tabOption);
    parser.addOption(noteOption);
    parser.addOption(noteStatusOption);
    parser.addOption(reviewSubjectOption);
    parser.process(app);

    QString reviewSubjectPath = parser.isSet(reviewSubjectOption)
        ? parser.value(reviewSubjectOption)
        : QStringLiteral(PROJECT_SOURCE_DIR) + QStringLiteral("/data/review_subjects/draftsman_ui_taxonomy.json");
    if (QFileInfo(reviewSubjectPath).isRelative()) {
        reviewSubjectPath = QDir::current().absoluteFilePath(reviewSubjectPath);
    }
    QVariant reviewSubject = QVariantMap();
    QFile reviewSubjectFile(reviewSubjectPath);
    if (reviewSubjectFile.open(QIODevice::ReadOnly)) {
        const QJsonDocument document = QJsonDocument::fromJson(reviewSubjectFile.readAll());
        if (document.isObject()) {
            reviewSubject = document.object().toVariantMap();
        }
    }

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty(QStringLiteral("initialReviewSubject"), reviewSubject);
    engine.rootContext()->setContextProperty(QStringLiteral("initialReviewSubjectPath"), reviewSubjectPath);
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
