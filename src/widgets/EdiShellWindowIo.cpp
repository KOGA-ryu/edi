#include "widgets/EdiShellWindow.h"

#include <QFile>
#include <QFileDialog>
#include <QSaveFile>
#include <QFileInfo>
#include <QMessageBox>
#include <QString>
#include <QStringList>
#include <QUrl>

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
