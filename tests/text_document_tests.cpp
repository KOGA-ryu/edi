#include "text/TextDocumentStore.h"

#include <cassert>

using namespace edi::text;

int main()
{
    TextDocumentStore store;
    TextDocument document = makeTextDocument("text_1", "Scratch");
    document.role = TextDocumentRole::Scratch;

    auto add = addDocument(store, document);
    assert(add.ok);
    assert(add.code == TextResultCode::None);
    assert(store.activeDocumentId == "text_1");

    auto duplicate = addDocument(store, document);
    assert(!duplicate.ok);
    assert(duplicate.code == TextResultCode::DuplicateDocumentId);

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

    assert(textResultCodeName(TextResultCode::InvalidRange) == std::string("invalid_range"));
    assert(isValidTitle("Title"));
    assert(!isValidTitle(""));

    return 0;
}
