#include "widgets/EdiShellWindow.h"

#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QSaveFile>
#include <QFileInfo>
#include <QComboBox>
#include <QFontDatabase>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPixmap>
#include <QString>
#include <QStringList>
#include <QUrl>
#include <QVBoxLayout>

#include <algorithm>
#include <string>
#include <vector>

#include "core/DrawingCore.h"
#include "recipe/RecipeOpsAscii.h"
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
        for (const edi::drafting::DraftingDeclaredConnection &conn : doc.connections) {
            list->addItem(QStringLiteral("⟷ %1 ↔ %2").arg(plugLabel(conn.plugA), plugLabel(conn.plugB)));
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

QString EdiShellWindow::renderOpsAsciiProjection(int projectionIndex)
{
    using namespace edi::recipe;
    if (m_opsStream.ops.empty()) {
        return QStringLiteral("No recipe loaded.\nOpen or edit an ops recipe to see its ASCII proof.");
    }
    // The same gate the CLI/export path uses (exportOpsPreviewsToDir): resolve
    // against the LIVE drawing, then compile, validate, render. A proof must
    // refuse what it cannot show, so every failure surfaces verbatim instead of
    // a misleading picture — "proof never guesses."
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
    const AsciiProjection projection = projectionIndex == 1 ? AsciiProjection::Side
                                     : projectionIndex == 2 ? AsciiProjection::Top
                                                            : AsciiProjection::Front;
    const AsciiRenderResult render = renderOpsProjection(compiled.ops, projection);
    if (!render.ok) {
        return QString::fromStdString(render.message);
    }
    return QString::fromStdString(render.text);
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
    header->addWidget(title);
    header->addStretch(1);
    header->addWidget(projection);
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
