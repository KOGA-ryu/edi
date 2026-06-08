#pragma once

#include "text/TextDocumentStore.h"
#include "text/TextSelection.h"

#include <variant>

namespace edi::text {

struct CreateTextDocumentCommand {
    TextDocument document;
};

struct RenameTextDocumentCommand {
    TextDocumentId documentId;
    std::string title;
};

struct SetTextDocumentRoleCommand {
    TextDocumentId documentId;
    TextDocumentRole role = TextDocumentRole::Scratch;
};

struct InsertTextCommand {
    TextDocumentId documentId;
    std::size_t offset = 0;
    std::string text;
};

struct ReplaceTextRangeCommand {
    TextDocumentId documentId;
    TextRange range;
    std::string text;
};

struct DeleteTextRangeCommand {
    TextDocumentId documentId;
    TextRange range;
};

using TextEditorCommand = std::variant<
    CreateTextDocumentCommand,
    RenameTextDocumentCommand,
    SetTextDocumentRoleCommand,
    InsertTextCommand,
    ReplaceTextRangeCommand,
    DeleteTextRangeCommand>;

struct TextCommandResult {
    bool ok = false;
    TextResultCode code = TextResultCode::None;
    std::string message;

    static TextCommandResult accepted();
    static TextCommandResult rejected(TextResultCode code, std::string message);
};

TextCommandResult applyTextEditorCommand(TextDocumentStore &store, const TextEditorCommand &command);

} // namespace edi::text
