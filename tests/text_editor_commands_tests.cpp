#include "text/TextEditorCommands.h"

#include "EdiAssert.h"
#include <string>

using namespace edi::text;

int main()
{
    TextDocumentStore store;
    auto acceptedResult = TextCommandResult::accepted();
    EDI_CHECK(acceptedResult.ok);
    EDI_CHECK(acceptedResult.code == TextResultCode::None);
    auto rejectedResult = TextCommandResult::rejected(TextResultCode::DocumentNotFound, "missing");
    EDI_CHECK(!rejectedResult.ok);
    EDI_CHECK(rejectedResult.code == TextResultCode::DocumentNotFound);

    auto create = applyTextEditorCommand(store, CreateTextDocumentCommand{makeTextDocument("text_1")});
    EDI_CHECK(create.ok);
    EDI_CHECK(create.code == TextResultCode::None);

    auto insert = applyTextEditorCommand(store, InsertTextCommand{"text_1", 0, "hello"});
    EDI_CHECK(insert.ok);
    EDI_CHECK(findDocument(store, "text_1")->text == "hello");
    const auto revisionAfterInsert = findDocument(store, "text_1")->revision;

    auto invalidInsert = applyTextEditorCommand(store, InsertTextCommand{"text_1", 99, "!"});
    EDI_CHECK(!invalidInsert.ok);
    EDI_CHECK(invalidInsert.code == TextResultCode::InvalidRange);
    EDI_CHECK(findDocument(store, "text_1")->text == "hello");
    EDI_CHECK(findDocument(store, "text_1")->revision == revisionAfterInsert);

    auto replace = applyTextEditorCommand(store, ReplaceTextRangeCommand{"text_1", {1, 4}, "ipp"});
    EDI_CHECK(replace.ok);
    EDI_CHECK(findDocument(store, "text_1")->text == "hippo");
    const auto revisionAfterReplace = findDocument(store, "text_1")->revision;

    auto invalidReplace = applyTextEditorCommand(store, ReplaceTextRangeCommand{"text_1", {4, 1}, "bad"});
    EDI_CHECK(!invalidReplace.ok);
    EDI_CHECK(invalidReplace.code == TextResultCode::InvalidRange);
    EDI_CHECK(findDocument(store, "text_1")->text == "hippo");
    EDI_CHECK(findDocument(store, "text_1")->revision == revisionAfterReplace);

    auto invalidDelete = applyTextEditorCommand(store, DeleteTextRangeCommand{"text_1", {0, 99}});
    EDI_CHECK(!invalidDelete.ok);
    EDI_CHECK(invalidDelete.code == TextResultCode::InvalidRange);
    EDI_CHECK(findDocument(store, "text_1")->text == "hippo");
    EDI_CHECK(findDocument(store, "text_1")->revision == revisionAfterReplace);

    auto emptyRename = applyTextEditorCommand(store, RenameTextDocumentCommand{"text_1", ""});
    EDI_CHECK(!emptyRename.ok);
    EDI_CHECK(emptyRename.code == TextResultCode::InvalidTitle);
    EDI_CHECK(findDocument(store, "text_1")->revision == revisionAfterReplace);

    auto missingDoc = applyTextEditorCommand(store, InsertTextCommand{"missing", 0, "x"});
    EDI_CHECK(!missingDoc.ok);
    EDI_CHECK(missingDoc.code == TextResultCode::DocumentNotFound);

    auto erase = applyTextEditorCommand(store, DeleteTextRangeCommand{"text_1", {1, 5}});
    EDI_CHECK(erase.ok);
    EDI_CHECK(findDocument(store, "text_1")->text == "h");

    return 0;
}
