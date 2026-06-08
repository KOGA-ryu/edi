#include "app/ProjectWorkspace.h"

#include <algorithm>
#include <utility>

namespace edi::app {

ProjectWorkspaceResult ProjectWorkspaceResult::accepted()
{
    return {true, {}};
}

ProjectWorkspaceResult ProjectWorkspaceResult::rejected(std::string message)
{
    return {false, std::move(message)};
}

ProjectWorkspace makeProjectWorkspace(std::string id, std::string name)
{
    ProjectWorkspace workspace;
    workspace.id = std::move(id);
    workspace.name = name.empty() ? workspace.id : std::move(name);
    return workspace;
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
    if (document.id.empty()) {
        return ProjectWorkspaceResult::rejected("drafting document id is required");
    }
    if (findDraftingDocument(workspace, document.id) != nullptr) {
        return ProjectWorkspaceResult::rejected("drafting document id already exists");
    }
    if (!workspace.activeDraftingDocumentId) {
        workspace.activeDraftingDocumentId = document.id;
    }
    workspace.draftingDocuments.push_back(std::move(document));
    return ProjectWorkspaceResult::accepted();
}

ProjectWorkspaceResult removeDraftingDocument(ProjectWorkspace &workspace, const edi::drafting::DraftingDocumentId &id)
{
    const auto before = workspace.draftingDocuments.size();
    workspace.draftingDocuments.erase(
        std::remove_if(workspace.draftingDocuments.begin(), workspace.draftingDocuments.end(), [&](const auto &document) {
            return document.id == id;
        }),
        workspace.draftingDocuments.end());
    if (workspace.draftingDocuments.size() == before) {
        return ProjectWorkspaceResult::rejected("drafting document does not exist");
    }
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
    if (findDraftingDocument(workspace, id) == nullptr) {
        return ProjectWorkspaceResult::rejected("drafting document does not exist");
    }
    workspace.activeDraftingDocumentId = std::move(id);
    return ProjectWorkspaceResult::accepted();
}

} // namespace edi::app
