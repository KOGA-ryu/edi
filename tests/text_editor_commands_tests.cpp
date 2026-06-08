#include "text/TextEditorCommands.h"

#include <cassert>
#include <string>

using namespace edi::text;

int main()
{
    TextDocumentStore store;
    auto create = applyTextEditorCommand(store, CreateTextDocumentCommand{makeTextDocument("text_1")});
    assert(create.ok);
    assert(create.code == TextResultCode::None);

    auto insert = applyTextEditorCommand(store, InsertTextCommand{"text_1", 0, "hello"});
    assert(insert.ok);
    assert(findDocument(store, "text_1")->text == "hello");
    const auto revisionAfterInsert = findDocument(store, "text_1")->revision;

    auto invalidInsert = applyTextEditorCommand(store, InsertTextCommand{"text_1", 99, "!"});
    assert(!invalidInsert.ok);
    assert(invalidInsert.code == TextResultCode::InvalidRange);
    assert(findDocument(store, "text_1")->text == "hello");
    assert(findDocument(store, "text_1")->revision == revisionAfterInsert);

    auto replace = applyTextEditorCommand(store, ReplaceTextRangeCommand{"text_1", {1, 4}, "ipp"});
    assert(replace.ok);
    assert(findDocument(store, "text_1")->text == "hippo");
    const auto revisionAfterReplace = findDocument(store, "text_1")->revision;

    auto invalidReplace = applyTextEditorCommand(store, ReplaceTextRangeCommand{"text_1", {4, 1}, "bad"});
    assert(!invalidReplace.ok);
    assert(invalidReplace.code == TextResultCode::InvalidRange);
    assert(findDocument(store, "text_1")->text == "hippo");
    assert(findDocument(store, "text_1")->revision == revisionAfterReplace);

    auto invalidDelete = applyTextEditorCommand(store, DeleteTextRangeCommand{"text_1", {0, 99}});
    assert(!invalidDelete.ok);
    assert(invalidDelete.code == TextResultCode::InvalidRange);
    assert(findDocument(store, "text_1")->text == "hippo");
    assert(findDocument(store, "text_1")->revision == revisionAfterReplace);

    auto emptyRename = applyTextEditorCommand(store, RenameTextDocumentCommand{"text_1", ""});
    assert(!emptyRename.ok);
    assert(emptyRename.code == TextResultCode::InvalidTitle);
    assert(findDocument(store, "text_1")->revision == revisionAfterReplace);

    auto missingDoc = applyTextEditorCommand(store, InsertTextCommand{"missing", 0, "x"});
    assert(!missingDoc.ok);
    assert(missingDoc.code == TextResultCode::DocumentNotFound);

    auto erase = applyTextEditorCommand(store, DeleteTextRangeCommand{"text_1", {1, 5}});
    assert(erase.ok);
    assert(findDocument(store, "text_1")->text == "h");

    return 0;
}
