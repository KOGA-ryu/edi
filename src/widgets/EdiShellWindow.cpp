#include "widgets/EdiShellWindow.h"

#include <QAbstractButton>
#include <QButtonGroup>
#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QFrame>
#include <QScrollArea>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPair>
#include <QPushButton>
#include <QCloseEvent>
#include <QShortcut>
#include <QSignalBlocker>
#include <QTimer>
#include <QSizePolicy>
#include <QSpinBox>
#include <QStyle>
#include <QStringList>
#include <QVariantList>
#include <QVariantMap>
#include <QVBoxLayout>
#include <QVector>

#include <optional>
#include <utility>

#include "core/DrawingCore.h"
#include "widgets/DrawingCanvasWidget.h"
#include "widgets/ShellWidgetHelpers.h"

using namespace edi::shell;

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

    auto *saveShortcut = new QShortcut(QKeySequence::Save, this);
    connect(saveShortcut, &QShortcut::activated, this, &EdiShellWindow::promptSaveDrawing);
    auto *openShortcut = new QShortcut(QKeySequence::Open, this);
    connect(openShortcut, &QShortcut::activated, this, &EdiShellWindow::promptOpenDrawing);
    auto *saveAsShortcut = new QShortcut(QKeySequence::SaveAs, this);
    connect(saveAsShortcut, &QShortcut::activated, this, &EdiShellWindow::promptSaveDrawingAs);
    auto *undoShortcut = new QShortcut(QKeySequence::Undo, this);
    connect(undoShortcut, &QShortcut::activated, m_controller, [this]() { m_controller->undo(); });
    auto *redoShortcut = new QShortcut(QKeySequence::Redo, this);
    connect(redoShortcut, &QShortcut::activated, m_controller, [this]() { m_controller->redo(); });

    // Debounced settings persistence: any model change (which includes
    // settings-affecting actions) schedules a save to the active settings file.
    m_settingsSaveTimer = new QTimer(this);
    m_settingsSaveTimer->setSingleShot(true);
    m_settingsSaveTimer->setInterval(250);
    connect(m_settingsSaveTimer, &QTimer::timeout, this, [this]() {
        if (!m_settingsPath.isEmpty()) {
            saveSettings(m_settingsPath);
        }
    });
    connect(m_controller, &DrawingDocumentController::modelChanged, this, &EdiShellWindow::scheduleSettingsSave);

    updateWindowTitle();
    refreshInspector();
}

void EdiShellWindow::closeEvent(QCloseEvent *event)
{
    if (!m_settingsPath.isEmpty()) {
        saveSettings(m_settingsPath); // flush settings immediately on close
    }
    QMainWindow::closeEvent(event);
}

bool EdiShellWindow::isDocumentDirty() const
{
    return m_controller->isDocumentDirty();
}

void EdiShellWindow::updateWindowTitle()
{
    const QString name = m_currentDrawingPath.isEmpty()
        ? QStringLiteral("Untitled")
        : QFileInfo(m_currentDrawingPath).fileName();
    const QString dirty = isDocumentDirty() ? QStringLiteral(" •") : QString();
    setWindowTitle(QStringLiteral("EDI — %1%2").arg(name, dirty));
}

bool EdiShellWindow::saveDrawingToPath(const QString &path)
{
    if (path.isEmpty()) {
        return false;
    }
    if (!m_controller->saveDocument(QUrl::fromLocalFile(path))) {
        return false;
    }
    m_currentDrawingPath = path;
    rememberRecentFile(path);
    updateWindowTitle();
    return true;
}

bool EdiShellWindow::openDrawingFromPath(const QString &path)
{
    if (path.isEmpty()) {
        return false;
    }
    if (!m_controller->openDocument(QUrl::fromLocalFile(path))) {
        return false;
    }
    m_currentDrawingPath = path;
    rememberRecentFile(path);
    updateWindowTitle();
    return true;
}

void EdiShellWindow::promptSaveDrawing()
{
    if (m_currentDrawingPath.isEmpty()) {
        promptSaveDrawingAs();
        return;
    }
    saveDrawingToPath(m_currentDrawingPath);
}

void EdiShellWindow::promptSaveDrawingAs()
{
    QString path = QFileDialog::getSaveFileName(
        this, QStringLiteral("Save Drawing"), m_currentDrawingPath,
        QStringLiteral("EDI Drawings (*.edidraw)"));
    if (path.isEmpty()) {
        return;
    }
    if (!path.endsWith(QStringLiteral(".edidraw"))) {
        path += QStringLiteral(".edidraw");
    }
    saveDrawingToPath(path);
}

