#pragma once

#include <QMainWindow>
#include <QString>
#include <QStringList>

#include <functional>
#include <memory>
#include <vector>

#include "app/AppState.h"
#include "io/SettingsStore.h"
#include "widgets/ShellHost.h"
#include "widgets/ShellPanels.h"
#include "widgets/ShellTheme.h"

class QAction;
class QMenu;
class QSplitter;

class QTimer;

class QButtonGroup;
class BeltCrossWidget;
class QLabel;
class QPushButton;
class QWidget;

class DraftingFeature;
class DrawingDocumentController;
class FloatingPalette;
class RecipeController;
class SettingsFeature;

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
    bool exportGcodeToPath(const QString &path);
    QString currentDrawingPath() const { return m_currentDrawingPath; }
    bool isDocumentDirty() const;

    // #18 close-without-saving guard (user decision 2026-06-11: modal
    // confirm). The dialog is a callable — variation point as data — so
    // offscreen tests install a canned choice instead of a modal that would
    // hang the harness; production leaves it unset and gets the QMessageBox.
    // resolveDirtyGuard() is the policy: clean documents pass, Save proceeds
    // only if the save actually landed (save-as can be cancelled), Cancel
    // blocks the action.
    enum class DirtyGuardChoice { Save, Discard, Cancel };
    void setDirtyGuardPrompt(std::function<DirtyGuardChoice()> prompt);
    bool resolveDirtyGuard();
    // The same data-not-dialog treatment for the two modals the Save choice
    // can reach: the save-as file picker (a never-saved document) and the
    // write-failure warning. Unset = real dialog; tests inject canned values
    // so answering "Save" through the guard can never hang an offscreen run.
    void setSaveAsPathProvider(std::function<QString()> provider);
    void setSaveFailedNotice(std::function<void(const QString &path)> notice);
    // The guarded open every user-facing entry uses (menu, shortcut, recent
    // files); openDrawingFromPath above stays the unguarded mechanism.
    bool openDrawingFromPathGuarded(const QString &path);

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

    // Live theming: the four palette inputs + fonts. Setting them re-derives
    // the whole token set and restyles shell QSS and canvas chrome instantly;
    // they persist as theme.* keys in edi.toml.
    edi::shell::ShellThemeInputs themeInputs() const { return m_themeInputs; }
    void setThemeInputs(const edi::shell::ShellThemeInputs &inputs);

    // Profiles: named theme bundles, one TOML file each under the profiles
    // directory (injectable for tests). edi.toml remembers the active name;
    // the live theme always persists in edi.toml regardless, so deleting a
    // profile file never breaks startup.
    QStringList availableProfiles() const;
    QString activeProfile() const { return m_activeProfile; }
    bool saveProfileAs(const QString &name);
    bool loadProfile(const QString &name);
    void setProfilesDirectory(const QString &dir) { m_profilesDir = dir; }

