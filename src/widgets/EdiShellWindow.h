#pragma once

#include <QMainWindow>
#include <QString>
#include <QStringList>

#include <memory>

#include "app/AppState.h"
#include "io/SettingsStore.h"
#include "widgets/ShellHost.h"
#include "widgets/ShellPanels.h"

class QSplitter;

class QTimer;

class QButtonGroup;
class QPushButton;
class QWidget;

class DraftingFeature;
class DrawingDocumentController;

class EdiShellWindow final : public QMainWindow {
    Q_OBJECT

public:
    explicit EdiShellWindow(QWidget *parent = nullptr);
    // Out-of-line so the unique_ptr<DraftingFeature> member can delete a type
    // that is only forward-declared here (the .cpp sees the full definition).
    ~EdiShellWindow() override;

    // Path-based drawing I/O seams (the toolbar/shortcut handlers wrap these in
    // QFileDialog; tests drive them directly to avoid modal dialogs).
    bool saveDrawingToPath(const QString &path);
    bool openDrawingFromPath(const QString &path);
    bool exportSvgToPath(const QString &path);
    bool exportHpglToPath(const QString &path);
    QString currentDrawingPath() const { return m_currentDrawingPath; }
    bool isDocumentDirty() const;

    // Settings persistence seams (TOML at path; tests inject a temp path).
    bool loadSettings(const QString &path);
    bool saveSettings(const QString &path) const;
    QStringList recentFiles() const { return m_recentFiles; }

    // Panel system (spec §2): collapse, presets, and auto-hide all funnel
    // through the pure ShellPanels model; these are the window-side verbs.
    // Public so tests (and, in H4, the title-bar toggles) can drive them.
    void setPanelCollapsed(edi::shell::ShellSlot slot, bool collapsed);
    void applyShellPanelPreset(edi::shell::PanelPreset preset);
    edi::shell::PanelVisibility shellPanelVisibility(edi::shell::ShellSlot slot) const;

    // Workspace layout persistence seams (TOML at path; tests inject a temp
    // path, main() passes defaultWorkspaceLayoutPath()). Load applies panel
    // geometry AND bindings (switching workspaces if they differ).
    bool loadWorkspaceLayout(const QString &path);
    bool saveWorkspaceLayout(const QString &path) const;

    // Tear down the mounted slots and rebuild them from a different layout.
    // The document is untouched — only the glass around it changes. Pushes
    // onto the workspace history (the chrome's back/forward buttons).
    void switchWorkspaceLayout(const edi::shell::WorkspaceLayout &layout);

protected:
    void closeEvent(QCloseEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    void promptSaveDrawing();
    void promptSaveDrawingAs();
    void promptOpenDrawing();
    void promptExportSvg();
    void promptExportHpgl();
    void updateWindowTitle();
    edi::formats::StaticConfig captureSettings() const;
    void applySettings(const edi::formats::StaticConfig &config);
    void rememberRecentFile(const QString &path);
    void scheduleSettingsSave();

private:
    QWidget *buildActivityRail();
    QWidget *buildTitleBar();
    void setWorkspaceMode(edi::app::WorkspaceMode mode);
    void applyShellStyle();
    void refreshPanelVisibility();
    void applyPanelSizesToSplitters();
    void capturePanelSizes();
    void refreshChrome();
    std::unique_ptr<DraftingFeature> createDraftingFeature();
    void mountWorkspace(const edi::shell::WorkspaceLayout &layout);
    // The switch mechanics without touching history — back/forward replay
    // history entries through this.
    void applyWorkspaceLayout(const edi::shell::WorkspaceLayout &layout);
    void navigateWorkspaceHistory(int delta);

    edi::app::AppState m_appState;
    // The cross-feature bus (docs/shell_architecture.md). Owned here so it
    // outlives every mounted feature widget.
    edi::shell::FeatureContext m_featureContext;
    // Panel state lives in the pure model; the splitters and visibility flags
    // are projections of it, never the other way around.
    edi::shell::ShellPanelsState m_panelsState;
    edi::shell::FeatureRegistry m_featureRegistry;
    edi::shell::WorkspaceLayout m_workspaceLayout;
    QString m_workspaceLayoutPath;
    // The "rabbit hole" trail: each switch pushes here, back/forward replay.
    // Plain data — a future detail-level drill-down is just more entries.
    std::vector<edi::shell::WorkspaceLayout> m_workspaceHistory;
    int m_workspaceHistoryIndex = -1;
    // Frameless custom chrome; flip to false for a native frame when
    // debugging window-manager weirdness (the title bar stays either way).
    bool m_framelessChrome = true;
    QSplitter *m_bodySplitter = nullptr;
    QSplitter *m_rootSplitter = nullptr;
    QWidget *m_leftPanelWidget = nullptr;
    QWidget *m_mainPanelWidget = nullptr;
    QWidget *m_rightPanelWidget = nullptr;
    QWidget *m_bottomPanelWidget = nullptr;
    DrawingDocumentController *m_controller = nullptr;
    QButtonGroup *m_activityGroup = nullptr;
    QWidget *m_titleBar = nullptr;
    QPushButton *m_backButton = nullptr;
    QPushButton *m_forwardButton = nullptr;
    QPushButton *m_toggleLeftButton = nullptr;
    QPushButton *m_toggleBottomButton = nullptr;
    QPushButton *m_toggleRightButton = nullptr;
    // The drafting workspace as a feature object: it owns the drafting panel
    // widgets' wiring and the inspector refresh, so its lifetime can later
    // match its widgets when workspace switching lands.
    std::unique_ptr<DraftingFeature> m_draftingFeature;
    QString m_currentDrawingPath;
    QString m_settingsPath;
    QStringList m_recentFiles;
    QTimer *m_settingsSaveTimer = nullptr;
};