void EdiShellWindow::promptOpenDrawing()
{
    const QString path = QFileDialog::getOpenFileName(
        this, QStringLiteral("Open Drawing"), QString(),
        QStringLiteral("EDI Drawings (*.edidraw)"));
    if (path.isEmpty()) {
        return;
    }
    openDrawingFromPath(path);
}

bool EdiShellWindow::exportSvgToPath(const QString &path)
{
    return !path.isEmpty() && m_controller->exportSvgDocument(QUrl::fromLocalFile(path));
}

bool EdiShellWindow::exportHpglToPath(const QString &path)
{
    return !path.isEmpty() && m_controller->exportHpglDocument(QUrl::fromLocalFile(path));
}

void EdiShellWindow::promptExportSvg()
{
    QString path = QFileDialog::getSaveFileName(
        this, QStringLiteral("Export SVG"), QString(), QStringLiteral("SVG (*.svg)"));
    if (path.isEmpty()) {
        return;
    }
    if (!path.endsWith(QStringLiteral(".svg"))) {
        path += QStringLiteral(".svg");
    }
    exportSvgToPath(path);
}

void EdiShellWindow::promptExportHpgl()
{
    QString path = QFileDialog::getSaveFileName(
        this, QStringLiteral("Export HPGL"), QString(), QStringLiteral("HPGL (*.hpgl)"));
    if (path.isEmpty()) {
        return;
    }
    if (!path.endsWith(QStringLiteral(".hpgl"))) {
        path += QStringLiteral(".hpgl");
    }
    exportHpglToPath(path);
}

edi::formats::StaticConfig EdiShellWindow::captureSettings() const
{
    using namespace edi::io;
    edi::formats::StaticConfig config;

    const QVariantMap grid = m_controller->modelDocument().value(QStringLiteral("grid")).toMap();
    setSettingsString(config, "grid.preset", grid.value(QStringLiteral("preset")).toString().toStdString());
    setSettingsString(config, "grid.unit", grid.value(QStringLiteral("unit")).toString().toStdString());
    setSettingsDouble(config, "grid.width", grid.value(QStringLiteral("width")).toDouble());
    setSettingsDouble(config, "grid.height", grid.value(QStringLiteral("height")).toDouble());
    setSettingsDouble(config, "grid.margin_left", grid.value(QStringLiteral("margin_left")).toDouble());
    setSettingsDouble(config, "grid.margin_top", grid.value(QStringLiteral("margin_top")).toDouble());
    setSettingsDouble(config, "grid.margin_right", grid.value(QStringLiteral("margin_right")).toDouble());
    setSettingsDouble(config, "grid.margin_bottom", grid.value(QStringLiteral("margin_bottom")).toDouble());
    setSettingsDouble(config, "grid.minor_step", grid.value(QStringLiteral("minor_step")).toDouble());
    setSettingsInt(config, "grid.major_line_every", grid.value(QStringLiteral("major_line_every")).toInt());
    setSettingsBool(config, "grid.visible", grid.value(QStringLiteral("visible")).toBool());

    setSettingsBool(config, "snap.grid_enabled", m_controller->gridSnapEnabled());
    setSettingsBool(config, "snap.object_enabled", m_controller->objectSnapEnabled());
    setSettingsBool(config, "snap.endpoint_enabled", m_controller->endpointSnapEnabled());
    setSettingsBool(config, "snap.vertex_enabled", m_controller->vertexSnapEnabled());
    setSettingsBool(config, "snap.midpoint_enabled", m_controller->midpointSnapEnabled());
    setSettingsBool(config, "snap.center_enabled", m_controller->centerSnapEnabled());
    setSettingsBool(config, "snap.guide_enabled", m_controller->guideSnapEnabled());
    setSettingsBool(config, "snap.guide_move_enabled", m_controller->guideMoveSnapEnabled());
    setSettingsBool(config, "snap.object_priority_before_grid", m_controller->objectSnapPriorityBeforeGrid());
    setSettingsString(config, "snap.object_tolerance_preset", m_controller->objectSnapTolerancePresetId().toStdString());

    setSettingsString(config, "plot.order_mode", m_controller->plotOrderModeId().toStdString());
    setSettingsString(config, "plot.direction_mode", m_controller->plotDirectionModeId().toStdString());

    setSettingsInt(config, "window.width", width());
    setSettingsInt(config, "window.height", height());

    std::vector<std::string> recent;
    for (const QString &path : m_recentFiles) {
        recent.push_back(path.toStdString());
    }
    writeRecentFilesToConfig(config, recent);
    return config;
}

