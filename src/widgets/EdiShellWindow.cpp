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
#include <QMouseEvent>
#include <QResizeEvent>
#include <QTimer>
#include <QWindow>
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
    if (m_framelessChrome) {
        setWindowFlag(Qt::FramelessWindowHint, true);
    }

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
    // (the layout). Registry, layout, and the feature instance are members:
    // switching workspaces re-reads all three. The context is a member because
    // features may hold onto the bus for as long as their widgets live.
    m_featureContext.drawingController = m_controller;
    m_draftingFeature = createDraftingFeature();

    FeatureDescriptor drafting;
    drafting.id = QStringLiteral("drafting");
    drafting.label = QStringLiteral("Drafting");
    drafting.supportedSlots = {ShellSlot::Main, ShellSlot::Left, ShellSlot::Right, ShellSlot::Bottom};
    // Reads the member on every call, so the descriptor survives the feature
    // instance being replaced across workspace switches.
    drafting.buildPanel = [this](ShellSlot slot, FeatureContext &) -> QWidget * {
        return m_draftingFeature->buildPanel(slot);
    };
    m_featureRegistry.features.push_back(drafting);

    // Empty splitters first; mountWorkspace() fills them — the same path a
    // runtime workspace switch takes, so the constructor cannot drift from it.
    m_bodySplitter = new QSplitter(Qt::Horizontal);
    m_bodySplitter->setObjectName(QStringLiteral("bodySplitter"));
    m_bodySplitter->setChildrenCollapsible(false); // collapse is a modeled state, not a drag accident
    m_bodySplitter->setHandleWidth(8);             // spec: 8px hit zone

    bodyLayout->addWidget(buildActivityRail());
    bodyLayout->addWidget(m_bodySplitter, 1);

    m_rootSplitter = new QSplitter(Qt::Vertical);
    m_rootSplitter->setObjectName(QStringLiteral("rootSplitter"));
    m_rootSplitter->setChildrenCollapsible(false);
    m_rootSplitter->setHandleWidth(8);
    m_rootSplitter->addWidget(body);
    m_rootSplitter->setStretchFactor(0, 1);
    root->addWidget(buildTitleBar());
    root->addWidget(m_rootSplitter, 1);

    connect(m_bodySplitter, &QSplitter::splitterMoved, this, [this]() { capturePanelSizes(); });
    connect(m_rootSplitter, &QSplitter::splitterMoved, this, [this]() { capturePanelSizes(); });

    m_panelsState = defaultShellPanelsState();

    WorkspaceLayout drafting_layout;
    drafting_layout.id = QStringLiteral("drafting");
    drafting_layout.label = QStringLiteral("Drafting");
    drafting_layout.bindings = {
        {ShellSlot::Left, QStringLiteral("drafting")},
        {ShellSlot::Main, QStringLiteral("drafting")},
        {ShellSlot::Right, QStringLiteral("drafting")},
        {ShellSlot::Bottom, QStringLiteral("drafting")},
    };
    mountWorkspace(drafting_layout);
    m_workspaceHistory = {m_workspaceLayout};
    m_workspaceHistoryIndex = 0;
    refreshChrome();

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

std::unique_ptr<DraftingFeature> EdiShellWindow::createDraftingFeature()
{
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
    return std::make_unique<DraftingFeature>(m_controller, std::move(actions));
}

