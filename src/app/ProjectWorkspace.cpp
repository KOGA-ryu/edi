#include "app/ProjectWorkspace.h"

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

bool isValidWorkspaceId(const std::string &id)
{
    return !id.empty();
}

std::optional<std::size_t> draftingDocumentIndexById(const ProjectWorkspace &workspace, const edi::drafting::DraftingDocumentId &id)
{
    for (std::size_t index = 0; index < workspace.draftingDocuments.size(); ++index) {
        if (workspace.draftingDocuments[index].id == id) {
            return index;
        }
    }
    return std::nullopt;
}

edi::drafting::DraftingDocument *findDraftingDocument(ProjectWorkspace &workspace, const edi::drafting::DraftingDocumentId &id)
{
    const auto index = draftingDocumentIndexById(workspace, id);
    return index ? &workspace.draftingDocuments[*index] : nullptr;
}

const edi::drafting::DraftingDocument *findDraftingDocument(const ProjectWorkspace &workspace, const edi::drafting::DraftingDocumentId &id)
{
    const auto index = draftingDocumentIndexById(workspace, id);
    return index ? &workspace.draftingDocuments[*index] : nullptr;
}

bool containsDraftingDocument(const ProjectWorkspace &workspace, const edi::drafting::DraftingDocumentId &id)
{
    return draftingDocumentIndexById(workspace, id).has_value();
}

ProjectWorkspaceResult addDraftingDocument(ProjectWorkspace &workspace, edi::drafting::DraftingDocument document)
{
    if (!isValidWorkspaceId(workspace.id)) {
        return ProjectWorkspaceResult::rejected(ProjectWorkspaceResultCode::EmptyWorkspaceId, "workspace id is required");
    }
    if (!isValidWorkspaceName(workspace.name)) {
        return ProjectWorkspaceResult::rejected(ProjectWorkspaceResultCode::InvalidWorkspaceName, "workspace name is required");
    }
    if (!edi::drafting::isValidDraftingDocumentId(document.id)) {
        return ProjectWorkspaceResult::rejected(ProjectWorkspaceResultCode::EmptyDocumentId, "drafting document id is required");
    }
    if (containsDraftingDocument(workspace, document.id)) {
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
    if (!edi::drafting::isValidDraftingDocumentId(id)) {
        return ProjectWorkspaceResult::rejected(ProjectWorkspaceResultCode::EmptyDocumentId, "drafting document id is required");
    }
    const auto index = draftingDocumentIndexById(workspace, id);
    if (!index) {
        return ProjectWorkspaceResult::rejected(ProjectWorkspaceResultCode::DocumentNotFound, "drafting document does not exist");
    }

    workspace.draftingDocuments.erase(workspace.draftingDocuments.begin() + static_cast<std::ptrdiff_t>(*index));
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
    if (!edi::drafting::isValidDraftingDocumentId(id)) {
        return ProjectWorkspaceResult::rejected(ProjectWorkspaceResultCode::EmptyDocumentId, "drafting document id is required");
    }
    if (!containsDraftingDocument(workspace, id)) {
        return ProjectWorkspaceResult::rejected(ProjectWorkspaceResultCode::DocumentNotFound, "drafting document does not exist");
    }
    workspace.activeDraftingDocumentId = std::move(id);
    return ProjectWorkspaceResult::accepted();
}

} // namespace edi::app
