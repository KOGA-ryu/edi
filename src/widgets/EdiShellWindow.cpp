#include "widgets/EdiShellWindow.h"

#include <QAbstractButton>
#include <QButtonGroup>
#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPair>
#include <QPushButton>
#include <QSizePolicy>
#include <QStringList>
#include <QVariantList>
#include <QVariantMap>
#include <QVBoxLayout>
#include <QVector>

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

QString formatNumber(double value)
{
    return QString::number(value, 'f', 3);
}

QVariantMap activeObjectProjection(const QVariantMap &document)
{
    const QString activeId = document.value(QStringLiteral("active_object_id")).toString();
    if (activeId.isEmpty()) {
        return {};
    }

    const QVariantList objects = document.value(QStringLiteral("drawing_objects")).toList();
    for (const QVariant &objectValue : objects) {
        const QVariantMap object = objectValue.toMap();
        if (object.value(QStringLiteral("id")).toString() == activeId) {
            return object;
        }
    }
    return {};
}

QString boundsSummary(const QVariantMap &object)
{
    const QVariantMap bounds = object.value(QStringLiteral("bounds")).toMap();
    if (bounds.isEmpty()) {
        return QStringLiteral("Bounds: unavailable");
    }
    return QStringLiteral("Bounds: x %1, y %2, w %3, h %4")
        .arg(formatNumber(bounds.value(QStringLiteral("x")).toDouble()))
        .arg(formatNumber(bounds.value(QStringLiteral("y")).toDouble()))
        .arg(formatNumber(bounds.value(QStringLiteral("width")).toDouble()))
        .arg(formatNumber(bounds.value(QStringLiteral("height")).toDouble()));
}

QString geometrySummary(const QVariantMap &object)
{
    const QString kind = object.value(QStringLiteral("kind")).toString();
    if (kind == QStringLiteral("point")) {
        return QStringLiteral("Geometry: point (%1, %2)")
            .arg(formatNumber(object.value(QStringLiteral("x")).toDouble()))
            .arg(formatNumber(object.value(QStringLiteral("y")).toDouble()));
    }
    if (kind == QStringLiteral("line")) {
        return QStringLiteral("Geometry: line (%1, %2) -> (%3, %4)")
            .arg(formatNumber(object.value(QStringLiteral("x1")).toDouble()))
            .arg(formatNumber(object.value(QStringLiteral("y1")).toDouble()))
            .arg(formatNumber(object.value(QStringLiteral("x2")).toDouble()))
            .arg(formatNumber(object.value(QStringLiteral("y2")).toDouble()));
    }
    if (kind == QStringLiteral("construction_line")) {
        return QStringLiteral("Geometry: construction (%1, %2) -> (%3, %4)")
            .arg(formatNumber(object.value(QStringLiteral("x1")).toDouble()))
            .arg(formatNumber(object.value(QStringLiteral("y1")).toDouble()))
            .arg(formatNumber(object.value(QStringLiteral("x2")).toDouble()))
            .arg(formatNumber(object.value(QStringLiteral("y2")).toDouble()));
    }
    if (kind == QStringLiteral("dimension")) {
        return QStringLiteral("Geometry: dimension (%1, %2) -> (%3, %4), %5")
            .arg(formatNumber(object.value(QStringLiteral("x1")).toDouble()))
            .arg(formatNumber(object.value(QStringLiteral("y1")).toDouble()))
            .arg(formatNumber(object.value(QStringLiteral("x2")).toDouble()))
            .arg(formatNumber(object.value(QStringLiteral("y2")).toDouble()))
            .arg(object.value(QStringLiteral("label")).toString());
    }
    if (kind == QStringLiteral("rectangle")) {
        return QStringLiteral("Geometry: rect x %1, y %2, w %3, h %4, rot %5")
            .arg(formatNumber(object.value(QStringLiteral("x")).toDouble()))
            .arg(formatNumber(object.value(QStringLiteral("y")).toDouble()))
            .arg(formatNumber(object.value(QStringLiteral("width")).toDouble()))
            .arg(formatNumber(object.value(QStringLiteral("height")).toDouble()))
            .arg(formatNumber(object.value(QStringLiteral("rotation_deg")).toDouble()));
    }
    if (kind == QStringLiteral("circle")) {
        return QStringLiteral("Geometry: circle cx %1, cy %2, r %3")
            .arg(formatNumber(object.value(QStringLiteral("cx")).toDouble()))
            .arg(formatNumber(object.value(QStringLiteral("cy")).toDouble()))
            .arg(formatNumber(object.value(QStringLiteral("radius")).toDouble()));
    }

    const QVariantList points = object.value(QStringLiteral("points")).toList();
    return QStringLiteral("Geometry: %1 points").arg(points.size());
}

} // namespace