void EdiShellWindow::applySettings(const edi::formats::StaticConfig &config)
{
    using namespace edi::io;
    // Grid: preset first (it resets dependent fields), then the explicit values.
    m_controller->setGridPresetId(QString::fromStdString(settingsString(config, "grid.preset", "square_art_board")));
    m_controller->setGridUnitId(QString::fromStdString(settingsString(config, "grid.unit", "inch")));
    m_controller->setGridSize(settingsDouble(config, "grid.width", 12.0), settingsDouble(config, "grid.height", 12.0));
    m_controller->setGridMargins(
        settingsDouble(config, "grid.margin_left", 0.25),
        settingsDouble(config, "grid.margin_top", 0.25),
        settingsDouble(config, "grid.margin_right", 0.25),
        settingsDouble(config, "grid.margin_bottom", 0.25));
    m_controller->setGridStep(settingsDouble(config, "grid.minor_step", 0.25));
    m_controller->setGridMajorLineEvery(settingsInt(config, "grid.major_line_every", 4));
    m_controller->setGridVisible(settingsBool(config, "grid.visible", true));

    m_controller->setGridSnapEnabled(settingsBool(config, "snap.grid_enabled", false));
    m_controller->setObjectSnapEnabled(settingsBool(config, "snap.object_enabled", false));
    m_controller->setEndpointSnapEnabled(settingsBool(config, "snap.endpoint_enabled", true));
    m_controller->setVertexSnapEnabled(settingsBool(config, "snap.vertex_enabled", true));
    m_controller->setMidpointSnapEnabled(settingsBool(config, "snap.midpoint_enabled", true));
    m_controller->setCenterSnapEnabled(settingsBool(config, "snap.center_enabled", true));
    m_controller->setGuideSnapEnabled(settingsBool(config, "snap.guide_enabled", true));
    m_controller->setGuideMoveSnapEnabled(settingsBool(config, "snap.guide_move_enabled", true));
    m_controller->setObjectSnapPriorityBeforeGrid(settingsBool(config, "snap.object_priority_before_grid", true));
    m_controller->setObjectSnapTolerancePreset(QString::fromStdString(settingsString(config, "snap.object_tolerance_preset", "normal")));

    m_controller->setPlotOrderModeId(QString::fromStdString(settingsString(config, "plot.order_mode", "layer_order")));
    m_controller->setPlotDirectionModeId(QString::fromStdString(settingsString(config, "plot.direction_mode", "preserve_direction")));

    const int w = settingsInt(config, "window.width", width());
    const int h = settingsInt(config, "window.height", height());
    if (w > 0 && h > 0) {
        resize(w, h);
    }
}

bool EdiShellWindow::loadSettings(const QString &path)
{
    m_settingsPath = path;
    const edi::formats::StaticConfig config = edi::io::loadSettingsFromPath(path);
    applySettings(config);

    m_recentFiles.clear();
    for (const std::string &recent : edi::io::recentFilesFromConfig(config)) {
        m_recentFiles.push_back(QString::fromStdString(recent));
    }
    rebuildRecentFileButtons();
    return true;
}

bool EdiShellWindow::saveSettings(const QString &path) const
{
    return edi::io::saveSettingsToPath(path, captureSettings());
}

void EdiShellWindow::rememberRecentFile(const QString &path)
{
    if (path.isEmpty()) {
        return;
    }
    m_recentFiles.removeAll(path);
    m_recentFiles.prepend(path);
    while (m_recentFiles.size() > static_cast<int>(edi::io::kRecentFilesCap)) {
        m_recentFiles.removeLast();
    }
    rebuildRecentFileButtons();
    scheduleSettingsSave();
}

void EdiShellWindow::rebuildRecentFileButtons()
{
    if (m_recentFilesContainer == nullptr) {
        return;
    }
    auto *layout = qobject_cast<QVBoxLayout *>(m_recentFilesContainer->layout());
    if (layout == nullptr) {
        return;
    }
    QLayoutItem *item = nullptr;
    while ((item = layout->takeAt(0)) != nullptr) {
        delete item->widget();
        delete item;
    }
    // Surface the five most recent files as quick-open buttons.
    const int shown = qMin(m_recentFiles.size(), 5);
    for (int i = 0; i < shown; ++i) {
        const QString path = m_recentFiles.at(i);
        auto *button = new QPushButton(QFileInfo(path).fileName());
        button->setObjectName(QStringLiteral("recentFileButton"));
        button->setToolTip(path);
        connect(button, &QPushButton::clicked, this, [this, path]() {
            openDrawingFromPath(path);
        });
        layout->addWidget(button);
    }
}

