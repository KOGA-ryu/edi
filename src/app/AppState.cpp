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
    case WorkspaceMode::Project:
        return "project";
    case WorkspaceMode::Planning:
        return "planning";
    case WorkspaceMode::Settings:
        return "settings";
    }
    return "drafting";
}

const char *workspaceModeLabel(WorkspaceMode mode)
{
    switch (mode) {
    case WorkspaceMode::Drafting:
        return "Drafting";
    case WorkspaceMode::Text:
        return "Text";
    case WorkspaceMode::Project:
        return "Project";
    case WorkspaceMode::Planning:
        return "Planning";
    case WorkspaceMode::Settings:
        return "Settings";
    }
    return "Drafting";
}

const char *workspaceModeIcon(WorkspaceMode mode)
{
    switch (mode) {
    case WorkspaceMode::Drafting:
        return "D";
    case WorkspaceMode::Text:
        return "T";
    case WorkspaceMode::Project:
        return "P";
    case WorkspaceMode::Planning:
        return "R";
    case WorkspaceMode::Settings:
        return "S";
    }
    return "D";
}

const char *workspaceModeTooltip(WorkspaceMode mode)
{
    switch (mode) {
    case WorkspaceMode::Drafting:
        return "Drafting canvas and drawing tools";
    case WorkspaceMode::Text:
        return "Text editor surface";
    case WorkspaceMode::Project:
        return "Project workspace surface";
    case WorkspaceMode::Planning:
        return "Review and planning surface";
    case WorkspaceMode::Settings:
        return "Application and project settings";
    }
    return "Drafting canvas and drawing tools";
}

std::optional<WorkspaceMode> workspaceModeFromName(const std::string &name)
{
    if (name == "drafting") {
        return WorkspaceMode::Drafting;
    }
    if (name == "text") {
        return WorkspaceMode::Text;
    }
    if (name == "project") {
        return WorkspaceMode::Project;
    }
    if (name == "planning") {
        return WorkspaceMode::Planning;
    }
    if (name == "settings") {
        return WorkspaceMode::Settings;
    }
    return std::nullopt;
}

std::vector<WorkspaceActivity> defaultWorkspaceActivities()
{
    return {
        {WorkspaceMode::Drafting, "drafting", "D", "Drafting", "Drafting canvas and drawing tools", true},
        {WorkspaceMode::Text, "text", "T", "Text", "Text editor surface", false},
        {WorkspaceMode::Project, "project", "P", "Project", "Project workspace surface", false},
        {WorkspaceMode::Planning, "planning", "R", "Planning", "Review and planning surface", false},
        {WorkspaceMode::Settings, "settings", "S", "Settings", "Application and project settings", false},
    };
}

} // namespace edi::app