EdiShellWindow::EdiShellWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_appState(edi::app::defaultAppState())
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

    m_activityGroup = new QButtonGroup(rail);
    m_activityGroup->setExclusive(true);

    for (const edi::app::WorkspaceActivity &activity : edi::app::defaultWorkspaceActivities()) {
        auto *button = makeRailButton(
            QString::fromLatin1(activity.icon),
            QString::fromLatin1(activity.tooltip),
            activity.mode == m_appState.mode,
            activity.enabled);
        button->setProperty("modeId", QString::fromLatin1(activity.id));
        m_activityGroup->addButton(button);
        layout->addWidget(button);
    }

    connect(m_activityGroup, &QButtonGroup::buttonClicked, this, [this](QAbstractButton *button) {
        const auto mode = edi::app::workspaceModeFromName(button->property("modeId").toString().toStdString());
        if (mode) {
            setWorkspaceMode(*mode);
        }
    });

    layout->addStretch(1);
    layout->addWidget(makeRailButton(QStringLiteral("+"), QStringLiteral("Reserved add action"), false, false));
    layout->addWidget(makeRailButton(QStringLiteral("?"), QStringLiteral("Help and docs"), false, false));

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
        {QStringLiteral("horizontal_guide_tool"), QStringLiteral("H Guide")},
        {QStringLiteral("vertical_guide_tool"), QStringLiteral("V Guide")},
        {QStringLiteral("horizontal_construction_line_tool"), QStringLiteral("H Construct")},
        {QStringLiteral("vertical_construction_line_tool"), QStringLiteral("V Construct")},
        {QStringLiteral("angled_construction_line_tool"), QStringLiteral("A Construct")},
        {QStringLiteral("distance_dimension_tool"), QStringLiteral("Dimension")},
    };

    for (const auto &tool : tools) {
        layout->addWidget(makeToolButton(tool.first, tool.second));
    }

    connect(m_toolGroup, &QButtonGroup::buttonClicked, m_controller, [this](QAbstractButton *button) {
        m_controller->setSelectedToolId(button->property("toolId").toString());
    });

    layout->addWidget(makeSectionLabel(QStringLiteral("Snap")));
    m_gridPreset = new QComboBox;
    m_gridPreset->addItem(QStringLiteral("Square art board"), QStringLiteral("square_art_board"));
    m_gridPreset->addItem(QStringLiteral("Letter"), QStringLiteral("letter"));
    m_gridPreset->addItem(QStringLiteral("A4"), QStringLiteral("a4"));
    const int presetIndex = m_gridPreset->findData(m_controller->gridPresetId());
    if (presetIndex >= 0) {
        m_gridPreset->setCurrentIndex(presetIndex);
    }
    layout->addWidget(m_gridPreset);

    m_gridSnap = new QCheckBox(QStringLiteral("Grid snap"));
    m_gridSnap->setChecked(m_controller->gridSnapEnabled());
    m_objectSnap = new QCheckBox(QStringLiteral("Object snap"));
    m_objectSnap->setChecked(m_controller->objectSnapEnabled());
    m_endpointSnap = new QCheckBox(QStringLiteral("Endpoint"));
    m_endpointSnap->setChecked(m_controller->endpointSnapEnabled());
    m_vertexSnap = new QCheckBox(QStringLiteral("Vertex"));
    m_vertexSnap->setChecked(m_controller->vertexSnapEnabled());
    m_midpointSnap = new QCheckBox(QStringLiteral("Midpoint"));
    m_midpointSnap->setChecked(m_controller->midpointSnapEnabled());
    m_centerSnap = new QCheckBox(QStringLiteral("Center"));
    m_centerSnap->setChecked(m_controller->centerSnapEnabled());
    m_objectPrioritySnap = new QCheckBox(QStringLiteral("Object before grid"));
    m_objectPrioritySnap->setChecked(m_controller->objectSnapPriorityBeforeGrid());
    m_objectTolerance = new QComboBox;
    m_objectTolerance->addItem(QStringLiteral("Tight tolerance"), QStringLiteral("tight"));
    m_objectTolerance->addItem(QStringLiteral("Normal tolerance"), QStringLiteral("normal"));
    m_objectTolerance->addItem(QStringLiteral("Loose tolerance"), QStringLiteral("loose"));
    const int toleranceIndex = m_objectTolerance->findData(m_controller->objectSnapTolerancePresetId());
    if (toleranceIndex >= 0) {
        m_objectTolerance->setCurrentIndex(toleranceIndex);
    }
    layout->addWidget(m_gridSnap);
    layout->addWidget(m_objectSnap);
    layout->addWidget(m_endpointSnap);
    layout->addWidget(m_vertexSnap);
    layout->addWidget(m_midpointSnap);
    layout->addWidget(m_centerSnap);
    layout->addWidget(m_objectPrioritySnap);
    layout->addWidget(m_objectTolerance);

    connect(m_gridPreset, &QComboBox::currentIndexChanged, m_controller, [this](int index) {
        m_controller->setGridPresetId(m_gridPreset->itemData(index).toString());
    });
    connect(m_gridSnap, &QCheckBox::toggled, m_controller, &DrawingDocumentController::setGridSnapEnabled);
    connect(m_objectSnap, &QCheckBox::toggled, m_controller, &DrawingDocumentController::setObjectSnapEnabled);
    connect(m_endpointSnap, &QCheckBox::toggled, m_controller, &DrawingDocumentController::setEndpointSnapEnabled);
    connect(m_vertexSnap, &QCheckBox::toggled, m_controller, &DrawingDocumentController::setVertexSnapEnabled);
    connect(m_midpointSnap, &QCheckBox::toggled, m_controller, &DrawingDocumentController::setMidpointSnapEnabled);
    connect(m_centerSnap, &QCheckBox::toggled, m_controller, &DrawingDocumentController::setCenterSnapEnabled);
    connect(m_objectPrioritySnap, &QCheckBox::toggled, m_controller, &DrawingDocumentController::setObjectSnapPriorityBeforeGrid);
    connect(m_objectTolerance, &QComboBox::currentIndexChanged, m_controller, [this](int index) {
        m_controller->setObjectSnapTolerancePreset(m_objectTolerance->itemData(index).toString());
    });

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

    m_workspaceTitle = new QLabel(QStringLiteral("Drawing Canvas"));
    m_workspaceTitle->setObjectName(QStringLiteral("workspaceTitle"));
    headerLayout->addWidget(m_workspaceTitle);
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

    layout->addWidget(makeSectionLabel(QStringLiteral("Selected Object")));
    m_objectKindValue = makeValueLabel();
    m_objectBoundsValue = makeValueLabel();
    m_objectGeometryValue = makeValueLabel();
    m_objectLayerValue = makeValueLabel();
    m_objectMeasurementValue = makeValueLabel();
    layout->addWidget(m_objectKindValue);
    layout->addWidget(m_objectBoundsValue);
    layout->addWidget(m_objectGeometryValue);
    layout->addWidget(m_objectLayerValue);
    layout->addWidget(m_objectMeasurementValue);
    m_geometryEditor = buildGeometryEditor();
    layout->addWidget(m_geometryEditor);
    layout->addWidget(buildNudgeControls());
    layout->addWidget(buildOffsetControls());
    layout->addWidget(buildMirrorControls());
    layout->addWidget(buildRepeatControls());

    layout->addWidget(makeSectionLabel(QStringLiteral("Document")));
    m_toolValue = makeValueLabel();
    m_objectsValue = makeValueLabel();
    m_revisionValue = makeValueLabel();
    layout->addWidget(m_toolValue);
    layout->addWidget(m_objectsValue);
    layout->addWidget(m_revisionValue);

    layout->addWidget(makeSectionLabel(QStringLiteral("Canvas State")));
    m_snapValue = makeValueLabel();
    m_gridValue = makeValueLabel();
    m_pointerValue = makeValueLabel();
    m_previewValue = makeValueLabel();
    layout->addWidget(m_snapValue);
    layout->addWidget(m_gridValue);
    layout->addWidget(m_pointerValue);
    layout->addWidget(m_previewValue);
    layout->addStretch(1);

    return panel;
}

