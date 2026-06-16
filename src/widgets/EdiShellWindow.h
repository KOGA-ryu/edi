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

#include "recipe/RecipeOps.h" // the op pipeline's stream, held for its verbs (R1-B05)
#include "text/TextDocumentStore.h" // the editor's documents, window-owned (E1)
#include "io/ProcessRunStore.h" // the Blender lab's subprocess seam
#include "scripting/BlenderRunPlan.h" // the pure plan the runner executes

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
    // The op pipeline's verbs (R1-B05; the ONLY recipe pipeline since A's
    // retirement in R1-B06). Open/Save are strict load/store with the
    // reader's named refusals; Export Resolved bakes against the LIVE
    // drafting document — exact measurements at the moment of export,
    // listing EVERY stale binding at once on refusal (never a guess);
    // Export Ops Previews resolves -> compiles -> validates -> renders the
    // three projections. All share m_lastRecipeError, so the chrome
    // surfaces one line.
    bool openOpsRecipeFromPath(const QString &path);
    bool saveOpsRecipeToPath(const QString &path);
    bool exportResolvedOpsToPath(const QString &path);
    bool exportOpsPreviewsToDir(const QString &dir);
    // E1 test seam: shell tests assert the document model, not widget text.
    edi::text::TextDocumentStore &textDocumentStore() { return m_textStore; }
    // E4 test seam: assert what the pipeline holds after Apply.
    const edi::recipe::RecipeOpStream &opsStream() const { return m_opsStream; }
    // E4: the strict-reader hook behind the editor's Apply button — parses
    // the script text, replaces the op stream, echoes canonical TOML back.
    QString applyOpsScript(const std::string &text);
    // The human op inspector's edit verb: set one editable double field of the
    // op at opIndex, then re-sync the TOML view (for the AI) and re-render the
    // proof/compiled. A no-op if the index or field is invalid. Public so the
    // inspector's spinboxes and tests drive it the same way.
    void applyOpFieldEdit(int opIndex, const QString &fieldKey, double value);
    // The palette's append verb: add a new step of the named op type (a unit
    // primitive) to the recipe, then sync + re-render. A no-op for a type the
    // palette does not offer. Public so the palette buttons and tests share it.
    void appendRecipeOp(const QString &typeName);
    QString lastRecipeError() const { return m_lastRecipeError; }
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
    // Workspace test seam: which job is mounted right now. The rail's
    // mode->layout switch is otherwise only observable through the panels,
    // and sibling jobs (Drafting/Map) share bindings — so tests read the id.
    QString currentWorkspaceId() const { return m_workspaceLayout.id; }

    // The text editor SESSION (E2): which documents are open and which is
    // active, restored across restarts. loadTextSession remembers the path (the
    // session writes itself there on close, beside the layout) and re-projects
    // the panel; saveTextSession writes the manifest. main() passes
    // defaultTextSessionPath().
    bool loadTextSession(const QString &path);
    bool saveTextSession(const QString &path) const;
    // The text editor's path chooser, injectable for offscreen tests (the
    // drawing Save As precedent). forSave picks save vs open; empty = cancel.
    void setTextEditorPathProvider(std::function<QString(bool forSave)> provider);
    // The Blender executable edi spawns for Build (settings: blender.executable_path).
    void setBlenderExecutablePath(const QString &path);
    QString blenderExecutablePath() const { return m_blenderExecutablePath; }
    // The Build runner, injectable for offscreen tests: the default spawns
    // Blender via ProcessRunStore; a test stub records the plan and never spawns.
    void setBlenderRunner(std::function<void(const edi::scripting::BlenderRunPlan &)> runner);
    // Load a rendered image into the Blender profile's preview pane (the app's
    // first raster surface). Remembered so a workspace remount re-shows it. The
    // build's async-finished handler calls this; a test calls it directly.
    void showRenderImage(const QString &imagePath);

    // Tear down the mounted slots and rebuild them from a different layout.
    // The document is untouched — only the glass around it changes. Pushes
    // onto the workspace history (the chrome's back/forward buttons).
    void switchWorkspaceLayout(const edi::shell::WorkspaceLayout &layout);
    // The rail's mode->layout verb: map a WorkspaceMode to its job and switch
    // to it (Settings is intercepted earlier as a pop-out). Public so tests can
    // drive the mode switch the rail performs, like switchWorkspaceLayout above.
    void setWorkspaceMode(edi::app::WorkspaceMode mode);

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
    // E2: seed one Scratch document only when the store is empty — the
    // constructor calls it (fresh window), and so does loadTextSession after an
    // empty manifest, so the terminal is never blank.
    void seedScratchIfEmpty();
    // The editor's Build hook: write the script to a temp .py, plan a Blender
    // render, hand it to the runner. Returns the immediate message (refusal or
    // a "building…" ack). onBlenderRunFinished surfaces the async outcome.
    QString buildBlenderScript(const QString &scriptText, const QString &scriptPath);
    void onBlenderRunFinished(const ProcessRunResult &result);
    // The Blender profile's render-preview pane (registry feature #4, Right slot):
    // a QLabel that shows m_lastRenderImagePath, rebuilt fresh per mount.
    QWidget *buildBlenderPreviewPanel();
    // The Map profile's graph browser (registry feature #5, Right slot): a
    // read-only list of the document's rooms/connections/plugs, rebuilt fresh
    // per mount and re-projected on modelChanged (the connection dies with the
    // panel on the next workspace switch).
    QWidget *buildMapBrowserPanel();
    // The recipe lab's ASCII proof pane (registry feature #6): renders the
    // current op stream's selected orthographic projection, rebuilt per mount
    // and re-rendered on opsStreamChanged. resolve -> compile -> render; a
    // refusal is shown verbatim, since a proof must never lie about parts.
    QWidget *buildAsciiPreviewPanel();
    // The lab's bottom terminal: a tab widget over the recipe's views — the
    // human's Steps inspector (default), the AI's TOML Editor, and the ASCII
    // Proof — one full-width view at a time.
    QWidget *buildRecipeTerminalPanel(edi::shell::FeatureContext &context);
    // The Steps inspector (the human's click-to-tune surface): a list of the
    // recipe's ops; selecting one shows its editable numeric fields as spinboxes
    // that commit through applyOpFieldEdit. Rebuilt per mount, re-listed on
    // opsStreamChanged. A field bound to a drafted measurement is read-only here.
    QWidget *buildOpStepsPanel();
    // The step palette (the human's click-to-CHAIN surface): one button per
    // palette op type; clicking appends a unit step via appendRecipeOp. Lives in
    // the lab's Right slot beside Render/Compiled.
    QWidget *buildStepPalettePanel();
    // The lab's Right slot: tabbed recipe OUTPUTS — the Blender render and the
    // Compiled recipe (resolve+compile, what the proof and Build consume). Tabs
    // compress both into one slot; Render is the default.
    QWidget *buildLabRightPanel();
    // The Compiled-recipe pane: a read-only monospace view of the resolved +
    // compiled op stream (the artifact `--compiled-out` emits), re-serialized on
    // opsStreamChanged. A refusal (no recipe / unresolved / invalid) shows verbatim.
    QWidget *buildCompiledRecipePanel();
    void resizeEvent(QResizeEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;

signals:
    // Emitted whenever the recipe op stream is replaced (Open Ops Recipe, the
    // editor's Apply). The ASCII proof pane subscribes to re-render — the lab's
    // first "one feature reacts to what another produced" coupling.
    void opsStreamChanged();

private:
    void promptSaveDrawing();
    void promptSaveDrawingAs();
    void promptOpenDrawing();
    DirtyGuardChoice promptDirtyGuardChoice();
    void promptExportSvg();
    void promptExportHpgl();
    void promptExportGcode();
    void syncOpsScriptDocument(); // E4: stream -> editor script doc
    // The gate every recipe view shares: resolve (against the live drawing) ->
    // compile -> validate. On success fills compiledOps and returns empty; on
    // failure returns a named human refusal (no recipe / unresolved / invalid).
    QString compileCurrentRecipe(std::vector<edi::recipe::RecipeOp> &compiledOps);
    // resolve -> compile -> validate -> render the current op stream into the
    // ASCII text for one projection (0=Front, 1=Side, 2=Top), or a human
    // message (no recipe / named refusal) when it cannot be shown.
    QString renderOpsAsciiProjection(int projectionIndex);
    // resolve -> compile -> validate -> serialize the current op stream to the
    // compiled-recipe TOML, or a human refusal message.
    QString renderCompiledRecipeText();
    // The text editor's path chooser, shared by the editor feature and the lab's
    // recipe terminal: a test-injected provider, else a real QFileDialog.
    std::function<QString(bool forSave)> textEditorPathProvider();
    void promptOpenOpsRecipe();
    void promptSaveOpsRecipe();
    void promptExportResolvedOps();
    void promptExportOpsPreviews();
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
    QString m_lastRecipeError;
    edi::recipe::RecipeOpStream m_opsStream; // the op pipeline's loaded stream (R1-B05)
    // The text editor's documents (E1): window-owned like m_opsStream, so
    // workspace remounts (which recreate feature instances) cannot lose
    // edits; the panel reaches it through the FeatureContext bus.
    edi::text::TextDocumentStore m_textStore;
    QString m_textSessionPath;
    std::function<QString(bool forSave)> m_textEditorPathProvider;
    // The Blender lab's Build path. The runner is the effect seam (default =
    // spawn via ProcessRunStore; tests inject a recorder); the executable path
    // and the in-flight output image ride as members so the async finished
    // handler can surface the result without threading state through the bus.
    ProcessRunStore *m_processRunStore = nullptr;
    std::function<void(const edi::scripting::BlenderRunPlan &)> m_blenderRunner;
    QString m_blenderExecutablePath;
    QString m_currentBuildOutput;
    QString m_lastRenderImagePath; // last successful render, re-shown across remounts
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