void EdiShellWindow::scheduleSettingsSave()
{
    if (m_settingsPath.isEmpty() || m_settingsSaveTimer == nullptr) {
        return;
    }
    m_settingsSaveTimer->start();
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
    m_physicalGeometryFields.clear();

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
        auto *spin = makeGeometryFieldSpin({
            .fieldId = fieldId,
            .fieldMode = QStringLiteral("normalized"),
            .decimals = field.value(QStringLiteral("decimals"), 4).toInt(),
            .step = field.value(QStringLiteral("step"), 0.01).toDouble(),
            .minimum = field.value(QStringLiteral("minimum"), -10.0).toDouble(),
            .maximum = field.value(QStringLiteral("maximum"), 10.0).toDouble(),
            .value = selectedObject.value(fieldId).toDouble(),
        });
        layout->addWidget(label, row, 0);
        layout->addWidget(spin, row, 1);
        if (field.value(QStringLiteral("physical_editable")).toBool() && physicalGeometry.contains(fieldId)) {
            auto *physicalSpin = makeGeometryFieldSpin({
                .fieldId = fieldId,
                .fieldMode = QStringLiteral("physical"),
                .decimals = field.value(QStringLiteral("physical_decimals"), field.value(QStringLiteral("decimals"), 4)).toInt(),
                .step = field.value(QStringLiteral("physical_step"), field.value(QStringLiteral("step"), 0.01)).toDouble(),
                .minimum = field.value(QStringLiteral("physical_minimum"), -100000.0).toDouble(),
                .maximum = field.value(QStringLiteral("physical_maximum"), 100000.0).toDouble(),
                .value = physicalGeometry.value(fieldId).toDouble(),
            });
            auto *physicalLabel = new QLabel(field.value(QStringLiteral("physical_unit_label"), unitLabel).toString());
            physicalLabel->setObjectName(QStringLiteral("valueLabel"));
            layout->addWidget(physicalSpin, row, 2);
            layout->addWidget(physicalLabel, row, 3);
            m_physicalGeometryFields.insert(fieldId, physicalSpin);
        }
        m_geometryFields.insert(fieldId, spin);
        ++row;
    }

    setGeometryEditorVisible(row > 0);
}