QWidget *EdiShellWindow::buildGeometryEditor()
{
    auto *editor = new QWidget;
    editor->setObjectName(QStringLiteral("geometryEditor"));
    auto *layout = new QGridLayout(editor);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setHorizontalSpacing(6);
    layout->setVerticalSpacing(6);
    return editor;
}

QWidget *EdiShellWindow::buildNudgeControls()
{
    auto *panel = new QWidget;
    panel->setObjectName(QStringLiteral("nudgeControls"));
    auto *layout = new QGridLayout(panel);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setHorizontalSpacing(6);
    layout->setVerticalSpacing(6);

    auto addButton = [this, layout](const QString &label, const QString &direction, const QString &stepMode, int row, int column) {
        auto *button = new QPushButton(label);
        button->setObjectName(QStringLiteral("nudgeButton"));
        connect(button, &QPushButton::clicked, this, [this, direction, stepMode]() {
            m_controller->nudgeSelection(direction, stepMode);
        });
        layout->addWidget(button, row, column);
    };

    layout->addWidget(makeSectionLabel(QStringLiteral("Nudge")), 0, 0, 1, 3);
    addButton(QStringLiteral("Grid Up"), QStringLiteral("up"), QStringLiteral("grid"), 1, 1);
    addButton(QStringLiteral("Grid Left"), QStringLiteral("left"), QStringLiteral("grid"), 2, 0);
    addButton(QStringLiteral("Grid Right"), QStringLiteral("right"), QStringLiteral("grid"), 2, 2);
    addButton(QStringLiteral("Grid Down"), QStringLiteral("down"), QStringLiteral("grid"), 3, 1);
    addButton(QStringLiteral("Fine Up"), QStringLiteral("up"), QStringLiteral("fine"), 4, 1);
    addButton(QStringLiteral("Fine Left"), QStringLiteral("left"), QStringLiteral("fine"), 5, 0);
    addButton(QStringLiteral("Fine Right"), QStringLiteral("right"), QStringLiteral("fine"), 5, 2);
    addButton(QStringLiteral("Fine Down"), QStringLiteral("down"), QStringLiteral("fine"), 6, 1);

    return panel;
}

