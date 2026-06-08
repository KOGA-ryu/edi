#pragma once

#include <optional>
#include <string>

namespace edi::app {

enum class WorkspaceMode {
    Drafting,
    Text,
    Planning
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

} // namespace edi::app
