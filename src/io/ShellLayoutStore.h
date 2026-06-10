#pragma once

#include "formats/TomlReader.h"
#include "widgets/ShellHost.h"
#include "widgets/ShellPanels.h"

#include <QString>

namespace edi::io {

// Workspace layout persistence — TOML, per the format decision table in
// docs/shell_architecture.md (layouts are jobs: human-editable, diffable).
// Free functions over plain structs, mirroring SettingsStore: the pure
// encode/decode pair is testable without touching a filesystem, and the path
// seams wrap it for the window.

// Decode result follows the plan-struct convention (ok + payload): a missing
// or unreadable file is not an error state the caller must unwind, it just
// means "use your built-in default layout".
struct ShellLayoutData {
    bool ok = false;
    edi::shell::WorkspaceLayout layout;
    edi::shell::ShellPanelsState panels;
};

edi::formats::StaticConfig workspaceLayoutToConfig(
    const edi::shell::WorkspaceLayout &layout, const edi::shell::ShellPanelsState &panels);

ShellLayoutData shellLayoutFromConfig(const edi::formats::StaticConfig &config);

ShellLayoutData loadShellLayoutFromPath(const QString &path);

bool saveShellLayoutToPath(const QString &path,
    const edi::shell::WorkspaceLayout &layout, const edi::shell::ShellPanelsState &panels);

// <AppConfigLocation>/workspace.toml — the current job, beside edi.toml.
QString defaultWorkspaceLayoutPath();

} // namespace edi::io
