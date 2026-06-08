#include "text/TextDocumentStore.h"

#include <algorithm>
#include <utility>

namespace edi::text {

TextStoreResult TextStoreResult::accepted()
{
    return {true, {}};
}

TextStoreResult TextStoreResult::rejected(std::string message)
{
    return {false, std::move(message)};
}

TextDocument *findDocument(TextDocumentStore &store, const TextDocumentId &id)
{
    auto it = std::find_if(store.documents.begin(), store.documents.end(), [&](const TextDocument &document) {
        return document.id == id;
    });
    return it == store.documents.end() ? nullptr : &*it;
}

const TextDocument *findDocument(const TextDocumentStore &store, const TextDocumentId &id)
{
    auto it = std::find_if(store.documents.begin(), store.documents.end(), [&](const TextDocument &document) {
        return document.id == id;
    });
    return it == store.documents.end() ? nullptr : &*it;
}

TextStoreResult addDocument(TextDocumentStore &store, TextDocument document)
{
    if (document.id.empty()) {
        return TextStoreResult::rejected("document id is required");
    }
    if (findDocument(store, document.id) != nullptr) {
        return TextStoreResult::rejected("document id already exists");
    }
    if (!store.activeDocumentId) {
        store.activeDocumentId = document.id;
    }
    store.documents.push_back(std::move(document));
    return TextStoreResult::accepted();
}

TextStoreResult removeDocument(TextDocumentStore &store, const TextDocumentId &id)
{
    const auto before = store.documents.size();
    store.documents.erase(
        std::remove_if(store.documents.begin(), store.documents.end(), [&](const TextDocument &document) {
            return document.id == id;
        }),
        store.documents.end());
    if (store.documents.size() == before) {
        return TextStoreResult::rejected("document does not exist");
    }
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
    if (findDocument(store, id) == nullptr) {
        return TextStoreResult::rejected("document does not exist");
    }
    store.activeDocumentId = std::move(id);
    return TextStoreResult::accepted();
}

TextStoreResult updateDocumentRole(TextDocumentStore &store, const TextDocumentId &id, TextDocumentRole role)
{
    TextDocument *document = findDocument(store, id);
    if (document == nullptr) {
        return TextStoreResult::rejected("document does not exist");
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
