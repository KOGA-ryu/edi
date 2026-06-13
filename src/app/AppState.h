#pragma once

#include <optional>
#include <string>
#include <vector>

namespace edi::app {

enum class WorkspaceMode {
    Drafting,
    Text,
    Project,
    Planning,
    Blender,
    Settings
};

struct WorkspaceActivity {
    WorkspaceMode mode = WorkspaceMode::Drafting;
    const char *id = "drafting";
    const char *icon = "D";
    const char *label = "Drafting";
    const char *tooltip = "Drafting tools";
    bool enabled = false;
};

struct AppState {
    std::optional<std::string> activeWorkspaceId;
    std::optional<std::string> activeDraftingDocumentId;
    std::optional<std::string> activeTextDocumentId;
    WorkspaceMode mode = WorkspaceMode::Drafting;
    bool dirty = false;
    std::string statusMessage;
};

AppState defaultAppState();
bool isDirty(const AppState &state);
void setWorkspaceMode(AppState &state, WorkspaceMode mode);
void setStatusMessage(AppState &state, std::string message);
const char *workspaceModeName(WorkspaceMode mode);
const char *workspaceModeLabel(WorkspaceMode mode);
const char *workspaceModeIcon(WorkspaceMode mode);
const char *workspaceModeTooltip(WorkspaceMode mode);
std::optional<WorkspaceMode> workspaceModeFromName(const std::string &name);
std::vector<WorkspaceActivity> defaultWorkspaceActivities();

} // namespace edi::app
