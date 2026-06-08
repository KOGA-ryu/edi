#include <QAbstractButton>
#include <QApplication>
#include <QButtonGroup>
#include <QCheckBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QMainWindow>
#include <QPair>
#include <QPushButton>
#include <QStatusBar>
#include <QVBoxLayout>
#include <QVector>

#include "core/DrawingCore.h"
#include "widgets/DrawingCanvasWidget.h"

int main(int argc, char **argv)
{
    QApplication app(argc, argv);

    QMainWindow window;
    window.setWindowTitle(QStringLiteral("EDI"));

    auto *controller = new DrawingDocumentController(&window);
    auto *central = new QWidget;
    auto *layout = new QVBoxLayout(central);

    auto *title = new QLabel(QStringLiteral("EDI"));
    QFont titleFont = title->font();
    titleFont.setPointSize(titleFont.pointSize() + 8);
    titleFont.setBold(true);
    title->setFont(titleFont);

    auto *tools = new QWidget;
    auto *toolLayout = new QHBoxLayout(tools);
    toolLayout->setContentsMargins(0, 0, 0, 0);
    auto *toolGroup = new QButtonGroup(tools);
    toolGroup->setExclusive(true);
    const QVector<QPair<QString, QString>> toolSpecs {
        {QStringLiteral("select_move"), QStringLiteral("Select")},
        {QStringLiteral("point_tool"), QStringLiteral("Point")},
        {QStringLiteral("line_tool"), QStringLiteral("Line")},
        {QStringLiteral("rectangle_tool"), QStringLiteral("Rect")},
        {QStringLiteral("circle_tool"), QStringLiteral("Circle")},
    };
    for (const auto &tool : toolSpecs) {
        auto *button = new QPushButton(tool.second);
        button->setCheckable(true);
        if (tool.first == controller->selectedToolId()) {
            button->setChecked(true);
        }
        toolGroup->addButton(button);
        button->setProperty("toolId", tool.first);
        toolLayout->addWidget(button);
    }
    toolLayout->addStretch(1);

    auto *gridSnap = new QCheckBox(QStringLiteral("Grid snap"));
    gridSnap->setChecked(controller->gridSnapEnabled());
    toolLayout->addWidget(gridSnap);

    auto *objectSnap = new QCheckBox(QStringLiteral("Object snap"));
    objectSnap->setChecked(controller->objectSnapEnabled());
    toolLayout->addWidget(objectSnap);

    auto *canvas = new DrawingCanvasWidget(controller);

    QObject::connect(toolGroup, &QButtonGroup::buttonClicked, controller, [controller](QAbstractButton *button) {
        controller->setSelectedToolId(button->property("toolId").toString());
    });
    QObject::connect(gridSnap, &QCheckBox::toggled, controller, &DrawingDocumentController::setGridSnapEnabled);
    QObject::connect(objectSnap, &QCheckBox::toggled, controller, &DrawingDocumentController::setObjectSnapEnabled);

    layout->addWidget(title);
    layout->addWidget(tools);
    layout->addWidget(canvas, 1);

    window.setCentralWidget(central);
    window.statusBar()->showMessage(QStringLiteral("drawing tools ready"));
    window.resize(900, 640);
    window.show();

    return app.exec();
}