QWidget *EdiShellWindow::buildOffsetControls()
{
    auto *panel = new QWidget;
    panel->setObjectName(QStringLiteral("offsetControls"));
    auto *layout = new QGridLayout(panel);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setHorizontalSpacing(6);
    layout->setVerticalSpacing(6);

    layout->addWidget(makeSectionLabel(QStringLiteral("Offset")), 0, 0, 1, 2);

    auto *left = new QPushButton(QStringLiteral("Left +0.05"));
    left->setObjectName(QStringLiteral("offsetButton"));
    connect(left, &QPushButton::clicked, this, [this]() {
        m_controller->offsetSelectedObject(QStringLiteral("left"));
    });
    layout->addWidget(left, 1, 0);

    auto *right = new QPushButton(QStringLiteral("Right +0.05"));
    right->setObjectName(QStringLiteral("offsetButton"));
    connect(right, &QPushButton::clicked, this, [this]() {
        m_controller->offsetSelectedObject(QStringLiteral("right"));
    });
    layout->addWidget(right, 1, 1);

    return panel;
}

QWidget *EdiShellWindow::buildMirrorControls()
{
    auto *panel = new QWidget;
    panel->setObjectName(QStringLiteral("mirrorControls"));
    auto *layout = new QGridLayout(panel);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setHorizontalSpacing(6);
    layout->setVerticalSpacing(6);

    layout->addWidget(makeSectionLabel(QStringLiteral("Mirror")), 0, 0, 1, 2);

    auto *horizontal = new QPushButton(QStringLiteral("Mirror H"));
    horizontal->setObjectName(QStringLiteral("mirrorButton"));
    connect(horizontal, &QPushButton::clicked, this, [this]() {
        m_controller->mirrorSelectedObject(QStringLiteral("horizontal"));
    });
    layout->addWidget(horizontal, 1, 0);

    auto *vertical = new QPushButton(QStringLiteral("Mirror V"));
    vertical->setObjectName(QStringLiteral("mirrorButton"));
    connect(vertical, &QPushButton::clicked, this, [this]() {
        m_controller->mirrorSelectedObject(QStringLiteral("vertical"));
    });
    layout->addWidget(vertical, 1, 1);

    return panel;
}

