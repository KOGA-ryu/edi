#include "widgets/EdiShellWindow.h"

#include <QAbstractButton>
#include <QApplication>
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
#include "core/RecipeController.h"
#include "io/ProfileStore.h"
#include "widgets/BeltCrossWidget.h"
#include "widgets/DraftingFeature.h"
#include "widgets/DrawingCanvasWidget.h"
#include "widgets/FloatingPalette.h"
#include "widgets/SettingsFeature.h"
#include "widgets/ShellTheme.h"
#include "widgets/ShellWidgetHelpers.h"

using namespace edi::shell;

namespace {

// The built-in job. Layouts are data; this is merely the one the shell
// ships with — loaded/saved ones replace it freely. (Settings is a pop-out
// window, not a workspace, so it has no layout here.)
WorkspaceLayout draftingWorkspaceLayout()
{
    WorkspaceLayout layout;
    layout.id = QStringLiteral("drafting");
    layout.label = QStringLiteral("Drafting");
    layout.bindings = {
        {ShellSlot::Left, QStringLiteral("drafting")},
        {ShellSlot::Main, QStringLiteral("drafting")},
        {ShellSlot::Right, QStringLiteral("drafting")},
        {ShellSlot::Bottom, QStringLiteral("drafting")},
    };
    // The belt arrangement ships with the job, not with the feature: a saved
    // workspace.toml overrides this wholesale.
    layout.belt = DraftingFeature::defaultBeltLayout();
    return layout;
}

} // namespace

