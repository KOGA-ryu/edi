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
#include <QLineEdit>
#include <QPair>
#include <QPushButton>
#include <QSignalBlocker>
#include <QSizePolicy>
#include <QSpinBox>
#include <QStringList>
#include <QVariantList>
#include <QVariantMap>
#include <QVBoxLayout>
#include <QVector>

#include <utility>

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

QString formatBoundsValue(const QVariantMap &bounds)
{
    if (bounds.isEmpty()) {
        return QStringLiteral("unavailable");
    }
    return QStringLiteral("x %1, y %2, w %3, h %4")
        .arg(formatNumber(bounds.value(QStringLiteral("x")).toDouble()))
        .arg(formatNumber(bounds.value(QStringLiteral("y")).toDouble()))
        .arg(formatNumber(bounds.value(QStringLiteral("width")).toDouble()))
        .arg(formatNumber(bounds.value(QStringLiteral("height")).toDouble()));
}

QString plotBoundsStatus(const QVariantMap &plot)
{
    if (!plot.value(QStringLiteral("has_plot_bounds")).toBool()) {
        return QStringLiteral("unavailable");
    }
    const QString warningKind = plot.value(QStringLiteral("first_warning_kind")).toString();
    if (warningKind == QStringLiteral("raw_out_of_drawable_bounds")
        || warningKind == QStringLiteral("calibrated_plot_out_of_drawable_bounds")) {
        return QStringLiteral("outside");
    }
    return QStringLiteral("inside");
}

QString plotBoundsSummary(const QVariantMap &grid, const QVariantMap &plot)
{
    const QString unit = grid.value(QStringLiteral("unit_label")).toString();
    const QString plotBounds = plot.value(QStringLiteral("has_plot_bounds")).toBool()
        ? formatBoundsValue(plot.value(QStringLiteral("plot_bounds")).toMap())
        : QStringLiteral("unavailable");

    return QStringLiteral("Bounds (%1):\nBed: %2\nDrawable: %3\nPlot: %4\nPlot status: %5")
        .arg(unit.isEmpty() ? QStringLiteral("unit") : unit)
        .arg(formatBoundsValue(grid.value(QStringLiteral("page_bounds")).toMap()))
        .arg(formatBoundsValue(grid.value(QStringLiteral("drawable_bounds")).toMap()))
        .arg(plotBounds)
        .arg(plotBoundsStatus(plot));
}

QString formatPlotReadinessChecklist(const QVariantList &layerStats, const QVariantList &penStats)
{
    int readyLayers = 0;
    int blockedLayers = 0;
    int readyPens = 0;
    int blockedPens = 0;
    QStringList blockers;

    for (const QVariant &statsValue : layerStats) {
        const QVariantMap stats = statsValue.toMap();
        if (stats.value(QStringLiteral("ready")).toBool()) {
            ++readyLayers;
            continue;
        }
        ++blockedLayers;
        if (blockers.size() < 4) {
            blockers.push_back(QStringLiteral("L:%1 %2")
                .arg(stats.value(QStringLiteral("layer_id")).toString())
                .arg(stats.value(QStringLiteral("blocked_reason")).toString()));
        }
    }

    for (const QVariant &statsValue : penStats) {
        const QVariantMap stats = statsValue.toMap();
        if (stats.value(QStringLiteral("ready")).toBool()) {
            ++readyPens;
            continue;
        }
        ++blockedPens;
        if (blockers.size() < 4) {
            const QString penId = stats.value(QStringLiteral("pen_id")).toString();
            blockers.push_back(QStringLiteral("P:%1 %2")
                .arg(penId.isEmpty() ? QStringLiteral("<none>") : penId)
                .arg(stats.value(QStringLiteral("blocked_reason")).toString()));
        }
    }

    QString summary = QStringLiteral("Readiness: layers %1 ready/%2 blocked, pens %3 ready/%4 blocked")
        .arg(readyLayers)
        .arg(blockedLayers)
        .arg(readyPens)
        .arg(blockedPens);
    if (!blockers.isEmpty()) {
        summary.append(QStringLiteral("\nBlocked: %1").arg(blockers.join(QStringLiteral("; "))));
    }
    return summary;
}

QVariantMap plotStatsByField(const QVariantList &statsList, const QString &field, const QString &value)
{
    for (const QVariant &statsValue : statsList) {
        const QVariantMap stats = statsValue.toMap();
        if (stats.value(field).toString() == value) {
            return stats;
        }
    }
    return {};
}

QString readinessText(const QVariantMap &stats)
{
    if (stats.isEmpty()) {
        return QStringLiteral("none");
    }
    return stats.value(QStringLiteral("ready")).toBool()
        ? QStringLiteral("ready")
        : stats.value(QStringLiteral("blocked_reason")).toString();
}

QString selectedPlotSafetySummary(const QVariantMap &object, const QVariantMap &plot)
{
    if (object.isEmpty()) {
        return QStringLiteral("Plot safety: no selection");
    }

    QString objectStatus = QStringLiteral("non-plotting");
    if (object.value(QStringLiteral("plot_blocked")).toBool()) {
        objectStatus = QStringLiteral("blocked");
    } else if (object.value(QStringLiteral("effective_plot_ready")).toBool()) {
        objectStatus = QStringLiteral("ready");
    }

    const QVariantMap layerStats = plotStatsByField(
        plot.value(QStringLiteral("layer_stats")).toList(),
        QStringLiteral("layer_id"),
        object.value(QStringLiteral("layer_id")).toString());
    const QVariantMap penStats = plotStatsByField(
        plot.value(QStringLiteral("pen_stats")).toList(),
        QStringLiteral("pen_id"),
        object.value(QStringLiteral("effective_pen_id")).toString());

    QStringList lines;
    lines.push_back(QStringLiteral("Object: %1").arg(objectStatus));
    lines.push_back(QStringLiteral("Warning: %1")
        .arg(object.value(QStringLiteral("plot_warning_kind")).toString().isEmpty()
                ? QStringLiteral("none")
                : object.value(QStringLiteral("plot_warning_kind")).toString()));
    const QString message = object.value(QStringLiteral("plot_warning_message")).toString();
    if (!message.isEmpty()) {
        lines.push_back(QStringLiteral("Message: %1").arg(message));
    }
    lines.push_back(QStringLiteral("Bounds: raw %1, calibrated %2")
        .arg(yesNo(object.value(QStringLiteral("outside_drawable")).toBool()))
        .arg(yesNo(object.value(QStringLiteral("calibrated_outside_drawable")).toBool())));
    lines.push_back(QStringLiteral("Layer: %1").arg(readinessText(layerStats)));
    lines.push_back(QStringLiteral("Pen: %1").arg(readinessText(penStats)));

    return QStringLiteral("Plot safety:\n%1").arg(lines.join(QLatin1Char('\n')));
}