void EdiShellWindow::mountWorkspace(const WorkspaceLayout &layout)
{
    m_workspaceLayout = layout;
    const std::vector<MountedSlot> mounted =
        mountWorkspaceLayout(m_workspaceLayout, m_featureRegistry, m_featureContext);

    // Geometry stays the window's job: slots have fixed places in the frame,
    // and an unbound slot simply isn't added. Side and bottom slots sit in
    // splitters so the user can drag-resize them; the pure ShellPanels model
    // is the source of truth and the splitters are a projection of it.
    m_leftPanelWidget = mountedSlotWidget(mounted, ShellSlot::Left);
    m_mainPanelWidget = mountedSlotWidget(mounted, ShellSlot::Main);
    m_rightPanelWidget = mountedSlotWidget(mounted, ShellSlot::Right);
    m_bottomPanelWidget = mountedSlotWidget(mounted, ShellSlot::Bottom);

    if (m_leftPanelWidget != nullptr) {
        m_bodySplitter->addWidget(m_leftPanelWidget);
    }
    if (m_mainPanelWidget != nullptr) {
        m_bodySplitter->addWidget(m_mainPanelWidget);
        m_bodySplitter->setStretchFactor(m_bodySplitter->count() - 1, 1);
    }
    if (m_rightPanelWidget != nullptr) {
        m_bodySplitter->addWidget(m_rightPanelWidget);
    }
    if (m_bottomPanelWidget != nullptr) {
        m_rootSplitter->addWidget(m_bottomPanelWidget);
    }

    applyPanelSizesToSplitters();
    refreshPanelVisibility();
}

void EdiShellWindow::applyWorkspaceLayout(const WorkspaceLayout &layout)
{
    // Tear down the mounted slots, then retire the feature instance itself:
    // its widget-pointer members die with it, so nothing can dangle into the
    // next mount. This destroy-and-rebuild is the entire reason the feature
    // moved out of the window. (Immediate delete, not deleteLater: nothing on
    // the call stack lives inside these widgets — switching is driven from
    // chrome, never from inside a panel.)
    const auto retire = [](QWidget *&widget) {
        delete widget;
        widget = nullptr;
    };
    retire(m_leftPanelWidget);
    retire(m_mainPanelWidget);
    retire(m_rightPanelWidget);
    retire(m_bottomPanelWidget);
    m_draftingFeature = createDraftingFeature();

    mountWorkspace(layout);

    // The fresh feature starts blank; re-feed it the shell-owned state.
    m_draftingFeature->setRecentFiles(m_recentFiles);
    m_draftingFeature->refreshInspector();
}

void EdiShellWindow::switchWorkspaceLayout(const WorkspaceLayout &layout)
{
    // A switch starts a new trail segment: drop any forward entries (same
    // rule as a browser), push, and apply.
    if (m_workspaceHistoryIndex >= 0
        && m_workspaceHistoryIndex + 1 < static_cast<int>(m_workspaceHistory.size())) {
        m_workspaceHistory.resize(m_workspaceHistoryIndex + 1);
    }
    m_workspaceHistory.push_back(layout);
    m_workspaceHistoryIndex = static_cast<int>(m_workspaceHistory.size()) - 1;
    applyWorkspaceLayout(layout);
    refreshChrome();
}

void EdiShellWindow::navigateWorkspaceHistory(int delta)
{
    const int next = m_workspaceHistoryIndex + delta;
    if (next < 0 || next >= static_cast<int>(m_workspaceHistory.size())) {
        return;
    }
    m_workspaceHistoryIndex = next;
    applyWorkspaceLayout(m_workspaceHistory[static_cast<std::size_t>(next)]);
    refreshChrome();
}

void EdiShellWindow::refreshChrome()
{
    // The chrome is a projection of shell state, recomputed whole — the same
    // discipline as the panels: no incremental flag-flipping to drift.
    const auto reflect = [this](QPushButton *button, ShellSlot slot) {
        if (button != nullptr) {
            button->setChecked(shellPanelVisibility(slot) == PanelVisibility::Visible);
        }
    };
    reflect(m_toggleLeftButton, ShellSlot::Left);
    reflect(m_toggleBottomButton, ShellSlot::Bottom);
    reflect(m_toggleRightButton, ShellSlot::Right);
    if (m_backButton != nullptr) {
        m_backButton->setEnabled(m_workspaceHistoryIndex > 0);
    }
    if (m_forwardButton != nullptr) {
        m_forwardButton->setEnabled(
            m_workspaceHistoryIndex + 1 < static_cast<int>(m_workspaceHistory.size()));
    }
}

