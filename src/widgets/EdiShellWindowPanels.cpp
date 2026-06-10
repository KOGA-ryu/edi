#include "widgets/EdiShellWindow.h"

#include <QAbstractButton>
#include <QButtonGroup>
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
        if (mode) {
            setWorkspaceMode(*mode);
        }
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
        connect(button, &QPushButton::clicked, this, onClick);
        layout->addWidget(button);
    };
    addTraffic(QStringLiteral("trafficClose"), QStringLiteral("Close window"), [this]() { close(); });
    addTraffic(QStringLiteral("trafficMinimize"), QStringLiteral("Minimize window"), [this]() { showMinimized(); });
    addTraffic(QStringLiteral("trafficZoom"), QStringLiteral("Zoom window"), [this]() {
        isMaximized() ? showNormal() : showMaximized();
    });
    layout->addSpacing(8);

    const auto addPanelToggle = [&](const QString &name, const QString &glyph, const QString &tooltip, ShellSlot slot) {
        auto *button = new QPushButton(glyph);
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
    m_toggleLeftButton = addPanelToggle(QStringLiteral("toggleLeftPanel"), QStringLiteral("◧"),
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
    fileMenu->addAction(QStringLiteral("Save"), this, [this]() { promptSaveDrawing(); });
    fileMenu->addAction(QStringLiteral("Save As…"), this, [this]() { promptSaveDrawingAs(); });
    fileMenu->addSeparator();
    fileMenu->addAction(QStringLiteral("Export SVG…"), this, [this]() { promptExportSvg(); });
    fileMenu->addAction(QStringLiteral("Export HPGL…"), this, [this]() { promptExportHpgl(); });

    QMenu *editMenu = addMenuButton(QStringLiteral("Edit"), QStringLiteral("editMenu"));
    editMenu->addAction(QStringLiteral("Undo"), this, [this]() { m_controller->undo(); });
    editMenu->addAction(QStringLiteral("Redo"), this, [this]() { m_controller->redo(); });

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

    layout->addStretch(1); // the drag region
    // The status line the drafting feature publishes (was the workspace
    // header's job; the header is gone — the grid speaks for itself).
    m_chromeStatus = new QLabel;
    m_chromeStatus->setObjectName(QStringLiteral("chromeStatus"));
    layout->addWidget(m_chromeStatus);
    layout->addStretch(1);

    m_toggleBottomButton = addPanelToggle(QStringLiteral("toggleBottomPanel"), QStringLiteral("⬓"),
        QStringLiteral("Toggle terminal"), ShellSlot::Bottom);
    m_toggleRightButton = addPanelToggle(QStringLiteral("toggleRightPanel"), QStringLiteral("◨"),
        QStringLiteral("Toggle right panel"), ShellSlot::Right);

    // Drag-to-move and double-click-to-zoom land on the bar itself; presses
    // on child buttons never reach this filter.
    bar->installEventFilter(this);
    m_titleBar = bar;
    return bar;
}
