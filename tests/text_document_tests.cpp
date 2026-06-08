#include "text/TextDocumentStore.h"

#include <cassert>
#include <optional>
#include <string>

using namespace edi::text;

int main()
{
    TextDocumentStore store;
    assert(isValidTextDocumentId("text_1"));
    assert(!isValidTextDocumentId(""));
    TextDocument fallbackTitle = makeTextDocument("text_fallback");
    assert(fallbackTitle.title == "text_fallback");
    TextDocument explicitTitle = makeTextDocument("text_explicit", "Notes");
    assert(explicitTitle.title == "Notes");
    TextDocument emptyTextDocument = makeTextDocument("");
    assert(emptyTextDocument.id.empty());
    assert(emptyTextDocument.title.empty());
    TextDocument document = makeTextDocument("text_1", "Scratch");
    document.role = TextDocumentRole::Scratch;

    auto add = addDocument(store, document);
    assert(add.ok);
    assert(add.code == TextResultCode::None);
    assert(store.activeDocumentId == "text_1");
    assert(textDocumentIndexById(store, "text_1") == 0);
    assert(findDocument(store, "text_1") == &store.documents[0]);
    assert(containsDocument(store, "text_1"));

    auto addSecond = addDocument(store, makeTextDocument("text_2", "Reference"));
    assert(addSecond.ok);
    assert(store.documents.size() == 2);
    assert(store.documents[0].id == "text_1");
    assert(store.documents[1].id == "text_2");
    assert(textDocumentIndexById(store, "text_2") == 1);
    assert(findDocument(store, "text_2") == &store.documents[1]);
    assert(containsDocument(store, "text_2"));
    assert(textDocumentIndexById(store, "missing") == std::nullopt);
    assert(findDocument(store, "missing") == nullptr);
    assert(!containsDocument(store, "missing"));

    auto duplicate = addDocument(store, document);
    assert(!duplicate.ok);
    assert(duplicate.code == TextResultCode::DuplicateDocumentId);
    assert(store.documents.size() == 2);
    assert(store.documents[0].id == "text_1");
    assert(store.documents[1].id == "text_2");

    TextDocument emptyId = makeTextDocument("");
    auto emptyIdResult = addDocument(store, emptyId);
    assert(!emptyIdResult.ok);
    assert(emptyIdResult.code == TextResultCode::EmptyDocumentId);

    auto role = updateDocumentRole(store, "text_1", TextDocumentRole::Prompt);
    assert(role.ok);
    const TextDocument *updated = findDocument(store, "text_1");
    assert(updated != nullptr);
    assert(updated->role == TextDocumentRole::Prompt);
    assert(updated->dirty);

    auto promptDocs = listDocumentsByRole(store, TextDocumentRole::Prompt);
    assert(promptDocs.size() == 1);

    auto missingActive = setActiveDocument(store, "missing");
    assert(!missingActive.ok);
    assert(missingActive.code == TextResultCode::DocumentNotFound);
    assert(store.activeDocumentId == "text_1");

    auto missingRemove = removeDocument(store, "missing");
    assert(!missingRemove.ok);
    assert(missingRemove.code == TextResultCode::DocumentNotFound);
    assert(store.activeDocumentId == "text_1");
    assert(store.documents.size() == 2);
    assert(store.documents[0].id == "text_1");
    assert(store.documents[1].id == "text_2");

    auto removeFirst = removeDocument(store, "text_1");
    assert(removeFirst.ok);
    assert(store.documents.size() == 1);
    assert(store.documents[0].id == "text_2");
    assert(store.activeDocumentId == "text_2");
    assert(textDocumentIndexById(store, "text_2") == 0);

    assert(textResultCodeName(TextResultCode::InvalidRange) == std::string("invalid_range"));
    assert(isValidTextDocumentTitle("Title"));
    assert(!isValidTextDocumentTitle(""));
    assert(isValidTitle("Title"));
    assert(!isValidTitle(""));

    return 0;
}
