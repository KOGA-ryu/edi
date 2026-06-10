#include "widgets/EdiShellWindow.h"

#include <QFileDialog>
#include <QFileInfo>
#include <QString>
#include <QUrl>

#include <algorithm>
#include <string>
#include <vector>

#include "core/DrawingCore.h"
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
        return false;
    }
    m_currentDrawingPath = path;
    rememberRecentFile(path);
    updateWindowTitle();
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
    updateWindowTitle();
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
    QString path = QFileDialog::getSaveFileName(
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
    const QString path = QFileDialog::getOpenFileName(
        this, QStringLiteral("Open Drawing"), QString(),
        QStringLiteral("EDI Drawings (*.edidraw)"));
    if (path.isEmpty()) {
        return;
    }
    openDrawingFromPath(path);
}

bool EdiShellWindow::exportSvgToPath(const QString &path)
{
    return !path.isEmpty() && m_controller->exportSvgDocument(QUrl::fromLocalFile(path));
}

bool EdiShellWindow::exportHpglToPath(const QString &path)
{
    return !path.isEmpty() && m_controller->exportHpglDocument(QUrl::fromLocalFile(path));
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
        switchWorkspaceLayout(data.layout);
    } else {
        // Same job: geometry only. Palette placements ride with the layout
        // like panel sizes do — adopt and re-apply without a remount.
        m_workspaceLayout.palettes = data.layout.palettes;
        applyPanelSizesToSplitters();
        refreshPanelVisibility();
        applyPalettePlacements();
    }
    return true;
}

bool EdiShellWindow::saveWorkspaceLayout(const QString &path) const
{
    return edi::io::saveShellLayoutToPath(path, m_workspaceLayout, m_panelsState);
}
