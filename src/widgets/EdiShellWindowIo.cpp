#include "widgets/EdiShellWindow.h"

#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QSaveFile>
#include <QFileInfo>
#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFontDatabase>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMenu>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QTabWidget>
#include <QTimer>
#include <QToolButton>
#include <QPixmap>
#include <QString>
#include <QStringList>
#include <QUrl>
#include <QVBoxLayout>

#include <algorithm>
#include <string>
#include <vector>

#include "core/DrawingCore.h"
#include "drafting/DraftingMapQuery.h" // deriveEdge / plugIsConnected — shared with MapToonExport so the browser and TOON never drift (DM-11)
#include "recipe/RecipeCraftsmen.h"
#include "recipe/RecipeOpsAscii.h"
#include "recipe/RecipeOpsBind.h"
#include "recipe/RecipeOpSchema.h"
#include "recipe/RecipeTextNumbers.h"
#include "recipe/RecipeOpsResolve.h"
#include "recipe/RecipeOpsStore.h"
#include "recipe/RecipeOpsValidate.h"
#include "text/TextEditorCommands.h"
#include "io/ShellLayoutStore.h"
#include "io/TextSessionStore.h"
#include "widgets/DraftingFeature.h"
#include "widgets/ShellWidgetHelpers.h"

using namespace edi::shell;

bool EdiShellWindow::saveDrawingToPath(const QString &path)
{
    if (path.isEmpty()) {
        return false;
    }
    if (!m_controller->saveDocument(QUrl::fromLocalFile(path))) {
        // An ATTEMPTED write that failed must say so — especially under the
        // dirty guard, where the user chose "Save" and the refused action
        // would otherwise be a silent dead end. (A cancelled dialog never
        // reaches here, so cancel stays legitimately quiet.)
        if (m_saveFailedNotice) {
            m_saveFailedNotice(path);
        } else {
            QMessageBox::warning(this, QStringLiteral("Save Failed"),
                                 QStringLiteral("Could not write %1. The drawing was not saved.").arg(path));
        }
        return false;
    }
    m_currentDrawingPath = path;
    rememberRecentFile(path);
    refreshDocumentStatus();
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
    refreshDocumentStatus();
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
    QString path = m_saveAsPathProvider
        ? m_saveAsPathProvider()
        : QFileDialog::getSaveFileName(
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
    // Guard BEFORE the file dialog: cancelling the guard should not make the
    // user pick a file first only to refuse it afterwards.
    if (!resolveDirtyGuard()) {
        return;
    }
    const QString path = QFileDialog::getOpenFileName(
        this, QStringLiteral("Open Drawing"), QString(),
        QStringLiteral("EDI Drawings (*.edidraw)"));
    if (path.isEmpty()) {
        return;
    }
    openDrawingFromPath(path);
}

void EdiShellWindow::setDirtyGuardPrompt(std::function<DirtyGuardChoice()> prompt)
{
    m_dirtyGuardPrompt = std::move(prompt);
}

void EdiShellWindow::setSaveAsPathProvider(std::function<QString()> provider)
{
    m_saveAsPathProvider = std::move(provider);
}

void EdiShellWindow::setSaveFailedNotice(std::function<void(const QString &path)> notice)
{
    m_saveFailedNotice = std::move(notice);
}

EdiShellWindow::DirtyGuardChoice EdiShellWindow::promptDirtyGuardChoice()
{
    const QMessageBox::StandardButton button = QMessageBox::warning(
        this, QStringLiteral("Unsaved Changes"),
        QStringLiteral("This drawing has unsaved changes. Save them before continuing?"),
        QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel,
        QMessageBox::Save);
    if (button == QMessageBox::Save) {
        return DirtyGuardChoice::Save;
    }
    if (button == QMessageBox::Discard) {
        return DirtyGuardChoice::Discard;
    }
    return DirtyGuardChoice::Cancel;
}

bool EdiShellWindow::resolveDirtyGuard()
{
    // Drawing only — ops-recipe dirty state is a known gap owed to the
    // R7 lab slice (m_opsStream has no dirty flag yet; the documented
    // obligation moved from pipeline A's controller when A retired, R1-B06).
    if (!m_controller->isDocumentDirty()) {
        return true; // nothing to lose, nothing to ask
    }
    const DirtyGuardChoice choice = m_dirtyGuardPrompt ? m_dirtyGuardPrompt() : promptDirtyGuardChoice();
    if (choice == DirtyGuardChoice::Cancel) {
        return false;
    }
    if (choice == DirtyGuardChoice::Save) {
        promptSaveDrawing();
        // Save-as can itself be cancelled: only a save that actually landed
        // (document now clean) unblocks the guarded action.
        return !m_controller->isDocumentDirty();
    }
    return true; // Discard: the user chose to lose the changes
}

bool EdiShellWindow::openDrawingFromPathGuarded(const QString &path)
{
    return resolveDirtyGuard() && openDrawingFromPath(path);
}

bool EdiShellWindow::exportSvgToPath(const QString &path)
{
    return !path.isEmpty() && m_controller->exportSvgDocument(QUrl::fromLocalFile(path));
}

bool EdiShellWindow::exportHpglToPath(const QString &path)
{
    return !path.isEmpty() && m_controller->exportHpglDocument(QUrl::fromLocalFile(path));
}

bool EdiShellWindow::exportGcodeToPath(const QString &path)
{
    return !path.isEmpty() && m_controller->exportGcodeDocument(QUrl::fromLocalFile(path));
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

void EdiShellWindow::promptExportGcode()
{
    QString path = QFileDialog::getSaveFileName(
        this, QStringLiteral("Export G-code"), QString(), QStringLiteral("G-code (*.gcode *.nc)"));
    if (path.isEmpty()) {
        return;
    }
    if (!path.endsWith(QStringLiteral(".gcode")) && !path.endsWith(QStringLiteral(".nc"))) {
        path += QStringLiteral(".gcode");
    }
    exportGcodeToPath(path);
}

bool EdiShellWindow::openOpsRecipeFromPath(const QString &path)
{
    using namespace edi::recipe;
    if (path.isEmpty()) {
        m_lastRecipeError = QStringLiteral("no path given");
        return false;
    }
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        m_lastRecipeError = QStringLiteral("could not read %1").arg(path);
        return false;
    }
    const QByteArray bytes = file.readAll();
    const OpStreamParseResult parsed = recipeOpsFromToml(
        std::string(bytes.constData(), static_cast<std::size_t>(bytes.size())),
        path.toStdString());
    if (!parsed.ok) {
        // The strict reader names the offending key — surface it verbatim.
        m_lastRecipeError = QString::fromStdString(parsed.message);
        return false;
    }
    m_opsStream = parsed.stream;
    m_lastRecipeError.clear();
    syncOpsScriptDocument(); // E4: the editor's script view follows the stream
    emit opsStreamChanged(); // the ASCII proof pane re-renders off the new stream
    return true;
}

// E4: project the op stream into the editor's SCRIPT document — canonical
// serialization, fixed id, role Context (the lab's working text). Upsert
// through the core's own paths: create once, then ReplaceTextRange — even
// the window mutates documents only via the choke point. No path field on
// purpose: this document is a projection of the STREAM, not a file; the
// ops File verbs own the recipe file and its gates.
void EdiShellWindow::syncOpsScriptDocument()
{
    using namespace edi::recipe;
    using namespace edi::text;
    const OpStreamTextResult canonical = recipeOpsToToml(m_opsStream);
    if (!canonical.ok) {
        return; // a stream the writer refuses stays out of the editor
    }
    const std::string id = "ops_recipe";
    if (!containsDocument(m_textStore, id)) {
        TextDocument document = makeTextDocument(id, "Ops Recipe");
        document.role = TextDocumentRole::Context;
        addDocument(m_textStore, std::move(document));
    }
    const TextDocument *existing = findDocument(m_textStore, id);
    applyTextEditorCommand(
        m_textStore,
        ReplaceTextRangeCommand{id, {0, existing->text.size()}, canonical.text});
    setActiveDocument(m_textStore, id);
    if (m_featureContext.refreshTextPanel) {
        m_featureContext.refreshTextPanel();
    }
}

// E4: the Apply hook — the editor's text, back through the STRICT reader.
// Success replaces the stream and echoes the canonical form into the
// document (the user sees exactly what the pipeline now holds); a refusal
// returns the reader's message verbatim, the named key intact.
QString EdiShellWindow::applyOpsScript(const std::string &text)
{
    using namespace edi::recipe;
    const OpStreamParseResult parsed = recipeOpsFromToml(text, "editor");
    if (!parsed.ok) {
        return QString::fromStdString(parsed.message);
    }
    m_opsStream = parsed.stream;
    syncOpsScriptDocument();
    emit opsStreamChanged(); // the ASCII proof pane re-renders off the new stream
    return {};
}

bool EdiShellWindow::saveOpsRecipeToPath(const QString &path)
{
    using namespace edi::recipe;
    if (path.isEmpty()) {
        m_lastRecipeError = QStringLiteral("no path given");
        return false;
    }
    const OpStreamTextResult written = recipeOpsToToml(m_opsStream);
    if (!written.ok) {
        m_lastRecipeError = QString::fromStdString(written.message);
        return false;
    }
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        m_lastRecipeError = QStringLiteral("could not write %1").arg(path);
        return false;
    }
    file.write(written.text.data(), static_cast<qint64>(written.text.size()));
    if (!file.commit()) {
        m_lastRecipeError = QStringLiteral("could not write %1").arg(path);
        return false;
    }
    m_lastRecipeError.clear();
    return true;
}