QWidget *EdiShellWindow::buildRepeatControls()
{
    auto *panel = new QWidget;
    panel->setObjectName(QStringLiteral("repeatControls"));
    auto *layout = new QGridLayout(panel);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setHorizontalSpacing(6);
    layout->setVerticalSpacing(6);

    layout->addWidget(makeSectionLabel(QStringLiteral("Repeat")), 0, 0, 1, 2);

    auto *x = new QPushButton(QStringLiteral("Repeat X"));
    x->setObjectName(QStringLiteral("repeatButton"));
    connect(x, &QPushButton::clicked, this, [this]() {
        m_controller->repeatSelectedObject(QStringLiteral("x"));
    });
    layout->addWidget(x, 1, 0);

    auto *y = new QPushButton(QStringLiteral("Repeat Y"));
    y->setObjectName(QStringLiteral("repeatButton"));
    connect(y, &QPushButton::clicked, this, [this]() {
        m_controller->repeatSelectedObject(QStringLiteral("y"));
    });
    layout->addWidget(y, 1, 1);

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
    tabsLayout->addWidget(makeRailButton(QStringLiteral("Commands"), QStringLiteral("Command review"), false, false));
    tabsLayout->addWidget(makeRailButton(QStringLiteral("Notes"), QStringLiteral("Workspace notes"), false, false));
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

QPushButton *EdiShellWindow::makeRailButton(const QString &label, const QString &tooltip, bool active, bool enabled)
{
    auto *button = new QPushButton(label);
    button->setObjectName(QStringLiteral("railButton"));
    button->setToolTip(tooltip);
    button->setCheckable(true);
    button->setChecked(active);
    button->setEnabled(enabled);
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

void EdiShellWindow::setWorkspaceMode(edi::app::WorkspaceMode mode)
{
    edi::app::setWorkspaceMode(m_appState, mode);
    edi::app::setStatusMessage(m_appState, QStringLiteral("%1 workspace active")
        .arg(QString::fromLatin1(edi::app::workspaceModeLabel(mode)))
        .toStdString());
    refreshInspector();
}

void EdiShellWindow::rebuildGeometryEditor(const QVariantMap &selectedObject)
{
    if (m_geometryEditor == nullptr) {
        return;
    }

    auto *layout = qobject_cast<QGridLayout *>(m_geometryEditor->layout());
    if (layout == nullptr) {
        return;
    }

    while (QLayoutItem *item = layout->takeAt(0)) {
        if (QWidget *widget = item->widget()) {
            widget->deleteLater();
        }
        delete item;
    }
    m_geometryFields.clear();

    const QString kind = selectedObject.value(QStringLiteral("kind")).toString();
    QVector<QPair<QString, QString>> fields;
    if (kind == QStringLiteral("point")) {
        fields = {{QStringLiteral("x"), QStringLiteral("X")}, {QStringLiteral("y"), QStringLiteral("Y")}};
    } else if (kind == QStringLiteral("line")) {
        fields = {
            {QStringLiteral("x1"), QStringLiteral("X1")},
            {QStringLiteral("y1"), QStringLiteral("Y1")},
            {QStringLiteral("x2"), QStringLiteral("X2")},
            {QStringLiteral("y2"), QStringLiteral("Y2")},
        };
    } else if (kind == QStringLiteral("rectangle")) {
        fields = {
            {QStringLiteral("x"), QStringLiteral("X")},
            {QStringLiteral("y"), QStringLiteral("Y")},
            {QStringLiteral("width"), QStringLiteral("W")},
            {QStringLiteral("height"), QStringLiteral("H")},
            {QStringLiteral("rotation_deg"), QStringLiteral("Rot")},
        };
    } else if (kind == QStringLiteral("circle")) {
        fields = {
            {QStringLiteral("cx"), QStringLiteral("CX")},
            {QStringLiteral("cy"), QStringLiteral("CY")},
            {QStringLiteral("radius"), QStringLiteral("R")},
        };
    }

    int row = 0;
    for (const auto &field : fields) {
        auto *label = new QLabel(field.second);
        label->setObjectName(QStringLiteral("fieldLabel"));
        auto *spin = new QDoubleSpinBox;
        spin->setObjectName(QStringLiteral("geometryField"));
        spin->setDecimals(4);
        spin->setSingleStep(0.01);
        spin->setRange(-10.0, 10.0);
        if (field.first == QStringLiteral("width") || field.first == QStringLiteral("height") || field.first == QStringLiteral("radius")) {
            spin->setRange(0.0, 10.0);
        } else if (field.first == QStringLiteral("rotation_deg")) {
            spin->setRange(-360.0, 360.0);
            spin->setSingleStep(1.0);
            spin->setDecimals(2);
        }
        spin->setValue(selectedObject.value(field.first).toDouble());
        spin->setProperty("fieldId", field.first);
        connect(spin, &QDoubleSpinBox::editingFinished, this, [this, spin]() {
            m_controller->updateSelectedObjectGeometryField(spin->property("fieldId").toString(), spin->value());
        });
        layout->addWidget(label, row, 0);
        layout->addWidget(spin, row, 1);
        m_geometryFields.insert(field.first, spin);
        ++row;
    }

    setGeometryEditorVisible(!fields.empty());
}

void EdiShellWindow::setGeometryEditorVisible(bool visible)
{
    if (m_geometryEditor != nullptr) {
        m_geometryEditor->setVisible(visible);
    }
}

void EdiShellWindow::refreshInspector()
{
    const QVariantMap document = m_controller->modelDocument();
    const QVariantList objects = document.value(QStringLiteral("drawing_objects")).toList();
    const QVariantList selected = document.value(QStringLiteral("selected_object_ids")).toList();
    const QVariantMap snap = document.value(QStringLiteral("snap")).toMap();
    const QVariantMap grid = document.value(QStringLiteral("grid")).toMap();
    const QVariantMap pointer = document.value(QStringLiteral("pointer")).toMap();
    const QVariantMap selectedObject = activeObjectProjection(document);
    const bool hasPreview = document.contains(QStringLiteral("preview_object"));

    if (m_workspaceTitle != nullptr) {
        m_workspaceTitle->setText(QStringLiteral("%1 Workspace")
            .arg(QString::fromLatin1(edi::app::workspaceModeLabel(m_appState.mode))));
    }

    if (m_toolValue != nullptr) {
        m_toolValue->setText(QStringLiteral("Tool: %1").arg(m_controller->selectedToolId()));
    }
    if (m_selectedValue != nullptr) {
        const QString activeObject = document.value(QStringLiteral("active_object_id")).toString();
        m_selectedValue->setText(activeObject.isEmpty()
            ? QStringLiteral("Selected: none")
            : QStringLiteral("Selected: %1").arg(activeObject));
    }
    if (m_objectKindValue != nullptr) {
        m_objectKindValue->setText(selectedObject.isEmpty()
            ? QStringLiteral("Kind: none")
            : QStringLiteral("Kind: %1   Id: %2")
                .arg(selectedObject.value(QStringLiteral("kind")).toString())
                .arg(selectedObject.value(QStringLiteral("id")).toString()));
    }
    if (m_objectBoundsValue != nullptr) {
        m_objectBoundsValue->setText(selectedObject.isEmpty() ? QStringLiteral("Bounds: none") : boundsSummary(selectedObject));
    }
    if (m_objectGeometryValue != nullptr) {
        m_objectGeometryValue->setText(selectedObject.isEmpty() ? QStringLiteral("Geometry: none") : geometrySummary(selectedObject));
    }
    if (m_objectLayerValue != nullptr) {
        m_objectLayerValue->setText(selectedObject.isEmpty()
            ? QStringLiteral("Layer: none")
            : QStringLiteral("Layer: %1   Locked: %2")
                .arg(selectedObject.value(QStringLiteral("layer_id")).toString())
                .arg(yesNo(selectedObject.value(QStringLiteral("locked")).toBool())));
    }
    if (m_objectMeasurementValue != nullptr) {
        const QVariantList lines = selectedObject.value(QStringLiteral("measurement_lines")).toList();
        QStringList formatted;
        for (const QVariant &line : lines) {
            formatted.push_back(line.toString());
        }
        m_objectMeasurementValue->setText(formatted.isEmpty()
            ? QStringLiteral("Measurement: none")
            : QStringLiteral("Measurement:\n%1").arg(formatted.join(QLatin1Char('\n'))));
    }
    rebuildGeometryEditor(selectedObject);
    if (m_objectsValue != nullptr) {
        m_objectsValue->setText(QStringLiteral("Objects: %1").arg(objects.size()));
    }
    if (m_revisionValue != nullptr) {
        m_revisionValue->setText(QStringLiteral("Revision: %1").arg(document.value(QStringLiteral("revision")).toInt()));
    }
    if (m_snapValue != nullptr) {
        m_snapValue->setText(QStringLiteral("Snap grid: %1   Object: %2   Priority: %3")
            .arg(yesNo(snap.value(QStringLiteral("grid_enabled")).toBool()))
            .arg(yesNo(snap.value(QStringLiteral("object_enabled")).toBool()))
            .arg(snap.value(QStringLiteral("object_priority_before_grid")).toBool() ? QStringLiteral("object") : QStringLiteral("grid")));
    }
    if (m_gridValue != nullptr) {
        m_gridValue->setText(QStringLiteral("Bed: %1, %2 x %3 %4, step %5")
            .arg(grid.value(QStringLiteral("preset_label")).toString())
            .arg(formatNumber(grid.value(QStringLiteral("width")).toDouble()))
            .arg(formatNumber(grid.value(QStringLiteral("height")).toDouble()))
            .arg(grid.value(QStringLiteral("unit_label")).toString())
            .arg(formatNumber(grid.value(QStringLiteral("minor_step")).toDouble())));
    }
    if (m_pointerValue != nullptr) {
        if (pointer.isEmpty()) {
            m_pointerValue->setText(QStringLiteral("Pointer: none"));
        } else {
            const QVariantMap raw = pointer.value(QStringLiteral("raw")).toMap();
            const QVariantMap snapped = pointer.value(QStringLiteral("snapped")).toMap();
            m_pointerValue->setText(QStringLiteral("Pointer: raw %1,%2  snap %3,%4 %5/%6  unit %7,%8 %9  %10")
                .arg(formatNumber(raw.value(QStringLiteral("x")).toDouble()))
                .arg(formatNumber(raw.value(QStringLiteral("y")).toDouble()))
                .arg(formatNumber(snapped.value(QStringLiteral("x")).toDouble()))
                .arg(formatNumber(snapped.value(QStringLiteral("y")).toDouble()))
                .arg(pointer.value(QStringLiteral("kind")).toString())
                .arg(pointer.value(QStringLiteral("source")).toString())
                .arg(formatNumber(pointer.value(QStringLiteral("snapped_unit_x")).toDouble()))
                .arg(formatNumber(pointer.value(QStringLiteral("snapped_unit_y")).toDouble()))
                .arg(pointer.value(QStringLiteral("unit_label")).toString())
                .arg(pointer.value(QStringLiteral("inside_drawable")).toBool() ? QStringLiteral("inside") : QStringLiteral("outside")));
        }
    }
    if (m_previewValue != nullptr) {
        m_previewValue->setText(QStringLiteral("Preview: %1").arg(hasPreview ? QStringLiteral("active") : QStringLiteral("none")));
    }
    if (m_statusValue != nullptr) {
        m_statusValue->setText(QStringLiteral("%1 | %2 selected | %3 objects")
            .arg(QString::fromLatin1(edi::app::workspaceModeName(m_appState.mode)))
            .arg(selected.size())
            .arg(objects.size()));
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
        #fieldLabel {
            color: #9aa8b6;
        }
        #geometryField {
            color: #dce5ee;
            background: #202a35;
            border: 1px solid #31404f;
            border-radius: 5px;
            padding: 4px 6px;
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
        QComboBox {
            color: #dce5ee;
            background: #202a35;
            border: 1px solid #31404f;
            border-radius: 5px;
            padding: 6px 8px;
        }
        QComboBox::drop-down {
            border: 0;
            width: 22px;
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
