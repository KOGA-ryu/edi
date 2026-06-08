#include "app/AppState.h"

#include <utility>

namespace edi::app {

AppState defaultAppState()
{
    return {};
}

bool isDirty(const AppState &state)
{
    return state.dirty;
}

void setWorkspaceMode(AppState &state, WorkspaceMode mode)
{
    state.mode = mode;
}

void setStatusMessage(AppState &state, std::string message)
{
    state.statusMessage = std::move(message);
}

const char *workspaceModeName(WorkspaceMode mode)
{
    switch (mode) {
    case WorkspaceMode::Drafting:
        return "drafting";
    case WorkspaceMode::Text:
        return "text";
    case WorkspaceMode::Planning:
        return "planning";
    }
    return "drafting";
}

} // namespace edi::app