namespace {

// A small "pop out" button (objectName per panel) for a panel header — wired by
// the caller to float a freshly-built copy of the panel.
QToolButton *makePopOutButton(const QString &objectName)
{
    auto *button = new QToolButton;
    button->setObjectName(objectName);
    button->setText(QStringLiteral("⇱"));
    button->setToolTip(QStringLiteral("Pop out into a floating window"));
    button->setAutoRaise(true);
    return button;
}

// Findings one per line ("op.3.radius: object not found: gone"), so the chrome
// shows EVERY stale binding at once — the op pipeline's per-binding isolation
// made visible (the same promise pipeline A made per parameter).
QString joinOpResolveFindings(const std::vector<edi::recipe::OpResolveFinding> &findings)
{
    QStringList lines;
    for (const edi::recipe::OpResolveFinding &finding : findings) {
        lines << QStringLiteral("%1: %2")
                     .arg(QString::fromStdString(finding.key),
                          QString::fromStdString(finding.message));
    }
    return lines.join(QLatin1Char('\n'));
}

} // namespace

bool EdiShellWindow::exportResolvedOpsToPath(const QString &path)
{
    using namespace edi::recipe;
    if (path.isEmpty()) {
        m_lastRecipeError = QStringLiteral("no path given");
        return false;
    }
    // Resolve against the LIVE drawing + grid — the seam pipeline A's
    // Export Blender Python used before A retired (R1-B06). A refusal
    // lists every stale binding at once.
    const OpResolveResult resolved = resolveRecipeOps(
        m_opsStream,
        m_controller->draftingDocument(),
        m_controller->draftingGridProjection());
    if (!resolved.ok) {
        m_lastRecipeError = joinOpResolveFindings(resolved.findings);
        return false;
    }
    const OpStreamTextResult written = recipeOpsToToml(resolved.stream);
    if (!written.ok) {
        m_lastRecipeError = QString::fromStdString(written.message);
        return false;
    }
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        m_lastRecipeError = QStringLiteral("could not write %1").arg(path);
        return false;
    }
    file.write(written.text.data(), static_cast<qint64>(written.text.size()));
    if (!file.commit()) {
        m_lastRecipeError = QStringLiteral("could not write %1").arg(path);
        return false;
    }
    m_lastRecipeError.clear();
    return true;
}

bool EdiShellWindow::exportOpsPreviewsToDir(const QString &dir)
{
    using namespace edi::recipe;
    if (dir.isEmpty()) {
        m_lastRecipeError = QStringLiteral("no path given");
        return false;
    }
    // resolve -> compile -> validate -> render: resolve is the gate, so nothing
    // downstream ever sees an unresolved stream.
    const OpResolveResult resolved = resolveRecipeOps(
        m_opsStream,
        m_controller->draftingDocument(),
        m_controller->draftingGridProjection());
    if (!resolved.ok) {
        m_lastRecipeError = joinOpResolveFindings(resolved.findings);
        return false;
    }
    const RecipeCompileResult compiled = compileRecipeOps(resolved.stream.ops);
    if (!compiled.ok) {
        m_lastRecipeError = QString::fromStdString(compiled.message);
        return false;
    }
    const OpValidationReport report = validateRecipeOps(compiled.ops);
    if (!report.ok) {
        QStringList lines;
        for (const OpFinding &finding : report.findings) {
            if (finding.severity == OpFinding::Severity::Error) {
                lines << QStringLiteral("%1: %2")
                             .arg(QString::fromStdString(finding.code),
                                  QString::fromStdString(finding.message));
            }
        }
        m_lastRecipeError = lines.join(QLatin1Char('\n'));
        return false;
    }
    const struct {
        AsciiProjection projection;
        const char *file;
    } views[] = {
        {AsciiProjection::Front, "front.txt"},
        {AsciiProjection::Side, "side.txt"},
        {AsciiProjection::Top, "top.txt"},
    };
    for (const auto &view : views) {
        const AsciiRenderResult render = renderOpsProjection(compiled.ops, view.projection);
        if (!render.ok) {
            m_lastRecipeError = QString::fromStdString(render.message);
            return false;
        }
        QSaveFile file(dir + QLatin1Char('/') + QLatin1String(view.file));
        if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            m_lastRecipeError = QStringLiteral("could not write %1").arg(file.fileName());
            return false;
        }
        file.write(render.text.data(), static_cast<qint64>(render.text.size()));
        if (!file.commit()) {
            m_lastRecipeError = QStringLiteral("could not write %1").arg(file.fileName());
            return false;
        }
    }
    m_lastRecipeError.clear();
    return true;
}

void EdiShellWindow::promptOpenOpsRecipe()
{
    const QString path = QFileDialog::getOpenFileName(
        this, QStringLiteral("Open Ops Recipe"), QString(),
        QStringLiteral("Op recipes (*.toml)"));
    if (path.isEmpty()) {
        return;
    }
    if (!openOpsRecipeFromPath(path)) {
        QMessageBox::warning(this, QStringLiteral("Open Ops Recipe Failed"), m_lastRecipeError);
    }
}

void EdiShellWindow::promptSaveOpsRecipe()
{
    QString path = QFileDialog::getSaveFileName(
        this, QStringLiteral("Save Ops Recipe"), QString(),
        QStringLiteral("Op recipes (*.toml)"));
    if (path.isEmpty()) {
        return;
    }
    if (!path.endsWith(QStringLiteral(".toml"))) {
        path += QStringLiteral(".toml");
    }
    if (!saveOpsRecipeToPath(path)) {
        QMessageBox::warning(this, QStringLiteral("Save Ops Recipe Failed"), m_lastRecipeError);
    }
}

void EdiShellWindow::promptExportResolvedOps()
{
    QString path = QFileDialog::getSaveFileName(
        this, QStringLiteral("Export Resolved"), QString(),
        QStringLiteral("Op recipes (*.toml)"));
    if (path.isEmpty()) {
        return;
    }
    if (!path.endsWith(QStringLiteral(".toml"))) {
        path += QStringLiteral(".toml");
    }
    if (!exportResolvedOpsToPath(path)) {
        // Refusal lists every stale binding — never a silent dead button.
        QMessageBox::warning(this, QStringLiteral("Export Refused"), m_lastRecipeError);
    }
}

