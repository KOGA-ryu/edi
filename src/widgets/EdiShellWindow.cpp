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
#include <QSplitter>
#include <QResizeEvent>
#include <QTimer>
#include <QSizePolicy>
#include <QSpinBox>
#include <QStyle>
#include <QStringList>
#include <QVariantList>
#include <QVariantMap>
#include <QVBoxLayout>
#include <QVector>

#include <algorithm>
#include <optional>
#include <utility>

#include "core/DrawingCore.h"
#include "widgets/DraftingFeature.h"
#include "widgets/DrawingCanvasWidget.h"
#include "widgets/ShellTheme.h"
#include "widgets/ShellWidgetHelpers.h"

using namespace edi::shell;

EdiShellWindow::EdiShellWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_appState(edi::app::defaultAppState())
{
    setWindowTitle(QStringLiteral("EDI"));
    // Spec §2 minimum (520x420) — auto-hide thresholds (640/520) must be
    // reachable, which the old 960x620 minimum made impossible by definition.
    setMinimumSize(520, 420);
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

    // The window no longer hard-wires drafting panels into regions: drafting
    // is feature #1 in a registry, and which feature fills which slot is data
    // (the layout). The layout is a member — persistence saves it — while the
    // registry stays a constructor local until workspace switching needs it
    // again; the context is a member because features may hold onto the bus
    // for as long as their widgets live.
    m_featureContext.drawingController = m_controller;

    // The drafting feature talks back to the shell only through callables:
    // the feature never sees the window type, so it stays mountable under any
    // shell that can supply these verbs.
    DraftingFeature::ShellActions actions;
    actions.saveDrawing = [this]() { promptSaveDrawing(); };
    actions.saveDrawingAs = [this]() { promptSaveDrawingAs(); };
    actions.openDrawing = [this]() { promptOpenDrawing(); };
    actions.exportSvg = [this]() { promptExportSvg(); };
    actions.exportHpgl = [this]() { promptExportHpgl(); };
    actions.openDrawingAtPath = [this](const QString &path) { openDrawingFromPath(path); };
    actions.workspaceModeLabel = [this]() { return QString::fromLatin1(edi::app::workspaceModeLabel(m_appState.mode)); };
    actions.workspaceModeName = [this]() { return QString::fromLatin1(edi::app::workspaceModeName(m_appState.mode)); };
    m_draftingFeature = std::make_unique<DraftingFeature>(m_controller, std::move(actions));

    FeatureDescriptor drafting;
    drafting.id = QStringLiteral("drafting");
    drafting.label = QStringLiteral("Drafting");
    drafting.supportedSlots = {ShellSlot::Main, ShellSlot::Left, ShellSlot::Right, ShellSlot::Bottom};
    drafting.buildPanel = [this](ShellSlot slot, FeatureContext &) -> QWidget * {
        return m_draftingFeature->buildPanel(slot);
    };

    FeatureRegistry registry;
    registry.features.push_back(drafting);

    m_workspaceLayout.id = QStringLiteral("drafting");
    m_workspaceLayout.label = QStringLiteral("Drafting");
    m_workspaceLayout.bindings = {
        {ShellSlot::Left, QStringLiteral("drafting")},
        {ShellSlot::Main, QStringLiteral("drafting")},
        {ShellSlot::Right, QStringLiteral("drafting")},
        {ShellSlot::Bottom, QStringLiteral("drafting")},
    };

    const std::vector<MountedSlot> mounted = mountWorkspaceLayout(m_workspaceLayout, registry, m_featureContext);

    // Geometry stays the window's job: slots have fixed places in the frame,
    // and an unbound slot simply isn't added. Side and bottom slots sit in
    // splitters so the user can drag-resize them; the pure ShellPanels model
    // is the source of truth and the splitters are a projection of it.
    m_leftPanelWidget = mountedSlotWidget(mounted, ShellSlot::Left);
    m_rightPanelWidget = mountedSlotWidget(mounted, ShellSlot::Right);
    m_bottomPanelWidget = mountedSlotWidget(mounted, ShellSlot::Bottom);
    QWidget *mainWidget = mountedSlotWidget(mounted, ShellSlot::Main);

    m_bodySplitter = new QSplitter(Qt::Horizontal);
    m_bodySplitter->setObjectName(QStringLiteral("bodySplitter"));
    m_bodySplitter->setChildrenCollapsible(false); // collapse is a modeled state, not a drag accident
    m_bodySplitter->setHandleWidth(8);             // spec: 8px hit zone
    if (m_leftPanelWidget != nullptr) {
        m_bodySplitter->addWidget(m_leftPanelWidget);
    }
    if (mainWidget != nullptr) {
        m_bodySplitter->addWidget(mainWidget);
        m_bodySplitter->setStretchFactor(m_bodySplitter->count() - 1, 1);
    }
    if (m_rightPanelWidget != nullptr) {
        m_bodySplitter->addWidget(m_rightPanelWidget);
    }

    bodyLayout->addWidget(buildActivityRail());
    bodyLayout->addWidget(m_bodySplitter, 1);

    m_rootSplitter = new QSplitter(Qt::Vertical);
    m_rootSplitter->setObjectName(QStringLiteral("rootSplitter"));
    m_rootSplitter->setChildrenCollapsible(false);
    m_rootSplitter->setHandleWidth(8);
    m_rootSplitter->addWidget(body);
    m_rootSplitter->setStretchFactor(0, 1);
    if (m_bottomPanelWidget != nullptr) {
        m_rootSplitter->addWidget(m_bottomPanelWidget);
    }
    root->addWidget(m_rootSplitter, 1);

    connect(m_bodySplitter, &QSplitter::splitterMoved, this, [this]() { capturePanelSizes(); });
    connect(m_rootSplitter, &QSplitter::splitterMoved, this, [this]() { capturePanelSizes(); });

    m_panelsState = defaultShellPanelsState();
    applyPanelSizesToSplitters();
    refreshPanelVisibility();

    setCentralWidget(central);
    applyShellStyle();

    // The feature keeps its own inspector in sync; the window only owns the
    // title bar (file name + dirty marker).
    connect(m_controller, &DrawingDocumentController::modelChanged, this, &EdiShellWindow::updateWindowTitle);

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
    m_draftingFeature->refreshInspector();
}

