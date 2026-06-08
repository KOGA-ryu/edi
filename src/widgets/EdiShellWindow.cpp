#include "widgets/EdiShellWindow.h"

#include <QAbstractButton>
#include <QButtonGroup>
#include <QCheckBox>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSizePolicy>
#include <QVariantList>
#include <QVariantMap>
#include <QVBoxLayout>

#include "core/DrawingCore.h"
#include "widgets/DrawingCanvasWidget.h"

namespace {

QFrame *makeRegionFrame(const QString &objectName)
{
    auto *frame = new QFrame;
    frame->setObjectName(objectName);
    frame->setFrameShape(QFrame::NoFrame);
    return frame;
}

void clearLayoutMargins(QLayout *layout)
{
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
}

QString yesNo(bool value)
{
    return value ? QStringLiteral("on") : QStringLiteral("off");
}

} // namespace

EdiShellWindow::EdiShellWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle(QStringLiteral("EDI"));
    setMinimumSize(960, 620);
    resize(1280, 820);

    m_controller = new DrawingDocumentController(this);

    auto *central = new QWidget;
    central->setObjectName(QStringLiteral("shellRoot"));
    auto *root = new QVBoxLayout(central);
    clearLayoutMargins(root);

    auto *body = new QWidget;
    body->setObjectName(QStringLiteral("shellBody"));
    auto *bodyLayout = new QHBoxLayout(body);
    clearLayoutMargins(bodyLayout);

    bodyLayout->addWidget(buildActivityRail());
    bodyLayout->addWidget(buildLeftPanel());
    bodyLayout->addWidget(buildWorkspaceColumn(), 1);
    bodyLayout->addWidget(buildRightPanel());

    root->addWidget(body, 1);
    root->addWidget(buildBottomPanel());

    setCentralWidget(central);
    applyShellStyle();

    connect(m_controller, &DrawingDocumentController::modelChanged, this, &EdiShellWindow::refreshInspector);
    refreshInspector();
}

QWidget *EdiShellWindow::buildActivityRail()
{
    auto *rail = makeRegionFrame(QStringLiteral("activityRail"));
    rail->setFixedWidth(52);

    auto *layout = new QVBoxLayout(rail);
    layout->setContentsMargins(8, 8, 8, 8);
    layout->setSpacing(6);

    layout->addWidget(makeRailButton(QStringLiteral("D"), QStringLiteral("Drafting"), true));
    layout->addWidget(makeRailButton(QStringLiteral("T"), QStringLiteral("Text editor")));
    layout->addWidget(makeRailButton(QStringLiteral("P"), QStringLiteral("Project workspace")));
    layout->addWidget(makeRailButton(QStringLiteral("R"), QStringLiteral("Review and planning")));
    layout->addStretch(1);
    layout->addWidget(makeRailButton(QStringLiteral("+"), QStringLiteral("Reserved add action")));
    layout->addWidget(makeRailButton(QStringLiteral("?"), QStringLiteral("Help and docs")));

    return rail;
}

QWidget *EdiShellWindow::buildLeftPanel()
{
    auto *panel = makeRegionFrame(QStringLiteral("leftPanel"));
    panel->setFixedWidth(260);

    auto *layout = new QVBoxLayout(panel);
    layout->setContentsMargins(12, 12, 12, 12);
    layout->setSpacing(8);

    auto *title = new QLabel(QStringLiteral("EDI Drafting"));
    title->setObjectName(QStringLiteral("panelTitle"));
    layout->addWidget(title);

    layout->addWidget(makeSectionLabel(QStringLiteral("Tools")));
    m_toolGroup = new QButtonGroup(panel);
    m_toolGroup->setExclusive(true);

    const QVector<QPair<QString, QString>> tools {
        {QStringLiteral("select_move"), QStringLiteral("Select")},
        {QStringLiteral("point_tool"), QStringLiteral("Point")},
        {QStringLiteral("line_tool"), QStringLiteral("Line")},
        {QStringLiteral("rectangle_tool"), QStringLiteral("Rectangle")},
        {QStringLiteral("circle_tool"), QStringLiteral("Circle")},
    };

    for (const auto &tool : tools) {
        layout->addWidget(makeToolButton(tool.first, tool.second));
    }

    connect(m_toolGroup, &QButtonGroup::buttonClicked, m_controller, [this](QAbstractButton *button) {
        m_controller->setSelectedToolId(button->property("toolId").toString());
    });

    layout->addWidget(makeSectionLabel(QStringLiteral("Snap")));
    m_gridSnap = new QCheckBox(QStringLiteral("Grid snap"));
    m_gridSnap->setChecked(m_controller->gridSnapEnabled());
    m_objectSnap = new QCheckBox(QStringLiteral("Object snap"));
    m_objectSnap->setChecked(m_controller->objectSnapEnabled());
    layout->addWidget(m_gridSnap);
    layout->addWidget(m_objectSnap);

    connect(m_gridSnap, &QCheckBox::toggled, m_controller, &DrawingDocumentController::setGridSnapEnabled);
    connect(m_objectSnap, &QCheckBox::toggled, m_controller, &DrawingDocumentController::setObjectSnapEnabled);

    layout->addWidget(makeSectionLabel(QStringLiteral("Next Surfaces")));
    layout->addWidget(makeValueLabel(QStringLiteral("Text editor")));
    layout->addWidget(makeValueLabel(QStringLiteral("Project files")));
    layout->addWidget(makeValueLabel(QStringLiteral("Settings")));
    layout->addStretch(1);

    return panel;
}

