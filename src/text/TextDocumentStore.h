#pragma once

#include "text/TextDocument.h"

#include <optional>
#include <vector>

namespace edi::text {

struct TextStoreResult {
    bool ok = false;
    TextResultCode code = TextResultCode::None;
    std::string message;

    static TextStoreResult accepted();
    static TextStoreResult rejected(TextResultCode code, std::string message);
};

struct TextDocumentStore {
    std::vector<TextDocument> documents;
    std::optional<TextDocumentId> activeDocumentId;
};

TextDocument *findDocument(TextDocumentStore &store, const TextDocumentId &id);
const TextDocument *findDocument(const TextDocumentStore &store, const TextDocumentId &id);
TextStoreResult addDocument(TextDocumentStore &store, TextDocument document);
TextStoreResult removeDocument(TextDocumentStore &store, const TextDocumentId &id);
TextStoreResult setActiveDocument(TextDocumentStore &store, TextDocumentId id);
TextStoreResult updateDocumentRole(TextDocumentStore &store, const TextDocumentId &id, TextDocumentRole role);
std::vector<TextDocumentId> listDocumentsByRole(const TextDocumentStore &store, TextDocumentRole role);

} // namespace edi::text
