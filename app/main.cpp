#include <QApplication>
#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QListWidget>
#include <QMainWindow>
#include <QMenuBar>
#include <QSplitter>
#include <QStandardPaths>
#include <QStatusBar>
#include <QTableWidget>
#include <QTextEdit>
#include <QVariantList>
#include <QVariantMap>
#include <QVBoxLayout>

#include "core/DrawingCore.h"
#include "io/DrawingDocumentStore.h"
#include "io/DrawingRecentFilesStore.h"
#include "io/ShellLayoutStore.h"
#include "io/TextEditorStore.h"
#include "runtime/DrawingRuntimeCore.h"
#include "widgets/DrawingCanvasWidget.h"

namespace {

QString absolutePath(const QString &path)
{
    if (path.isEmpty() || QFileInfo(path).isAbsolute()) {
        return path;
    }
    return QDir::current().absoluteFilePath(path);
}

QString projectSourcePath(const QString &path)
{
    if (path.isEmpty() || QFileInfo(path).isAbsolute()) {
        return path;
    }
    return QDir(QStringLiteral(PROJECT_SOURCE_DIR)).absoluteFilePath(path);
}

QVariantMap loadJsonObject(const QString &path)
{
    QFile file(path);
    if (!path.isEmpty() && file.open(QIODevice::ReadOnly)) {
        const QJsonDocument document = QJsonDocument::fromJson(file.readAll());
        if (document.isObject()) {
            return document.object().toVariantMap();
        }
    }
    return {};
}

QString textValue(const QVariantMap &map, const QString &key, const QString &fallback = {})
{
    const QString value = map.value(key).toString();
    return value.isEmpty() ? fallback : value;
}

QListWidget *listWidget(const QString &title, const QVariantList &rows)
{
    auto *list = new QListWidget;
    list->setObjectName(title);
    for (const QVariant &value : rows) {
        const QVariantMap row = value.toMap();
        const QString label = textValue(row, "label", row.value("id").toString());
        const QString meta = row.value("meta").toString();
        list->addItem(meta.isEmpty() ? label : QString("%1  [%2]").arg(label, meta));
    }
    return list;
}

QGroupBox *panel(const QString &title, QWidget *content)
{
    auto *box = new QGroupBox(title);
    auto *layout = new QVBoxLayout(box);
    layout->setContentsMargins(8, 8, 8, 8);
    layout->addWidget(content);
    return box;
}

QTableWidget *tableFromRows(const QStringList &headers, const QVariantList &rows)
{
    auto *table = new QTableWidget(rows.size(), headers.size());
    table->setHorizontalHeaderLabels(headers);
    table->verticalHeader()->setVisible(false);
    table->horizontalHeader()->setStretchLastSection(true);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    for (int rowIndex = 0; rowIndex < rows.size(); ++rowIndex) {
        const QVariantMap row = rows[rowIndex].toMap();
        for (int column = 0; column < headers.size(); ++column) {
            const QString key = headers[column].toLower();
            table->setItem(rowIndex, column, new QTableWidgetItem(row.value(key).toString()));
        }
    }
    return table;
}

QVariantList profileRows(const QVariantMap &profile)
{
    const QVariantList rows = profile.value("left_panel").toMap().value("project_rows").toList();
    return rows.isEmpty() ? QVariantList{QVariantMap{{"label", "Project"}, {"meta", "loaded"}}} : rows;
}

QVariantList validationRows(const QVariantMap &model)
{
    QVariantList rows;
    for (const QVariant &value : model.value("validation").toList()) {
        const QVariantMap validation = value.toMap();
        rows.push_back(QVariantMap{
            {"label", textValue(validation, "id", "validation")},
            {"value", validation.value("status").toString() + " / " + validation.value("detail").toString()},
        });
    }
    if (rows.isEmpty()) {
        rows.push_back(QVariantMap{{"label", "validation"}, {"value", "not available"}});
    }
    return rows;
}

QWidget *buildWorkspace(const QVariantMap &profile, DrawingDocumentController &drawingController)
{
    auto *workspace = new QWidget;
    auto *layout = new QVBoxLayout(workspace);
    const QVariantMap model = drawingController.modelDocument();

    auto *summary = new QLabel(QString("Profile: %1\nFeature: %2\nDrawing engine: %3\nObjects: %4")
        .arg(textValue(profile.value("profile").toMap(), "label", "Draftsman"))
        .arg(textValue(profile.value("main_workspace").toMap(), "feature", "unknown"))
        .arg(textValue(model, "engine", "cpp_drawing_core_v1"))
        .arg(model.value("generated_objects").toList().size()));
    summary->setTextInteractionFlags(Qt::TextSelectableByMouse);
    layout->addWidget(summary);

    layout->addWidget(new DrawingCanvasWidget(&drawingController), 1);
    return workspace;
}

} // namespace

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setOrganizationName("KOGA-ryu");
    app.setApplicationName("EDI");

    QCommandLineParser parser;
    parser.setApplicationDescription("EDI C++ Qt Widgets shell");
    parser.addHelpOption();
    const QCommandLineOption reviewSubjectOption(QStringList() << "review-subject", "Load review subject JSON from <path>.", "path");
    const QCommandLineOption themeOption(QStringList() << "theme", "Load UI theme JSON from <path>.", "path");
    const QCommandLineOption projectProfileOption(QStringList() << "project-profile", "Load project profile JSON from <path>.", "path");
    const QCommandLineOption shellLayoutOption(QStringList() << "shell-layout", "Load shell layout JSON from <path>.", "path");
    const QCommandLineOption actionOption(QStringList() << "action", "Accepted for compatibility with older shell automation.", "action_id");
    const QCommandLineOption drawingControlScriptOption(QStringList() << "drawing-control-script", "Validate drawing control script JSON from <path>.", "path");
    const QCommandLineOption drawingControlLibraryOption(QStringList() << "drawing-control-library", "Load reusable drawing control script library JSON from <path>.", "path");
    const QCommandLineOption drawingControlScriptExitOption(QStringList() << "drawing-control-script-exit", "Exit after control script validation.");
    parser.addOption(reviewSubjectOption);
    parser.addOption(themeOption);
    parser.addOption(projectProfileOption);
    parser.addOption(shellLayoutOption);
    parser.addOption(actionOption);
    parser.addOption(drawingControlScriptOption);
    parser.addOption(drawingControlLibraryOption);
    parser.addOption(drawingControlScriptExitOption);
    parser.process(app);

    QString projectProfilePath = parser.isSet(projectProfileOption)
        ? absolutePath(parser.value(projectProfileOption))
        : QStringLiteral(PROJECT_SOURCE_DIR) + QStringLiteral("/data/project_profiles/draftsman_blank.json");
    const QVariantMap projectProfile = loadJsonObject(projectProfilePath);
    const QVariantMap dataSources = projectProfile.value("data_sources").toMap();

    const QString textEditorManifestPath = projectSourcePath(dataSources.value("text_documents").toString().trimmed());
    TextEditorStore textEditorStore(textEditorManifestPath);
    const QVariantList textEditorDocuments = textEditorStore.load();

    const QString shellLayoutPath = parser.isSet(shellLayoutOption)
        ? absolutePath(parser.value(shellLayoutOption))
        : QStringLiteral(PROJECT_SOURCE_DIR) + QStringLiteral("/data/shell_layout.json");
    ShellLayoutStore shellLayoutStore(shellLayoutPath);
    Q_UNUSED(shellLayoutStore);

    QString drawingRecentFilesPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (drawingRecentFilesPath.trimmed().isEmpty()) {
        drawingRecentFilesPath = QStringLiteral(PROJECT_SOURCE_DIR) + QStringLiteral("/.draftsman_runtime");
    }
    DrawingRecentFilesStore drawingRecentFilesStore(QDir(drawingRecentFilesPath).filePath(QStringLiteral("drawing_recent_files.json")));
    DrawingDocumentStore drawingDocumentStore;
    Q_UNUSED(drawingDocumentStore);

    DrawingDocumentController drawingController;
    DrawingToolCatalog drawingToolCatalog;
    DrawingToolScriptRuntime toolScriptRuntime;

    if (parser.isSet(drawingControlScriptOption)) {
        const QVariantMap script = loadJsonObject(absolutePath(parser.value(drawingControlScriptOption)));
        const QVariantMap library = parser.isSet(drawingControlLibraryOption)
            ? loadJsonObject(absolutePath(parser.value(drawingControlLibraryOption)))
            : QVariantMap();
        const QVariantMap plan = toolScriptRuntime.executionPlan(script, library);
        QTextStream(stdout) << QString::fromUtf8(QJsonDocument(QJsonObject::fromVariantMap(plan)).toJson(QJsonDocument::Compact)) << '\n';
        if (parser.isSet(drawingControlScriptExitOption)) {
            return plan.value("ok").toBool() ? 0 : 1;
        }
    }

    QMainWindow window;
    window.setWindowTitle("EDI");
    window.resize(1280, 820);
    window.menuBar()->addMenu("File");
    window.menuBar()->addMenu("View");

    auto *root = new QWidget;
    auto *rootLayout = new QVBoxLayout(root);

    auto *title = new QLabel(QString("EDI  |  %1").arg(textValue(projectProfile.value("profile").toMap(), "label", "Draftsman")));
    title->setStyleSheet("font-size: 18px; font-weight: 600; padding: 6px;");
    rootLayout->addWidget(title);

    auto *mainSplitter = new QSplitter(Qt::Horizontal);
    mainSplitter->addWidget(panel("Activity", listWidget("activity", drawingToolCatalog.toolModes())));
    mainSplitter->addWidget(panel("Project", listWidget("project", profileRows(projectProfile))));
    mainSplitter->addWidget(panel("Workspace", buildWorkspace(projectProfile, drawingController)));
    mainSplitter->addWidget(panel("Inspector", tableFromRows({"Label", "Value"}, validationRows(drawingController.modelDocument()))));
    mainSplitter->setStretchFactor(0, 0);
    mainSplitter->setStretchFactor(1, 1);
    mainSplitter->setStretchFactor(2, 5);
    mainSplitter->setStretchFactor(3, 2);
    rootLayout->addWidget(mainSplitter, 1);

    auto *bottom = new QTextEdit;
    bottom->setReadOnly(true);
    bottom->setMaximumHeight(140);
    bottom->setPlainText(QString("Text documents: %1\nRecent drawing files: %2")
        .arg(textEditorDocuments.size())
        .arg(drawingRecentFilesStore.load().size()));
    rootLayout->addWidget(panel("Status", bottom));

    window.setCentralWidget(root);
    window.statusBar()->showMessage("C++ Widgets shell active");
    window.show();
    return app.exec();
}