EdiShellWindow::EdiShellWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_appState(edi::app::defaultAppState())
{
    setWindowTitle(QStringLiteral("EDI"));
    // Spec §2 minimum (520x420) — auto-hide thresholds (640/520) must be
    // reachable, which the old 960x620 minimum made impossible by definition.
    setMinimumSize(520, 420);
    resize(900, 760); // spec §2 first-run default; the settings restore overrides it
    if (m_framelessChrome) {
        setWindowFlag(Qt::FramelessWindowHint, true);
    }

    m_controller = new DrawingDocumentController(this);
    m_recipeController = new RecipeController(this);
    m_profilesDir = edi::io::defaultProfilesDirPath();

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
    m_featureContext.recipeController = m_recipeController;

    // Per-feature knowledge lives ONLY in these registry rows — how to build
    // a fresh instance, and what shell-owned state to re-feed it after a
    // mount. The switching machinery below loops the registry and never names
    // a feature; adding feature #3 is appending a row here.
    FeatureDescriptor drafting;
    drafting.id = QStringLiteral("drafting");
    drafting.label = QStringLiteral("Drafting");
    drafting.supportedSlots = {ShellSlot::Main, ShellSlot::Left, ShellSlot::Right, ShellSlot::Bottom};
    // Reads the member on every call, so the descriptor survives the feature
    // instance being replaced across workspace switches.
    drafting.buildPanel = [this](ShellSlot slot, FeatureContext &) -> QWidget * {
        return m_draftingFeature->buildPanel(slot);
    };
    drafting.recreateInstance = [this]() { m_draftingFeature = createDraftingFeature(); };
    drafting.buildPalettes = [this]() { return m_draftingFeature->buildPalettes(); };
    drafting.buildChromePanels = [this]() { return m_draftingFeature->buildChromePanels(); };
    drafting.instanceMounted = [this]() {
        m_draftingFeature->refreshInspector();
    };
    m_featureRegistry.features.push_back(drafting);

    FeatureDescriptor settings;
    settings.id = QStringLiteral("settings");
    settings.label = QStringLiteral("Settings");
    settings.supportedSlots = {ShellSlot::Main};
    settings.buildPanel = [this](ShellSlot slot, FeatureContext &) -> QWidget * {
        return m_settingsFeature->buildPanel(slot);
    };
    settings.recreateInstance = [this]() { m_settingsFeature = createSettingsFeature(); };
    m_featureRegistry.features.push_back(settings);

    // The splitter carries only the in-flow left panel beside the main area.
    // Right and bottom panels are overlays INSIDE the main area: they cover
    // the grid, never squeeze it, and the terminal can grow to become the
    // main view. mountWorkspace() fills everything — the same path a runtime
    // workspace switch takes, so the constructor cannot drift from it.
    m_bodySplitter = new QSplitter(Qt::Horizontal);
    m_bodySplitter->setObjectName(QStringLiteral("bodySplitter"));
    m_bodySplitter->setChildrenCollapsible(false); // collapse is a modeled state, not a drag accident
    m_bodySplitter->setHandleWidth(8);             // spec: 8px hit zone

    m_mainArea = new QWidget;
    m_mainArea->setObjectName(QStringLiteral("workspaceColumn"));
    m_mainArea->installEventFilter(this); // resize -> re-layout the overlays
    m_bodySplitter->addWidget(m_mainArea);
    m_bodySplitter->setStretchFactor(0, 1);

    // Overlay resize grips: thin strips on the panels' inner edges. Owned by
    // the window (they survive workspace switches), positioned by
    // layoutMainArea alongside the panels they resize.
    // WA_StyledBackground: plain QWidgets ignore stylesheet backgrounds
    // without it, and the grips' hover affordance is pure QSS.
    m_rightGrip = new QWidget(m_mainArea);
    m_rightGrip->setObjectName(QStringLiteral("rightPanelGrip"));
    m_rightGrip->setCursor(Qt::SplitHCursor);
    m_rightGrip->setAttribute(Qt::WA_StyledBackground, true);
    m_rightGrip->installEventFilter(this);
    m_rightGrip->hide();
    m_bottomGrip = new QWidget(m_mainArea);
    m_bottomGrip->setObjectName(QStringLiteral("bottomPanelGrip"));
    m_bottomGrip->setCursor(Qt::SplitVCursor);
    m_bottomGrip->setAttribute(Qt::WA_StyledBackground, true);
    m_bottomGrip->installEventFilter(this);
    m_bottomGrip->hide();

    bodyLayout->addWidget(buildActivityRail());
    bodyLayout->addWidget(m_bodySplitter, 1);

    root->addWidget(buildTitleBar());
    root->addWidget(body, 1);
    root->addWidget(buildStatusBar());

    connect(m_bodySplitter, &QSplitter::splitterMoved, this, [this]() { capturePanelSizes(); });

    m_panelsState = defaultShellPanelsState();

    // F5: the settings pop-out. Qt::Tool keeps it floating above the main
    // window without stealing the taskbar; as a child widget it inherits the
    // shell QSS, so theme edits restyle it live together with everything
    // else. Hidden until the rail's S button asks for it.
    m_settingsWindow = new QWidget(this, Qt::Tool);
    m_settingsWindow->setObjectName(QStringLiteral("settingsWindow"));
    // Plain QWidgets ignore stylesheet backgrounds without this; the pop-out
    // otherwise renders the platform window gray behind themed content.
    m_settingsWindow->setAttribute(Qt::WA_StyledBackground, true);
    m_settingsWindow->setWindowTitle(QStringLiteral("Settings"));
    auto *settingsWindowLayout = new QVBoxLayout(m_settingsWindow);
    clearLayoutMargins(settingsWindowLayout);

    // The first mount IS a workspace application — same lifecycle path as a
    // runtime switch (recreate instances -> mount -> re-feed state), so the
    // constructor cannot drift from it.
    applyWorkspaceLayout(draftingWorkspaceLayout());
    m_workspaceHistory = {m_workspaceLayout};
    m_workspaceHistoryIndex = 0;
    refreshChrome();

    setCentralWidget(central);
    applyShellStyle();

    // The feature keeps its own inspector in sync; the window only owns the
    // title bar (file name + dirty marker).
    connect(m_controller, &DrawingDocumentController::modelChanged, this, &EdiShellWindow::refreshDocumentStatus);

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

    refreshDocumentStatus(); // instanceMounted already refreshed the inspector
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

void EdiShellWindow::refreshDocumentStatus()
{
    const QString name = m_currentDrawingPath.isEmpty()
        ? QStringLiteral("Untitled")
        : QFileInfo(m_currentDrawingPath).fileName();
    const bool dirty = isDocumentDirty();
    const QString marker = dirty ? QStringLiteral(" •") : QString();
    setWindowTitle(QStringLiteral("EDI — %1%2").arg(name, marker));
    if (m_statusFileLabel != nullptr) {
        m_statusFileLabel->setText(name + marker);
        // Dynamic property + unpolish/polish: the warning color is a sheet
        // rule keyed on data, not a setStyleSheet scattered into logic.
        if (m_statusFileLabel->property("documentDirty").toBool() != dirty) {
            m_statusFileLabel->setProperty("documentDirty", dirty);
            m_statusFileLabel->style()->unpolish(m_statusFileLabel);
            m_statusFileLabel->style()->polish(m_statusFileLabel);
        }
    }
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

    edi::io::writeThemeInputsToConfig(config, m_themeInputs);
    setSettingsString(config, "profile.active", m_activeProfile.toStdString());

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

    // Theme first, so everything below renders under the loaded palette.
    // (readThemeInputsFromConfig degrades absent keys to stock defaults.)
    setThemeInputs(edi::io::readThemeInputsFromConfig(config));
    m_activeProfile = QString::fromStdString(settingsString(config, "profile.active", ""));

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
    rebuildRecentFilesMenu();
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

    // The rail is the workspace switcher: a mode maps to a layout, and the
    // switch goes through the same trail-pushing path as everything else.
    // (Settings is no longer a layout — the rail intercepts it before here
    // and opens the pop-out instead; see buildActivityRail.)
    const WorkspaceLayout target = draftingWorkspaceLayout();
    if (target.id != m_workspaceLayout.id) {
        switchWorkspaceLayout(target);
    } else {
        m_draftingFeature->refreshInspector();
    }
}

void EdiShellWindow::applyShellStyle()
{
    // One derivation feeds both render paths: the shell QSS and the canvas
    // chrome palette. Restyling the whole app is resetting a string plus one
    // struct — which is what makes live theme editing possible at all.
    const ShellTheme theme = deriveShellTheme(m_themeInputs);
    setStyleSheet(buildShellStyleSheet(theme));
    qApp->setStyleSheet(buildToolTipStyleSheet(theme)); // tooltips are top-level; only the app sheet reaches them
    if (m_draftingFeature != nullptr && m_draftingFeature->canvas() != nullptr) {
        m_draftingFeature->canvas()->setCanvasPalette(drawing_canvas::deriveCanvasPalette(theme));
    }
    // Self-painting widgets read QPalette roles, which QSS cannot set from a
    // rule — push the derived palette the same way the canvas gets its
    // struct. findChildren keeps this mount-agnostic: belts live inside
    // floating palettes that are rebuilt on workspace switches, and this
    // runs again after every mount.
    const QPalette paintingPalette = paintingPaletteFromTheme(theme);
    for (BeltCrossWidget *belt : findChildren<BeltCrossWidget *>()) {
        belt->setPalette(paintingPalette);
    }
    // Pointing-hand on every clickable (spec §4): QSS has no cursor
    // property, so this is the one per-widget sweep. Running here covers
    // every mount path the same way the palette push does; buttons created
    // outside a mount (none today) would need their own setCursor.
    for (QPushButton *button : findChildren<QPushButton *>()) {
        button->setCursor(Qt::PointingHandCursor);
    }
}

void EdiShellWindow::setThemeInputs(const ShellThemeInputs &inputs)
{
    m_themeInputs = inputs;
    applyShellStyle();
    scheduleSettingsSave(); // theme.* keys ride in edi.toml with everything else
}

QStringList EdiShellWindow::availableProfiles() const
{
    return edi::io::listProfiles(m_profilesDir);
}

bool EdiShellWindow::saveProfileAs(const QString &name)
{
    if (!edi::io::saveProfile(m_profilesDir, name, m_themeInputs)) {
        return false;
    }
    m_activeProfile = name;
    scheduleSettingsSave(); // remember the active name in edi.toml
    return true;
}

bool EdiShellWindow::loadProfile(const QString &name)
{
    const edi::io::ProfileData data = edi::io::loadProfile(m_profilesDir, name);
    if (!data.ok) {
        return false; // keep the current theme
    }
    m_activeProfile = name;
    setThemeInputs(data.inputs);
    return true;
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
    actions.setStatusText = [this](const QString &text) {
        if (m_statusModeLabel != nullptr) {
            m_statusModeLabel->setText(text);
        }
    };
    // Read the member at call time: the feature is recreated on every
    // workspace switch and must see the layout being mounted, not the one
    // that existed when the actions were wired.
    actions.beltLayout = [this]() { return m_workspaceLayout.belt; };
    return std::make_unique<DraftingFeature>(m_controller, std::move(actions));
}

std::unique_ptr<SettingsFeature> EdiShellWindow::createSettingsFeature()
{
    SettingsFeature::ShellHooks hooks;
    hooks.themeInputs = [this]() { return m_themeInputs; };
    hooks.setThemeInputs = [this](const ShellThemeInputs &inputs) { setThemeInputs(inputs); };
    hooks.profiles = [this]() { return availableProfiles(); };
    hooks.activeProfile = [this]() { return m_activeProfile; };
    hooks.loadProfile = [this](const QString &name) { return loadProfile(name); };
    hooks.saveProfile = [this](const QString &name) { return saveProfileAs(name); };
    // F6, the belt page: vocabulary from the drafting tool table, current
    // state from the mounted layout, and writes go through the same
    // arrangement derivation the default belt uses. The live belt re-dresses
    // in place (no remount): the checklist and the belt stay on screen
    // together, the same live-edit contract as theming.
    hooks.toolInventory = []() { return DraftingFeature::toolInventory(); };
    hooks.beltToolIds = [this]() {
        QStringList ids;
        for (const QString &id : m_workspaceLayout.belt.itemIds) {
            if (!id.isEmpty()) {
                ids.push_back(id);
            }
        }
        return ids;
    };
    hooks.setBeltToolIds = [this](const QStringList &enabledIds) {
        m_workspaceLayout.belt = DraftingFeature::beltLayoutForTools(enabledIds);
        if (m_draftingFeature != nullptr) {
            m_draftingFeature->refreshBelt(m_workspaceLayout.belt);
        }
    };
    return std::make_unique<SettingsFeature>(std::move(hooks));
}

void EdiShellWindow::mountWorkspace(const WorkspaceLayout &layout)
{
    m_workspaceLayout = layout;
    const std::vector<MountedSlot> mounted =
        mountWorkspaceLayout(m_workspaceLayout, m_featureRegistry, m_featureContext);

    // Geometry stays the window's job: slots have fixed places in the frame,
    // and an unbound slot simply isn't added. The left panel goes in-flow
    // beside the main area (it pushes everything, including the terminal);
    // main fills the whole area, right and bottom become overlays on it.
    m_leftPanelWidget = mountedSlotWidget(mounted, ShellSlot::Left);
    m_mainPanelWidget = mountedSlotWidget(mounted, ShellSlot::Main);
    m_rightPanelWidget = mountedSlotWidget(mounted, ShellSlot::Right);
    m_bottomPanelWidget = mountedSlotWidget(mounted, ShellSlot::Bottom);

    if (m_leftPanelWidget != nullptr) {
        m_bodySplitter->insertWidget(0, m_leftPanelWidget);
    }
    const auto adopt = [this](QWidget *widget) {
        if (widget != nullptr) {
            widget->setParent(m_mainArea);
            widget->show(); // manual parenting starts widgets hidden
        }
    };
    adopt(m_mainPanelWidget);
    adopt(m_rightPanelWidget);
    adopt(m_bottomPanelWidget);
    if (m_mainPanelWidget != nullptr) {
        m_mainPanelWidget->lower(); // the grid sits under every overlay
    }

    rebuildPalettes();
    rebuildChromePanels();
    rebuildSettingsWindowContent();

    applyPanelSizesToSplitters();
    refreshPanelVisibility();
    applyShellStyle(); // a freshly mounted canvas self-derives defaults; re-push the live theme
}

void EdiShellWindow::connectBeltPins(BeltCrossWidget *belt)
{
    connect(belt, &BeltCrossWidget::pinsChanged, this, [this, belt]() {
        m_workspaceLayout.belt.pinnedRows = belt->pinnedRows();
    });
}

void EdiShellWindow::rebuildPalettes()
{
    // Palettes are feature-level, not slot-level: any feature bound anywhere
    // in the layout may float its palettes over the main area. One palette
    // per spec id; placement comes from the layout (workspace data).
    for (const SlotBinding &binding : m_workspaceLayout.bindings) {
        const FeatureDescriptor *feature = findFeature(m_featureRegistry, binding.featureId);
        if (feature == nullptr || !feature->buildPalettes) {
            continue;
        }
        // A feature bound to several slots must still build its palettes
        // only once: skip feature ids an earlier binding already served.
        bool served = false;
        for (const SlotBinding &earlier : m_workspaceLayout.bindings) {
            if (&earlier == &binding) {
                break;
            }
            if (earlier.featureId == binding.featureId) {
                served = true;
                break;
            }
        }
        if (served) {
            continue;
        }
        for (const FeaturePaletteSpec &spec : feature->buildPalettes()) {
            if (spec.content == nullptr || spec.id.isEmpty()) {
                continue;
            }
            auto *palette = new FloatingPalette(spec.id, spec.title, spec.content, m_mainArea);
            connect(palette, &FloatingPalette::placementChanged, this,
                [this](const QString &paletteId, int x, int y) {
                    // The drag settled: remember the spot in the layout, so
                    // saveWorkspaceLayout writes it without extra capture.
                    setPalettePlacement(m_workspaceLayout, {paletteId, x, y});
                });
            // Pins are the same deal as placements: a user gesture on the
            // widget, remembered by the layout. Connected here, ON the fresh
            // palette content — a window-wide findChildren would miss the
            // first mount, which runs before setCentralWidget attaches the
            // body tree to this window.
            for (BeltCrossWidget *belt : spec.content->findChildren<BeltCrossWidget *>(QString(), Qt::FindChildrenRecursively)) {
                connectBeltPins(belt);
            }
            if (auto *contentBelt = qobject_cast<BeltCrossWidget *>(spec.content)) {
                connectBeltPins(contentBelt);
            }
            palette->show();
            palette->raise(); // above the canvas and the edge overlays
            m_palettes.push_back(palette);
        }
    }
    applyPalettePlacements();
}

void EdiShellWindow::applyPalettePlacements()
{
    for (FloatingPalette *palette : m_palettes) {
        // adjustSize first: a fresh palette has no geometry until shown, and
        // clamping against a 0x0 size would over-allow.
        palette->adjustSize();
        const PalettePlacement placement = palettePlacement(m_workspaceLayout, palette->paletteId());
        palette->applyPlacement(placement.x, placement.y);
    }
}

void EdiShellWindow::rebuildSettingsWindowContent()
{
    if (m_settingsWindow == nullptr || m_settingsFeature == nullptr) {
        return;
    }
    // The frame (and its visibility) survives the mount; only the page is
    // fresh. Settings stays open across workspace switches by design — the
    // whole point of the pop-out is that it floats over whatever job runs.
    m_settingsWindowContent = m_settingsFeature->buildPanel(ShellSlot::Main);
    if (m_settingsWindowContent != nullptr) {
        m_settingsWindow->layout()->addWidget(m_settingsWindowContent);
    }
}

void EdiShellWindow::openSettingsWindow()
{
    if (m_settingsWindow == nullptr) {
        return;
    }
    m_settingsWindow->show();
    m_settingsWindow->raise();
    m_settingsWindow->activateWindow();
}

void EdiShellWindow::rebuildChromePanels()
{
    if (m_chromePanelHost == nullptr) {
        return;
    }
    for (const SlotBinding &binding : m_workspaceLayout.bindings) {
        const FeatureDescriptor *feature = findFeature(m_featureRegistry, binding.featureId);
        if (feature == nullptr || !feature->buildChromePanels) {
            continue;
        }
        // One build per feature id even when it fills several slots.
        bool served = false;
        for (const SlotBinding &earlier : m_workspaceLayout.bindings) {
            if (&earlier == &binding) {
                break;
            }
            if (earlier.featureId == binding.featureId) {
                served = true;
                break;
            }
        }
        if (served) {
            continue;
        }
        for (const FeatureChromePanelSpec &spec : feature->buildChromePanels()) {
            if (spec.content == nullptr || spec.id.isEmpty()) {
                continue;
            }
            // Qt::Popup: clicking anywhere outside dismisses it — chrome
            // settings are a glance-and-tweak surface, not a dialog. The
            // frame stays a QObject child of the window so tests can reach
            // the controls inside without showing anything.
            auto *popup = new QFrame(this, Qt::Popup);
            popup->setObjectName(QStringLiteral("chromePopup_%1").arg(spec.id));
            // objectName is per-feature (tests address popups by id); the
            // property is the one stable hook the stylesheet can select on.
            popup->setProperty("chromePopup", true);
            auto *popupLayout = new QVBoxLayout(popup);
            popupLayout->setContentsMargins(12, 12, 12, 12);
            spec.content->setParent(popup);
            popupLayout->addWidget(spec.content);
            m_chromePopups.push_back(popup);

            auto *button = new QPushButton(spec.label, m_chromePanelHost);
            button->setObjectName(QStringLiteral("chromePanel_%1").arg(spec.id));
            connect(button, &QPushButton::clicked, this, [button, popup]() {
                popup->adjustSize();
                popup->move(button->mapToGlobal(QPoint(0, button->height() + 4)));
                popup->show();
            });
            m_chromePanelHost->layout()->addWidget(button);
            button->show();
        }
    }
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
    for (FloatingPalette *palette : m_palettes) {
        delete palette;
    }
    m_palettes.clear();
    for (QWidget *popup : m_chromePopups) {
        delete popup;
    }
    m_chromePopups.clear();
    // The pop-out's content must die BEFORE recreateInstance retires the
    // settings feature: its widgets call hooks owned by that instance.
    delete m_settingsWindowContent;
    m_settingsWindowContent = nullptr;
    if (m_chromePanelHost != nullptr) {
        // The buttons are children of the host strip; clearing them leaves
        // the strip itself in the bar for the next mount.
        for (QWidget *button : m_chromePanelHost->findChildren<QWidget *>(QString(), Qt::FindDirectChildrenOnly)) {
            delete button;
        }
    }
    retire(m_leftPanelWidget);
    retire(m_mainPanelWidget);
    retire(m_rightPanelWidget);
    retire(m_bottomPanelWidget);

    // Lifecycle is registry data: every feature rebuilds its instance, mounts,
    // then gets its shell-owned state re-fed — no feature is named here.
    for (const FeatureDescriptor &feature : m_featureRegistry.features) {
        if (feature.recreateInstance) {
            feature.recreateInstance();
        }
    }

    mountWorkspace(layout);

    for (const FeatureDescriptor &feature : m_featureRegistry.features) {
        if (feature.instanceMounted) {
            feature.instanceMounted();
        }
    }
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
        if (button == nullptr) {
            return;
        }
        const PanelVisibility visibility = shellPanelVisibility(slot);
        button->setChecked(visibility == PanelVisibility::Visible);
        // Tri-state for the sheet (spec §3): a collapsed panel and an
        // auto-hidden one must read differently (faint vs warning), and a
        // checked bool cannot carry three states — the property can.
        const QString stateName = visibility == PanelVisibility::Visible
            ? QStringLiteral("visible")
            : visibility == PanelVisibility::Collapsed ? QStringLiteral("collapsed")
                                                       : QStringLiteral("auto_hidden");
        if (button->property("panelState").toString() != stateName) {
            button->setProperty("panelState", stateName);
            button->style()->unpolish(button);
            button->style()->polish(button);
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
    // The main area re-lays its overlays whenever it changes size.
    if (watched == m_mainArea && event->type() == QEvent::Resize) {
        layoutMainArea();
        return false;
    }

    // Overlay grips: dragging moves the panel edge through the model, and
    // the geometry is recomputed from the model — same one-way flow as the
    // splitter. The press is claimed so nothing underneath starts a gesture.
    if (watched == m_rightGrip || watched == m_bottomGrip) {
        if (event->type() == QEvent::MouseButtonPress) {
            return true;
        }
        if (event->type() == QEvent::MouseMove) {
            auto *mouse = static_cast<QMouseEvent *>(event);
            if (mouse->buttons() & Qt::LeftButton) {
                const QPoint pos = m_mainArea->mapFromGlobal(mouse->globalPosition().toPoint());
                if (watched == m_rightGrip) {
                    m_panelsState.right.size =
                        clampPanelSize(ShellSlot::Right, m_mainArea->width() - pos.x());
                } else {
                    m_panelsState.bottom.size =
                        clampPanelSize(ShellSlot::Bottom, m_mainArea->height() - pos.y());
                }
                layoutMainArea();
                return true;
            }
        }
    }

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
    layoutMainArea(); // overlay geometry follows visibility
    refreshChrome();  // the toggle buttons mirror whatever just happened
}

void EdiShellWindow::applyPanelSizesToSplitters()
{
    // Only the left panel lives in the splitter; the main area takes the
    // remainder. Right/bottom geometry is overlay work — layoutMainArea.
    if (m_bodySplitter != nullptr && m_bodySplitter->indexOf(m_leftPanelWidget) >= 0) {
        const int left = m_panelsState.left.size;
        m_bodySplitter->setSizes({left, std::max(0, m_bodySplitter->width() - left)});
    }
    layoutMainArea();
}

void EdiShellWindow::capturePanelSizes()
{
    // Drag feedback flows one way: splitter -> clamp -> model. A hidden panel
    // reports size 0, which must not overwrite the stored size it will reopen
    // to — hence the visibility guard. (Right/bottom drags are captured by
    // the overlay grips in eventFilter, not here.)
    if (m_bodySplitter != nullptr && m_leftPanelWidget != nullptr && m_leftPanelWidget->isVisible()) {
        const int index = m_bodySplitter->indexOf(m_leftPanelWidget);
        if (index >= 0) {
            m_panelsState.left.size =
                clampPanelSize(ShellSlot::Left, m_bodySplitter->sizes().value(index));
        }
    }
}

void EdiShellWindow::layoutMainArea()
{
    // The grid always fills the whole main area — overlays cover it, they
    // never resize it (so zoom/board geometry stays put while panels come and
    // go). The bottom overlay spans the full width and may grow to the whole
    // area ("the terminal becomes the main window"); the right overlay stops
    // where the terminal starts. isVisibleTo() rather than isVisible() so the
    // layout is correct even before the window is first shown.
    if (m_mainArea == nullptr) {
        return;
    }
    const int width = m_mainArea->width();
    const int height = m_mainArea->height();
    const bool rightShown = m_rightPanelWidget != nullptr && m_rightPanelWidget->isVisibleTo(m_mainArea);
    const bool bottomShown = m_bottomPanelWidget != nullptr && m_bottomPanelWidget->isVisibleTo(m_mainArea);
    const int bottomHeight = bottomShown ? std::min(m_panelsState.bottom.size, height) : 0;
    const int rightWidth = rightShown ? std::min(m_panelsState.right.size, width) : 0;

    if (m_mainPanelWidget != nullptr) {
        m_mainPanelWidget->setGeometry(0, 0, width, height);
    }
    if (rightShown) {
        m_rightPanelWidget->setGeometry(width - rightWidth, 0, rightWidth, height - bottomHeight);
        m_rightPanelWidget->raise();
    }
    if (bottomShown) {
        m_bottomPanelWidget->setGeometry(0, height - bottomHeight, width, bottomHeight);
        m_bottomPanelWidget->raise();
    }
    // Grip offsets clamp to 0: when a panel fills the whole area (terminal at
    // full height), the grip must stay fully inside it and grabbable — at -4
    // half the hit zone would be clipped at exactly the moment it matters.
    if (m_rightGrip != nullptr) {
        m_rightGrip->setVisible(rightShown);
        if (rightShown) {
            m_rightGrip->setGeometry(std::max(0, width - rightWidth - 4), 0, 8, height - bottomHeight);
            m_rightGrip->raise();
        }
    }
    if (m_bottomGrip != nullptr) {
        m_bottomGrip->setVisible(bottomShown);
        if (bottomShown) {
            m_bottomGrip->setGeometry(0, std::max(0, height - bottomHeight - 4), width, 8);
            m_bottomGrip->raise();
        }
    }
    // Palettes re-clamp against the new area and stay above the overlays —
    // they float over everything in the main area by design.
    for (FloatingPalette *palette : m_palettes) {
        const PalettePlacement placement = palettePlacement(m_workspaceLayout, palette->paletteId());
        palette->applyPlacement(placement.x, placement.y);
        palette->raise();
    }
}
