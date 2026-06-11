#include "widgets/EdiShellWindow.h"

#include <QAbstractButton>
#include <QButtonGroup>
#include <QFileInfo>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QMenu>
#include <QPushButton>
#include <QVBoxLayout>

#include <functional>

#include "core/DrawingCore.h"
#include "widgets/ShellWidgetHelpers.h"

using namespace edi::shell;

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
        if (!mode) {
            return;
        }
        // F5: Settings is a pop-out, not a workspace. Open it and hand the
        // exclusive check back to the mounted mode's button — the rail shows
        // where you ARE, and you are still in the drafting job.
        if (*mode == edi::app::WorkspaceMode::Settings) {
            openSettingsWindow();
            for (QAbstractButton *railButton : m_activityGroup->buttons()) {
                railButton->setChecked(railButton->property("modeId").toString().toStdString()
                    == edi::app::workspaceModeName(m_appState.mode));
            }
            return;
        }
        setWorkspaceMode(*mode);
    });

    layout->addStretch(1);
    layout->addWidget(makeRailButton(QStringLiteral("+"), QStringLiteral("Reserved add action"), false, false));
    layout->addWidget(makeRailButton(QStringLiteral("?"), QStringLiteral("Help and docs"), false, false));

    return rail;
}

QWidget *EdiShellWindow::buildTitleBar()
{
    // The chrome row, exactly as the user described the legacy bar: traffic
    // lights, left-panel collapse, back/forward (workspace history — the
    // "rabbit hole" trail, NOT undo/redo), File/Edit/Settings, then on the
    // right the terminal (bottom) and right-panel collapse toggles.
    auto *bar = makeRegionFrame(QStringLiteral("titleBar"));
    bar->setFixedHeight(42);

    auto *layout = new QHBoxLayout(bar);
    layout->setContentsMargins(10, 0, 10, 0);
    layout->setSpacing(6);

    const auto addTraffic = [&](const QString &name, const QString &tooltip, const std::function<void()> &onClick) {
        auto *button = new QPushButton;
        button->setObjectName(name);
        button->setToolTip(tooltip);
        // Size comes from the QSS traffic rule (12px content + 1px border =
        // the 14px dot): QStyleSheetStyle::polish enforces stylesheet
        // geometry as the widget's min/max, so a setFixedSize here would be
        // silently overwritten — one source of truth, and it is the sheet.
        connect(button, &QPushButton::clicked, this, onClick);
        layout->addWidget(button);
    };
    addTraffic(QStringLiteral("trafficClose"), QStringLiteral("Close window"), [this]() { close(); });
    addTraffic(QStringLiteral("trafficMinimize"), QStringLiteral("Minimize window"), [this]() { showMinimized(); });
    addTraffic(QStringLiteral("trafficZoom"), QStringLiteral("Zoom window"), [this]() {
        isMaximized() ? showNormal() : showMaximized();
    });
    layout->addSpacing(8);

    const auto addPanelToggle = [&](const QString &name, const QString &tooltip, ShellSlot slot) {
        // Face-only button: refreshChrome paints the spec's frame+bar icon
        // (panelToggleFace) on every state projection; no text glyph.
        auto *button = new QPushButton;
        button->setObjectName(name);
        button->setToolTip(tooltip);
        button->setCheckable(true);
        connect(button, &QPushButton::clicked, this, [this, slot]() {
            // Toggle the user's intent: visible -> collapse, anything else ->
            // ask for it back (an auto-hidden panel returns when room exists).
            setPanelCollapsed(slot, shellPanelVisibility(slot) == PanelVisibility::Visible);
        });
        layout->addWidget(button);
        return button;
    };
    m_toggleLeftButton = addPanelToggle(QStringLiteral("toggleLeftPanel"),
        QStringLiteral("Toggle left panel"), ShellSlot::Left);

    m_backButton = new QPushButton(QStringLiteral("‹"));
    m_backButton->setObjectName(QStringLiteral("workspaceBack"));
    m_backButton->setToolTip(QStringLiteral("Back (workspace history)"));
    connect(m_backButton, &QPushButton::clicked, this, [this]() { navigateWorkspaceHistory(-1); });
    layout->addWidget(m_backButton);

    m_forwardButton = new QPushButton(QStringLiteral("›"));
    m_forwardButton->setObjectName(QStringLiteral("workspaceForward"));
    m_forwardButton->setToolTip(QStringLiteral("Forward (workspace history)"));
    connect(m_forwardButton, &QPushButton::clicked, this, [this]() { navigateWorkspaceHistory(+1); });
    layout->addWidget(m_forwardButton);

    const auto addMenuButton = [&](const QString &label, const QString &menuName) {
        auto *button = new QPushButton(label);
        button->setObjectName(QStringLiteral("chromeMenuButton"));
        auto *menu = new QMenu(button);
        menu->setObjectName(menuName);
        button->setMenu(menu);
        layout->addWidget(button);
        return menu;
    };
    QMenu *fileMenu = addMenuButton(QStringLiteral("File"), QStringLiteral("fileMenu"));
    fileMenu->addAction(QStringLiteral("Open…"), this, [this]() { promptOpenDrawing(); });
    // Quick-open recents moved here from the left panel; rebuilt eagerly by
    // rebuildRecentFilesMenu whenever the list changes.
    m_recentFilesMenu = fileMenu->addMenu(QStringLiteral("Open Recent"));
    m_recentFilesMenu->setObjectName(QStringLiteral("recentFilesMenu"));
    fileMenu->addAction(QStringLiteral("Save"), this, [this]() { promptSaveDrawing(); });
    fileMenu->addAction(QStringLiteral("Save As…"), this, [this]() { promptSaveDrawingAs(); });
    fileMenu->addSeparator();
    fileMenu->addAction(QStringLiteral("Export SVG…"), this, [this]() { promptExportSvg(); });
    fileMenu->addAction(QStringLiteral("Export HPGL…"), this, [this]() { promptExportHpgl(); });
    fileMenu->addAction(QStringLiteral("Export G-code…"), this, [this]() { promptExportGcode(); });
    fileMenu->addSeparator();
    fileMenu->addAction(QStringLiteral("Open Recipe…"), this, [this]() { promptOpenRecipe(); });
    fileMenu->addAction(QStringLiteral("Save Recipe…"), this, [this]() { promptSaveRecipe(); });
    fileMenu->addAction(QStringLiteral("Export Blender Python…"), this, [this]() { promptExportRecipePython(); });
    rebuildRecentFilesMenu();

    QMenu *editMenu = addMenuButton(QStringLiteral("Edit"), QStringLiteral("editMenu"));
    m_undoAction = editMenu->addAction(QStringLiteral("Undo"), this, [this]() { m_controller->undo(); });
    m_undoAction->setObjectName(QStringLiteral("undoAction"));
    m_redoAction = editMenu->addAction(QStringLiteral("Redo"), this, [this]() { m_controller->redo(); });
    m_redoAction->setObjectName(QStringLiteral("redoAction"));
    // Enabled-state follows the undo stacks on every model change — a menu
    // action that pretends it can undo is chrome lying about the document.
    connect(m_controller, &DrawingDocumentController::modelChanged, this, [this]() { refreshUndoRedoActions(); });
    refreshUndoRedoActions();

    // Settings holds the layout presets for now — their first UI trigger;
    // app-settings entries arrive when there is something to configure.
    QMenu *settingsMenu = addMenuButton(QStringLiteral("Settings"), QStringLiteral("settingsMenu"));
    settingsMenu->addAction(QStringLiteral("Full layout"), this, [this]() { applyShellPanelPreset(PanelPreset::Full); });
    settingsMenu->addAction(QStringLiteral("Focus layout"), this, [this]() { applyShellPanelPreset(PanelPreset::Focus); });
    settingsMenu->addAction(QStringLiteral("Review layout"), this, [this]() { applyShellPanelPreset(PanelPreset::Review); });

    // Feature-supplied chrome buttons mount here (rebuildChromePanels): the
    // strip keeps its place in the bar while workspaces swap its children.
    m_chromePanelHost = new QWidget;
    m_chromePanelHost->setObjectName(QStringLiteral("chromePanelHost"));
    auto *chromePanelLayout = new QHBoxLayout(m_chromePanelHost);
    chromePanelLayout->setContentsMargins(0, 0, 0, 0);
    chromePanelLayout->setSpacing(6);
    layout->addWidget(m_chromePanelHost);

    // The drag region. Nothing else lives here: the user's bar inventory is
    // traffic lights, collapse toggles, history, menus, Snap — "that's it".
    // (The status line that used to squat here moved to the spec'd 28px
    // status bar; see buildStatusBar.)
    layout->addStretch(1);

    m_toggleBottomButton = addPanelToggle(QStringLiteral("toggleBottomPanel"),
        QStringLiteral("Toggle terminal"), ShellSlot::Bottom);
    m_toggleRightButton = addPanelToggle(QStringLiteral("toggleRightPanel"),
        QStringLiteral("Toggle right panel"), ShellSlot::Right);

    // Drag-to-move and double-click-to-zoom land on the bar itself; presses
    // on child buttons never reach this filter.
    bar->installEventFilter(this);
    m_titleBar = bar;
    return bar;
}