void EdiShellWindow::promptExportOpsPreviews()
{
    const QString dir = QFileDialog::getExistingDirectory(
        this, QStringLiteral("Export Ops Previews"));
    if (dir.isEmpty()) {
        return;
    }
    if (!exportOpsPreviewsToDir(dir)) {
        QMessageBox::warning(this, QStringLiteral("Export Refused"), m_lastRecipeError);
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
    rebuildRecentFilesMenu();
    return true;
}

bool EdiShellWindow::saveSettings(const QString &path) const
{
    return edi::io::saveSettingsToPath(path, captureSettings());
}

bool EdiShellWindow::loadWorkspaceLayout(const QString &path)
{
    // Remember the path either way: a first run has no file yet, but closing
    // should still write one there.
    m_workspaceLayoutPath = path;
    edi::io::ShellLayoutData data = edi::io::loadShellLayoutFromPath(path);
    if (!data.ok) {
        return false; // keep the built-in default layout and panel state
    }
    m_panelsState = data.panels;
    // A workspace file from before the belt existed decodes to an all-empty
    // grid; treat that as "no opinion" and keep the mounted layout's belt
    // rather than blanking the user's tools.
    const bool loadedBeltHasItems = std::any_of(
        data.layout.belt.itemIds.begin(), data.layout.belt.itemIds.end(),
        [](const QString &id) { return !id.isEmpty(); });
    if (!loadedBeltHasItems) {
        data.layout.belt = m_workspaceLayout.belt;
    }
    if (data.layout.id != m_workspaceLayout.id
        || data.layout.bindings != m_workspaceLayout.bindings
        || !(data.layout.belt == m_workspaceLayout.belt)) {
        // Different job (or a different belt arrangement): tear down and
        // rebuild the slots from the loaded layout — the belt widget reads
        // its items at build time, so a belt change needs the same remount.
        // Startup restore is NOT a navigation event: apply directly and
        // reset the trail to a single root. Routing through
        // switchWorkspaceLayout would push a second entry, leaving Back
        // born-enabled and pointing at a factory default the user never
        // visited.
        applyWorkspaceLayout(data.layout);
        m_workspaceHistory = {m_workspaceLayout};
        m_workspaceHistoryIndex = 0;
        refreshChrome();
    } else {
        // Same job: geometry only. Palette placements and panel-content
        // assignments ride with the layout like panel sizes do — adopt and
        // re-apply without a remount.
        m_workspaceLayout.palettes = data.layout.palettes;
        m_workspaceLayout.panelContent = data.layout.panelContent;
        applyPanelSizesToSplitters();
        refreshPanelVisibility();
        applyPalettePlacements();
        if (m_draftingFeature != nullptr) {
            m_draftingFeature->applyPanelAssignments();
        }
    }
    return true;
}

bool EdiShellWindow::saveWorkspaceLayout(const QString &path) const
{
    return edi::io::saveShellLayoutToPath(path, m_workspaceLayout, m_panelsState);
}

void EdiShellWindow::setTextEditorPathProvider(std::function<QString(bool forSave)> provider)
{
    m_textEditorPathProvider = std::move(provider);
}

void EdiShellWindow::setBlenderExecutablePath(const QString &path)
{
    m_blenderExecutablePath = path;
}

void EdiShellWindow::setBlenderRunner(std::function<void(const edi::scripting::BlenderRunPlan &)> runner)
{
    m_blenderRunner = std::move(runner);
}

QString EdiShellWindow::buildBlenderScript(const QString &scriptText, const QString &scriptPath)
{
    // Write the CURRENT buffer to a temp .py — unsaved edits build too, and the
    // user's file on disk is never touched. A sibling .png is where the script
    // is told to render; that path rides past '--' to the script.
    const QString stem = QFileInfo(scriptPath).completeBaseName();
    const QString base = stem.isEmpty() ? QStringLiteral("edi_build") : stem;
    const QDir tempDir(QDir::tempPath());
    const QString tempPy = tempDir.filePath(base + QStringLiteral("_edi.py"));
    m_currentBuildOutput = tempDir.filePath(base + QStringLiteral("_edi.png"));

    QFile scriptFile(tempPy);
    if (!scriptFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return QStringLiteral("could not write the build script to %1").arg(tempPy);
    }
    scriptFile.write(scriptText.toUtf8());
    scriptFile.close();

    // Pure decision: argv or a named refusal (no Blender configured / nothing to
    // build). The effect (spawn) is the runner — injectable, so tests never launch.
    const edi::scripting::BlenderRunPlan plan = edi::scripting::planBlenderRender(
        m_blenderExecutablePath.toStdString(), tempPy.toStdString(), m_currentBuildOutput.toStdString());
    if (!plan.ok) {
        return QString::fromStdString(plan.message);
    }
    if (m_blenderRunner) {
        m_blenderRunner(plan);
    }
    return QStringLiteral("building %1 in Blender…")
        .arg(QFileInfo(scriptPath).fileName().isEmpty() ? base : QFileInfo(scriptPath).fileName());
}

void EdiShellWindow::onBlenderRunFinished(const ProcessRunResult &result)
{
    // Surface the outcome on the editor's status line. (The rendered PNG itself
    // is shown by the render-preview slice; here the text result is the
    // feedback — the editor panel may have been remounted, so look it up live.)
    QString message;
    if (result.ok) {
        message = QStringLiteral("built ok → %1").arg(m_currentBuildOutput);
        showRenderImage(m_currentBuildOutput); // the rendered PNG into the preview
    } else {
        const QString lastError = result.standardError.trimmed().section(QLatin1Char('\n'), -1);
        message = lastError.isEmpty()
            ? QStringLiteral("build failed: %1").arg(result.message)
            : QStringLiteral("build failed: %1 — %2").arg(result.message, lastError);
    }
    if (auto *status = findChild<QLabel *>(QStringLiteral("textEditorStatus"))) {
        status->setText(message);
    }
}

QWidget *EdiShellWindow::buildBlenderPreviewPanel()
{
    QFrame *panel = edi::shell::makeRegionFrame(QStringLiteral("blenderPreviewPanel"));
    auto *layout = new QVBoxLayout(panel);
    layout->setContentsMargins(12, 8, 12, 8);
    layout->setSpacing(6);
    auto *title = new QLabel(QStringLiteral("Render"));
    title->setObjectName(QStringLiteral("blenderPreviewTitle"));
    auto *image = new QLabel(QStringLiteral("No render yet.\nBuild a .py that renders to the\npath passed after “--”."));
    image->setObjectName(QStringLiteral("blenderPreview"));
    image->setAlignment(Qt::AlignCenter);
    image->setWordWrap(true);
    image->setMinimumSize(160, 120);
    layout->addWidget(title);
    layout->addWidget(image, 1);
    // Re-show the last render after a workspace remount (the pane is rebuilt
    // fresh, so load straight into THIS label — findChild would not reach it
    // until it is mounted).
    if (!m_lastRenderImagePath.isEmpty()) {
        const QPixmap pixmap(m_lastRenderImagePath);
        if (!pixmap.isNull()) {
            image->setPixmap(pixmap.scaledToWidth(320, Qt::SmoothTransformation));
        }
    }
    return panel;
}

QWidget *EdiShellWindow::buildMapBrowserPanel()
{
    QFrame *panel = edi::shell::makeRegionFrame(QStringLiteral("mapBrowserPanel"));
    auto *layout = new QVBoxLayout(panel);
    layout->setContentsMargins(12, 8, 12, 8);
    layout->setSpacing(6);

    auto *title = new QLabel(QStringLiteral("Map"));
    title->setObjectName(QStringLiteral("mapBrowserTitle"));
    auto *summary = new QLabel;
    summary->setObjectName(QStringLiteral("mapBrowserSummary"));
    summary->setWordWrap(true);
    auto *list = new QListWidget;
    list->setObjectName(QStringLiteral("mapBrowserList"));
    layout->addWidget(title);
    layout->addWidget(summary);
    layout->addWidget(list, 1);

    // The browser is a READ-ONLY projection of the map graph, which lives in the
    // document (rooms/plugs/connections beside objects). So the single source is
    // the stable controller's document and a refresh just re-reads it; authoring
    // stays on the canvas and in .map.toml — this pane only shows what is there.
    auto refresh = [this, summary, list]() {
        const edi::drafting::DraftingDocument &doc = m_controller->draftingDocument();
        // Pluralize each count ("1 connection", "2 connections") — the browser
        // reads as prose, so a fixed "%1 connections" would print "1 connections".
        const auto countLabel = [](std::size_t n, const QString &noun) {
            return QStringLiteral("%1 %2%3")
                .arg(static_cast<int>(n))
                .arg(noun, n == 1 ? QString() : QStringLiteral("s"));
        };
        summary->setText(QStringLiteral("%1 · %2 · %3")
                             .arg(countLabel(doc.rooms.size(), QStringLiteral("room")),
                                  countLabel(doc.connections.size(), QStringLiteral("connection")),
                                  countLabel(doc.plugs.size(), QStringLiteral("plug"))));
        list->clear();
        // Footprints are stored in CANVAS units; show them in the AUTHORED units
        // the map was drawn in (feet), which is what the engine export speaks and
        // what an author recognizes — "12 × 11", not "0.24 × 0.22". canvasPerAuthoredUnit
        // is canvas-per-authored, so authored = canvas / scale; 1.0 (a hand-drawn doc
        // with no map scale) shows coordinates as-is. Guard a degenerate 0 scale.
        const double scale = doc.canvasPerAuthoredUnit > 0.0 ? doc.canvasPerAuthoredUnit : 1.0;
        for (const edi::drafting::DraftingMapRoom &room : doc.rooms) {
            list->addItem(QStringLiteral("▸ %1   %2 × %3")
                              .arg(QString::fromStdString(room.name))
                              .arg(room.width / scale, 0, 'g', 3)
                              .arg(room.height / scale, 0, 'g', 3));
        }
        // Connections reference plugs BY ID; show the authored plug names when
        // resolvable (a cleared name falls back to the opaque id).
        const auto plugLabel = [&doc](const edi::drafting::DraftingPlugId &id) -> QString {
            for (const edi::drafting::DraftingPlug &plug : doc.plugs) {
                if (plug.id == id) {
                    return plug.name.empty() ? QString::fromStdString(id)
                                             : QString::fromStdString(plug.name);
                }
            }
            return QString::fromStdString(id);
        };

        // --- Plugs section (DM-11) ------------------------------------------
        // The plug's edge is NOT stored — derive it read-only the SAME way the
        // TOON export does (deriveEdge over the plug's room footprint), through
        // the shared DraftingMapQuery helper so the two views can never disagree.
        // A plug names its room as the "room.plug" prefix of plug.name; find that
        // room to pick the nearest side. No room (or no '.' prefix) → "?".
        const auto findRoom = [&doc](const std::string &name) -> const edi::drafting::DraftingMapRoom * {
            for (const edi::drafting::DraftingMapRoom &room : doc.rooms) {
                if (room.name == name) {
                    return &room;
                }
            }
            return nullptr;
        };
        list->addItem(QStringLiteral("── Plugs ──"));
        for (const edi::drafting::DraftingPlug &plug : doc.plugs) {
            const QString name = plug.name.empty() ? QString::fromStdString(plug.id)
                                                   : QString::fromStdString(plug.name);
            // Resolve the room from the "room.plug" prefix (mirrors MapToonExport's
            // splitRoomPlug); the edge falls back to "?" when unresolvable.
            QString edge = QStringLiteral("?");
            const std::size_t dot = plug.name.find('.');
            const std::string roomName = dot == std::string::npos ? plug.name : plug.name.substr(0, dot);
            if (const edi::drafting::DraftingMapRoom *room = findRoom(roomName)) {
                edge = QString::fromStdString(edi::drafting::deriveEdge(*room, plug.anchor));
            }
            const QString type = plug.type.empty() ? QStringLiteral("door") : QString::fromStdString(plug.type);
            const bool connected = edi::drafting::plugIsConnected(doc.connections, plug.id);
            QString row = QStringLiteral("◦ %1   %2 · %3 · %4")
                              .arg(name, type, edge,
                                   connected ? QStringLiteral("linked") : QStringLiteral("unlinked"));
            // Flags are an OPEN vocabulary (DM-04): show them verbatim as a
            // `·`-joined run in brackets, omitted entirely when empty (Fork 2).
            if (!plug.flags.empty()) {
                QStringList flagTokens;
                flagTokens.reserve(static_cast<int>(plug.flags.size()));
                for (const std::string &flag : plug.flags) {
                    flagTokens << QString::fromStdString(flag);
                }
                row += QStringLiteral("   [%1]").arg(flagTokens.join(QStringLiteral(" · ")));
            }
            list->addItem(row);
        }

        // --- Connections section (DM-11) ------------------------------------
        list->addItem(QStringLiteral("── Connections ──"));
        for (const edi::drafting::DraftingDeclaredConnection &conn : doc.connections) {
            QString row = QStringLiteral("⟷ %1 ↔ %2").arg(plugLabel(conn.plugA), plugLabel(conn.plugB));
            // The neutral role tag is optional — show it only when authored.
            if (!conn.type.empty()) {
                row += QStringLiteral("   %1").arg(QString::fromStdString(conn.type));
            }
            list->addItem(row);
        }
    };
    refresh();

    // Track the live document: re-project on every model change. Bind the
    // connection to `panel` (not `this`) so it is torn down with the pane on the
    // next workspace switch — the controller outlives both, and a lambda writing
    // into deleted labels is exactly the per-mount dangling hazard to avoid.
    connect(m_controller, &DrawingDocumentController::modelChanged, panel, refresh);

    return panel;
}

QString EdiShellWindow::compileCurrentRecipe(std::vector<edi::recipe::RecipeOp> &compiledOps)
{
    using namespace edi::recipe;
    if (m_opsStream.ops.empty()) {
        return QStringLiteral("No recipe loaded.\nOpen or edit an ops recipe.");
    }
    // The gate every recipe VIEW shares (the same one exportOpsPreviewsToDir
    // uses): resolve against the LIVE drawing, then compile, then validate. A
    // view must refuse what it cannot show, so every failure surfaces verbatim
    // instead of a misleading picture — "proof never guesses."
    const OpResolveResult resolved = resolveRecipeOps(
        m_opsStream,
        m_controller->draftingDocument(),
        m_controller->draftingGridProjection());
    if (!resolved.ok) {
        return QStringLiteral("Unresolved bindings:\n%1").arg(joinOpResolveFindings(resolved.findings));
    }
    const RecipeCompileResult compiled = compileRecipeOps(resolved.stream.ops);
    if (!compiled.ok) {
        return QString::fromStdString(compiled.message);
    }
    const OpValidationReport report = validateRecipeOps(compiled.ops);
    if (!report.ok) {
        QStringList lines;
        for (const OpFinding &finding : report.findings) {
            if (finding.severity == OpFinding::Severity::Error) {
                lines << QStringLiteral("%1: %2")
                             .arg(QString::fromStdString(finding.code),
                                  QString::fromStdString(finding.message));
            }
        }
        return QStringLiteral("Invalid recipe:\n%1").arg(lines.join(QLatin1Char('\n')));
    }
    compiledOps = compiled.ops;
    return {}; // success — compiledOps is filled
}

QString EdiShellWindow::renderOpsAsciiProjection(int projectionIndex)
{
    using namespace edi::recipe;
    std::vector<RecipeOp> ops;
    const QString refusal = compileCurrentRecipe(ops);
    if (!refusal.isEmpty()) {
        return refusal;
    }
    const AsciiProjection projection = projectionIndex == 1 ? AsciiProjection::Side
                                     : projectionIndex == 2 ? AsciiProjection::Top
                                                            : AsciiProjection::Front;
    const AsciiRenderResult render = renderOpsProjection(ops, projection);
    return render.ok ? QString::fromStdString(render.text)
                     : QString::fromStdString(render.message);
}

QString EdiShellWindow::renderCompiledRecipeText()
{
    using namespace edi::recipe;
    std::vector<RecipeOp> ops;
    const QString refusal = compileCurrentRecipe(ops);
    if (!refusal.isEmpty()) {
        return refusal;
    }
    // The compiled stream is the artifact the Blender driver reads — bindings
    // resolved to numbers, mouldings compiled to explicit terms. Serialize it
    // through the byte-stable writer, the same form `--compiled-out` emits.
    RecipeOpStream compiledStream;
    compiledStream.id = m_opsStream.id;
    compiledStream.name = m_opsStream.name;
    compiledStream.ops = ops;
    const OpStreamTextResult written = recipeOpsToToml(compiledStream);
    return written.ok ? QString::fromStdString(written.text)
                      : QString::fromStdString(written.message);
}

QWidget *EdiShellWindow::buildAsciiPreviewPanel()
{
    QFrame *panel = edi::shell::makeRegionFrame(QStringLiteral("asciiPreviewPanel"));
    auto *layout = new QVBoxLayout(panel);
    layout->setContentsMargins(12, 8, 12, 8);
    layout->setSpacing(6);

    auto *header = new QHBoxLayout;
    auto *title = new QLabel(QStringLiteral("ASCII Proof"));
    title->setObjectName(QStringLiteral("asciiPreviewTitle"));
    auto *projection = new QComboBox;
    projection->setObjectName(QStringLiteral("asciiPreviewProjection"));
    projection->addItem(QStringLiteral("Front")); // index 0/1/2 -> Front/Side/Top
    projection->addItem(QStringLiteral("Side"));
    projection->addItem(QStringLiteral("Top"));
    auto *popOut = makePopOutButton(QStringLiteral("popOutAscii"));
    connect(popOut, &QToolButton::clicked, panel,
            [this]() { popOutPanel(QStringLiteral("ASCII Proof"), buildAsciiPreviewPanel()); });
    header->addWidget(title);
    header->addStretch(1);
    header->addWidget(projection);
    header->addWidget(popOut);
    layout->addLayout(header);

    auto *view = new QPlainTextEdit;
    view->setObjectName(QStringLiteral("asciiPreviewText"));
    view->setReadOnly(true);
    // Monospace + no wrap so the orthographic grid lines up column-for-column.
    view->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
    view->setLineWrapMode(QPlainTextEdit::NoWrap);
    layout->addWidget(view, 1);

    // Re-render the selected projection from the current op stream. The view is
    // read-only: authoring stays in the editor; this pane only PROVES.
    auto refresh = [this, projection, view]() {
        view->setPlainText(renderOpsAsciiProjection(projection->currentIndex()));
    };
    refresh();
    // Switching the projection re-renders; so does any op-stream change (the
    // editor's Apply / Open Ops Recipe, via opsStreamChanged). Both connections
    // bind to `panel`, so they die when the pane is torn down on a switch — the
    // window (signal source) and controller outlive it.
    connect(projection, &QComboBox::currentIndexChanged, panel, [refresh](int) { refresh(); });
    connect(this, &EdiShellWindow::opsStreamChanged, panel, [refresh]() { refresh(); });

    return panel;
}

QWidget *EdiShellWindow::buildCompiledRecipePanel()
{
    QFrame *panel = edi::shell::makeRegionFrame(QStringLiteral("compiledRecipePanel"));
    auto *layout = new QVBoxLayout(panel);
    layout->setContentsMargins(12, 8, 12, 8);
    layout->setSpacing(6);
    auto *header = new QHBoxLayout;
    auto *title = new QLabel(QStringLiteral("Compiled"));
    title->setObjectName(QStringLiteral("compiledRecipeTitle"));
    auto *popOut = makePopOutButton(QStringLiteral("popOutCompiled"));
    connect(popOut, &QToolButton::clicked, panel,
            [this]() { popOutPanel(QStringLiteral("Compiled"), buildCompiledRecipePanel()); });
    header->addWidget(title);
    header->addStretch(1);
    header->addWidget(popOut);
    auto *view = new QPlainTextEdit;
    view->setObjectName(QStringLiteral("compiledRecipeText"));
    view->setReadOnly(true);
    view->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
    view->setLineWrapMode(QPlainTextEdit::NoWrap);
    layout->addLayout(header);
    layout->addWidget(view, 1);

    // Read-only projection of the compiled stream; re-serialized on every recipe
    // change, bound to `panel` so it dies with the pane on a workspace switch.
    auto refresh = [this, view]() { view->setPlainText(renderCompiledRecipeText()); };
    refresh();
    connect(this, &EdiShellWindow::opsStreamChanged, panel, [refresh]() { refresh(); });

    return panel;
}

QWidget *EdiShellWindow::buildLabRightPanel()
{
    // The lab's Right: the step PALETTE (the human's click-to-chain add-surface,
    // the default tab) plus the recipe OUTPUTS — the Blender Render ("script is
    // execution") and the Compiled recipe (resolve+compile, what the proof and
    // Build consume). Tabs compress all three into one slot. Each pane is the
    // same per-mount, re-render-on-change panel as the rest, so a workspace
    // switch tears the whole tab widget down cleanly. The render pane keeps its
    // "blenderPreview" label, so showRenderImage finds it whichever tab is up.
    auto *tabs = new QTabWidget;
    tabs->setObjectName(QStringLiteral("recipeOutput"));
    tabs->setDocumentMode(true);
    tabs->addTab(buildStepPalettePanel(), QStringLiteral("Palette"));
    tabs->addTab(buildBlenderPreviewPanel(), QStringLiteral("Render"));
    tabs->addTab(buildCompiledRecipePanel(), QStringLiteral("Compiled"));
    // Inset the tab strip inside a margined region like every other panel — a
    // bare tab widget bleeds flush to the window edge ("outside" the frame); the
    // margin pulls the tabs inside it. (recipeOutput stays findable underneath.)
    QFrame *panel = edi::shell::makeRegionFrame(QStringLiteral("recipeOutputPanel"));
    auto *panelLayout = new QVBoxLayout(panel);
    panelLayout->setContentsMargins(8, 8, 8, 8);
    panelLayout->addWidget(tabs);
    return panel;
}

void EdiShellWindow::applyOpScalarEdit(int opIndex, const QString &fieldKey,
                                      const edi::recipe::RecipeScalarValue &value)
{
    if (opIndex < 0 || opIndex >= static_cast<int>(m_opsStream.ops.size())) {
        return;
    }
    // Edit a COPY, commit only if the key+kind is a real scalar of this op — an
    // unknown key or wrong kind leaves the stream untouched. Then re-sync the
    // TOML view (the AI's surface follows the human) and re-render the proof.
    edi::recipe::RecipeOp edited = m_opsStream.ops[static_cast<std::size_t>(opIndex)];
    if (!edi::recipe::setOpScalar(edited, fieldKey.toStdString(), value)) {
        return;
    }
    m_opsStream.ops[static_cast<std::size_t>(opIndex)] = std::move(edited);
    syncOpsScriptDocument();
    emit opsStreamChanged();
}

void EdiShellWindow::applyOpFieldEdit(int opIndex, const QString &fieldKey, double value)
{
    applyOpScalarEdit(opIndex, fieldKey, edi::recipe::RecipeScalarValue{value});
}

QWidget *EdiShellWindow::popOutPanel(const QString &title, QWidget *content)
{
    // A top-level tool window parented to the shell (so it rides with the app and
    // tests still find it via findChild). The content is a freshly BUILT panel —
    // it carries its own live subscriptions, independent of the docked one, so a
    // workspace switch never orphans it. WA_DeleteOnClose disposes both on close.
    auto *node = new QWidget(this, Qt::Tool);
    node->setObjectName(QStringLiteral("floatingNode"));
    node->setWindowTitle(title);
    node->setAttribute(Qt::WA_DeleteOnClose);
    auto *layout = new QVBoxLayout(node);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(content);
    const QSize hint = content->sizeHint();
    node->resize(hint.isValid() && !hint.isEmpty() ? hint : QSize(380, 480));
    node->show();
    return node;
}


void EdiShellWindow::appendRecipeOp(const QString &typeName)
{
    // Name the step "<type>_<position>" — unique while we only ever append.
    const QString label = typeName.startsWith(QStringLiteral("Add")) ? typeName.mid(3) : typeName;
    const std::string name = QStringLiteral("%1_%2")
                                 .arg(label.toLower())
                                 .arg(m_opsStream.ops.size())
                                 .toStdString();
    std::optional<edi::recipe::RecipeOp> op = edi::recipe::makeRecipeOp(typeName.toStdString(), name);
    if (!op) {
        return; // a type the palette does not offer
    }
    m_opsStream.ops.push_back(std::move(*op));
    syncOpsScriptDocument(); // the AI's TOML grows the step too
    emit opsStreamChanged();  // the Steps list, the proof, and the compiled view follow
}

void EdiShellWindow::appendScriptStep(const QString &craftsmanId)
{
    const edi::recipe::CraftsmanManifest *manifest =
        edi::recipe::findCraftsman(m_craftsmen, craftsmanId.toStdString());
    if (manifest == nullptr) {
        return; // not a registered craftsman
    }
    // Name the step "<id>_<position>" — unique while we only ever append, the
    // same scheme appendRecipeOp uses. makeScriptOp seeds every param at its
    // manifest default, so the step is immediately visible in the proof.
    const std::string name =
        QStringLiteral("%1_%2").arg(craftsmanId).arg(m_opsStream.ops.size()).toStdString();
    m_opsStream.ops.push_back(edi::recipe::makeScriptOp(*manifest, name));
    syncOpsScriptDocument();
    emit opsStreamChanged();
}

void EdiShellWindow::setCraftsmanRegistryToml(const QString &toml)
{
    // Parse-and-store; a bad registry leaves the list empty (no craftsman
    // buttons) rather than throwing. The palette reads m_craftsmen when it is
    // built, so this must run before the lab workspace mounts (main.cpp feeds
    // it at startup; tests feed it before switching to the Blender workspace).
    const edi::recipe::CraftsmanRegistryResult parsed =
        edi::recipe::parseCraftsmanRegistryToml(toml.toStdString(), "craftsmen");
    m_craftsmen = parsed.craftsmen;
}

void EdiShellWindow::removeRecipeOpAt(int opIndex)
{
    if (opIndex < 0 || opIndex >= static_cast<int>(m_opsStream.ops.size())) {
        return;
    }
    edi::recipe::removeRecipeOp(m_opsStream, static_cast<std::size_t>(opIndex));
    syncOpsScriptDocument();
    emit opsStreamChanged();
}

void EdiShellWindow::moveRecipeOpAt(int opIndex, int delta)
{
    const int to = opIndex + delta;
    if (opIndex < 0 || opIndex >= static_cast<int>(m_opsStream.ops.size())
        || to < 0 || to >= static_cast<int>(m_opsStream.ops.size())) {
        return;
    }
    edi::recipe::moveRecipeOp(m_opsStream, static_cast<std::size_t>(opIndex), static_cast<std::size_t>(to));
    syncOpsScriptDocument();
    emit opsStreamChanged();
}

void EdiShellWindow::bindOpField(int opIndex, const QString &fieldKey, const QString &objectId,
                                 const QString &measurement)
{
    if (opIndex < 0) {
        return;
    }
    if (edi::recipe::addRecipeBinding(m_opsStream, static_cast<std::size_t>(opIndex), fieldKey.toStdString(),
                                      objectId.toStdString(), measurement.toStdString())) {
        syncOpsScriptDocument();
        emit opsStreamChanged();
    }
}

void EdiShellWindow::unbindOpField(int opIndex, const QString &fieldKey)
{
    if (opIndex < 0) {
        return;
    }
    edi::recipe::clearRecipeBinding(m_opsStream, static_cast<std::size_t>(opIndex), fieldKey.toStdString());
    syncOpsScriptDocument();
    emit opsStreamChanged();
}

void EdiShellWindow::showOpBindMenu(int opIndex, const QString &fieldKey, bool bound, const QPoint &globalPos)
{
    // The actions DEFER (singleShot 0): binding rebuilds the inspector fields,
    // and doing that while still inside the spinbox's context-menu event would
    // delete the spinbox mid-signal. The zero-timer runs after the event returns.
    QMenu menu;
    if (bound) {
        connect(menu.addAction(QStringLiteral("Unbind")), &QAction::triggered, &menu, [this, opIndex, fieldKey]() {
            QTimer::singleShot(0, this, [this, opIndex, fieldKey]() { unbindOpField(opIndex, fieldKey); });
        });
    } else {
        // Bind to a drafted object's measurement: one submenu per object, each
        // offering the four measurement fields the resolve pass understands.
        const auto &objects = m_controller->draftingDocument().objects;
        if (objects.empty()) {
            menu.addAction(QStringLiteral("(draw something to bind to)"))->setEnabled(false);
        }
        for (const edi::drafting::DraftingObject &object : objects) {
            const QString objectId = QString::fromStdString(object.id);
            QMenu *sub = menu.addMenu(objectId);
            for (const QString &measurement :
                 {QStringLiteral("width"), QStringLiteral("height"), QStringLiteral("length"), QStringLiteral("radius")}) {
                connect(sub->addAction(measurement), &QAction::triggered, &menu,
                        [this, opIndex, fieldKey, objectId, measurement]() {
                            QTimer::singleShot(0, this, [this, opIndex, fieldKey, objectId, measurement]() {
                                bindOpField(opIndex, fieldKey, objectId, measurement);
                            });
                        });
            }
        }
    }
    menu.exec(globalPos);
}

namespace {
// "tube_height" -> "Tube Height": the human label for a TOML field key.
QString prettyFieldLabel(const QString &key)
{
    QStringList words = key.split(QLatin1Char('_'), Qt::SkipEmptyParts);
    for (QString &word : words) {
        word[0] = word[0].toUpper();
    }
    return words.join(QLatin1Char(' '));
}
} // namespace

QWidget *EdiShellWindow::buildOpStepsPanel()
{
    QFrame *panel = edi::shell::makeRegionFrame(QStringLiteral("opStepsPanel"));
    auto *layout = new QHBoxLayout(panel);
    layout->setContentsMargins(12, 8, 12, 8);
    layout->setSpacing(12);

    // Left column: the step list with Remove / Up / Down beneath it.
    auto *leftColumn = new QVBoxLayout;
    auto *list = new QListWidget;
    list->setObjectName(QStringLiteral("opStepsList"));
    list->setMaximumWidth(220);
    leftColumn->addWidget(list);
    auto *stepButtons = new QHBoxLayout;
    auto *removeButton = new QPushButton(QStringLiteral("Remove"));
    removeButton->setObjectName(QStringLiteral("removeStep"));
    auto *upButton = new QPushButton(QStringLiteral("↑"));
    upButton->setObjectName(QStringLiteral("moveStepUp"));
    auto *downButton = new QPushButton(QStringLiteral("↓"));
    downButton->setObjectName(QStringLiteral("moveStepDown"));
    stepButtons->addWidget(removeButton);
    stepButtons->addWidget(upButton);
    stepButtons->addWidget(downButton);
    leftColumn->addLayout(stepButtons);
    layout->addLayout(leftColumn);
    // Remove acts on the selection; Up/Down move it and keep it selected so a
    // run of clicks walks a step through the recipe.
    connect(removeButton, &QPushButton::clicked, panel, [this, list]() {
        removeRecipeOpAt(list->currentRow());
    });
    connect(upButton, &QPushButton::clicked, panel, [this, list]() {
        const int row = list->currentRow();
        moveRecipeOpAt(row, -1);
        if (row - 1 >= 0) {
            list->setCurrentRow(row - 1);
        }
    });
    connect(downButton, &QPushButton::clicked, panel, [this, list]() {
        const int row = list->currentRow();
        moveRecipeOpAt(row, 1);
        if (row + 1 < list->count()) {
            list->setCurrentRow(row + 1);
        }
    });

    auto *fields = new QWidget;
    fields->setObjectName(QStringLiteral("opStepsFields"));
    auto *form = new QFormLayout(fields);
    form->setContentsMargins(0, 0, 0, 0);
    // Keep each field at its natural size instead of stretching it across the
    // whole panel — a full-width numeric spin reads as broken. Fields pack to
    // the left of the form column; the empty space to their right is fine.
    form->setFieldGrowthPolicy(QFormLayout::FieldsStayAtSizeHint);
    layout->addWidget(fields, 1);

    // For a ScriptOp param, the manifest's declaration (its label + type);
    // nullptr for a built-in field, a non-Script op, or an unregistered
    // craftsman. The inspector renders a craftsman param with a widget the
    // MANIFEST types (number/integer/material/…) instead of the plain text the
    // pure schema reports — the type lives in the python script, so this overlay
    // lives here, where the registry is, not in the Qt-free schema.
    auto paramSpec = [this](const edi::recipe::RecipeOp &op, const std::string &key)
        -> const edi::recipe::CraftsmanParam * {
        const auto *script = std::get_if<edi::recipe::ScriptOp>(&op);
        if (script == nullptr) {
            return nullptr;
        }
        const edi::recipe::CraftsmanManifest *manifest =
            edi::recipe::findCraftsman(m_craftsmen, script->scriptId);
        if (manifest == nullptr) {
            return nullptr;
        }
        for (const edi::recipe::CraftsmanParam &param : manifest->params) {
            if (param.key == key) {
                return &param;
            }
        }
        return nullptr;
    };

    // The op's editable scalars in DISPLAY order. For a Script op the store
    // sweeps params from a sorted map (so a loaded step lists them
    // alphabetically), but the craftsman declared a deliberate order — render
    // the built-ins (x/y/z/name/script) first, then the params in MANIFEST
    // order. A plain op (or unregistered craftsman) keeps the schema's order.
    auto orderedScalars =
        [this](const edi::recipe::RecipeOp &op) -> std::vector<edi::recipe::RecipeOpScalar> {
        std::vector<edi::recipe::RecipeOpScalar> all = edi::recipe::opEditableScalars(op);
        const auto *script = std::get_if<edi::recipe::ScriptOp>(&op);
        if (script == nullptr) {
            return all;
        }
        const edi::recipe::CraftsmanManifest *manifest =
            edi::recipe::findCraftsman(m_craftsmen, script->scriptId);
        if (manifest == nullptr) {
            return all;
        }
        const auto isParam = [manifest](const std::string &key) {
            for (const edi::recipe::CraftsmanParam &p : manifest->params) {
                if (p.key == key) {
                    return true;
                }
            }
            return false;
        };
        std::vector<edi::recipe::RecipeOpScalar> ordered;
        ordered.reserve(all.size());
        for (const edi::recipe::RecipeOpScalar &scalar : all) {
            if (!isParam(scalar.key)) {
                ordered.push_back(scalar); // built-ins first, in their own order
            }
        }
        for (const edi::recipe::CraftsmanParam &param : manifest->params) {
            for (const edi::recipe::RecipeOpScalar &scalar : all) {
                if (scalar.key == param.key) {
                    ordered.push_back(scalar);
                    break;
                }
            }
        }
        return ordered;
    };

    // A manifest-typed editor for one craftsman param. The param's VALUE is a
    // string in the op's bag, so whatever the widget, the commit formats back to
    // a string (numberKeyText for a number, "true"/"false" for a bool, …). Params
    // are not bindable, so there is no bind menu — just the editor.
    auto buildParamWidget = [this, fields](int index, const QString &key, const std::string &type,
                                           const std::string &valueText) -> QWidget * {
        using edi::recipe::RecipeScalarValue;
        if (type == "number") {
            auto *spin = new QDoubleSpinBox;
            spin->setDecimals(3);
            spin->setRange(-1000000.0, 1000000.0);
            double value = 0.0;
            edi::recipe::parseNumberText(valueText, value);
            spin->setValue(value);
            connect(spin, &QDoubleSpinBox::editingFinished, fields, [this, index, key, spin]() {
                applyOpScalarEdit(index, key, RecipeScalarValue{edi::recipe::numberKeyText(spin->value())});
            });
            return spin;
        }
        if (type == "integer") {
            auto *spin = new QSpinBox;
            spin->setRange(-1000000, 1000000);
            spin->setValue(QString::fromStdString(valueText).toInt());
            connect(spin, &QSpinBox::editingFinished, fields, [this, index, key, spin]() {
                applyOpScalarEdit(index, key, RecipeScalarValue{std::to_string(spin->value())});
            });
            return spin;
        }
        if (type == "material") {
            auto *combo = new QComboBox;
            for (const std::string &material : edi::recipe::recipeMaterialTable()) {
                combo->addItem(QString::fromStdString(material));
            }
            combo->setCurrentText(QString::fromStdString(valueText));
            connect(combo, &QComboBox::activated, fields, [this, index, key, combo](int) {
                applyOpScalarEdit(index, key, RecipeScalarValue{combo->currentText().toStdString()});
            });
            return combo;
        }
        if (type == "bool" || type == "boolean") {
            auto *check = new QCheckBox;
            check->setChecked(valueText == "true");
            connect(check, &QCheckBox::clicked, fields, [this, index, key](bool on) {
                applyOpScalarEdit(index, key, RecipeScalarValue{std::string(on ? "true" : "false")});
            });
            return check;
        }
        // text / string / anything unrecognized: a plain line edit (the value is
        // already a string; an unknown manifest type degrades to editable text).
        auto *edit = new QLineEdit(QString::fromStdString(valueText));
        connect(edit, &QLineEdit::editingFinished, fields, [this, index, key, edit]() {
            applyOpScalarEdit(index, key, RecipeScalarValue{edit->text().toStdString()});
        });
        return edit;
    };

    // Build fresh field widgets for the selected op. Called ONLY on selection
    // change (and the first build) — never from opsStreamChanged — so it can
    // delete the old widgets without ever deleting one mid-signal during its own
    // commit. A field bound to a drafted measurement (its number comes from the
    // canvas via resolve) renders disabled, with the binding named in its label.
    auto rebuildFields = [this, form, fields, paramSpec, buildParamWidget, orderedScalars](int index) {
        while (form->rowCount() > 0) {
            form->removeRow(0);
        }
        if (index < 0 || index >= static_cast<int>(m_opsStream.ops.size())) {
            return;
        }
        const edi::recipe::RecipeOp &op = m_opsStream.ops[static_cast<std::size_t>(index)];
        // orderedScalars (not opEditableScalars) so a craftsman's params render in
        // MANIFEST order; only this row-building path cares about field order —
        // the refresh/shape paths key off objectName + count, which are unchanged.
        for (const edi::recipe::RecipeOpScalar &scalar : orderedScalars(op)) {
            using edi::recipe::RecipeFieldKind;
            using edi::recipe::RecipeScalarValue;
            const QString key = QString::fromStdString(scalar.key);
            // A craftsman param renders by its MANIFEST type, with the manifest's
            // label; the pure schema reported it as plain text. (Unknown craftsman
            // ⇒ no spec ⇒ it falls through to that text widget below.)
            if (const edi::recipe::CraftsmanParam *spec = paramSpec(op, scalar.key)) {
                QWidget *paramWidget = buildParamWidget(index, key, spec->type, scalar.text);
                paramWidget->setObjectName(QStringLiteral("opField_%1").arg(key));
                form->addRow(QString::fromStdString(spec->label), paramWidget);
                continue;
            }
            // A bound double is read here, not edited (its number comes from the
            // canvas through resolve); a read-only reference (a revolved profile
            // id) likewise. Both render disabled.
            const edi::recipe::RecipeFieldBinding *binding =
                scalar.kind == RecipeFieldKind::Number
                    ? edi::recipe::findRecipeBinding(m_opsStream, static_cast<std::size_t>(index), scalar.key)
                    : nullptr;
            const bool bound = binding != nullptr;
            const bool enabled = scalar.editable && !bound;
            // Each widget commits through a USER-only signal (editingFinished /
            // activated / clicked) — never one a programmatic refresh fires — so
            // re-rendering after a commit never loops back into another edit.
            QWidget *widget = nullptr;
            switch (scalar.kind) {
            case RecipeFieldKind::Number: {
                auto *spin = new QDoubleSpinBox;
                spin->setDecimals(3);
                spin->setRange(-1000000.0, 1000000.0);
                spin->setValue(scalar.number);
                // A bound number is read-only (not disabled) so it still takes a
                // right-click to unbind; its value comes from resolve, not here.
                spin->setReadOnly(bound);
                if (!bound) {
                    connect(spin, &QDoubleSpinBox::editingFinished, fields, [this, index, key, spin]() {
                        applyOpScalarEdit(index, key, RecipeScalarValue{spin->value()});
                    });
                }
                spin->setToolTip(bound ? QStringLiteral("Bound — right-click to unbind")
                                       : QStringLiteral("Right-click to bind to a drafted measurement"));
                spin->setContextMenuPolicy(Qt::CustomContextMenu);
                connect(spin, &QWidget::customContextMenuRequested, spin,
                        [this, index, key, bound, spin](const QPoint &at) {
                            showOpBindMenu(index, key, bound, spin->mapToGlobal(at));
                        });
                widget = spin;
                break;
            }
            case RecipeFieldKind::Integer: {
                auto *spin = new QSpinBox;
                spin->setRange(0, 1000000);
                spin->setValue(scalar.integer);
                if (enabled) {
                    connect(spin, &QSpinBox::editingFinished, fields, [this, index, key, spin]() {
                        applyOpScalarEdit(index, key, RecipeScalarValue{spin->value()});
                    });
                }
                widget = spin;
                break;
            }
            case RecipeFieldKind::Boolean: {
                auto *check = new QCheckBox;
                check->setChecked(scalar.boolean);
                if (enabled) {
                    connect(check, &QCheckBox::clicked, fields, [this, index, key](bool on) {
                        applyOpScalarEdit(index, key, RecipeScalarValue{on});
                    });
                }
                widget = check;
                break;
            }
            case RecipeFieldKind::Choice: {
                auto *combo = new QComboBox;
                for (const std::string &choice : scalar.choices) {
                    combo->addItem(QString::fromStdString(choice));
                }
                combo->setCurrentText(QString::fromStdString(scalar.text));
                if (enabled) {
                    connect(combo, &QComboBox::activated, fields, [this, index, key, combo](int) {
                        applyOpScalarEdit(index, key, RecipeScalarValue{combo->currentText().toStdString()});
                    });
                }
                widget = combo;
                break;
            }
            case RecipeFieldKind::Text: {
                auto *edit = new QLineEdit(QString::fromStdString(scalar.text));
                if (enabled) {
                    connect(edit, &QLineEdit::editingFinished, fields, [this, index, key, edit]() {
                        applyOpScalarEdit(index, key, RecipeScalarValue{edit->text().toStdString()});
                    });
                }
                widget = edit;
                break;
            }
            }
            widget->setObjectName(QStringLiteral("opField_%1").arg(key));
            // A bound number stays ENABLED (read-only) so its bind menu is
            // reachable; every other disabled state is the plain enabled flag.
            widget->setEnabled(scalar.kind == RecipeFieldKind::Number ? true : enabled);
            QString rowLabel = QString::fromStdString(scalar.label);
            if (binding != nullptr) {
                rowLabel += QStringLiteral("  ← %1.%2")
                                .arg(QString::fromStdString(binding->objectId),
                                     QString::fromStdString(binding->field));
            }
            form->addRow(rowLabel, widget);
        }
    };

    // Re-read the selected op's values into the EXISTING spins (no widget
    // delete) — signal-safe, so it runs on every opsStreamChanged, whether the
    // change came from this inspector, the editor's Apply, or an Open.
    auto refreshFieldValues = [this, fields, paramSpec](int index) {
        if (index < 0 || index >= static_cast<int>(m_opsStream.ops.size())) {
            return;
        }
        using edi::recipe::RecipeFieldKind;
        const edi::recipe::RecipeOp &op = m_opsStream.ops[static_cast<std::size_t>(index)];
        for (const edi::recipe::RecipeOpScalar &scalar : edi::recipe::opEditableScalars(op)) {
            const QString name = QStringLiteral("opField_%1").arg(QString::fromStdString(scalar.key));
            // A craftsman param's widget is whatever its manifest type built — so
            // refresh it by that type, not the schema's plain-text kind, or its
            // typed spin/combo would never follow an external (editor) edit.
            if (const edi::recipe::CraftsmanParam *spec = paramSpec(op, scalar.key)) {
                const std::string &type = spec->type;
                if (type == "number") {
                    if (auto *w = fields->findChild<QDoubleSpinBox *>(name)) {
                        double value = 0.0;
                        edi::recipe::parseNumberText(scalar.text, value);
                        w->setValue(value);
                    }
                } else if (type == "integer") {
                    if (auto *w = fields->findChild<QSpinBox *>(name)) {
                        w->setValue(QString::fromStdString(scalar.text).toInt());
                    }
                } else if (type == "material") {
                    if (auto *w = fields->findChild<QComboBox *>(name)) {
                        w->setCurrentText(QString::fromStdString(scalar.text));
                    }
                } else if (type == "bool" || type == "boolean") {
                    if (auto *w = fields->findChild<QCheckBox *>(name)) {
                        w->setChecked(scalar.text == "true");
                    }
                } else if (auto *w = fields->findChild<QLineEdit *>(name)) {
                    w->setText(QString::fromStdString(scalar.text));
                }
                continue;
            }
            switch (scalar.kind) {
            case RecipeFieldKind::Number:
                if (auto *w = fields->findChild<QDoubleSpinBox *>(name)) { w->setValue(scalar.number); }
                break;
            case RecipeFieldKind::Integer:
                if (auto *w = fields->findChild<QSpinBox *>(name)) { w->setValue(scalar.integer); }
                break;
            case RecipeFieldKind::Boolean:
                if (auto *w = fields->findChild<QCheckBox *>(name)) { w->setChecked(scalar.boolean); }
                break;
            case RecipeFieldKind::Choice:
                if (auto *w = fields->findChild<QComboBox *>(name)) { w->setCurrentText(QString::fromStdString(scalar.text)); }
                break;
            case RecipeFieldKind::Text:
                if (auto *w = fields->findChild<QLineEdit *>(name)) { w->setText(QString::fromStdString(scalar.text)); }
                break;
            }
        }
    };

    // Re-list the ops, preserving the selected row. Block the list's signals so
    // the programmatic reselect does not rebuild fields mid-refresh.
    auto rebuildList = [this, list]() {
        const QSignalBlocker block(list);
        const int keep = list->currentRow();
        list->clear();
        int i = 0;
        for (const edi::recipe::RecipeOp &op : m_opsStream.ops) {
            list->addItem(QStringLiteral("%1  %2")
                              .arg(i, 2, 10, QLatin1Char('0'))
                              .arg(QString::fromLatin1(edi::recipe::recipeOpTypeName(op))));
            ++i;
        }
        if (keep >= 0 && keep < list->count()) {
            list->setCurrentRow(keep);
        } else if (list->count() > 0) {
            list->setCurrentRow(0);
        }
    };

    // On a stream change, rebuild the spins only if the selected op's field
    // SHAPE changed (count or first key) — which happens only via Open/Apply, a
    // structural edit, never a spin's own value commit. So a rebuild here is
    // never mid-signal; a value-only change just refreshes the existing spins.
    auto syncToSelection = [this, fields, form, rebuildFields, refreshFieldValues](int index) {
        if (index < 0 || index >= static_cast<int>(m_opsStream.ops.size())) {
            rebuildFields(index); // clears the form
            return;
        }
        const std::vector<edi::recipe::RecipeOpScalar> desired =
            edi::recipe::opEditableScalars(m_opsStream.ops[static_cast<std::size_t>(index)]);
        bool sameShape = form->rowCount() == static_cast<int>(desired.size());
        if (sameShape && !desired.empty()) {
            sameShape = fields->findChild<QWidget *>(
                            QStringLiteral("opField_%1").arg(QString::fromStdString(desired.front().key)))
                        != nullptr;
        }
        // A binding flip (bound <-> literal) changes a number's read-only state
        // but not the field shape; detect it so the widget is rebuilt. A binding
        // only ever changes via the menu/verbs, never a value commit, so this
        // rebuild is still never reached mid-signal.
        if (sameShape) {
            for (const edi::recipe::RecipeOpScalar &scalar : desired) {
                if (scalar.kind != edi::recipe::RecipeFieldKind::Number) {
                    continue;
                }
                const bool boundNow = edi::recipe::findRecipeBinding(
                                          m_opsStream, static_cast<std::size_t>(index), scalar.key)
                                      != nullptr;
                auto *spin = fields->findChild<QDoubleSpinBox *>(
                    QStringLiteral("opField_%1").arg(QString::fromStdString(scalar.key)));
                if (spin != nullptr && spin->isReadOnly() != boundNow) {
                    sameShape = false;
                    break;
                }
            }
        }
        if (sameShape) {
            refreshFieldValues(index);
        } else {
            rebuildFields(index);
        }
    };

    connect(list, &QListWidget::currentRowChanged, panel,
            [rebuildFields](int row) { rebuildFields(row); });
    connect(this, &EdiShellWindow::opsStreamChanged, panel,
            [rebuildList, syncToSelection, list]() {
                rebuildList();
                syncToSelection(list->currentRow());
            });
    rebuildList();
    rebuildFields(list->currentRow());

    return panel;
}

QWidget *EdiShellWindow::buildStepPalettePanel()
{
    QFrame *panel = edi::shell::makeRegionFrame(QStringLiteral("stepPalettePanel"));
    auto *layout = new QVBoxLayout(panel);
    layout->setContentsMargins(12, 8, 12, 8);
    layout->setSpacing(6);
    auto *title = new QLabel(QStringLiteral("Add Step"));
    title->setObjectName(QStringLiteral("stepPaletteTitle"));
    layout->addWidget(title);

    // One button per palette op type — clicking appends a unit step (the
    // "string scripts by clicking" gesture); the Steps inspector and the proof
    // pick it up off opsStreamChanged. Data-driven from recipePaletteOpTypes(),
    // so a new palette type is a row in that table, not a button here.
    for (const std::string &type : edi::recipe::recipePaletteOpTypes()) {
        const QString typeName = QString::fromStdString(type);
        const QString label = typeName.startsWith(QStringLiteral("Add")) ? typeName.mid(3) : typeName;
        auto *button = new QPushButton(label);
        button->setObjectName(QStringLiteral("addStep_%1").arg(typeName));
        connect(button, &QPushButton::clicked, panel, [this, typeName]() { appendRecipeOp(typeName); });
        layout->addWidget(button);
    }

    // The custom craftsmen, from the loaded registry (edi_craft --list-craftsmen):
    // one button per craftsman, under its own heading, appending a Script step
    // seeded from the manifest. Empty registry ⇒ no heading, no buttons — the
    // built-in palette stands alone until a craftsman is installed + fed in.
    if (!m_craftsmen.empty()) {
        auto *craftsmenTitle = new QLabel(QStringLiteral("Craftsmen"));
        craftsmenTitle->setObjectName(QStringLiteral("craftsmanPaletteTitle"));
        layout->addWidget(craftsmenTitle);
        for (const edi::recipe::CraftsmanManifest &manifest : m_craftsmen) {
            const QString id = QString::fromStdString(manifest.id);
            auto *button = new QPushButton(QString::fromStdString(manifest.label));
            button->setObjectName(QStringLiteral("addCraftsman_%1").arg(id));
            connect(button, &QPushButton::clicked, panel, [this, id]() { appendScriptStep(id); });
            layout->addWidget(button);
        }
    }

    layout->addStretch(1);
    return panel;
}

void EdiShellWindow::showRenderImage(const QString &imagePath)
{
    m_lastRenderImagePath = imagePath;
    auto *label = findChild<QLabel *>(QStringLiteral("blenderPreview"));
    if (label == nullptr) {
        return; // the preview pane is not mounted (not in the Blender profile)
    }
    const QPixmap pixmap(imagePath);
    if (pixmap.isNull()) {
        label->setText(QStringLiteral("could not load the render: %1").arg(imagePath));
        return;
    }
    const int targetWidth = label->width() > 16 ? label->width() : 320;
    label->setPixmap(pixmap.scaledToWidth(targetWidth, Qt::SmoothTransformation));
}

bool EdiShellWindow::saveTextSession(const QString &path) const
{
    return edi::io::saveTextSessionToPath(m_textStore, path).ok;
}

bool EdiShellWindow::loadTextSession(const QString &path)
{
    // Remember the path either way: a first run has no manifest yet, but closing
    // should still write one there (the loadWorkspaceLayout precedent).
    m_textSessionPath = path;
    if (!QFile::exists(path)) {
        return true; // first run: keep the seeded scratch — nothing to restore
    }
    edi::io::TextSessionLoad loaded = edi::io::loadTextSessionFromPath(path);
    if (!loaded.ok) {
        // A manifest that EXISTS but will not parse is a full refusal, named:
        // surface it and keep the current store (corrupt state is not adopted).
        m_featureContext.textSessionNote = QString::fromStdString(loaded.message);
        if (m_featureContext.refreshTextPanel) {
            m_featureContext.refreshTextPanel();
        }
        return false;
    }
    m_textStore = std::move(loaded.store);
    seedScratchIfEmpty(); // an empty manifest never leaves the editor blank
    // Degrade notes (a skipped backing file) ride the bus to the status line.
    QStringList notes;
    for (const std::string &note : loaded.skipped) {
        notes << QString::fromStdString(note);
    }
    m_featureContext.textSessionNote = notes.join(QLatin1Char('\n'));
    if (m_featureContext.refreshTextPanel) {
        m_featureContext.refreshTextPanel(); // re-project the swapped store
    }
    return true;
}
