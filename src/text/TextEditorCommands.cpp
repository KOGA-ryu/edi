#include "text/TextEditorCommands.h"

#include <utility>

namespace edi::text {

namespace {

TextCommandResult fromStoreResult(const TextStoreResult &result)
{
    return {result.ok, result.code, result.message};
}

TextCommandResult replaceRange(TextDocument &document, TextRange range, const std::string &text)
{
    const auto validation = validateTextRange(range, document.text.size());
    if (!validation.ok) {
        return {false, validation.code, validation.message};
    }
    document.text.replace(range.start, range.end - range.start, text);
    markDirty(document);
    return {true, TextResultCode::None, {}};
}

} // namespace

TextCommandResult applyTextEditorCommand(TextDocumentStore &store, const TextEditorCommand &command)
{
    return std::visit([&](const auto &typedCommand) -> TextCommandResult {
        using Command = std::decay_t<decltype(typedCommand)>;
        if constexpr (std::is_same_v<Command, CreateTextDocumentCommand>) {
            return fromStoreResult(addDocument(store, typedCommand.document));
        } else if constexpr (std::is_same_v<Command, RenameTextDocumentCommand>) {
            TextDocument *document = findDocument(store, typedCommand.documentId);
            if (document == nullptr) {
                return {false, TextResultCode::DocumentNotFound, "document does not exist"};
            }
            if (!isValidTitle(typedCommand.title)) {
                return {false, TextResultCode::InvalidTitle, "document title is required"};
            }
            document->title = typedCommand.title;
            markDirty(*document);
            return {true, TextResultCode::None, {}};
        } else if constexpr (std::is_same_v<Command, SetTextDocumentRoleCommand>) {
            return fromStoreResult(updateDocumentRole(store, typedCommand.documentId, typedCommand.role));
        } else if constexpr (std::is_same_v<Command, InsertTextCommand>) {
            TextDocument *document = findDocument(store, typedCommand.documentId);
            if (document == nullptr) {
                return {false, TextResultCode::DocumentNotFound, "document does not exist"};
            }
            const auto validation = validateInsertionOffset(typedCommand.offset, document->text.size());
            if (!validation.ok) {
                return {false, validation.code, validation.message};
            }
            return replaceRange(*document, {typedCommand.offset, typedCommand.offset}, typedCommand.text);
        } else if constexpr (std::is_same_v<Command, ReplaceTextRangeCommand>) {
            TextDocument *document = findDocument(store, typedCommand.documentId);
            if (document == nullptr) {
                return {false, TextResultCode::DocumentNotFound, "document does not exist"};
            }
            return replaceRange(*document, typedCommand.range, typedCommand.text);
        } else {
            TextDocument *document = findDocument(store, typedCommand.documentId);
            if (document == nullptr) {
                return {false, TextResultCode::DocumentNotFound, "document does not exist"};
            }
            return replaceRange(*document, typedCommand.range, {});
        }
    }, command);
}

} // namespace edi::text