QWidget *EdiShellWindow::buildStatusBar()
{
    // Spec §2/§3: a 28px strip under the body. Left: the mode line the
    // active feature publishes (selection counts, tool). Right: which
    // document and how clean — the dirty marker recolors to the warning
    // token via a dynamic property, so state lives in data and the color
    // lives in the sheet.
    auto *bar = makeRegionFrame(QStringLiteral("statusBar"));
    bar->setFixedHeight(28);
    auto *layout = new QHBoxLayout(bar);
    layout->setContentsMargins(10, 0, 10, 0);
    layout->setSpacing(12);

    m_statusModeLabel = new QLabel;
    m_statusModeLabel->setObjectName(QStringLiteral("statusMode"));
    layout->addWidget(m_statusModeLabel);
    layout->addStretch(1);
    m_statusFileLabel = new QLabel;
    m_statusFileLabel->setObjectName(QStringLiteral("statusFile"));
    layout->addWidget(m_statusFileLabel);
    return bar;
}

void EdiShellWindow::rebuildRecentFilesMenu()
{
    if (m_recentFilesMenu == nullptr) {
        return;
    }
    m_recentFilesMenu->clear();
    const int shown = qMin(m_recentFiles.size(), 5);
    for (int i = 0; i < shown; ++i) {
        const QString path = m_recentFiles.at(i);
        QAction *action = m_recentFilesMenu->addAction(QFileInfo(path).fileName(), this, [this, path]() {
            openDrawingFromPathGuarded(path);
        });
        action->setObjectName(QStringLiteral("recentFileAction"));
        action->setToolTip(path);
        action->setData(path);
    }
    if (shown == 0) {
        QAction *empty = m_recentFilesMenu->addAction(QStringLiteral("No recent files"));
        empty->setEnabled(false);
    }
}

void EdiShellWindow::refreshUndoRedoActions()
{
    if (m_undoAction != nullptr) {
        m_undoAction->setEnabled(m_controller->canUndo());
    }
    if (m_redoAction != nullptr) {
        m_redoAction->setEnabled(m_controller->canRedo());
    }
}