EdiShellWindow::~EdiShellWindow() = default;

void EdiShellWindow::closeEvent(QCloseEvent *event)
{
    if (!m_settingsPath.isEmpty()) {
        saveSettings(m_settingsPath); // flush settings immediately on close
    }
    if (!m_workspaceLayoutPath.isEmpty()) {
        saveWorkspaceLayout(m_workspaceLayoutPath); // panel geometry survives restart
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
    m_draftingFeature->setRecentFiles(m_recentFiles);
    scheduleSettingsSave();
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
    m_draftingFeature->refreshInspector();
}

void EdiShellWindow::applyShellStyle()
{
    // The palette lives in ShellTheme as data; this method only asks for the
    // default inputs and applies the derived sheet. Custom themes later become
    // "construct different inputs here" — no QSS edits.
    setStyleSheet(buildShellStyleSheet(deriveShellTheme(ShellThemeInputs{})));
}

void EdiShellWindow::setPanelCollapsed(ShellSlot slot, bool collapsed)
{
    panelStateFor(m_panelsState, slot).collapsed = collapsed;
    refreshPanelVisibility();
}

void EdiShellWindow::applyShellPanelPreset(PanelPreset preset)
{
    m_panelsState = applyPanelPreset(m_panelsState, preset);
    applyPanelSizesToSplitters();
    refreshPanelVisibility();
}

PanelVisibility EdiShellWindow::shellPanelVisibility(ShellSlot slot) const
{
    return panelVisibility(slot, panelStateFor(m_panelsState, slot), width(), height());
}

void EdiShellWindow::resizeEvent(QResizeEvent *event)
{
    QMainWindow::resizeEvent(event);
    refreshPanelVisibility(); // auto-hide reacts to the window, not to clicks
}

void EdiShellWindow::refreshPanelVisibility()
{
    const auto apply = [this](ShellSlot slot, QWidget *widget) {
        if (widget != nullptr) {
            widget->setVisible(shellPanelVisibility(slot) == PanelVisibility::Visible);
        }
    };
    apply(ShellSlot::Left, m_leftPanelWidget);
    apply(ShellSlot::Right, m_rightPanelWidget);
    apply(ShellSlot::Bottom, m_bottomPanelWidget);
}

void EdiShellWindow::applyPanelSizesToSplitters()
{
    // QSplitter::setSizes wants every section; the main slot gets whatever the
    // panels leave over. Hidden sections keep their stored size in the model,
    // so reopening restores the user's layout.
    if (m_bodySplitter != nullptr && m_bodySplitter->count() == 3) {
        const int left = m_panelsState.left.size;
        const int right = m_panelsState.right.size;
        m_bodySplitter->setSizes({left, std::max(0, m_bodySplitter->width() - left - right), right});
    }
    if (m_rootSplitter != nullptr && m_rootSplitter->count() == 2) {
        const int bottom = m_panelsState.bottom.size;
        m_rootSplitter->setSizes({std::max(0, m_rootSplitter->height() - bottom), bottom});
    }
}

void EdiShellWindow::capturePanelSizes()
{
    // Drag feedback flows one way: splitter -> clamp -> model. Hidden panels
    // report size 0 here, which must not overwrite the stored size they will
    // reopen to — hence the visibility guards.
    if (m_bodySplitter != nullptr && m_bodySplitter->count() == 3) {
        const QList<int> sizes = m_bodySplitter->sizes();
        if (m_leftPanelWidget != nullptr && m_leftPanelWidget->isVisible()) {
            m_panelsState.left.size = clampPanelSize(ShellSlot::Left, sizes.value(0));
        }
        if (m_rightPanelWidget != nullptr && m_rightPanelWidget->isVisible()) {
            m_panelsState.right.size = clampPanelSize(ShellSlot::Right, sizes.value(2));
        }
    }
    if (m_rootSplitter != nullptr && m_rootSplitter->count() == 2
        && m_bottomPanelWidget != nullptr && m_bottomPanelWidget->isVisible()) {
        m_panelsState.bottom.size = clampPanelSize(ShellSlot::Bottom, m_rootSplitter->sizes().value(1));
    }
}