QString selectionPlotBoundsSummary(const QVariantMap &document)
{
    if (!document.value(QStringLiteral("has_selection_plot_bounds")).toBool()) {
        return QStringLiteral("Selection plot bounds: unavailable");
    }
    return QStringLiteral("Selection plot bounds:\n%1\nw %2, h %3\n%4")
        .arg(formatBoundsValue(document.value(QStringLiteral("selection_plot_bounds")).toMap()))
        .arg(formatNumber(document.value(QStringLiteral("selection_plot_bounds_width")).toDouble()))
        .arg(formatNumber(document.value(QStringLiteral("selection_plot_bounds_height")).toDouble()))
        .arg(document.value(QStringLiteral("selection_drawable_relation")).toString());
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

QVariantMap layerProjection(const QVariantMap &document, const QString &layerId)
{
    const QVariantList layers = document.value(QStringLiteral("layers")).toList();
    for (const QVariant &layerValue : layers) {
        const QVariantMap layer = layerValue.toMap();
        if (layer.value(QStringLiteral("id")).toString() == layerId) {
            return layer;
        }
    }
    return {};
}

void refreshLayerCombo(QComboBox *combo, const QVariantList &layers, const QString &currentLayerId, bool enabled)
{
    if (combo == nullptr) {
        return;
    }

    const QSignalBlocker blocker(combo);
    combo->clear();
    for (const QVariant &layerValue : layers) {
        const QVariantMap layer = layerValue.toMap();
        const QString id = layer.value(QStringLiteral("id")).toString();
        const QString label = QStringLiteral("%1 (%2%3)")
            .arg(layer.value(QStringLiteral("name")).toString())
            .arg(layer.value(QStringLiteral("visible")).toBool() ? QStringLiteral("V") : QStringLiteral("-"))
            .arg(layer.value(QStringLiteral("locked")).toBool() ? QStringLiteral("L") : QStringLiteral("-"));
        combo->addItem(label, id);
    }

    const int index = combo->findData(currentLayerId);
    if (index >= 0) {
        combo->setCurrentIndex(index);
    }
    combo->setEnabled(enabled && !layers.empty());
}

QString strokeWidthPresetId(double width)
{
    if (width <= 1.5) {
        return QStringLiteral("fine");
    }
    if (width >= 2.5) {
        return QStringLiteral("bold");
    }
    return QStringLiteral("normal");
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
        return QStringLiteral("Geometry: line (%1, %2) -> (%3, %4), len %5, ang %6")
            .arg(formatNumber(object.value(QStringLiteral("x1")).toDouble()))
            .arg(formatNumber(object.value(QStringLiteral("y1")).toDouble()))
            .arg(formatNumber(object.value(QStringLiteral("x2")).toDouble()))
            .arg(formatNumber(object.value(QStringLiteral("y2")).toDouble()))
            .arg(formatNumber(object.value(QStringLiteral("line_length")).toDouble()))
            .arg(formatNumber(object.value(QStringLiteral("line_angle_deg")).toDouble()));
    }
    if (kind == QStringLiteral("construction_line")) {
        return QStringLiteral("Geometry: construction (%1, %2) -> (%3, %4)")
            .arg(formatNumber(object.value(QStringLiteral("x1")).toDouble()))
            .arg(formatNumber(object.value(QStringLiteral("y1")).toDouble()))
            .arg(formatNumber(object.value(QStringLiteral("x2")).toDouble()))
            .arg(formatNumber(object.value(QStringLiteral("y2")).toDouble()));
    }
    if (kind == QStringLiteral("guide")) {
        return QStringLiteral("Geometry: %1 guide at %2")
            .arg(object.value(QStringLiteral("orientation")).toString())
            .arg(formatNumber(object.value(QStringLiteral("position")).toDouble()));
    }
    if (kind == QStringLiteral("dimension")) {
        return QStringLiteral("Geometry: dimension (%1, %2) -> (%3, %4), off %5, %6")
            .arg(formatNumber(object.value(QStringLiteral("x1")).toDouble()))
            .arg(formatNumber(object.value(QStringLiteral("y1")).toDouble()))
            .arg(formatNumber(object.value(QStringLiteral("x2")).toDouble()))
            .arg(formatNumber(object.value(QStringLiteral("y2")).toDouble()))
            .arg(formatNumber(object.value(QStringLiteral("offset")).toDouble()))
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
        return QStringLiteral("Geometry: circle cx %1, cy %2, r %3, d %4")
            .arg(formatNumber(object.value(QStringLiteral("cx")).toDouble()))
            .arg(formatNumber(object.value(QStringLiteral("cy")).toDouble()))
            .arg(formatNumber(object.value(QStringLiteral("radius")).toDouble()))
            .arg(formatNumber(object.value(QStringLiteral("diameter")).toDouble()));
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
        {QStringLiteral("distance_dimension_tool"), QStringLiteral("Dim Distance")},
        {QStringLiteral("width_dimension_tool"), QStringLiteral("Dim Width")},
        {QStringLiteral("height_dimension_tool"), QStringLiteral("Dim Height")},
        {QStringLiteral("radius_dimension_tool"), QStringLiteral("Dim Radius")},
        {QStringLiteral("diameter_dimension_tool"), QStringLiteral("Dim Diameter")},
    };

    for (const auto &tool : tools) {
        layout->addWidget(makeToolButton(tool.first, tool.second));
    }

    connect(m_toolGroup, &QButtonGroup::buttonClicked, m_controller, [this](QAbstractButton *button) {
        m_controller->setSelectedToolId(button->property("toolId").toString());
    });

    layout->addWidget(makeSectionLabel(QStringLiteral("Snap")));
    m_gridPreset = new QComboBox;
    m_gridPreset->setObjectName(QStringLiteral("controlInput"));
    m_gridPreset->addItem(QStringLiteral("Square art board"), QStringLiteral("square_art_board"));
    m_gridPreset->addItem(QStringLiteral("Letter"), QStringLiteral("letter"));
    m_gridPreset->addItem(QStringLiteral("A4"), QStringLiteral("a4"));
    m_gridPreset->addItem(QStringLiteral("Custom"), QStringLiteral("custom"));
    const int presetIndex = m_gridPreset->findData(m_controller->gridPresetId());
    if (presetIndex >= 0) {
        m_gridPreset->setCurrentIndex(presetIndex);
    }
    layout->addWidget(m_gridPreset);

    auto *gridSettingsLayout = new QGridLayout;
    gridSettingsLayout->setContentsMargins(0, 0, 0, 0);
    gridSettingsLayout->setHorizontalSpacing(6);
    gridSettingsLayout->setVerticalSpacing(6);

    m_gridUnit = new QComboBox;
    m_gridUnit->setObjectName(QStringLiteral("controlInput"));
    m_gridUnit->addItem(QStringLiteral("cu"), QStringLiteral("canvas_unit"));
    m_gridUnit->addItem(QStringLiteral("mm"), QStringLiteral("millimeter"));
    m_gridUnit->addItem(QStringLiteral("cm"), QStringLiteral("centimeter"));
    m_gridUnit->addItem(QStringLiteral("in"), QStringLiteral("inch"));
    m_gridUnit->addItem(QStringLiteral("ft"), QStringLiteral("foot"));

    auto makeGridSpin = []() {
        auto *spin = new QDoubleSpinBox;
        spin->setObjectName(QStringLiteral("geometryField"));
        spin->setDecimals(4);
        spin->setSingleStep(0.25);
        spin->setRange(0.0, 100000.0);
        return spin;
    };
    m_gridWidth = makeGridSpin();
    m_gridHeight = makeGridSpin();
    m_gridMarginLeft = makeGridSpin();
    m_gridMarginTop = makeGridSpin();
    m_gridMarginRight = makeGridSpin();
    m_gridMarginBottom = makeGridSpin();
    m_gridMinorStep = makeGridSpin();
    m_gridMinorStep->setMinimum(0.0001);
    m_gridMajorEvery = new QSpinBox;
    m_gridMajorEvery->setObjectName(QStringLiteral("geometryField"));
    m_gridMajorEvery->setRange(1, 1000);
    m_gridVisible = new QCheckBox(QStringLiteral("Visible"));

    gridSettingsLayout->addWidget(new QLabel(QStringLiteral("Unit")), 0, 0);
    gridSettingsLayout->addWidget(m_gridUnit, 0, 1);
    gridSettingsLayout->addWidget(new QLabel(QStringLiteral("W")), 1, 0);
    gridSettingsLayout->addWidget(m_gridWidth, 1, 1);
    gridSettingsLayout->addWidget(new QLabel(QStringLiteral("H")), 2, 0);
    gridSettingsLayout->addWidget(m_gridHeight, 2, 1);
    gridSettingsLayout->addWidget(new QLabel(QStringLiteral("ML")), 3, 0);
    gridSettingsLayout->addWidget(m_gridMarginLeft, 3, 1);
    gridSettingsLayout->addWidget(new QLabel(QStringLiteral("MT")), 4, 0);
    gridSettingsLayout->addWidget(m_gridMarginTop, 4, 1);
    gridSettingsLayout->addWidget(new QLabel(QStringLiteral("MR")), 5, 0);
    gridSettingsLayout->addWidget(m_gridMarginRight, 5, 1);
    gridSettingsLayout->addWidget(new QLabel(QStringLiteral("MB")), 6, 0);
    gridSettingsLayout->addWidget(m_gridMarginBottom, 6, 1);
    gridSettingsLayout->addWidget(new QLabel(QStringLiteral("Step")), 7, 0);
    gridSettingsLayout->addWidget(m_gridMinorStep, 7, 1);
    gridSettingsLayout->addWidget(new QLabel(QStringLiteral("Major")), 8, 0);
    gridSettingsLayout->addWidget(m_gridMajorEvery, 8, 1);
    gridSettingsLayout->addWidget(m_gridVisible, 9, 0, 1, 2);
    layout->addLayout(gridSettingsLayout);

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
    m_guideSnap = new QCheckBox(QStringLiteral("Guide"));
    m_guideSnap->setChecked(m_controller->guideSnapEnabled());
    m_guideMoveSnap = new QCheckBox(QStringLiteral("Guide move snap"));
    m_guideMoveSnap->setChecked(m_controller->guideMoveSnapEnabled());
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
    layout->addWidget(m_guideSnap);
    layout->addWidget(m_guideMoveSnap);
    layout->addWidget(m_objectPrioritySnap);
    layout->addWidget(m_objectTolerance);

    connect(m_gridPreset, &QComboBox::currentIndexChanged, m_controller, [this](int index) {
        m_controller->setGridPresetId(m_gridPreset->itemData(index).toString());
    });
    connect(m_gridUnit, &QComboBox::currentIndexChanged, m_controller, [this](int index) {
        m_controller->setGridUnitId(m_gridUnit->itemData(index).toString());
    });
    connect(m_gridWidth, &QDoubleSpinBox::editingFinished, m_controller, [this]() {
        m_controller->setGridSize(m_gridWidth->value(), m_gridHeight->value());
    });
    connect(m_gridHeight, &QDoubleSpinBox::editingFinished, m_controller, [this]() {
        m_controller->setGridSize(m_gridWidth->value(), m_gridHeight->value());
    });
    const auto applyGridMargins = [this]() {
        m_controller->setGridMargins(
            m_gridMarginLeft->value(),
            m_gridMarginTop->value(),
            m_gridMarginRight->value(),
            m_gridMarginBottom->value());
    };
    connect(m_gridMarginLeft, &QDoubleSpinBox::editingFinished, m_controller, applyGridMargins);
    connect(m_gridMarginTop, &QDoubleSpinBox::editingFinished, m_controller, applyGridMargins);
    connect(m_gridMarginRight, &QDoubleSpinBox::editingFinished, m_controller, applyGridMargins);
    connect(m_gridMarginBottom, &QDoubleSpinBox::editingFinished, m_controller, applyGridMargins);
    connect(m_gridMinorStep, &QDoubleSpinBox::editingFinished, m_controller, [this]() {
        m_controller->setGridStep(m_gridMinorStep->value());
    });
    connect(m_gridMajorEvery, &QSpinBox::editingFinished, m_controller, [this]() {
        m_controller->setGridMajorLineEvery(m_gridMajorEvery->value());
    });
    connect(m_gridVisible, &QCheckBox::toggled, m_controller, &DrawingDocumentController::setGridVisible);
    connect(m_gridSnap, &QCheckBox::toggled, m_controller, &DrawingDocumentController::setGridSnapEnabled);
    connect(m_objectSnap, &QCheckBox::toggled, m_controller, &DrawingDocumentController::setObjectSnapEnabled);
    connect(m_endpointSnap, &QCheckBox::toggled, m_controller, &DrawingDocumentController::setEndpointSnapEnabled);
    connect(m_vertexSnap, &QCheckBox::toggled, m_controller, &DrawingDocumentController::setVertexSnapEnabled);
    connect(m_midpointSnap, &QCheckBox::toggled, m_controller, &DrawingDocumentController::setMidpointSnapEnabled);
    connect(m_centerSnap, &QCheckBox::toggled, m_controller, &DrawingDocumentController::setCenterSnapEnabled);
    connect(m_guideSnap, &QCheckBox::toggled, m_controller, &DrawingDocumentController::setGuideSnapEnabled);
    connect(m_guideMoveSnap, &QCheckBox::toggled, m_controller, &DrawingDocumentController::setGuideMoveSnapEnabled);
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
    m_objectPlotSafetyValue = makeValueLabel();
    m_selectionPlotBoundsValue = makeValueLabel();
    layout->addWidget(m_objectKindValue);
    layout->addWidget(m_objectBoundsValue);
    layout->addWidget(m_objectGeometryValue);
    layout->addWidget(m_objectLayerValue);
    layout->addWidget(m_objectMeasurementValue);
    layout->addWidget(m_objectPlotSafetyValue);
    layout->addWidget(m_selectionPlotBoundsValue);
    m_fitSelectionToDrawableButton = new QPushButton(QStringLiteral("Fit To Drawable"));
    m_fitSelectionToDrawableButton->setObjectName(QStringLiteral("fitToDrawableButton"));
    connect(m_fitSelectionToDrawableButton, &QPushButton::clicked, this, [this]() {
        m_controller->fitSelectionToDrawableBounds();
    });
    layout->addWidget(m_fitSelectionToDrawableButton);
    m_centerSelectionInDrawableButton = new QPushButton(QStringLiteral("Center In Drawable"));
    m_centerSelectionInDrawableButton->setObjectName(QStringLiteral("centerInDrawableButton"));
    connect(m_centerSelectionInDrawableButton, &QPushButton::clicked, this, [this]() {
        m_controller->centerSelectionInDrawable();
    });
    layout->addWidget(m_centerSelectionInDrawableButton);
    m_moveSelectionToDrawableOriginButton = new QPushButton(QStringLiteral("Move To Drawable Origin"));
    m_moveSelectionToDrawableOriginButton->setObjectName(QStringLiteral("moveToDrawableOriginButton"));
    connect(m_moveSelectionToDrawableOriginButton, &QPushButton::clicked, this, [this]() {
        m_controller->moveSelectionToDrawableOrigin();
    });
    layout->addWidget(m_moveSelectionToDrawableOriginButton);
    m_moveGuideToDrawableOriginButton = new QPushButton(QStringLiteral("Guide To Drawable Origin"));
    m_moveGuideToDrawableOriginButton->setObjectName(QStringLiteral("guideToDrawableOriginButton"));
    connect(m_moveGuideToDrawableOriginButton, &QPushButton::clicked, this, [this]() {
        m_controller->moveSelectedGuideToDrawableOrigin();
    });
    layout->addWidget(m_moveGuideToDrawableOriginButton);
    m_centerGuideInDrawableButton = new QPushButton(QStringLiteral("Center Guide"));
    m_centerGuideInDrawableButton->setObjectName(QStringLiteral("centerGuideButton"));
    connect(m_centerGuideInDrawableButton, &QPushButton::clicked, this, [this]() {
        m_controller->centerSelectedGuideInDrawable();
    });
    layout->addWidget(m_centerGuideInDrawableButton);
    m_moveGuideToDrawableMaxButton = new QPushButton(QStringLiteral("Guide To Drawable Max"));
    m_moveGuideToDrawableMaxButton->setObjectName(QStringLiteral("guideToDrawableMaxButton"));
    connect(m_moveGuideToDrawableMaxButton, &QPushButton::clicked, this, [this]() {
        m_controller->moveSelectedGuideToDrawableMax();
    });
    layout->addWidget(m_moveGuideToDrawableMaxButton);
    const QVector<QPair<QString, QString>> guideOffsetButtons {
        {QStringLiteral("negative_fine"), QStringLiteral("Guide - Fine")},
        {QStringLiteral("positive_fine"), QStringLiteral("Guide + Fine")},
        {QStringLiteral("negative_grid"), QStringLiteral("Guide - Grid")},
        {QStringLiteral("positive_grid"), QStringLiteral("Guide + Grid")},
        {QStringLiteral("negative_coarse"), QStringLiteral("Guide - Coarse")},
        {QStringLiteral("positive_coarse"), QStringLiteral("Guide + Coarse")},
    };
    for (const auto &buttonSpec : guideOffsetButtons) {
        auto *button = new QPushButton(buttonSpec.second);
        button->setObjectName(QStringLiteral("guideOffset_%1").arg(buttonSpec.first));
        const QStringList parts = buttonSpec.first.split(QLatin1Char('_'));
        connect(button, &QPushButton::clicked, this, [this, direction = parts.value(0), stepMode = parts.value(1)]() {
            m_controller->offsetSelectedGuide(direction, stepMode);
        });
        m_guideOffsetButtons.insert(buttonSpec.first, button);
        layout->addWidget(button);
    }
    layout->addWidget(makeSectionLabel(QStringLiteral("Guide Presets")));
    const QVector<QPair<QString, QString>> guidePresetButtons {
        {QStringLiteral("drawable_bounds"), QStringLiteral("Preset Bounds")},
        {QStringLiteral("drawable_centerlines"), QStringLiteral("Preset Centerlines")},
        {QStringLiteral("thirds"), QStringLiteral("Preset Thirds")},
        {QStringLiteral("quarters"), QStringLiteral("Preset Quarters")},
        {QStringLiteral("margin_safe"), QStringLiteral("Preset Margin Safe")},
    };
    for (const auto &buttonSpec : guidePresetButtons) {
        auto *button = new QPushButton(buttonSpec.second);
        button->setObjectName(QStringLiteral("guidePreset_%1").arg(buttonSpec.first));
        connect(button, &QPushButton::clicked, this, [this, presetId = buttonSpec.first]() {
            m_controller->applyGuidePreset(presetId);
        });
        layout->addWidget(button);
    }
    m_fitConstructionToDrawableButton = new QPushButton(QStringLiteral("Fit Construction To Drawable"));
    m_fitConstructionToDrawableButton->setObjectName(QStringLiteral("fitConstructionToDrawableButton"));
    connect(m_fitConstructionToDrawableButton, &QPushButton::clicked, this, [this]() {
        m_controller->fitSelectedConstructionLineToDrawable();
    });
    layout->addWidget(m_fitConstructionToDrawableButton);
    layout->addWidget(makeSectionLabel(QStringLiteral("Bounds Guides")));
    const QVector<QPair<QString, QString>> boundsGuideButtons {
        {QStringLiteral("left"), QStringLiteral("Guide Left")},
        {QStringLiteral("right"), QStringLiteral("Guide Right")},
        {QStringLiteral("top"), QStringLiteral("Guide Top")},
        {QStringLiteral("bottom"), QStringLiteral("Guide Bottom")},
        {QStringLiteral("vertical_center"), QStringLiteral("Guide V Center")},
        {QStringLiteral("horizontal_center"), QStringLiteral("Guide H Center")},
    };
    for (const auto &buttonSpec : boundsGuideButtons) {
        auto *button = new QPushButton(buttonSpec.second);
        button->setObjectName(QStringLiteral("boundsGuide_%1").arg(buttonSpec.first));
        connect(button, &QPushButton::clicked, this, [this, placementId = buttonSpec.first]() {
            m_controller->createGuideFromSelectedBounds(placementId);
        });
        m_boundsGuideButtons.insert(buttonSpec.first, button);
        layout->addWidget(button);
    }
    layout->addWidget(makeSectionLabel(QStringLiteral("Offset Guides")));
    const QVector<QPair<QString, QString>> offsetGuideButtons {
        {QStringLiteral("left"), QStringLiteral("Offset V Left")},
        {QStringLiteral("right"), QStringLiteral("Offset V Right")},
        {QStringLiteral("top"), QStringLiteral("Offset H Top")},
        {QStringLiteral("bottom"), QStringLiteral("Offset H Bottom")},
    };
    for (const auto &buttonSpec : offsetGuideButtons) {
        auto *button = new QPushButton(buttonSpec.second);
        button->setObjectName(QStringLiteral("offsetGuide_%1").arg(buttonSpec.first));
        connect(button, &QPushButton::clicked, this, [this, placementId = buttonSpec.first]() {
            m_controller->createOffsetGuideFromSelectedBounds(placementId, QStringLiteral("grid"));
        });
        m_offsetGuideButtons.insert(buttonSpec.first, button);
        layout->addWidget(button);
    }
    layout->addWidget(makeSectionLabel(QStringLiteral("Align To Guide")));
    const QVector<QPair<QString, QString>> alignToGuideButtons {
        {QStringLiteral("left"), QStringLiteral("To V Guide Left")},
        {QStringLiteral("center_x"), QStringLiteral("To V Guide Center")},
        {QStringLiteral("right"), QStringLiteral("To V Guide Right")},
        {QStringLiteral("top"), QStringLiteral("To H Guide Top")},
        {QStringLiteral("center_y"), QStringLiteral("To H Guide Center")},
        {QStringLiteral("bottom"), QStringLiteral("To H Guide Bottom")},
    };
    for (const auto &buttonSpec : alignToGuideButtons) {
        auto *button = new QPushButton(buttonSpec.second);
        button->setObjectName(QStringLiteral("alignToGuide_%1").arg(buttonSpec.first));
        connect(button, &QPushButton::clicked, this, [this, modeId = buttonSpec.first]() {
            m_controller->alignSelectionToNearestGuide(modeId);
        });
        m_alignToGuideButtons.insert(buttonSpec.first, button);
        layout->addWidget(button);
    }
    layout->addWidget(makeSectionLabel(QStringLiteral("Guide Lifecycle")));
    m_deleteSelectedGuideButton = new QPushButton(QStringLiteral("Delete Selected Guide"));
    m_deleteSelectedGuideButton->setObjectName(QStringLiteral("deleteSelectedGuideButton"));
    connect(m_deleteSelectedGuideButton, &QPushButton::clicked, this, [this]() {
        m_controller->deleteSelectedGuide();
    });
    layout->addWidget(m_deleteSelectedGuideButton);
    m_deleteAllGuidesButton = new QPushButton(QStringLiteral("Delete All Guides"));
    m_deleteAllGuidesButton->setObjectName(QStringLiteral("deleteAllGuidesButton"));
    connect(m_deleteAllGuidesButton, &QPushButton::clicked, this, [this]() {
        m_controller->deleteAllGuides();
    });
    layout->addWidget(m_deleteAllGuidesButton);
    m_mergeDuplicateGuidesButton = new QPushButton(QStringLiteral("Merge Duplicate Guides"));
    m_mergeDuplicateGuidesButton->setObjectName(QStringLiteral("mergeDuplicateGuidesButton"));
    connect(m_mergeDuplicateGuidesButton, &QPushButton::clicked, this, [this]() {
        m_controller->mergeDuplicateGuides();
    });
    layout->addWidget(m_mergeDuplicateGuidesButton);
    m_hideAllGuidesButton = new QPushButton(QStringLiteral("Hide All Guides"));
    m_hideAllGuidesButton->setObjectName(QStringLiteral("hideAllGuidesButton"));
    connect(m_hideAllGuidesButton, &QPushButton::clicked, this, [this]() {
        m_controller->setAllGuidesVisible(false);
    });
    layout->addWidget(m_hideAllGuidesButton);
    m_showAllGuidesButton = new QPushButton(QStringLiteral("Show All Guides"));
    m_showAllGuidesButton->setObjectName(QStringLiteral("showAllGuidesButton"));
    connect(m_showAllGuidesButton, &QPushButton::clicked, this, [this]() {
        m_controller->setAllGuidesVisible(true);
    });
    layout->addWidget(m_showAllGuidesButton);
    m_lockAllGuidesButton = new QPushButton(QStringLiteral("Lock All Guides"));
    m_lockAllGuidesButton->setObjectName(QStringLiteral("lockAllGuidesButton"));
    connect(m_lockAllGuidesButton, &QPushButton::clicked, this, [this]() {
        m_controller->setAllGuidesLocked(true);
    });
    layout->addWidget(m_lockAllGuidesButton);
    m_unlockAllGuidesButton = new QPushButton(QStringLiteral("Unlock All Guides"));
    m_unlockAllGuidesButton->setObjectName(QStringLiteral("unlockAllGuidesButton"));
    connect(m_unlockAllGuidesButton, &QPushButton::clicked, this, [this]() {
        m_controller->setAllGuidesLocked(false);
    });
    layout->addWidget(m_unlockAllGuidesButton);
    layout->addWidget(makeSectionLabel(QStringLiteral("Guide Visuals")));
    m_guideLabel = new QLineEdit;
    m_guideLabel->setObjectName(QStringLiteral("guideLabelField"));
    m_guideLabel->setPlaceholderText(QStringLiteral("Default guide label"));
    connect(m_guideLabel, &QLineEdit::editingFinished, this, [this]() {
        m_controller->setSelectedGuideLabel(m_guideLabel->text());
    });
    layout->addWidget(m_guideLabel);
    m_guideColor = new QComboBox;
    m_guideColor->setObjectName(QStringLiteral("guideColorCombo"));
    m_guideColor->addItem(QStringLiteral("Guide blue"), QStringLiteral("#83aeca"));
    m_guideColor->addItem(QStringLiteral("Guide teal"), QStringLiteral("#54d2c6"));
    m_guideColor->addItem(QStringLiteral("Guide amber"), QStringLiteral("#f6c65b"));
    m_guideColor->addItem(QStringLiteral("Guide red"), QStringLiteral("#d98b8b"));
    m_guideColor->addItem(QStringLiteral("Guide green"), QStringLiteral("#91c89b"));
    connect(m_guideColor, &QComboBox::currentIndexChanged, this, [this](int index) {
        m_controller->setSelectedGuideColor(m_guideColor->itemData(index).toString());
    });
    layout->addWidget(m_guideColor);
    m_guideDashStyle = new QComboBox;
    m_guideDashStyle->setObjectName(QStringLiteral("guideDashStyleCombo"));
    m_guideDashStyle->addItem(QStringLiteral("Dash line"), QStringLiteral("dash"));
    m_guideDashStyle->addItem(QStringLiteral("Solid line"), QStringLiteral("solid"));
    m_guideDashStyle->addItem(QStringLiteral("Dot line"), QStringLiteral("dot"));
    connect(m_guideDashStyle, &QComboBox::currentIndexChanged, this, [this](int index) {
        m_controller->setSelectedGuideDashStyle(m_guideDashStyle->itemData(index).toString());
    });
    layout->addWidget(m_guideDashStyle);
    m_guideShowLabel = new QCheckBox(QStringLiteral("Show guide label"));
    m_guideShowLabel->setObjectName(QStringLiteral("guideShowLabelCheckbox"));
    connect(m_guideShowLabel, &QCheckBox::toggled, this, [this](bool checked) {
        m_controller->setSelectedGuideLabelVisible(checked);
    });
    layout->addWidget(m_guideShowLabel);
    layout->addWidget(makeSectionLabel(QStringLiteral("Dimension")));
    m_dimensionReadout = makeValueLabel(QStringLiteral("Dimension: none"));
    layout->addWidget(m_dimensionReadout);
    m_dimensionKind = new QComboBox;
    m_dimensionKind->setObjectName(QStringLiteral("dimensionKindCombo"));
    m_dimensionKind->addItem(QStringLiteral("Distance"), QStringLiteral("distance"));
    m_dimensionKind->addItem(QStringLiteral("Width"), QStringLiteral("width"));
    m_dimensionKind->addItem(QStringLiteral("Height"), QStringLiteral("height"));
    m_dimensionKind->addItem(QStringLiteral("Radius"), QStringLiteral("radius"));
    m_dimensionKind->addItem(QStringLiteral("Diameter"), QStringLiteral("diameter"));
    connect(m_dimensionKind, &QComboBox::currentIndexChanged, this, [this](int index) {
        m_controller->setSelectedDimensionKind(m_dimensionKind->itemData(index).toString());
    });
    layout->addWidget(m_dimensionKind);
    m_dimensionShowLabel = new QCheckBox(QStringLiteral("Show dimension label"));
    m_dimensionShowLabel->setObjectName(QStringLiteral("dimensionShowLabelCheckbox"));
    connect(m_dimensionShowLabel, &QCheckBox::toggled, this, [this](bool checked) {
        m_controller->setSelectedDimensionLabelVisible(checked);
    });
    layout->addWidget(m_dimensionShowLabel);
    layout->addWidget(buildObjectFlagControls());
    layout->addWidget(buildLayerControls());
    m_geometryEditor = buildGeometryEditor();
    layout->addWidget(m_geometryEditor);
    layout->addWidget(buildNudgeControls());
    layout->addWidget(buildAlignControls());
    layout->addWidget(buildOffsetControls());
    layout->addWidget(buildMirrorControls());
    layout->addWidget(buildRepeatControls());
    layout->addWidget(buildCalibrationControls());

    layout->addWidget(makeSectionLabel(QStringLiteral("Document")));
    m_toolValue = makeValueLabel();
    m_objectsValue = makeValueLabel();
    m_guidesValue = makeValueLabel();
    m_revisionValue = makeValueLabel();
    layout->addWidget(m_toolValue);
    layout->addWidget(m_objectsValue);
    layout->addWidget(m_guidesValue);
    layout->addWidget(m_revisionValue);

    layout->addWidget(makeSectionLabel(QStringLiteral("Canvas State")));
    m_snapValue = makeValueLabel();
    m_gridValue = makeValueLabel();
    m_plotValue = makeValueLabel();
    m_plotBoundsValue = makeValueLabel();
    m_plotLayerStatsValue = makeValueLabel();
    m_plotPenStatsValue = makeValueLabel();
    m_plotReadinessValue = makeValueLabel();
    m_plotOrderMode = new QComboBox;
    m_plotOrderMode->setObjectName(QStringLiteral("plotOrderMode"));
    m_plotOrderMode->addItem(QStringLiteral("Plot order: layer"), QStringLiteral("layer_order"));
    m_plotOrderMode->addItem(QStringLiteral("Plot order: nearest"), QStringLiteral("nearest_next"));
    const int plotOrderIndex = m_plotOrderMode->findData(m_controller->plotOrderModeId());
    if (plotOrderIndex >= 0) {
        m_plotOrderMode->setCurrentIndex(plotOrderIndex);
    }
    connect(m_plotOrderMode, &QComboBox::currentIndexChanged, this, [this](int index) {
        m_controller->setPlotOrderModeId(m_plotOrderMode->itemData(index).toString());
    });
    m_plotDirectionMode = new QComboBox;
    m_plotDirectionMode->setObjectName(QStringLiteral("plotDirectionMode"));
    m_plotDirectionMode->addItem(QStringLiteral("Direction: preserve"), QStringLiteral("preserve_direction"));
    m_plotDirectionMode->addItem(QStringLiteral("Direction: reversible"), QStringLiteral("allow_reverse_segments"));
    const int plotDirectionIndex = m_plotDirectionMode->findData(m_controller->plotDirectionModeId());
    if (plotDirectionIndex >= 0) {
        m_plotDirectionMode->setCurrentIndex(plotDirectionIndex);
    }
    connect(m_plotDirectionMode, &QComboBox::currentIndexChanged, this, [this](int index) {
        m_controller->setPlotDirectionModeId(m_plotDirectionMode->itemData(index).toString());
    });
    m_plotPreviewVisible = new QCheckBox(QStringLiteral("Show plot preview"));
    m_plotPreviewVisible->setObjectName(QStringLiteral("plotPreviewCheckbox"));
    m_plotPreviewVisible->setChecked(false);
    connect(m_plotPreviewVisible, &QCheckBox::toggled, this, [this](bool checked) {
        if (m_canvas != nullptr) {
            m_canvas->setPlotPreviewVisible(checked);
        }
    });
    m_pointerValue = makeValueLabel();
    m_guideDragValue = makeValueLabel();
    m_previewValue = makeValueLabel();
    layout->addWidget(m_snapValue);
    layout->addWidget(m_gridValue);
    layout->addWidget(m_plotValue);
    layout->addWidget(m_plotBoundsValue);
    layout->addWidget(m_plotLayerStatsValue);
    layout->addWidget(m_plotPenStatsValue);
    layout->addWidget(m_plotReadinessValue);
    layout->addWidget(m_plotOrderMode);
    layout->addWidget(m_plotDirectionMode);
    layout->addWidget(m_plotPreviewVisible);
    layout->addWidget(m_pointerValue);
    layout->addWidget(m_guideDragValue);
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

QWidget *EdiShellWindow::buildObjectFlagControls()
{
    auto *panel = new QWidget;
    panel->setObjectName(QStringLiteral("objectFlagControls"));
    auto *layout = new QHBoxLayout(panel);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(10);

    m_selectedLocked = new QCheckBox(QStringLiteral("Locked"));
    m_selectedLocked->setObjectName(QStringLiteral("objectFlagCheckbox"));
    connect(m_selectedLocked, &QCheckBox::toggled, this, [this](bool checked) {
        m_controller->setSelectedObjectLocked(checked);
    });

    m_selectedVisible = new QCheckBox(QStringLiteral("Visible"));
    m_selectedVisible->setObjectName(QStringLiteral("objectFlagCheckbox"));
    connect(m_selectedVisible, &QCheckBox::toggled, this, [this](bool checked) {
        m_controller->setSelectedObjectVisible(checked);
    });

    layout->addWidget(m_selectedLocked);
    layout->addWidget(m_selectedVisible);
    layout->addStretch(1);
    return panel;
}

QWidget *EdiShellWindow::buildLayerControls()
{
    auto *panel = new QWidget;
    panel->setObjectName(QStringLiteral("layerControls"));
    auto *layout = new QVBoxLayout(panel);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(4);

    layout->addWidget(makeSectionLabel(QStringLiteral("Layers")));

    m_activeLayer = new QComboBox;
    m_activeLayer->setObjectName(QStringLiteral("activeLayerCombo"));
    connect(m_activeLayer, &QComboBox::currentIndexChanged, this, [this](int index) {
        if (index < 0) {
            return;
        }
        m_controller->setActiveLayerId(m_activeLayer->itemData(index).toString());
    });
    layout->addWidget(m_activeLayer);

    m_addLayerButton = new QPushButton(QStringLiteral("Add Layer"));
    m_addLayerButton->setObjectName(QStringLiteral("addLayerButton"));
    connect(m_addLayerButton, &QPushButton::clicked, this, [this]() {
        m_controller->createLayer();
    });
    layout->addWidget(m_addLayerButton);

    auto *orderRow = new QWidget;
    auto *orderLayout = new QHBoxLayout(orderRow);
    orderLayout->setContentsMargins(0, 0, 0, 0);
    orderLayout->setSpacing(6);

    m_layerDownButton = new QPushButton(QStringLiteral("Down"));
    m_layerDownButton->setObjectName(QStringLiteral("layerOrderButton"));
    connect(m_layerDownButton, &QPushButton::clicked, this, [this]() {
        m_controller->moveActiveLayer(QStringLiteral("down"));
    });

    m_layerUpButton = new QPushButton(QStringLiteral("Up"));
    m_layerUpButton->setObjectName(QStringLiteral("layerOrderButton"));
    connect(m_layerUpButton, &QPushButton::clicked, this, [this]() {
        m_controller->moveActiveLayer(QStringLiteral("up"));
    });

    orderLayout->addWidget(m_layerDownButton);
    orderLayout->addWidget(m_layerUpButton);
    orderLayout->addStretch(1);
    layout->addWidget(orderRow);

    auto *row = new QWidget;
    auto *rowLayout = new QHBoxLayout(row);
    rowLayout->setContentsMargins(0, 0, 0, 0);
    rowLayout->setSpacing(10);

    m_defaultLayerLocked = new QCheckBox(QStringLiteral("Locked"));
    m_defaultLayerLocked->setObjectName(QStringLiteral("layerFlagCheckbox"));
    connect(m_defaultLayerLocked, &QCheckBox::toggled, this, [this](bool checked) {
        m_controller->setActiveLayerLocked(checked);
    });

    m_defaultLayerVisible = new QCheckBox(QStringLiteral("Visible"));
    m_defaultLayerVisible->setObjectName(QStringLiteral("layerFlagCheckbox"));
    connect(m_defaultLayerVisible, &QCheckBox::toggled, this, [this](bool checked) {
        m_controller->setActiveLayerVisible(checked);
    });

    rowLayout->addWidget(m_defaultLayerLocked);
    rowLayout->addWidget(m_defaultLayerVisible);
    rowLayout->addStretch(1);
    layout->addWidget(row);

    m_activeLayerPlotEnabled = new QCheckBox(QStringLiteral("Plot"));
    m_activeLayerPlotEnabled->setObjectName(QStringLiteral("layerPlotCheckbox"));
    connect(m_activeLayerPlotEnabled, &QCheckBox::toggled, this, [this](bool checked) {
        m_controller->setActiveLayerPlotEnabled(checked);
    });
    layout->addWidget(m_activeLayerPlotEnabled);

    m_activeLayerPen = new QComboBox;
    m_activeLayerPen->setObjectName(QStringLiteral("layerPenCombo"));
    m_activeLayerPen->addItem(QStringLiteral("Black pen"), QStringLiteral("pen_black"));
    m_activeLayerPen->addItem(QStringLiteral("Blue pen"), QStringLiteral("pen_blue"));
    m_activeLayerPen->addItem(QStringLiteral("Red pen"), QStringLiteral("pen_red"));
    connect(m_activeLayerPen, &QComboBox::currentIndexChanged, this, [this](int index) {
        if (index < 0) {
            return;
        }
        m_controller->setActiveLayerPenPreset(m_activeLayerPen->itemData(index).toString());
    });
    layout->addWidget(m_activeLayerPen);

    m_activeLayerStrokeWidth = new QComboBox;
    m_activeLayerStrokeWidth->setObjectName(QStringLiteral("layerStrokeWidthCombo"));
    m_activeLayerStrokeWidth->addItem(QStringLiteral("Fine stroke"), QStringLiteral("fine"));
    m_activeLayerStrokeWidth->addItem(QStringLiteral("Normal stroke"), QStringLiteral("normal"));
    m_activeLayerStrokeWidth->addItem(QStringLiteral("Bold stroke"), QStringLiteral("bold"));
    connect(m_activeLayerStrokeWidth, &QComboBox::currentIndexChanged, this, [this](int index) {
        if (index < 0) {
            return;
        }
        m_controller->setActiveLayerStrokeWidthPreset(m_activeLayerStrokeWidth->itemData(index).toString());
    });
    layout->addWidget(m_activeLayerStrokeWidth);

    m_selectedObjectLayer = new QComboBox;
    m_selectedObjectLayer->setObjectName(QStringLiteral("selectedObjectLayerCombo"));
    connect(m_selectedObjectLayer, &QComboBox::currentIndexChanged, this, [this](int index) {
        if (index < 0) {
            return;
        }
        m_controller->moveSelectedObjectToLayer(m_selectedObjectLayer->itemData(index).toString());
    });
    layout->addWidget(m_selectedObjectLayer);
    return panel;
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
    auto addSafeButton = [this, layout](const QString &label, const QString &direction, int row, int column) {
        auto *button = new QPushButton(label);
        button->setObjectName(QStringLiteral("nudgeButton"));
        connect(button, &QPushButton::clicked, this, [this, direction]() {
            m_controller->nudgeSelectionInsideDrawable(direction, QStringLiteral("grid"));
        });
        layout->addWidget(button, row, column);
    };

    layout->addWidget(makeSectionLabel(QStringLiteral("Nudge")), 0, 0, 1, 4);
    addButton(QStringLiteral("Grid Up"), QStringLiteral("up"), QStringLiteral("grid"), 1, 1);
    addButton(QStringLiteral("Grid Left"), QStringLiteral("left"), QStringLiteral("grid"), 2, 0);
    addButton(QStringLiteral("Grid Right"), QStringLiteral("right"), QStringLiteral("grid"), 2, 2);
    addButton(QStringLiteral("Grid Down"), QStringLiteral("down"), QStringLiteral("grid"), 3, 1);
    addSafeButton(QStringLiteral("Safe Up"), QStringLiteral("up"), 1, 3);
    addSafeButton(QStringLiteral("Safe Left"), QStringLiteral("left"), 2, 3);
    addSafeButton(QStringLiteral("Safe Right"), QStringLiteral("right"), 3, 3);
    addSafeButton(QStringLiteral("Safe Down"), QStringLiteral("down"), 4, 3);
    addButton(QStringLiteral("Fine Up"), QStringLiteral("up"), QStringLiteral("fine"), 4, 1);
    addButton(QStringLiteral("Fine Left"), QStringLiteral("left"), QStringLiteral("fine"), 5, 0);
    addButton(QStringLiteral("Fine Right"), QStringLiteral("right"), QStringLiteral("fine"), 5, 2);
    addButton(QStringLiteral("Fine Down"), QStringLiteral("down"), QStringLiteral("fine"), 6, 1);

    return panel;
}

QWidget *EdiShellWindow::buildAlignControls()
{
    auto *panel = new QWidget;
    panel->setObjectName(QStringLiteral("alignControls"));
    auto *layout = new QGridLayout(panel);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setHorizontalSpacing(6);
    layout->setVerticalSpacing(6);

    auto addAlignButton = [this, layout](const QString &label, const QString &modeId, int row, int column) {
        auto *button = new QPushButton(label);
        button->setObjectName(QStringLiteral("alignButton"));
        connect(button, &QPushButton::clicked, this, [this, modeId]() {
            m_controller->alignSelection(modeId);
        });
        layout->addWidget(button, row, column);
    };
    auto addDistributeButton = [this, layout](const QString &label, const QString &axisId, int row, int column) {
        auto *button = new QPushButton(label);
        button->setObjectName(QStringLiteral("distributeButton"));
        connect(button, &QPushButton::clicked, this, [this, axisId]() {
            m_controller->distributeSelection(axisId);
        });
        layout->addWidget(button, row, column);
    };

    layout->addWidget(makeSectionLabel(QStringLiteral("Align")), 0, 0, 1, 3);
    addAlignButton(QStringLiteral("Left"), QStringLiteral("left"), 1, 0);
    addAlignButton(QStringLiteral("Center X"), QStringLiteral("center_x"), 1, 1);
    addAlignButton(QStringLiteral("Right"), QStringLiteral("right"), 1, 2);
    addAlignButton(QStringLiteral("Top"), QStringLiteral("top"), 2, 0);
    addAlignButton(QStringLiteral("Center Y"), QStringLiteral("center_y"), 2, 1);
    addAlignButton(QStringLiteral("Bottom"), QStringLiteral("bottom"), 2, 2);
    addDistributeButton(QStringLiteral("Distribute X"), QStringLiteral("x"), 3, 0);
    addDistributeButton(QStringLiteral("Distribute Y"), QStringLiteral("y"), 3, 1);

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

QWidget *EdiShellWindow::buildCalibrationControls()
{
    auto *panel = new QWidget;
    panel->setObjectName(QStringLiteral("calibrationControls"));
    auto *layout = new QGridLayout(panel);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setHorizontalSpacing(6);
    layout->setVerticalSpacing(6);

    auto addButton = [this, layout](const QString &label, const QString &patternId, int row, int column) {
        auto *button = new QPushButton(label);
        button->setObjectName(QStringLiteral("calibrationButton"));
        connect(button, &QPushButton::clicked, this, [this, patternId]() {
            m_controller->createCalibrationPattern(patternId);
        });
        layout->addWidget(button, row, column);
    };

    layout->addWidget(makeSectionLabel(QStringLiteral("Calibration")), 0, 0, 1, 3);
    addButton(QStringLiteral("Test square"), QStringLiteral("test_square"), 1, 0);
    addButton(QStringLiteral("Test circle"), QStringLiteral("test_circle"), 1, 1);
    addButton(QStringLiteral("Line spacing"), QStringLiteral("line_spacing"), 1, 2);

    auto *measuredLabel = new QLabel(QStringLiteral("Measured"));
    measuredLabel->setObjectName(QStringLiteral("fieldLabel"));
    layout->addWidget(measuredLabel, 2, 0);

    m_calibrationMeasuredValue = new QDoubleSpinBox;
    m_calibrationMeasuredValue->setObjectName(QStringLiteral("geometryField"));
    m_calibrationMeasuredValue->setRange(0.000001, 1000000.0);
    m_calibrationMeasuredValue->setDecimals(6);
    m_calibrationMeasuredValue->setSingleStep(0.001);
    m_calibrationMeasuredValue->setValue(0.24);
    layout->addWidget(m_calibrationMeasuredValue, 2, 1);

    auto *record = new QPushButton(QStringLiteral("Record"));
    record->setObjectName(QStringLiteral("calibrationButton"));
    connect(record, &QPushButton::clicked, this, [this]() {
        if (m_calibrationMeasuredValue != nullptr) {
            m_controller->recordCalibrationMeasurement(m_calibrationMeasuredValue->value());
        }
    });
    layout->addWidget(record, 2, 2);

    auto *applyScale = new QPushButton(QStringLiteral("Apply scale"));
    applyScale->setObjectName(QStringLiteral("calibrationButton"));
    connect(applyScale, &QPushButton::clicked, this, [this]() {
        m_controller->applyCalibrationCorrection();
    });
    layout->addWidget(applyScale, 3, 0, 1, 3);

    m_calibrationMeasurementValue = makeValueLabel(QStringLiteral("Calibration measurement: none"));
    layout->addWidget(m_calibrationMeasurementValue, 4, 0, 1, 3);

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

    const QVariantList fields = selectedObject.value(QStringLiteral("numeric_fields")).toList();
    const QVariantMap physicalGeometry = selectedObject.value(QStringLiteral("physical_geometry")).toMap();
    const QString unitLabel = physicalGeometry.value(QStringLiteral("unit_label")).toString();

    int row = 0;
    for (const QVariant &fieldValue : fields) {
        const QVariantMap field = fieldValue.toMap();
        const QString fieldId = field.value(QStringLiteral("id")).toString();
        if (fieldId.isEmpty() || !selectedObject.contains(fieldId)) {
            continue;
        }

        auto *label = new QLabel(field.value(QStringLiteral("label")).toString());
        label->setObjectName(QStringLiteral("fieldLabel"));
        auto *spin = new QDoubleSpinBox;
        spin->setObjectName(QStringLiteral("geometryField"));
        spin->setDecimals(field.value(QStringLiteral("decimals"), 4).toInt());
        spin->setSingleStep(field.value(QStringLiteral("step"), 0.01).toDouble());
        spin->setRange(
            field.value(QStringLiteral("minimum"), -10.0).toDouble(),
            field.value(QStringLiteral("maximum"), 10.0).toDouble());
        spin->setValue(selectedObject.value(fieldId).toDouble());
        spin->setProperty("fieldId", fieldId);
        connect(spin, &QDoubleSpinBox::editingFinished, this, [this, spin]() {
            if (!m_controller->updateSelectedObjectGeometryField(spin->property("fieldId").toString(), spin->value())) {
                refreshInspector();
            }
        });
        layout->addWidget(label, row, 0);
        layout->addWidget(spin, row, 1);
        if (physicalGeometry.contains(fieldId)) {
            const bool angleValue = fieldId == QStringLiteral("line_angle_deg")
                || fieldId == QStringLiteral("dimension_angle_deg")
                || fieldId == QStringLiteral("rotation_deg");
            auto *physicalSpin = new QDoubleSpinBox;
            physicalSpin->setObjectName(QStringLiteral("geometryField"));
            physicalSpin->setDecimals(angleValue ? 2 : field.value(QStringLiteral("decimals"), 4).toInt());
            physicalSpin->setSingleStep(angleValue ? 1.0 : field.value(QStringLiteral("step"), 0.01).toDouble());
            if (fieldId == QStringLiteral("width")
                || fieldId == QStringLiteral("height")
                || fieldId == QStringLiteral("radius")
                || fieldId == QStringLiteral("diameter")
                || fieldId == QStringLiteral("line_length")
                || fieldId == QStringLiteral("dimension_length")) {
                physicalSpin->setRange(0.0, 100000.0);
            } else if (angleValue) {
                physicalSpin->setRange(-360.0, 360.0);
            } else {
                physicalSpin->setRange(-100000.0, 100000.0);
            }
            physicalSpin->setValue(physicalGeometry.value(fieldId).toDouble());
            physicalSpin->setProperty("fieldId", fieldId);
            connect(physicalSpin, &QDoubleSpinBox::editingFinished, this, [this, physicalSpin]() {
                if (!m_controller->updateSelectedObjectPhysicalGeometryField(physicalSpin->property("fieldId").toString(), physicalSpin->value())) {
                    refreshInspector();
                }
            });
            auto *physicalLabel = new QLabel(angleValue ? QStringLiteral("deg") : unitLabel);
            physicalLabel->setObjectName(QStringLiteral("valueLabel"));
            layout->addWidget(physicalSpin, row, 2);
            layout->addWidget(physicalLabel, row, 3);
        }
        m_geometryFields.insert(fieldId, spin);
        ++row;
    }

    setGeometryEditorVisible(row > 0);
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
    const QVariantMap plot = document.value(QStringLiteral("plot_summary")).toMap();
    const QVariantMap pointer = document.value(QStringLiteral("pointer")).toMap();
    const QVariantMap guideDragSnap = document.value(QStringLiteral("guide_drag_snap")).toMap();
    const QVariantMap calibrationMeasurement = document.value(QStringLiteral("calibration_measurement")).toMap();
    const QVariantMap calibrationCorrection = document.value(QStringLiteral("calibration_correction")).toMap();
    const QVariantMap selectedObject = activeObjectProjection(document);
    const QVariantList layers = document.value(QStringLiteral("layers")).toList();
    const QString activeLayerId = document.value(QStringLiteral("active_layer_id")).toString();
    const QVariantMap activeLayer = layerProjection(document, activeLayerId);
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
            : QStringLiteral("Layer: %1   Obj L/V: %2/%3   Effective L/V: %4/%5   Pen: %6")
                .arg(selectedObject.value(QStringLiteral("layer_id")).toString())
                .arg(yesNo(selectedObject.value(QStringLiteral("locked")).toBool()))
                .arg(yesNo(selectedObject.value(QStringLiteral("visible")).toBool()))
                .arg(yesNo(selectedObject.value(QStringLiteral("effective_locked")).toBool()))
                .arg(yesNo(selectedObject.value(QStringLiteral("effective_visible")).toBool()))
                .arg(selectedObject.value(QStringLiteral("effective_pen_id")).toString()));
    }
    if (m_selectedLocked != nullptr) {
        const QSignalBlocker blocker(m_selectedLocked);
        m_selectedLocked->setEnabled(!selectedObject.isEmpty());
        m_selectedLocked->setChecked(selectedObject.value(QStringLiteral("locked")).toBool());
    }
    if (m_selectedVisible != nullptr) {
        const QSignalBlocker blocker(m_selectedVisible);
        m_selectedVisible->setEnabled(!selectedObject.isEmpty());
        m_selectedVisible->setChecked(selectedObject.isEmpty() ? false : selectedObject.value(QStringLiteral("visible")).toBool());
    }
    if (m_defaultLayerLocked != nullptr) {
        const QSignalBlocker blocker(m_defaultLayerLocked);
        m_defaultLayerLocked->setEnabled(!activeLayer.isEmpty());
        m_defaultLayerLocked->setChecked(activeLayer.value(QStringLiteral("locked")).toBool());
    }
    if (m_defaultLayerVisible != nullptr) {
        const QSignalBlocker blocker(m_defaultLayerVisible);
        m_defaultLayerVisible->setEnabled(!activeLayer.isEmpty());
        m_defaultLayerVisible->setChecked(activeLayer.isEmpty() ? false : activeLayer.value(QStringLiteral("visible")).toBool());
    }
    if (m_layerDownButton != nullptr) {
        m_layerDownButton->setEnabled(!activeLayer.isEmpty() && activeLayer.value(QStringLiteral("order")).toInt() > 0);
    }
    if (m_layerUpButton != nullptr) {
        m_layerUpButton->setEnabled(!activeLayer.isEmpty() && activeLayer.value(QStringLiteral("order")).toInt() + 1 < layers.size());
    }
    if (m_activeLayerPlotEnabled != nullptr) {
        const QSignalBlocker blocker(m_activeLayerPlotEnabled);
        m_activeLayerPlotEnabled->setEnabled(!activeLayer.isEmpty());
        m_activeLayerPlotEnabled->setChecked(activeLayer.value(QStringLiteral("plot_enabled")).toBool());
    }
    if (m_activeLayerPen != nullptr) {
        const QSignalBlocker blocker(m_activeLayerPen);
        m_activeLayerPen->setEnabled(!activeLayer.isEmpty());
        const int index = m_activeLayerPen->findData(activeLayer.value(QStringLiteral("pen_id")).toString());
        m_activeLayerPen->setCurrentIndex(index >= 0 ? index : 0);
    }
    if (m_activeLayerStrokeWidth != nullptr) {
        const QSignalBlocker blocker(m_activeLayerStrokeWidth);
        m_activeLayerStrokeWidth->setEnabled(!activeLayer.isEmpty());
        const int index = m_activeLayerStrokeWidth->findData(strokeWidthPresetId(activeLayer.value(QStringLiteral("stroke_width")).toDouble()));
        m_activeLayerStrokeWidth->setCurrentIndex(index >= 0 ? index : 1);
    }
    refreshLayerCombo(m_activeLayer, layers, activeLayerId, true);
    refreshLayerCombo(
        m_selectedObjectLayer,
        layers,
        selectedObject.value(QStringLiteral("layer_id")).toString(),
        !selectedObject.isEmpty());
    if (m_gridPreset != nullptr) {
        const QSignalBlocker blocker(m_gridPreset);
        const int index = m_gridPreset->findData(grid.value(QStringLiteral("preset")).toString());
        m_gridPreset->setCurrentIndex(index >= 0 ? index : 0);
    }
    if (m_gridUnit != nullptr) {
        const QSignalBlocker blocker(m_gridUnit);
        const int index = m_gridUnit->findData(grid.value(QStringLiteral("unit")).toString());
        m_gridUnit->setCurrentIndex(index >= 0 ? index : 0);
    }
    if (m_gridWidth != nullptr) {
        const QSignalBlocker blocker(m_gridWidth);
        m_gridWidth->setValue(grid.value(QStringLiteral("width")).toDouble());
    }
    if (m_gridHeight != nullptr) {
        const QSignalBlocker blocker(m_gridHeight);
        m_gridHeight->setValue(grid.value(QStringLiteral("height")).toDouble());
    }
    if (m_gridMarginLeft != nullptr) {
        const QSignalBlocker blocker(m_gridMarginLeft);
        m_gridMarginLeft->setValue(grid.value(QStringLiteral("margin_left")).toDouble());
    }
    if (m_gridMarginTop != nullptr) {
        const QSignalBlocker blocker(m_gridMarginTop);
        m_gridMarginTop->setValue(grid.value(QStringLiteral("margin_top")).toDouble());
    }
    if (m_gridMarginRight != nullptr) {
        const QSignalBlocker blocker(m_gridMarginRight);
        m_gridMarginRight->setValue(grid.value(QStringLiteral("margin_right")).toDouble());
    }
    if (m_gridMarginBottom != nullptr) {
        const QSignalBlocker blocker(m_gridMarginBottom);
        m_gridMarginBottom->setValue(grid.value(QStringLiteral("margin_bottom")).toDouble());
    }
    if (m_gridMinorStep != nullptr) {
        const QSignalBlocker blocker(m_gridMinorStep);
        m_gridMinorStep->setValue(grid.value(QStringLiteral("minor_step")).toDouble());
    }
    if (m_gridMajorEvery != nullptr) {
        const QSignalBlocker blocker(m_gridMajorEvery);
        m_gridMajorEvery->setValue(grid.value(QStringLiteral("major_line_every")).toInt());
    }
    if (m_gridVisible != nullptr) {
        const QSignalBlocker blocker(m_gridVisible);
        m_gridVisible->setChecked(grid.value(QStringLiteral("visible")).toBool());
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
    if (m_objectPlotSafetyValue != nullptr) {
        m_objectPlotSafetyValue->setText(selectedPlotSafetySummary(selectedObject, plot));
    }
    if (m_selectionPlotBoundsValue != nullptr) {
        m_selectionPlotBoundsValue->setText(selectionPlotBoundsSummary(document));
    }
    if (m_fitSelectionToDrawableButton != nullptr) {
        m_fitSelectionToDrawableButton->setEnabled(!selectedObject.isEmpty());
    }
    if (m_centerSelectionInDrawableButton != nullptr) {
        m_centerSelectionInDrawableButton->setEnabled(!selectedObject.isEmpty());
    }
    if (m_moveSelectionToDrawableOriginButton != nullptr) {
        m_moveSelectionToDrawableOriginButton->setEnabled(!selectedObject.isEmpty());
    }
    if (m_moveGuideToDrawableOriginButton != nullptr) {
        m_moveGuideToDrawableOriginButton->setEnabled(selectedObject.value(QStringLiteral("guide_drawable_controls")).toBool());
    }
    if (m_centerGuideInDrawableButton != nullptr) {
        m_centerGuideInDrawableButton->setEnabled(selectedObject.value(QStringLiteral("guide_drawable_controls")).toBool());
    }
    if (m_moveGuideToDrawableMaxButton != nullptr) {
        m_moveGuideToDrawableMaxButton->setEnabled(selectedObject.value(QStringLiteral("guide_drawable_controls")).toBool());
    }
    if (m_fitConstructionToDrawableButton != nullptr) {
        m_fitConstructionToDrawableButton->setEnabled(selectedObject.value(QStringLiteral("construction_drawable_controls")).toBool());
    }
    const bool boundsGuideControlsEnabled = selectedObject.value(QStringLiteral("bounds_guide_controls")).toBool();
    for (QPushButton *button : std::as_const(m_boundsGuideButtons)) {
        button->setEnabled(boundsGuideControlsEnabled);
    }
    for (QPushButton *button : std::as_const(m_offsetGuideButtons)) {
        button->setEnabled(boundsGuideControlsEnabled);
    }
    const bool alignToGuideControlsEnabled = selectedObject.value(QStringLiteral("align_to_guide_controls")).toBool();
    for (QPushButton *button : std::as_const(m_alignToGuideButtons)) {
        button->setEnabled(alignToGuideControlsEnabled);
    }
    const bool selectedGuideControlsEnabled = selectedObject.value(QStringLiteral("guide_drawable_controls")).toBool();
    if (m_deleteSelectedGuideButton != nullptr) {
        m_deleteSelectedGuideButton->setEnabled(selectedGuideControlsEnabled);
    }
    for (QPushButton *button : std::as_const(m_guideOffsetButtons)) {
        button->setEnabled(selectedGuideControlsEnabled);
    }
    const bool guideVisualControlsEnabled = selectedObject.value(QStringLiteral("guide_visual_controls")).toBool();
    if (m_guideLabel != nullptr) {
        const QSignalBlocker blocker(m_guideLabel);
        m_guideLabel->setEnabled(guideVisualControlsEnabled);
        m_guideLabel->setText(guideVisualControlsEnabled ? selectedObject.value(QStringLiteral("guide_custom_label")).toString() : QString());
    }
    if (m_guideColor != nullptr) {
        const QSignalBlocker blocker(m_guideColor);
        m_guideColor->setEnabled(guideVisualControlsEnabled);
        const int index = m_guideColor->findData(selectedObject.value(QStringLiteral("guide_color")).toString());
        m_guideColor->setCurrentIndex(index >= 0 ? index : 0);
    }
    if (m_guideDashStyle != nullptr) {
        const QSignalBlocker blocker(m_guideDashStyle);
        m_guideDashStyle->setEnabled(guideVisualControlsEnabled);
        const int index = m_guideDashStyle->findData(selectedObject.value(QStringLiteral("guide_dash_style")).toString());
        m_guideDashStyle->setCurrentIndex(index >= 0 ? index : 0);
    }
    if (m_guideShowLabel != nullptr) {
        const QSignalBlocker blocker(m_guideShowLabel);
        m_guideShowLabel->setEnabled(guideVisualControlsEnabled);
        m_guideShowLabel->setChecked(selectedObject.value(QStringLiteral("guide_show_label"), true).toBool());
    }
    const bool dimensionControlsEnabled = selectedObject.value(QStringLiteral("dimension_visual_controls")).toBool();
    if (m_dimensionReadout != nullptr) {
        const QVariantMap physical = selectedObject.value(QStringLiteral("physical_geometry")).toMap();
        m_dimensionReadout->setText(dimensionControlsEnabled
                ? QStringLiteral("Dimension: %1   Physical: %2   Kind: %3")
                    .arg(selectedObject.value(QStringLiteral("label")).toString(),
                        physical.value(QStringLiteral("dimension_label")).toString(),
                        selectedObject.value(QStringLiteral("dimension_kind")).toString())
                : QStringLiteral("Dimension: none"));
    }
    if (m_dimensionKind != nullptr) {
        const QSignalBlocker blocker(m_dimensionKind);
        const QString dimensionKind = selectedObject.value(QStringLiteral("dimension_kind")).toString();
        const int index = m_dimensionKind->findData(dimensionKind);
        m_dimensionKind->setEnabled(dimensionControlsEnabled);
        m_dimensionKind->setCurrentIndex(index >= 0 ? index : 0);
    }
    if (m_dimensionShowLabel != nullptr) {
        const QSignalBlocker blocker(m_dimensionShowLabel);
        m_dimensionShowLabel->setEnabled(dimensionControlsEnabled);
        m_dimensionShowLabel->setChecked(selectedObject.value(QStringLiteral("dimension_show_label"), true).toBool());
    }
    rebuildGeometryEditor(selectedObject);
    if (m_objectsValue != nullptr) {
        m_objectsValue->setText(QStringLiteral("Objects: %1").arg(objects.size()));
    }
    if (m_guidesValue != nullptr) {
        m_guidesValue->setText(QStringLiteral("Guides: %1 visible / %2 total / %3 duplicate")
            .arg(document.value(QStringLiteral("visible_guide_count")).toInt())
            .arg(document.value(QStringLiteral("guide_count")).toInt())
            .arg(document.value(QStringLiteral("duplicate_guide_count")).toInt()));
    }
    if (m_revisionValue != nullptr) {
        m_revisionValue->setText(QStringLiteral("Revision: %1").arg(document.value(QStringLiteral("revision")).toInt()));
    }
    if (m_snapValue != nullptr) {
        m_snapValue->setText(QStringLiteral("Snap grid: %1   Object: %2   Guide: %3   Move: %4   Priority: %5")
            .arg(yesNo(snap.value(QStringLiteral("grid_enabled")).toBool()))
            .arg(yesNo(snap.value(QStringLiteral("object_enabled")).toBool()))
            .arg(yesNo(snap.value(QStringLiteral("guide_enabled")).toBool()))
            .arg(yesNo(snap.value(QStringLiteral("guide_move_enabled")).toBool()))
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
    if (m_plotValue != nullptr) {
        const bool blocked = plot.value(QStringLiteral("blocked")).toBool();
        const int travelSegmentCount = plot.value(QStringLiteral("travel_segment_count")).toInt();
        const QString travelDistance = formatNumber(plot.value(QStringLiteral("travel_distance")).toDouble());
        m_plotValue->setText(blocked
            ? QStringLiteral("Plot: %1 objects, %2 strokes, %3 travel (%4), %5 warnings, first %6")
                .arg(plot.value(QStringLiteral("plot_object_count")).toInt())
                .arg(plot.value(QStringLiteral("segment_count")).toInt())
                .arg(travelSegmentCount)
                .arg(travelDistance)
                .arg(plot.value(QStringLiteral("warning_count")).toInt())
                .arg(plot.value(QStringLiteral("first_warning_object_id")).toString())
            : QStringLiteral("Plot: %1 objects, %2 strokes, %3 travel (%4), ready")
                .arg(plot.value(QStringLiteral("plot_object_count")).toInt())
                .arg(plot.value(QStringLiteral("segment_count")).toInt())
                .arg(travelSegmentCount)
                .arg(travelDistance));
    }
    if (m_plotBoundsValue != nullptr) {
        m_plotBoundsValue->setText(plotBoundsSummary(grid, plot));
    }
    if (m_plotLayerStatsValue != nullptr) {
        QVariantMap activeLayerStats;
        const QVariantList layerStats = plot.value(QStringLiteral("layer_stats")).toList();
        for (const QVariant &statsValue : layerStats) {
            const QVariantMap stats = statsValue.toMap();
            if (stats.value(QStringLiteral("layer_id")).toString() == activeLayerId) {
                activeLayerStats = stats;
                break;
            }
        }
        m_plotLayerStatsValue->setText(activeLayerStats.isEmpty()
            ? QStringLiteral("Layer plot: none")
            : QStringLiteral("Layer plot: %1, %2 obj, %3 strokes, draw %4, travel %5")
                .arg(activeLayerStats.value(QStringLiteral("ready")).toBool()
                    ? QStringLiteral("ready")
                    : activeLayerStats.value(QStringLiteral("blocked_reason")).toString())
                .arg(activeLayerStats.value(QStringLiteral("object_count")).toInt())
                .arg(activeLayerStats.value(QStringLiteral("segment_count")).toInt())
                .arg(formatNumber(activeLayerStats.value(QStringLiteral("stroke_distance")).toDouble()))
                .arg(formatNumber(activeLayerStats.value(QStringLiteral("travel_distance")).toDouble())));
    }
    if (m_plotPenStatsValue != nullptr) {
        QVariantMap activePenStats;
        const QString activePenId = activeLayer.value(QStringLiteral("pen_id")).toString();
        const QVariantList penStats = plot.value(QStringLiteral("pen_stats")).toList();
        for (const QVariant &statsValue : penStats) {
            const QVariantMap stats = statsValue.toMap();
            if (stats.value(QStringLiteral("pen_id")).toString() == activePenId) {
                activePenStats = stats;
                break;
            }
        }
        m_plotPenStatsValue->setText(activePenStats.isEmpty()
            ? QStringLiteral("Pen plot: none")
            : QStringLiteral("Pen plot: %1, %2, %3 obj, %4 strokes, draw %5, travel %6")
                .arg(activePenStats.value(QStringLiteral("pen_id")).toString())
                .arg(activePenStats.value(QStringLiteral("ready")).toBool()
                    ? QStringLiteral("ready")
                    : activePenStats.value(QStringLiteral("blocked_reason")).toString())
                .arg(activePenStats.value(QStringLiteral("object_count")).toInt())
                .arg(activePenStats.value(QStringLiteral("segment_count")).toInt())
                .arg(formatNumber(activePenStats.value(QStringLiteral("stroke_distance")).toDouble()))
                .arg(formatNumber(activePenStats.value(QStringLiteral("travel_distance")).toDouble())));
    }
    if (m_plotReadinessValue != nullptr) {
        m_plotReadinessValue->setText(formatPlotReadinessChecklist(
            plot.value(QStringLiteral("layer_stats")).toList(),
            plot.value(QStringLiteral("pen_stats")).toList()));
    }
    if (m_calibrationMeasurementValue != nullptr) {
        m_calibrationMeasurementValue->setText(calibrationMeasurement.isEmpty()
            ? QStringLiteral("Calibration measurement: none")
            : QStringLiteral("Calibration: %1 expected %2 measured %3 error %4 (%5%)\nApply scale: %6   active scale: %7")
                .arg(calibrationMeasurement.value(QStringLiteral("pattern_id")).toString())
                .arg(formatNumber(calibrationMeasurement.value(QStringLiteral("expected_value")).toDouble()))
                .arg(formatNumber(calibrationMeasurement.value(QStringLiteral("measured_value")).toDouble()))
                .arg(formatNumber(calibrationMeasurement.value(QStringLiteral("error_value")).toDouble()))
                .arg(formatNumber(calibrationMeasurement.value(QStringLiteral("percent_error")).toDouble()))
                .arg(calibrationCorrection.isEmpty()
                    ? QStringLiteral("none")
                    : formatNumber(calibrationCorrection.value(QStringLiteral("scale_factor")).toDouble()))
                .arg(formatNumber(plot.value(QStringLiteral("calibration_scale")).toDouble())));
    }
    if (m_plotOrderMode != nullptr) {
        const QSignalBlocker blocker(m_plotOrderMode);
        const int index = m_plotOrderMode->findData(plot.value(QStringLiteral("order_mode")).toString());
        m_plotOrderMode->setCurrentIndex(index >= 0 ? index : 0);
    }
    if (m_plotDirectionMode != nullptr) {
        const QSignalBlocker blocker(m_plotDirectionMode);
        const int index = m_plotDirectionMode->findData(plot.value(QStringLiteral("direction_mode")).toString());
        m_plotDirectionMode->setCurrentIndex(index >= 0 ? index : 0);
    }
    if (m_pointerValue != nullptr) {
        if (pointer.isEmpty()) {
            m_pointerValue->setText(QStringLiteral("Pointer: none"));
        } else {
            const QVariantMap raw = pointer.value(QStringLiteral("raw")).toMap();
            const QVariantMap snapped = pointer.value(QStringLiteral("snapped")).toMap();
            const QString sourceObjectId = pointer.value(QStringLiteral("source_object_id")).toString();
            m_pointerValue->setText(QStringLiteral("Pointer: raw %1,%2  snap %3,%4 %5/%6  source %7  unit %8,%9 %10  %11")
                .arg(formatNumber(raw.value(QStringLiteral("x")).toDouble()))
                .arg(formatNumber(raw.value(QStringLiteral("y")).toDouble()))
                .arg(formatNumber(snapped.value(QStringLiteral("x")).toDouble()))
                .arg(formatNumber(snapped.value(QStringLiteral("y")).toDouble()))
                .arg(pointer.value(QStringLiteral("kind")).toString())
                .arg(pointer.value(QStringLiteral("source")).toString())
                .arg(sourceObjectId.isEmpty() ? QStringLiteral("none") : sourceObjectId)
                .arg(formatNumber(pointer.value(QStringLiteral("snapped_unit_x")).toDouble()))
                .arg(formatNumber(pointer.value(QStringLiteral("snapped_unit_y")).toDouble()))
                .arg(pointer.value(QStringLiteral("unit_label")).toString())
                .arg(pointer.value(QStringLiteral("inside_drawable")).toBool() ? QStringLiteral("inside") : QStringLiteral("outside")));
        }
    }
    if (m_guideDragValue != nullptr) {
        if (guideDragSnap.isEmpty()) {
            m_guideDragValue->setText(QStringLiteral("Guide drag: none"));
        } else {
            const QString sourceObjectId = guideDragSnap.value(QStringLiteral("source_object_id")).toString();
            m_guideDragValue->setText(QStringLiteral("Guide drag: %1 -> %2 %3 dist %4")
                .arg(guideDragSnap.value(QStringLiteral("anchor_label")).toString())
                .arg(sourceObjectId.isEmpty() ? QStringLiteral("guide") : sourceObjectId)
                .arg(guideDragSnap.value(QStringLiteral("intersection")).toBool() ? QStringLiteral("intersection") : QStringLiteral("axis"))
                .arg(formatNumber(guideDragSnap.value(QStringLiteral("distance")).toDouble())));
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
