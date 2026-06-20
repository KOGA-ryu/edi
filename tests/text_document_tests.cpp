#include "text/TextDocumentStore.h"

#include "EdiAssert.h"
#include <optional>
#include <string>

using namespace edi::text;

int main()
{
    TextDocumentStore store;
    EDI_CHECK(isValidTextDocumentId("text_1"));
    EDI_CHECK(!isValidTextDocumentId(""));
    TextDocument fallbackTitle = makeTextDocument("text_fallback");
    EDI_CHECK(fallbackTitle.title == "text_fallback");
    TextDocument explicitTitle = makeTextDocument("text_explicit", "Notes");
    EDI_CHECK(explicitTitle.title == "Notes");
    TextDocument emptyTextDocument = makeTextDocument("");
    EDI_CHECK(emptyTextDocument.id.empty());
    EDI_CHECK(emptyTextDocument.title.empty());
    TextDocument document = makeTextDocument("text_1", "Scratch");
    document.role = TextDocumentRole::Scratch;

    auto add = addDocument(store, document);
    EDI_CHECK(add.ok);
    EDI_CHECK(add.code == TextResultCode::None);
    EDI_CHECK(store.activeDocumentId == "text_1");
    EDI_CHECK(textDocumentIndexById(store, "text_1") == 0);
    EDI_CHECK(findDocument(store, "text_1") == &store.documents[0]);
    EDI_CHECK(containsDocument(store, "text_1"));

    auto addSecond = addDocument(store, makeTextDocument("text_2", "Reference"));
    EDI_CHECK(addSecond.ok);
    EDI_CHECK(store.documents.size() == 2);
    EDI_CHECK(store.documents[0].id == "text_1");
    EDI_CHECK(store.documents[1].id == "text_2");
    EDI_CHECK(textDocumentIndexById(store, "text_2") == 1);
    EDI_CHECK(findDocument(store, "text_2") == &store.documents[1]);
    EDI_CHECK(containsDocument(store, "text_2"));
    EDI_CHECK(textDocumentIndexById(store, "missing") == std::nullopt);
    EDI_CHECK(findDocument(store, "missing") == nullptr);
    EDI_CHECK(!containsDocument(store, "missing"));

    auto duplicate = addDocument(store, document);
    EDI_CHECK(!duplicate.ok);
    EDI_CHECK(duplicate.code == TextResultCode::DuplicateDocumentId);
    EDI_CHECK(store.documents.size() == 2);
    EDI_CHECK(store.documents[0].id == "text_1");
    EDI_CHECK(store.documents[1].id == "text_2");

    TextDocument emptyId = makeTextDocument("");
    auto emptyIdResult = addDocument(store, emptyId);
    EDI_CHECK(!emptyIdResult.ok);
    EDI_CHECK(emptyIdResult.code == TextResultCode::EmptyDocumentId);

    auto role = updateDocumentRole(store, "text_1", TextDocumentRole::Prompt);
    EDI_CHECK(role.ok);
    const TextDocument *updated = findDocument(store, "text_1");
    EDI_CHECK(updated != nullptr);
    EDI_CHECK(updated->role == TextDocumentRole::Prompt);
    EDI_CHECK(updated->dirty);

    auto promptDocs = listDocumentsByRole(store, TextDocumentRole::Prompt);
    EDI_CHECK(promptDocs.size() == 1);

    auto missingActive = setActiveDocument(store, "missing");
    EDI_CHECK(!missingActive.ok);
    EDI_CHECK(missingActive.code == TextResultCode::DocumentNotFound);
    EDI_CHECK(store.activeDocumentId == "text_1");

    auto missingRemove = removeDocument(store, "missing");
    EDI_CHECK(!missingRemove.ok);
    EDI_CHECK(missingRemove.code == TextResultCode::DocumentNotFound);
    EDI_CHECK(store.activeDocumentId == "text_1");
    EDI_CHECK(store.documents.size() == 2);
    EDI_CHECK(store.documents[0].id == "text_1");
    EDI_CHECK(store.documents[1].id == "text_2");

    auto removeFirst = removeDocument(store, "text_1");
    EDI_CHECK(removeFirst.ok);
    EDI_CHECK(store.documents.size() == 1);
    EDI_CHECK(store.documents[0].id == "text_2");
    EDI_CHECK(store.activeDocumentId == "text_2");
    EDI_CHECK(textDocumentIndexById(store, "text_2") == 0);

    EDI_CHECK(textResultCodeName(TextResultCode::InvalidRange) == std::string("invalid_range"));
    EDI_CHECK(isValidTextDocumentTitle("Title"));
    EDI_CHECK(!isValidTextDocumentTitle(""));
    EDI_CHECK(isValidTitle("Title"));
    EDI_CHECK(!isValidTitle(""));

    return 0;
}
