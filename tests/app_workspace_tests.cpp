#include "app/AppState.h"
#include "app/ProjectWorkspace.h"

#include <cassert>
#include <optional>
#include <string>

using namespace edi::app;
using namespace edi::drafting;

int main()
{
    AppState state = defaultAppState();
    assert(!isDirty(state));
    assert(state.mode == WorkspaceMode::Drafting);
    setWorkspaceMode(state, WorkspaceMode::Text);
    assert(workspaceModeName(state.mode) == std::string("text"));
    assert(workspaceModeLabel(WorkspaceMode::Project) == std::string("Project"));
    assert(workspaceModeIcon(WorkspaceMode::Planning) == std::string("R"));
    assert(workspaceModeTooltip(WorkspaceMode::Settings) == std::string("Application and project settings"));
    assert(workspaceModeFromName("project") == WorkspaceMode::Project);
    assert(!workspaceModeFromName("missing"));
    const auto activities = defaultWorkspaceActivities();
    assert(activities.size() == 5);
    assert(activities.front().mode == WorkspaceMode::Drafting);
    assert(activities.front().enabled);
    assert(!activities[1].enabled);
    setStatusMessage(state, "ready");
    assert(state.statusMessage == "ready");

    ProjectWorkspace workspace = makeProjectWorkspace("project_1");
    assert(workspace.id == "project_1");
    assert(workspace.name == "project_1");
    ProjectWorkspace explicitName = makeProjectWorkspace("project_named", "Named Project");
    assert(explicitName.name == "Named Project");
    ProjectWorkspace emptyWorkspace = makeProjectWorkspace("");
    assert(emptyWorkspace.id.empty());
    assert(emptyWorkspace.name.empty());
    assert(isValidWorkspaceId("project_1"));
    assert(!isValidWorkspaceId(""));
    assert(projectWorkspaceResultCodeName(ProjectWorkspaceResultCode::DuplicateDocumentId) == std::string("duplicate_document_id"));
    assert(isValidWorkspaceName(workspace.name));

    auto first = addDraftingDocument(workspace, makeDraftingDocument("draft_1"));
    assert(first.ok);
    assert(first.code == ProjectWorkspaceResultCode::None);
    assert(workspace.activeDraftingDocumentId == "draft_1");
    assert(draftingDocumentIndexById(workspace, "draft_1") == 0);
    assert(findDraftingDocument(workspace, "draft_1") == &workspace.draftingDocuments[0]);
    assert(containsDraftingDocument(workspace, "draft_1"));

    auto second = addDraftingDocument(workspace, makeDraftingDocument("draft_2"));
    assert(second.ok);
    assert(workspace.activeDraftingDocumentId == "draft_1");
    assert(workspace.draftingDocuments[0].id == "draft_1");
    assert(workspace.draftingDocuments[1].id == "draft_2");
    assert(draftingDocumentIndexById(workspace, "draft_2") == 1);
    assert(findDraftingDocument(workspace, "draft_2") == &workspace.draftingDocuments[1]);
    assert(containsDraftingDocument(workspace, "draft_2"));
    assert(draftingDocumentIndexById(workspace, "missing") == std::nullopt);
    assert(findDraftingDocument(workspace, "missing") == nullptr);
    assert(!containsDraftingDocument(workspace, "missing"));

    auto duplicate = addDraftingDocument(workspace, makeDraftingDocument("draft_1"));
    assert(!duplicate.ok);
    assert(duplicate.code == ProjectWorkspaceResultCode::DuplicateDocumentId);
    assert(workspace.draftingDocuments.size() == 2);
    assert(workspace.activeDraftingDocumentId == "draft_1");
    assert(workspace.draftingDocuments[0].id == "draft_1");
    assert(workspace.draftingDocuments[1].id == "draft_2");

    auto missingActive = setActiveDraftingDocument(workspace, "missing");
    assert(!missingActive.ok);
    assert(missingActive.code == ProjectWorkspaceResultCode::DocumentNotFound);
    assert(workspace.activeDraftingDocumentId == "draft_1");
    assert(workspace.draftingDocuments[0].id == "draft_1");
    assert(workspace.draftingDocuments[1].id == "draft_2");

    auto setActive = setActiveDraftingDocument(workspace, "draft_2");
    assert(setActive.ok);
    assert(workspace.activeDraftingDocumentId == "draft_2");

    auto removeActive = removeDraftingDocument(workspace, "draft_2");
    assert(removeActive.ok);
    assert(workspace.draftingDocuments.size() == 1);
    assert(workspace.draftingDocuments[0].id == "draft_1");
    assert(workspace.activeDraftingDocumentId == "draft_1");

    auto removeLast = removeDraftingDocument(workspace, "draft_1");
    assert(removeLast.ok);
    assert(!workspace.activeDraftingDocumentId);

    auto missingRemove = removeDraftingDocument(workspace, "draft_1");
    assert(!missingRemove.ok);
    assert(missingRemove.code == ProjectWorkspaceResultCode::DocumentNotFound);

    ProjectWorkspace invalidWorkspace = makeProjectWorkspace("");
    auto invalidAdd = addDraftingDocument(invalidWorkspace, makeDraftingDocument("draft"));
    assert(!invalidAdd.ok);
    assert(invalidAdd.code == ProjectWorkspaceResultCode::EmptyWorkspaceId);

    ProjectWorkspace invalidName = makeProjectWorkspace("project_2", "Project");
    invalidName.name.clear();
    auto invalidNameAdd = addDraftingDocument(invalidName, makeDraftingDocument("draft"));
    assert(!invalidNameAdd.ok);
    assert(invalidNameAdd.code == ProjectWorkspaceResultCode::InvalidWorkspaceName);

    return 0;
}