void EdiShellWindow::applyGeometryEditStatus(const QVariantMap &editStatus)
{
    const bool showFailure = !editStatus.isEmpty() && !editStatus.value(QStringLiteral("ok")).toBool();
    const QString failedFieldId = showFailure ? editStatus.value(QStringLiteral("field_id")).toString() : QString();
    const QString failedMode = showFailure ? editStatus.value(QStringLiteral("mode")).toString() : QString();

    auto updateField = [](QDoubleSpinBox *field, bool invalid) {
        if (field == nullptr || field->property("editInvalid").toBool() == invalid) {
            return;
        }
        field->setProperty("editInvalid", invalid);
        field->style()->unpolish(field);
        field->style()->polish(field);
        field->update();
    };

    for (auto it = m_geometryFields.begin(); it != m_geometryFields.end(); ++it) {
        updateField(it.value(), failedMode == QStringLiteral("normalized") && it.key() == failedFieldId);
    }
    for (auto it = m_physicalGeometryFields.begin(); it != m_physicalGeometryFields.end(); ++it) {
        updateField(it.value(), failedMode == QStringLiteral("physical") && it.key() == failedFieldId);
    }
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
    const QVariantMap quickMeasurement = document.value(QStringLiteral("quick_measurement")).toMap();
    const QVariantMap guideDragSnap = document.value(QStringLiteral("guide_drag_snap")).toMap();
    const QVariantMap calibrationMeasurement = document.value(QStringLiteral("calibration_measurement")).toMap();
    const QVariantMap calibrationCorrection = document.value(QStringLiteral("calibration_correction")).toMap();
    const QVariantMap editStatus = document.value(QStringLiteral("edit_status")).toMap();
    const QVariantMap selectedObject = activeObjectProjection(document);
    const QVariantList layers = document.value(QStringLiteral("layers")).toList();
    const QString activeLayerId = document.value(QStringLiteral("active_layer_id")).toString();
    const QVariantMap activeLayer = layerProjection(document, activeLayerId);
    const bool hasPreview = document.contains(QStringLiteral("preview_object"));

    if (m_workspaceTitle != nullptr) {
        m_workspaceTitle->setText(QStringLiteral("%1 Workspace")
            .arg(QString::fromLatin1(edi::app::workspaceModeLabel(m_appState.mode))));
    }

    setLabelText(m_toolValue, QStringLiteral("Tool: %1").arg(m_controller->selectedToolId()));
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
    setLabelText(m_objectBoundsValue, selectedObject.isEmpty() ? QStringLiteral("Bounds: none") : boundsSummary(selectedObject));
    setLabelText(m_objectGeometryValue, selectedObject.isEmpty() ? QStringLiteral("Geometry: none") : geometrySummary(selectedObject));
    if (m_geometryEditStatus != nullptr) {
        const bool showFailure = !editStatus.isEmpty() && !editStatus.value(QStringLiteral("ok")).toBool();
        m_geometryEditStatus->setVisible(showFailure);
        m_geometryEditStatus->setText(showFailure
            ? QStringLiteral("Edit rejected: %1").arg(editStatus.value(QStringLiteral("message")).toString())
            : QString());
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
    refreshToggle(m_selectedLocked, selectedObject.value(QStringLiteral("locked")).toBool(), !selectedObject.isEmpty());
    refreshToggle(m_selectedVisible, selectedObject.isEmpty() ? false : selectedObject.value(QStringLiteral("visible")).toBool(), !selectedObject.isEmpty());
    refreshToggle(m_defaultLayerLocked, activeLayer.value(QStringLiteral("locked")).toBool(), !activeLayer.isEmpty());
    refreshToggle(m_defaultLayerVisible, activeLayer.isEmpty() ? false : activeLayer.value(QStringLiteral("visible")).toBool(), !activeLayer.isEmpty());
    setWidgetEnabled(m_layerDownButton, !activeLayer.isEmpty() && activeLayer.value(QStringLiteral("order")).toInt() > 0);
    setWidgetEnabled(m_layerUpButton, !activeLayer.isEmpty() && activeLayer.value(QStringLiteral("order")).toInt() + 1 < layers.size());
    refreshToggle(m_activeLayerPlotEnabled, activeLayer.value(QStringLiteral("plot_enabled")).toBool(), !activeLayer.isEmpty());
    refreshComboData(m_activeLayerPen, activeLayer.value(QStringLiteral("pen_id")).toString(), 0, !activeLayer.isEmpty());
    refreshComboData(m_activeLayerStrokeWidth, strokeWidthPresetId(activeLayer.value(QStringLiteral("stroke_width")).toDouble()), 1, !activeLayer.isEmpty());
    refreshLayerCombo(m_activeLayer, layers, activeLayerId, true);
    refreshLayerCombo(
        m_selectedObjectLayer,
        layers,
        selectedObject.value(QStringLiteral("layer_id")).toString(),
        !selectedObject.isEmpty());
    refreshComboData(m_gridPreset, grid.value(QStringLiteral("preset")).toString(), 0);
    refreshComboData(m_gridUnit, grid.value(QStringLiteral("unit")).toString(), 0);
    refreshSpinValue(m_gridWidth, grid.value(QStringLiteral("width")).toDouble());
    refreshSpinValue(m_gridHeight, grid.value(QStringLiteral("height")).toDouble());
    refreshSpinValue(m_gridMarginLeft, grid.value(QStringLiteral("margin_left")).toDouble());
    refreshSpinValue(m_gridMarginTop, grid.value(QStringLiteral("margin_top")).toDouble());
    refreshSpinValue(m_gridMarginRight, grid.value(QStringLiteral("margin_right")).toDouble());
    refreshSpinValue(m_gridMarginBottom, grid.value(QStringLiteral("margin_bottom")).toDouble());
    refreshSpinValue(m_gridMinorStep, grid.value(QStringLiteral("minor_step")).toDouble());
    refreshSpinValue(m_gridMajorEvery, grid.value(QStringLiteral("major_line_every")).toInt());
    refreshToggle(m_gridVisible, grid.value(QStringLiteral("visible")).toBool());
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
    setLabelText(m_objectPlotSafetyValue, selectedPlotSafetySummary(selectedObject, plot));
    setLabelText(m_selectionPlotBoundsValue, selectionPlotBoundsSummary(document));
    for (const auto &conditional : std::as_const(m_conditionalButtons)) {
        conditional.first->setEnabled(conditional.second == QStringLiteral("has_selection")
            ? !selectedObject.isEmpty()
            : selectedObject.value(conditional.second).toBool());
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
    for (QPushButton *button : std::as_const(m_guideOffsetButtons)) {
        button->setEnabled(selectedGuideControlsEnabled);
    }
    const bool guideVisualControlsEnabled = selectedObject.value(QStringLiteral("guide_visual_controls")).toBool();
    if (m_guideLabel != nullptr) {
        const QSignalBlocker blocker(m_guideLabel);
        m_guideLabel->setEnabled(guideVisualControlsEnabled);
        m_guideLabel->setText(guideVisualControlsEnabled ? selectedObject.value(QStringLiteral("guide_custom_label")).toString() : QString());
    }
    refreshComboData(m_guideColor, selectedObject.value(QStringLiteral("guide_color")).toString(), 0, guideVisualControlsEnabled);
    refreshComboData(m_guideDashStyle, selectedObject.value(QStringLiteral("guide_dash_style")).toString(), 0, guideVisualControlsEnabled);
    refreshToggle(m_guideShowLabel, selectedObject.value(QStringLiteral("guide_show_label"), true).toBool(), guideVisualControlsEnabled);
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
    refreshComboData(m_dimensionKind, selectedObject.value(QStringLiteral("dimension_kind")).toString(), 0, dimensionControlsEnabled);
    refreshToggle(m_dimensionShowLabel, selectedObject.value(QStringLiteral("dimension_show_label"), true).toBool(), dimensionControlsEnabled);
    rebuildGeometryEditor(selectedObject);
    applyGeometryEditStatus(editStatus);
    setLabelText(m_objectsValue, QStringLiteral("Objects: %1").arg(objects.size()));
    if (m_guidesValue != nullptr) {
        m_guidesValue->setText(QStringLiteral("Guides: %1 visible / %2 total / %3 duplicate")
            .arg(document.value(QStringLiteral("visible_guide_count")).toInt())
            .arg(document.value(QStringLiteral("guide_count")).toInt())
            .arg(document.value(QStringLiteral("duplicate_guide_count")).toInt()));
    }
    setLabelText(m_revisionValue, QStringLiteral("Revision: %1").arg(document.value(QStringLiteral("revision")).toInt()));
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
    setLabelText(m_plotBoundsValue, plotBoundsSummary(grid, plot));
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
    refreshComboData(m_plotOrderMode, plot.value(QStringLiteral("order_mode")).toString(), 0);
    refreshComboData(m_plotDirectionMode, plot.value(QStringLiteral("direction_mode")).toString(), 0);
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
    if (m_quickMeasureValue != nullptr) {
        if (quickMeasurement.isEmpty() || !quickMeasurement.value(QStringLiteral("ok")).toBool()) {
            m_quickMeasureValue->setText(QStringLiteral("Quick measure: none"));
        } else {
            m_quickMeasureValue->setText(QStringLiteral("Quick measure: %1  target %2")
                .arg(quickMeasurement.value(QStringLiteral("label")).toString())
                .arg(quickMeasurement.value(QStringLiteral("object_id")).toString()));
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
    setLabelText(m_previewValue, QStringLiteral("Preview: %1").arg(hasPreview ? QStringLiteral("active") : QStringLiteral("none")));
    if (m_statusValue != nullptr) {
        m_statusValue->setText(QStringLiteral("%1 | %2 selected | %3 objects")
            .arg(QString::fromLatin1(edi::app::workspaceModeName(m_appState.mode)))
            .arg(selected.size())
            .arg(objects.size()));
    }

    if (m_undoButton != nullptr) {
        m_undoButton->setEnabled(m_controller->canUndo());
    }
    if (m_redoButton != nullptr) {
        m_redoButton->setEnabled(m_controller->canRedo());
    }

    // Keep the window title's dirty marker in sync with live edits. The
    // controller reports content dirtiness (selection alone is not dirty).
    const QString name = m_currentDrawingPath.isEmpty()
        ? QStringLiteral("Untitled")
        : QFileInfo(m_currentDrawingPath).fileName();
    setWindowTitle(QStringLiteral("EDI — %1%2").arg(name, m_controller->isDocumentDirty() ? QStringLiteral(" •") : QString()));
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
        #editErrorLabel {
            color: #f0c8c8;
            background: #2a1d21;
            border: 1px solid #6b343f;
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
        #geometryField[editInvalid="true"] {
            color: #f4dede;
            background: #332127;
            border: 1px solid #c65d6a;
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