protected:
    void closeEvent(QCloseEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    void promptSaveDrawing();
    void promptSaveDrawingAs();
    void promptOpenDrawing();
    DirtyGuardChoice promptDirtyGuardChoice();
    void promptExportSvg();
    void promptExportHpgl();
    void promptExportGcode();
    // One funnel for "which document, how clean": window title AND the
    // status-bar file label (with its dirty recoloring) update together, so
    // they can never disagree. Connected to modelChanged.
    void refreshDocumentStatus();
    edi::formats::StaticConfig captureSettings() const;
    void applySettings(const edi::formats::StaticConfig &config);
    void rememberRecentFile(const QString &path);
    void scheduleSettingsSave();

private:
    QWidget *buildActivityRail();
    QWidget *buildTitleBar();
    QWidget *buildStatusBar();
    void setWorkspaceMode(edi::app::WorkspaceMode mode);
    void applyShellStyle();
    void refreshPanelVisibility();
    void applyPanelSizesToSplitters();
    void capturePanelSizes();
    void refreshChrome();
    void layoutMainArea();
    // F4: destroy-and-rebuild the floating palettes from the registry and the
    // mounted layout (same lifecycle as slot widgets), and re-apply stored
    // placements (clamped against the live main area).
    void connectBeltPins(BeltCrossWidget *belt);
    void rebuildPalettes();
    void applyPalettePlacements();
    // Chrome panels: per-feature popup settings behind title-bar buttons
    // (e.g. drafting's Snap panel). Same destroy-and-rebuild lifecycle as
    // palettes; the buttons live in m_chromePanelHost inside the title bar.
    void rebuildChromePanels();
    // The File menu's Open Recent submenu, rebuilt whenever m_recentFiles
    // changes (eager, not aboutToShow: deterministic for tests and cheap).
    void rebuildRecentFilesMenu();
    void refreshUndoRedoActions();
    // F5: the settings pop-out — a modeless tool window over the canvas, so
    // theme edits are visible live. The frame is window-owned and permanent;
    // its content is rebuilt each workspace mount like every feature widget.
    void rebuildSettingsWindowContent();
    void openSettingsWindow();
    std::unique_ptr<DraftingFeature> createDraftingFeature();
    std::unique_ptr<SettingsFeature> createSettingsFeature();
    void mountWorkspace(const edi::shell::WorkspaceLayout &layout);
    // The switch mechanics without touching history — back/forward replay
    // history entries through this.
    void applyWorkspaceLayout(const edi::shell::WorkspaceLayout &layout);
    void navigateWorkspaceHistory(int delta);

    edi::app::AppState m_appState;
    edi::shell::ShellThemeInputs m_themeInputs;
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
    // The main area hosts the canvas at full size; right and bottom panels
    // are overlays positioned on top of it (user decision 2026-06-10: panels
    // cover the grid, they never squeeze it). Only the left panel is in-flow.
    QWidget *m_mainArea = nullptr;
    QWidget *m_rightGrip = nullptr;
    QWidget *m_bottomGrip = nullptr;
    std::vector<FloatingPalette *> m_palettes; // children of m_mainArea
    RecipeController *m_recipeController = nullptr;
    QWidget *m_chromePanelHost = nullptr;      // title-bar strip for feature buttons
    QMenu *m_recentFilesMenu = nullptr;
    QWidget *m_settingsWindow = nullptr;        // the pop-out frame (persistent)
    QWidget *m_settingsWindowContent = nullptr; // rebuilt per workspace mount
    QAction *m_undoAction = nullptr;
    QAction *m_redoAction = nullptr;
    std::vector<QWidget *> m_chromePopups;     // popup frames, window children
    QWidget *m_leftPanelWidget = nullptr;
    QWidget *m_mainPanelWidget = nullptr;
    QWidget *m_rightPanelWidget = nullptr;
    QWidget *m_bottomPanelWidget = nullptr;
    DrawingDocumentController *m_controller = nullptr;
    QButtonGroup *m_activityGroup = nullptr;
    QWidget *m_titleBar = nullptr;
    // Status bar (spec §2/§3: 28px strip under the body): the feature's
    // published mode line on the left, file + dirty marker on the right.
    QLabel *m_statusModeLabel = nullptr;
    QLabel *m_statusFileLabel = nullptr;
    QPushButton *m_backButton = nullptr;
    QPushButton *m_forwardButton = nullptr;
    QPushButton *m_toggleLeftButton = nullptr;
    QPushButton *m_toggleBottomButton = nullptr;
    QPushButton *m_toggleRightButton = nullptr;
    // Feature instances: lifetimes match their widgets — both are recreated
    // on every workspace switch so no member pointer can dangle.
    std::unique_ptr<DraftingFeature> m_draftingFeature;
    std::unique_ptr<SettingsFeature> m_settingsFeature;
    QString m_currentDrawingPath;
    // Unset = use the modal QMessageBox; tests inject a canned answer.
    std::function<DirtyGuardChoice()> m_dirtyGuardPrompt;
    std::function<QString()> m_saveAsPathProvider;
    std::function<void(const QString &path)> m_saveFailedNotice;
    QString m_settingsPath;
    QString m_profilesDir;
    QString m_activeProfile;
    QStringList m_recentFiles;
    QTimer *m_settingsSaveTimer = nullptr;
};
