#include <QApplication>
#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
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

