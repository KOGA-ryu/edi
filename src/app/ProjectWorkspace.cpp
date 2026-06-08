#include "app/ProjectWorkspace.h"

#include <algorithm>
#include <utility>

namespace edi::app {

ProjectWorkspaceResult ProjectWorkspaceResult::accepted()
{
    return {true, ProjectWorkspaceResultCode::None, {}};
}

ProjectWorkspaceResult ProjectWorkspaceResult::rejected(ProjectWorkspaceResultCode code, std::string message)
{
    return {false, code, std::move(message)};
}

ProjectWorkspace makeProjectWorkspace(std::string id, std::string name)
{
    ProjectWorkspace workspace;
    workspace.id = std::move(id);
    workspace.name = name.empty() ? workspace.id : std::move(name);
    return workspace;
}

const char *projectWorkspaceResultCodeName(ProjectWorkspaceResultCode code)
{
    switch (code) {
    case ProjectWorkspaceResultCode::None:
        return "none";
    case ProjectWorkspaceResultCode::EmptyWorkspaceId:
        return "empty_workspace_id";
    case ProjectWorkspaceResultCode::EmptyDocumentId:
        return "empty_document_id";
    case ProjectWorkspaceResultCode::DuplicateDocumentId:
        return "duplicate_document_id";
    case ProjectWorkspaceResultCode::DocumentNotFound:
        return "document_not_found";
    case ProjectWorkspaceResultCode::InvalidWorkspaceName:
        return "invalid_workspace_name";
    }
    return "unknown";
}

bool isValidWorkspaceName(const std::string &name)
{
    return !name.empty();
}

edi::drafting::DraftingDocument *findDraftingDocument(ProjectWorkspace &workspace, const edi::drafting::DraftingDocumentId &id)
{
    auto it = std::find_if(workspace.draftingDocuments.begin(), workspace.draftingDocuments.end(), [&](const auto &document) {
        return document.id == id;
    });
    return it == workspace.draftingDocuments.end() ? nullptr : &*it;
}

const edi::drafting::DraftingDocument *findDraftingDocument(const ProjectWorkspace &workspace, const edi::drafting::DraftingDocumentId &id)
{
    auto it = std::find_if(workspace.draftingDocuments.begin(), workspace.draftingDocuments.end(), [&](const auto &document) {
        return document.id == id;
    });
    return it == workspace.draftingDocuments.end() ? nullptr : &*it;
}

ProjectWorkspaceResult addDraftingDocument(ProjectWorkspace &workspace, edi::drafting::DraftingDocument document)
{
    if (workspace.id.empty()) {
        return ProjectWorkspaceResult::rejected(ProjectWorkspaceResultCode::EmptyWorkspaceId, "workspace id is required");
    }
    if (!isValidWorkspaceName(workspace.name)) {
        return ProjectWorkspaceResult::rejected(ProjectWorkspaceResultCode::InvalidWorkspaceName, "workspace name is required");
    }
    if (document.id.empty()) {
        return ProjectWorkspaceResult::rejected(ProjectWorkspaceResultCode::EmptyDocumentId, "drafting document id is required");
    }
    if (findDraftingDocument(workspace, document.id) != nullptr) {
        return ProjectWorkspaceResult::rejected(ProjectWorkspaceResultCode::DuplicateDocumentId, "drafting document id already exists");
    }
    if (!workspace.activeDraftingDocumentId) {
        workspace.activeDraftingDocumentId = document.id;
    }
    workspace.draftingDocuments.push_back(std::move(document));
    return ProjectWorkspaceResult::accepted();
}

ProjectWorkspaceResult removeDraftingDocument(ProjectWorkspace &workspace, const edi::drafting::DraftingDocumentId &id)
{
    if (id.empty()) {
        return ProjectWorkspaceResult::rejected(ProjectWorkspaceResultCode::EmptyDocumentId, "drafting document id is required");
    }
    if (findDraftingDocument(workspace, id) == nullptr) {
        return ProjectWorkspaceResult::rejected(ProjectWorkspaceResultCode::DocumentNotFound, "drafting document does not exist");
    }

    const auto before = workspace.draftingDocuments.size();
    workspace.draftingDocuments.erase(
        std::remove_if(workspace.draftingDocuments.begin(), workspace.draftingDocuments.end(), [&](const auto &document) {
            return document.id == id;
        }),
        workspace.draftingDocuments.end());
    if (workspace.activeDraftingDocumentId == id) {
        if (workspace.draftingDocuments.empty()) {
            workspace.activeDraftingDocumentId.reset();
        } else {
            workspace.activeDraftingDocumentId = workspace.draftingDocuments.front().id;
        }
    }
    return ProjectWorkspaceResult::accepted();
}

ProjectWorkspaceResult setActiveDraftingDocument(ProjectWorkspace &workspace, edi::drafting::DraftingDocumentId id)
{
    if (id.empty()) {
        return ProjectWorkspaceResult::rejected(ProjectWorkspaceResultCode::EmptyDocumentId, "drafting document id is required");
    }
    if (findDraftingDocument(workspace, id) == nullptr) {
        return ProjectWorkspaceResult::rejected(ProjectWorkspaceResultCode::DocumentNotFound, "drafting document does not exist");
    }
    workspace.activeDraftingDocumentId = std::move(id);
    return ProjectWorkspaceResult::accepted();
}

} // namespace edi::app
