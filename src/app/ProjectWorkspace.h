#pragma once

#include "drafting/DraftingDocument.h"
#include "text/TextDocumentStore.h"

#include <optional>
#include <string>
#include <vector>

namespace edi::app {

enum class ProjectWorkspaceResultCode {
    None,
    EmptyWorkspaceId,
    EmptyDocumentId,
    DuplicateDocumentId,
    DocumentNotFound,
    InvalidWorkspaceName
};

struct ProjectWorkspace {
    std::string id;
    std::string name;
    std::vector<edi::drafting::DraftingDocument> draftingDocuments;
    edi::text::TextDocumentStore textDocuments;
    std::vector<std::string> assetLibraryIds;
    std::vector<std::string> toolPresetIds;
    std::optional<edi::drafting::DraftingDocumentId> activeDraftingDocumentId;
};

struct ProjectWorkspaceResult {
    bool ok = false;
    ProjectWorkspaceResultCode code = ProjectWorkspaceResultCode::None;
    std::string message;

    static ProjectWorkspaceResult accepted();
    static ProjectWorkspaceResult rejected(ProjectWorkspaceResultCode code, std::string message);
};

ProjectWorkspace makeProjectWorkspace(std::string id, std::string name = {});
const char *projectWorkspaceResultCodeName(ProjectWorkspaceResultCode code);
bool isValidWorkspaceName(const std::string &name);
edi::drafting::DraftingDocument *findDraftingDocument(ProjectWorkspace &workspace, const edi::drafting::DraftingDocumentId &id);
const edi::drafting::DraftingDocument *findDraftingDocument(const ProjectWorkspace &workspace, const edi::drafting::DraftingDocumentId &id);
ProjectWorkspaceResult addDraftingDocument(ProjectWorkspace &workspace, edi::drafting::DraftingDocument document);
ProjectWorkspaceResult removeDraftingDocument(ProjectWorkspace &workspace, const edi::drafting::DraftingDocumentId &id);
ProjectWorkspaceResult setActiveDraftingDocument(ProjectWorkspace &workspace, edi::drafting::DraftingDocumentId id);

} // namespace edi::app
