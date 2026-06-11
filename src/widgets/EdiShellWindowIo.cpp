#include "widgets/EdiShellWindow.h"

#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>
#include <QString>
#include <QUrl>

#include <algorithm>
#include <string>
#include <vector>

#include "core/DrawingCore.h"
#include "core/RecipeController.h"
#include "recipe/RecipeEmit.h"
#include "recipe/RecipeStore.h"
#include "io/ShellLayoutStore.h"
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

bool EdiShellWindow::saveRecipeToPath(const QString &path)
{
    using namespace edi::recipe;
    if (path.isEmpty()) {
        return false;
    }
    const RecipeTextResult written = recipeToToml(m_recipeController->document());
    if (!written.ok) {
        m_lastRecipeError = QString::fromStdString(written.message);
        return false;
    }
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        m_lastRecipeError = QStringLiteral("could not write %1").arg(path);
        return false;
    }
    file.write(written.text.data(), static_cast<qint64>(written.text.size()));
    m_lastRecipeError.clear();
    return true;
}

bool EdiShellWindow::openRecipeFromPath(const QString &path)
{
    using namespace edi::recipe;
    if (path.isEmpty()) {
        return false;
    }
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        m_lastRecipeError = QStringLiteral("could not read %1").arg(path);
        return false;
    }
    const QByteArray bytes = file.readAll();
    const RecipeParseResult parsed = recipeFromToml(
        std::string(bytes.constData(), static_cast<std::size_t>(bytes.size())),
        path.toStdString());
    if (!parsed.ok) {
        // The strict loader names the offending key — surface it verbatim,
        // it IS the pointable diagnosis.
        m_lastRecipeError = QString::fromStdString(parsed.message);
        return false;
    }
    m_recipeController->adoptDocument(parsed.document);
    m_lastRecipeError.clear();
    return true;
}

bool EdiShellWindow::exportRecipePythonToPath(const QString &path)
{
    using namespace edi::recipe;
    if (path.isEmpty()) {
        return false;
    }
    // Resolve against the LIVE drafting document and grid — the bus seam the
    // recipe feature was built around. Refusal (stale binding, missing
    // profile) is a hard stop: a script with a guess in it is the failure
    // mode this pipeline exists to kill.
    const ResolvedRecipe resolved = resolveRecipe(
        m_recipeController->document(),
        m_controller->draftingDocument(),
        m_controller->draftingGridProjection());
    const RecipeEmitResult emitted = emitBlenderPython(m_recipeController->document(), resolved);
    if (!emitted.ok) {
        m_lastRecipeError = QString::fromStdString(emitted.message);
        return false;
    }
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        m_lastRecipeError = QStringLiteral("could not write %1").arg(path);
        return false;
    }
    file.write(emitted.script.data(), static_cast<qint64>(emitted.script.size()));
    m_lastRecipeError.clear();
    return true;
}

void EdiShellWindow::promptOpenRecipe()
{
    const QString path = QFileDialog::getOpenFileName(
        this, QStringLiteral("Open Recipe"), QString(),
        QStringLiteral("Recipes (*.toml)"));
    if (path.isEmpty()) {
        return;
    }
    openRecipeFromPath(path);
}

void EdiShellWindow::promptSaveRecipe()
{
    QString path = QFileDialog::getSaveFileName(
        this, QStringLiteral("Save Recipe"), QString(),
        QStringLiteral("Recipes (*.toml)"));
    if (path.isEmpty()) {
        return;
    }
    if (!path.endsWith(QStringLiteral(".toml"))) {
        path += QStringLiteral(".toml");
    }
    saveRecipeToPath(path);
}

void EdiShellWindow::promptExportRecipePython()
{
    QString path = QFileDialog::getSaveFileName(
        this, QStringLiteral("Export Blender Python"), QString(),
        QStringLiteral("Python (*.py)"));
    if (path.isEmpty()) {
        return;
    }
    if (!path.endsWith(QStringLiteral(".py"))) {
        path += QStringLiteral(".py");
    }
    exportRecipePythonToPath(path);
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
