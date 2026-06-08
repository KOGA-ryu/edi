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
    assert(store.activeDocumentId == "text_1");

    auto duplicate = addDocument(store, document);
    assert(!duplicate.ok);

    auto role = updateDocumentRole(store, "text_1", TextDocumentRole::Prompt);
    assert(role.ok);
    const TextDocument *updated = findDocument(store, "text_1");
    assert(updated != nullptr);
    assert(updated->role == TextDocumentRole::Prompt);
    assert(updated->dirty);

    auto promptDocs = listDocumentsByRole(store, TextDocumentRole::Prompt);
    assert(promptDocs.size() == 1);

    return 0;
}