QWidget *EdiShellWindow::buildWorkspaceColumn()
{
    auto *column = new QWidget;
    column->setObjectName(QStringLiteral("workspaceColumn"));
    auto *layout = new QVBoxLayout(column);
    clearLayoutMargins(layout);

    auto *header = makeRegionFrame(QStringLiteral("workspaceHeader"));
    auto *headerLayout = new QHBoxLayout(header);
    headerLayout->setContentsMargins(12, 8, 12, 8);
    headerLayout->setSpacing(10);

    auto *title = new QLabel(QStringLiteral("Drawing Canvas"));
    title->setObjectName(QStringLiteral("workspaceTitle"));
    headerLayout->addWidget(title);
    headerLayout->addStretch(1);
    m_statusValue = makeValueLabel();
    headerLayout->addWidget(m_statusValue);

    m_canvas = new DrawingCanvasWidget(m_controller);
    m_canvas->setObjectName(QStringLiteral("drawingCanvas"));

    layout->addWidget(header);
    layout->addWidget(m_canvas, 1);
    return column;
}

QWidget *EdiShellWindow::buildRightPanel()
{
    auto *panel = makeRegionFrame(QStringLiteral("rightPanel"));
    panel->setFixedWidth(300);

    auto *layout = new QVBoxLayout(panel);
    layout->setContentsMargins(12, 12, 12, 12);
    layout->setSpacing(8);

    auto *title = new QLabel(QStringLiteral("Inspector"));
    title->setObjectName(QStringLiteral("panelTitle"));
    layout->addWidget(title);

    layout->addWidget(makeSectionLabel(QStringLiteral("Selection")));
    m_selectedValue = makeValueLabel();
    layout->addWidget(m_selectedValue);

    layout->addWidget(makeSectionLabel(QStringLiteral("Document")));
    m_toolValue = makeValueLabel();
    m_objectsValue = makeValueLabel();
    m_revisionValue = makeValueLabel();
    layout->addWidget(m_toolValue);
    layout->addWidget(m_objectsValue);
    layout->addWidget(m_revisionValue);

    layout->addWidget(makeSectionLabel(QStringLiteral("Canvas State")));
    m_snapValue = makeValueLabel();
    m_previewValue = makeValueLabel();
    layout->addWidget(m_snapValue);
    layout->addWidget(m_previewValue);
    layout->addStretch(1);

    return panel;
}

QWidget *EdiShellWindow::buildBottomPanel()
{
    auto *panel = makeRegionFrame(QStringLiteral("bottomPanel"));
    panel->setFixedHeight(86);

    auto *layout = new QVBoxLayout(panel);
    layout->setContentsMargins(12, 8, 12, 8);
    layout->setSpacing(6);

    auto *tabs = new QWidget;
    auto *tabsLayout = new QHBoxLayout(tabs);
    tabsLayout->setContentsMargins(0, 0, 0, 0);
    tabsLayout->setSpacing(6);
    tabsLayout->addWidget(makeRailButton(QStringLiteral("State"), QStringLiteral("Current drafting state"), true));
    tabsLayout->addWidget(makeRailButton(QStringLiteral("Commands"), QStringLiteral("Command review")));
    tabsLayout->addWidget(makeRailButton(QStringLiteral("Notes"), QStringLiteral("Workspace notes")));
    tabsLayout->addStretch(1);

    auto *status = makeValueLabel(QStringLiteral("C++ Widgets shell. Drafting state is owned by DrawingDocumentController."));
    status->setObjectName(QStringLiteral("bottomStatus"));

    layout->addWidget(tabs);
    layout->addWidget(status);
    return panel;
}

QPushButton *EdiShellWindow::makeToolButton(const QString &toolId, const QString &label)
{
    auto *button = new QPushButton(label);
    button->setCheckable(true);
    button->setProperty("toolId", toolId);
    button->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    if (toolId == m_controller->selectedToolId()) {
        button->setChecked(true);
    }
    m_toolGroup->addButton(button);
    return button;
}

