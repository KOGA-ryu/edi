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
#include "widgets/ShellTheme.h"
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

    // The window no longer hard-wires drafting panels into regions: drafting
    // is feature #1 in a registry, and which feature fills which slot is data
    // (the layout). Registry and layout are constructor locals until H5 makes
    // layouts persistent/switchable; the context is a member because features
    // may hold onto the bus for as long as their widgets live.
    m_featureContext.drawingController = m_controller;

    FeatureDescriptor drafting;
    drafting.id = QStringLiteral("drafting");
    drafting.label = QStringLiteral("Drafting");
    drafting.supportedSlots = {ShellSlot::Main, ShellSlot::Left, ShellSlot::Right, ShellSlot::Bottom};
    // The panels read this window's members, so the factory captures `this`
    // rather than going through the context; a standalone feature module would
    // take everything from the context instead.
    drafting.buildPanel = [this](ShellSlot slot, FeatureContext &) -> QWidget * {
        switch (slot) {
        case ShellSlot::Main: return buildWorkspaceColumn();
        case ShellSlot::Left: return buildLeftPanel();
        case ShellSlot::Right: return buildRightPanel();
        case ShellSlot::Bottom: return buildBottomPanel();
        }
        return nullptr;
    };

    FeatureRegistry registry;
    registry.features.push_back(drafting);

    WorkspaceLayout layout;
    layout.id = QStringLiteral("drafting");
    layout.label = QStringLiteral("Drafting");
    layout.bindings = {
        {ShellSlot::Left, QStringLiteral("drafting")},
        {ShellSlot::Main, QStringLiteral("drafting")},
        {ShellSlot::Right, QStringLiteral("drafting")},
        {ShellSlot::Bottom, QStringLiteral("drafting")},
    };

    const std::vector<MountedSlot> mounted = mountWorkspaceLayout(layout, registry, m_featureContext);

    // Geometry stays the window's job: slots have fixed places in the frame,
    // and an unbound slot simply isn't added.
    bodyLayout->addWidget(buildActivityRail());
    if (QWidget *left = mountedSlotWidget(mounted, ShellSlot::Left)) {
        bodyLayout->addWidget(left);
    }
    if (QWidget *main = mountedSlotWidget(mounted, ShellSlot::Main)) {
        bodyLayout->addWidget(main, 1);
    }
    if (QWidget *right = mountedSlotWidget(mounted, ShellSlot::Right)) {
        bodyLayout->addWidget(right);
    }

    root->addWidget(body, 1);
    if (QWidget *bottom = mountedSlotWidget(mounted, ShellSlot::Bottom)) {
        root->addWidget(bottom);
    }

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

void EdiShellWindow::applyShellStyle()
{
    // The palette lives in ShellTheme as data; this method only asks for the
    // default inputs and applies the derived sheet. Custom themes later become
    // "construct different inputs here" — no QSS edits.
    setStyleSheet(buildShellStyleSheet(deriveShellTheme(ShellThemeInputs{})));
}
