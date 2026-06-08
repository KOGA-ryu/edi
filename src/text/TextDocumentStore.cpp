#include "text/TextDocumentStore.h"

#include <utility>

namespace edi::text {

TextStoreResult TextStoreResult::accepted()
{
    return {true, TextResultCode::None, {}};
}

TextStoreResult TextStoreResult::rejected(TextResultCode code, std::string message)
{
    return {false, code, std::move(message)};
}

std::optional<std::size_t> textDocumentIndexById(const TextDocumentStore &store, const TextDocumentId &id)
{
    for (std::size_t index = 0; index < store.documents.size(); ++index) {
        if (store.documents[index].id == id) {
            return index;
        }
    }
    return std::nullopt;
}

TextDocument *findDocument(TextDocumentStore &store, const TextDocumentId &id)
{
    const auto index = textDocumentIndexById(store, id);
    return index ? &store.documents[*index] : nullptr;
}

const TextDocument *findDocument(const TextDocumentStore &store, const TextDocumentId &id)
{
    const auto index = textDocumentIndexById(store, id);
    return index ? &store.documents[*index] : nullptr;
}

bool containsDocument(const TextDocumentStore &store, const TextDocumentId &id)
{
    return textDocumentIndexById(store, id).has_value();
}

TextStoreResult addDocument(TextDocumentStore &store, TextDocument document)
{
    if (!isValidTextDocumentId(document.id)) {
        return TextStoreResult::rejected(TextResultCode::EmptyDocumentId, "document id is required");
    }
    if (containsDocument(store, document.id)) {
        return TextStoreResult::rejected(TextResultCode::DuplicateDocumentId, "document id already exists");
    }
    if (!store.activeDocumentId) {
        store.activeDocumentId = document.id;
    }
    store.documents.push_back(std::move(document));
    return TextStoreResult::accepted();
}

TextStoreResult removeDocument(TextDocumentStore &store, const TextDocumentId &id)
{
    const auto index = textDocumentIndexById(store, id);
    if (!index) {
        return TextStoreResult::rejected(TextResultCode::DocumentNotFound, "document does not exist");
    }
    store.documents.erase(store.documents.begin() + static_cast<std::ptrdiff_t>(*index));
    if (store.activeDocumentId == id) {
        if (store.documents.empty()) {
            store.activeDocumentId.reset();
        } else {
            store.activeDocumentId = store.documents.front().id;
        }
    }
    return TextStoreResult::accepted();
}

TextStoreResult setActiveDocument(TextDocumentStore &store, TextDocumentId id)
{
    if (!containsDocument(store, id)) {
        return TextStoreResult::rejected(TextResultCode::DocumentNotFound, "document does not exist");
    }
    store.activeDocumentId = std::move(id);
    return TextStoreResult::accepted();
}

TextStoreResult updateDocumentRole(TextDocumentStore &store, const TextDocumentId &id, TextDocumentRole role)
{
    TextDocument *document = findDocument(store, id);
    if (document == nullptr) {
        return TextStoreResult::rejected(TextResultCode::DocumentNotFound, "document does not exist");
    }
    document->role = role;
    markDirty(*document);
    return TextStoreResult::accepted();
}

std::vector<TextDocumentId> listDocumentsByRole(const TextDocumentStore &store, TextDocumentRole role)
{
    std::vector<TextDocumentId> ids;
    for (const TextDocument &document : store.documents) {
        if (document.role == role) {
            ids.push_back(document.id);
        }
    }
    return ids;
}

} // namespace edi::text