QPushButton *EdiShellWindow::makeRailButton(const QString &label, const QString &tooltip, bool active)
{
    auto *button = new QPushButton(label);
    button->setObjectName(QStringLiteral("railButton"));
    button->setToolTip(tooltip);
    button->setCheckable(true);
    button->setChecked(active);
    button->setMinimumHeight(30);
    return button;
}

QLabel *EdiShellWindow::makeSectionLabel(const QString &text) const
{
    auto *label = new QLabel(text);
    label->setObjectName(QStringLiteral("sectionLabel"));
    return label;
}

QLabel *EdiShellWindow::makeValueLabel(const QString &text) const
{
    auto *label = new QLabel(text);
    label->setObjectName(QStringLiteral("valueLabel"));
    label->setWordWrap(true);
    return label;
}

void EdiShellWindow::refreshInspector()
{
    const QVariantMap document = m_controller->modelDocument();
    const QVariantList objects = document.value(QStringLiteral("drawing_objects")).toList();
    const QVariantList selected = document.value(QStringLiteral("selected_object_ids")).toList();
    const QVariantMap snap = document.value(QStringLiteral("snap")).toMap();
    const bool hasPreview = document.contains(QStringLiteral("preview_object"));

    if (m_toolValue != nullptr) {
        m_toolValue->setText(QStringLiteral("Tool: %1").arg(m_controller->selectedToolId()));
    }
    if (m_selectedValue != nullptr) {
        const QString activeObject = document.value(QStringLiteral("active_object_id")).toString();
        m_selectedValue->setText(activeObject.isEmpty()
            ? QStringLiteral("Selected: none")
            : QStringLiteral("Selected: %1").arg(activeObject));
    }
    if (m_objectsValue != nullptr) {
        m_objectsValue->setText(QStringLiteral("Objects: %1").arg(objects.size()));
    }
    if (m_revisionValue != nullptr) {
        m_revisionValue->setText(QStringLiteral("Revision: %1").arg(document.value(QStringLiteral("revision")).toInt()));
    }
    if (m_snapValue != nullptr) {
        m_snapValue->setText(QStringLiteral("Grid: %1   Object: %2")
            .arg(yesNo(snap.value(QStringLiteral("grid_enabled")).toBool()))
            .arg(yesNo(snap.value(QStringLiteral("object_enabled")).toBool())));
    }
    if (m_previewValue != nullptr) {
        m_previewValue->setText(QStringLiteral("Preview: %1").arg(hasPreview ? QStringLiteral("active") : QStringLiteral("none")));
    }
    if (m_statusValue != nullptr) {
        m_statusValue->setText(QStringLiteral("%1 selected | %2 objects").arg(selected.size()).arg(objects.size()));
    }
}

void EdiShellWindow::applyShellStyle()
{
    setStyleSheet(QStringLiteral(R"(
        #shellRoot {
            background: #101418;
            color: #dce5ee;
            font-family: "Avenir Next", "Inter", sans-serif;
            font-size: 12px;
        }
        #activityRail {
            background: #121920;
            border-right: 1px solid #31404f;
        }
        #leftPanel, #rightPanel {
            background: #171d24;
            border-right: 1px solid #31404f;
            border-left: 1px solid #24313e;
        }
        #workspaceColumn {
            background: #111821;
        }
        #workspaceHeader {
            background: #1b232d;
            border-bottom: 1px solid #31404f;
        }
        #bottomPanel {
            background: #121920;
            border-top: 1px solid #31404f;
        }
        QLabel {
            color: #dce5ee;
        }
        #panelTitle, #workspaceTitle {
            color: #dce5ee;
            font-size: 14px;
            font-weight: 600;
        }
        #sectionLabel {
            color: #8fb4d8;
            font-size: 11px;
            font-weight: 600;
            padding-top: 8px;
            text-transform: uppercase;
        }
        #valueLabel, #bottomStatus {
            color: #9aa8b6;
            background: #1b232d;
            border: 1px solid #24313e;
            border-radius: 5px;
            padding: 6px 8px;
        }
        QPushButton {
            color: #dce5ee;
            background: #202a35;
            border: 1px solid #31404f;
            border-radius: 5px;
            padding: 7px 9px;
            text-align: left;
        }
        QPushButton:hover {
            background: #283542;
        }
        QPushButton:checked {
            background: #304052;
            border-color: #5e7892;
        }
        #railButton {
            min-width: 32px;
            text-align: center;
            padding-left: 6px;
            padding-right: 6px;
        }
        QCheckBox {
            color: #dce5ee;
            spacing: 8px;
        }
        QCheckBox::indicator {
            width: 15px;
            height: 15px;
        }
        QCheckBox::indicator:unchecked {
            background: #202a35;
            border: 1px solid #31404f;
            border-radius: 3px;
        }
        QCheckBox::indicator:checked {
            background: #8fb4d8;
            border: 1px solid #8fb4d8;
            border-radius: 3px;
        }
    )"));
}
