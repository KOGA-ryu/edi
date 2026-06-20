#include "app/AppState.h"
#include "app/ProjectWorkspace.h"

#include "EdiAssert.h"
#include <optional>
#include <string>

using namespace edi::app;
using namespace edi::drafting;

int main()
{
    AppState state = defaultAppState();
    EDI_CHECK(!isDirty(state));
    EDI_CHECK(state.mode == WorkspaceMode::Drafting);
    setWorkspaceMode(state, WorkspaceMode::Text);
    EDI_CHECK(workspaceModeName(state.mode) == std::string("text"));
    EDI_CHECK(workspaceModeLabel(WorkspaceMode::Project) == std::string("Project"));
    EDI_CHECK(workspaceModeIcon(WorkspaceMode::Planning) == std::string("R"));
    EDI_CHECK(workspaceModeTooltip(WorkspaceMode::Settings) == std::string("Application and project settings"));
    EDI_CHECK(workspaceModeFromName("project") == WorkspaceMode::Project);
    EDI_CHECK(workspaceModeFromName("blender") == WorkspaceMode::Blender);
    EDI_CHECK(!workspaceModeFromName("missing"));
    // Blender is the second real workspace (the rest are still placeholders).
    EDI_CHECK(workspaceModeName(WorkspaceMode::Blender) == std::string("blender"));
    EDI_CHECK(workspaceModeLabel(WorkspaceMode::Blender) == std::string("Blender"));
    // Map is the third real workspace (the dungeon-authoring surface): every
    // mode helper must round-trip it, since the rail builds its button from
    // these tables by name.
    EDI_CHECK(workspaceModeName(WorkspaceMode::Map) == std::string("map"));
    EDI_CHECK(workspaceModeLabel(WorkspaceMode::Map) == std::string("Map"));
    EDI_CHECK(workspaceModeIcon(WorkspaceMode::Map) == std::string("M"));
    EDI_CHECK(workspaceModeFromName("map") == WorkspaceMode::Map);
    const auto activities = defaultWorkspaceActivities();
    EDI_CHECK(activities.size() == 7);
    EDI_CHECK(activities.front().mode == WorkspaceMode::Drafting);
    EDI_CHECK(activities.front().enabled);
    EDI_CHECK(!activities[1].enabled);
    EDI_CHECK(activities[4].mode == WorkspaceMode::Blender && activities[4].enabled);
    EDI_CHECK(activities[5].mode == WorkspaceMode::Map && activities[5].enabled);
    EDI_CHECK(activities[6].mode == WorkspaceMode::Settings);
    setStatusMessage(state, "ready");
    EDI_CHECK(state.statusMessage == "ready");

    ProjectWorkspace workspace = makeProjectWorkspace("project_1");
    EDI_CHECK(workspace.id == "project_1");
    EDI_CHECK(workspace.name == "project_1");
    ProjectWorkspace explicitName = makeProjectWorkspace("project_named", "Named Project");
    EDI_CHECK(explicitName.name == "Named Project");
    ProjectWorkspace emptyWorkspace = makeProjectWorkspace("");
    EDI_CHECK(emptyWorkspace.id.empty());
    EDI_CHECK(emptyWorkspace.name.empty());
    EDI_CHECK(isValidWorkspaceId("project_1"));
    EDI_CHECK(!isValidWorkspaceId(""));
    EDI_CHECK(projectWorkspaceResultCodeName(ProjectWorkspaceResultCode::DuplicateDocumentId) == std::string("duplicate_document_id"));
    EDI_CHECK(isValidWorkspaceName(workspace.name));

    auto first = addDraftingDocument(workspace, makeDraftingDocument("draft_1"));
    EDI_CHECK(first.ok);
    EDI_CHECK(first.code == ProjectWorkspaceResultCode::None);
    EDI_CHECK(workspace.activeDraftingDocumentId == "draft_1");
    EDI_CHECK(draftingDocumentIndexById(workspace, "draft_1") == 0);
    EDI_CHECK(findDraftingDocument(workspace, "draft_1") == &workspace.draftingDocuments[0]);
    EDI_CHECK(containsDraftingDocument(workspace, "draft_1"));

    auto second = addDraftingDocument(workspace, makeDraftingDocument("draft_2"));
    EDI_CHECK(second.ok);
    EDI_CHECK(workspace.activeDraftingDocumentId == "draft_1");
    EDI_CHECK(workspace.draftingDocuments[0].id == "draft_1");
    EDI_CHECK(workspace.draftingDocuments[1].id == "draft_2");
    EDI_CHECK(draftingDocumentIndexById(workspace, "draft_2") == 1);
    EDI_CHECK(findDraftingDocument(workspace, "draft_2") == &workspace.draftingDocuments[1]);
    EDI_CHECK(containsDraftingDocument(workspace, "draft_2"));
    EDI_CHECK(draftingDocumentIndexById(workspace, "missing") == std::nullopt);
    EDI_CHECK(findDraftingDocument(workspace, "missing") == nullptr);
    EDI_CHECK(!containsDraftingDocument(workspace, "missing"));

    auto duplicate = addDraftingDocument(workspace, makeDraftingDocument("draft_1"));
    EDI_CHECK(!duplicate.ok);
    EDI_CHECK(duplicate.code == ProjectWorkspaceResultCode::DuplicateDocumentId);
    EDI_CHECK(workspace.draftingDocuments.size() == 2);
    EDI_CHECK(workspace.activeDraftingDocumentId == "draft_1");
    EDI_CHECK(workspace.draftingDocuments[0].id == "draft_1");
    EDI_CHECK(workspace.draftingDocuments[1].id == "draft_2");

    auto missingActive = setActiveDraftingDocument(workspace, "missing");
    EDI_CHECK(!missingActive.ok);
    EDI_CHECK(missingActive.code == ProjectWorkspaceResultCode::DocumentNotFound);
    EDI_CHECK(workspace.activeDraftingDocumentId == "draft_1");
    EDI_CHECK(workspace.draftingDocuments[0].id == "draft_1");
    EDI_CHECK(workspace.draftingDocuments[1].id == "draft_2");

    auto setActive = setActiveDraftingDocument(workspace, "draft_2");
    EDI_CHECK(setActive.ok);
    EDI_CHECK(workspace.activeDraftingDocumentId == "draft_2");

    auto removeActive = removeDraftingDocument(workspace, "draft_2");
    EDI_CHECK(removeActive.ok);
    EDI_CHECK(workspace.draftingDocuments.size() == 1);
    EDI_CHECK(workspace.draftingDocuments[0].id == "draft_1");
    EDI_CHECK(workspace.activeDraftingDocumentId == "draft_1");

    auto removeLast = removeDraftingDocument(workspace, "draft_1");
    EDI_CHECK(removeLast.ok);
    EDI_CHECK(!workspace.activeDraftingDocumentId);

    auto missingRemove = removeDraftingDocument(workspace, "draft_1");
    EDI_CHECK(!missingRemove.ok);
    EDI_CHECK(missingRemove.code == ProjectWorkspaceResultCode::DocumentNotFound);

    ProjectWorkspace invalidWorkspace = makeProjectWorkspace("");
    auto invalidAdd = addDraftingDocument(invalidWorkspace, makeDraftingDocument("draft"));
    EDI_CHECK(!invalidAdd.ok);
    EDI_CHECK(invalidAdd.code == ProjectWorkspaceResultCode::EmptyWorkspaceId);

    ProjectWorkspace invalidName = makeProjectWorkspace("project_2", "Project");
    invalidName.name.clear();
    auto invalidNameAdd = addDraftingDocument(invalidName, makeDraftingDocument("draft"));
    EDI_CHECK(!invalidNameAdd.ok);
    EDI_CHECK(invalidNameAdd.code == ProjectWorkspaceResultCode::InvalidWorkspaceName);

    return 0;
}