bool EdiShellWindow::eventFilter(QObject *watched, QEvent *event)
{
    // Frameless windows lose the system drag region; the title bar's own
    // surface becomes it. Child buttons consume their presses first, so only
    // bar-background presses arrive here.
    if (watched == m_titleBar) {
        if (event->type() == QEvent::MouseButtonPress) {
            auto *mouse = static_cast<QMouseEvent *>(event);
            if (mouse->button() == Qt::LeftButton && windowHandle() != nullptr) {
                windowHandle()->startSystemMove();
                return true;
            }
        }
        if (event->type() == QEvent::MouseButtonDblClick) {
            isMaximized() ? showNormal() : showMaximized();
            return true;
        }
    }
    return QMainWindow::eventFilter(watched, event);
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
    refreshChrome(); // the toggle buttons mirror whatever just happened
}

void EdiShellWindow::applyPanelSizesToSplitters()
{
    // QSplitter::setSizes wants every section, and a layout may leave any slot
    // unbound — so sections are resolved by widget identity, never by index.
    // The main slot absorbs whatever the panels leave over. Hidden sections
    // keep their stored size in the model, so reopening restores the layout.
    if (m_bodySplitter != nullptr && m_bodySplitter->count() > 0) {
        QList<int> sizes;
        int panelTotal = 0;
        for (int i = 0; i < m_bodySplitter->count(); ++i) {
            QWidget *widget = m_bodySplitter->widget(i);
            int size = 0; // main: placeholder, filled with the remainder below
            if (widget == m_leftPanelWidget) {
                size = m_panelsState.left.size;
            } else if (widget == m_rightPanelWidget) {
                size = m_panelsState.right.size;
            }
            panelTotal += size;
            sizes.push_back(size);
        }
        const int mainIndex = m_bodySplitter->indexOf(m_mainPanelWidget);
        if (mainIndex >= 0) {
            sizes[mainIndex] = std::max(0, m_bodySplitter->width() - panelTotal);
        }
        m_bodySplitter->setSizes(sizes);
    }
    if (m_rootSplitter != nullptr && m_rootSplitter->indexOf(m_bottomPanelWidget) >= 0) {
        const int bottom = m_panelsState.bottom.size;
        m_rootSplitter->setSizes({std::max(0, m_rootSplitter->height() - bottom), bottom});
    }
}

void EdiShellWindow::capturePanelSizes()
{
    // Drag feedback flows one way: splitter -> clamp -> model. Sections are
    // found by widget identity (layouts may leave slots unbound), and hidden
    // panels report size 0 here, which must not overwrite the stored size they
    // will reopen to — hence the visibility guards.
    if (m_bodySplitter != nullptr) {
        const QList<int> sizes = m_bodySplitter->sizes();
        if (m_leftPanelWidget != nullptr && m_leftPanelWidget->isVisible()) {
            m_panelsState.left.size =
                clampPanelSize(ShellSlot::Left, sizes.value(m_bodySplitter->indexOf(m_leftPanelWidget)));
        }
        if (m_rightPanelWidget != nullptr && m_rightPanelWidget->isVisible()) {
            m_panelsState.right.size =
                clampPanelSize(ShellSlot::Right, sizes.value(m_bodySplitter->indexOf(m_rightPanelWidget)));
        }
    }
    if (m_rootSplitter != nullptr && m_bottomPanelWidget != nullptr && m_bottomPanelWidget->isVisible()
        && m_rootSplitter->indexOf(m_bottomPanelWidget) >= 0) {
        m_panelsState.bottom.size = clampPanelSize(
            ShellSlot::Bottom, m_rootSplitter->sizes().value(m_rootSplitter->indexOf(m_bottomPanelWidget)));
    }
}
