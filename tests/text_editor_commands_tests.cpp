#include "text/TextEditorCommands.h"

#include <cassert>

using namespace edi::text;

int main()
{
    TextDocumentStore store;
    auto create = applyTextEditorCommand(store, CreateTextDocumentCommand{makeTextDocument("text_1")});
    assert(create.ok);

    auto insert = applyTextEditorCommand(store, InsertTextCommand{"text_1", 0, "hello"});
    assert(insert.ok);
    assert(findDocument(store, "text_1")->text == "hello");

    auto replace = applyTextEditorCommand(store, ReplaceTextRangeCommand{"text_1", {1, 4}, "ipp"});
    assert(replace.ok);
    assert(findDocument(store, "text_1")->text == "hippo");

    auto erase = applyTextEditorCommand(store, DeleteTextRangeCommand{"text_1", {1, 5}});
    assert(erase.ok);
    assert(findDocument(store, "text_1")->text == "h");

    return 0;
}
