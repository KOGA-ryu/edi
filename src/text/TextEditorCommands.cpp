#include "text/TextEditorCommands.h"

#include <utility>

namespace edi::text {

TextCommandResult TextCommandResult::accepted()
{
    return {true, TextResultCode::None, {}};
}

TextCommandResult TextCommandResult::rejected(TextResultCode code, std::string message)
{
    return {false, code, std::move(message)};
}

namespace {

TextCommandResult fromStoreResult(const TextStoreResult &result)
{
    if (result.ok) {
        return TextCommandResult::accepted();
    }
    return TextCommandResult::rejected(result.code, result.message);
}

TextCommandResult replaceRange(TextDocument &document, TextRange range, const std::string &text)
{
    const auto validation = validateTextRange(range, document.text.size());
    if (!validation.ok) {
        return TextCommandResult::rejected(validation.code, validation.message);
    }
    document.text.replace(range.start, range.end - range.start, text);
    markDirty(document);
    return TextCommandResult::accepted();
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
                return TextCommandResult::rejected(TextResultCode::DocumentNotFound, "document does not exist");
            }
            if (!isValidTextDocumentTitle(typedCommand.title)) {
                return TextCommandResult::rejected(TextResultCode::InvalidTitle, "document title is required");
            }
            document->title = typedCommand.title;
            markDirty(*document);
            return TextCommandResult::accepted();
        } else if constexpr (std::is_same_v<Command, SetTextDocumentRoleCommand>) {
            return fromStoreResult(updateDocumentRole(store, typedCommand.documentId, typedCommand.role));
        } else if constexpr (std::is_same_v<Command, InsertTextCommand>) {
            TextDocument *document = findDocument(store, typedCommand.documentId);
            if (document == nullptr) {
                return TextCommandResult::rejected(TextResultCode::DocumentNotFound, "document does not exist");
            }
            const auto validation = validateInsertionOffset(typedCommand.offset, document->text.size());
            if (!validation.ok) {
                return TextCommandResult::rejected(validation.code, validation.message);
            }
            return replaceRange(*document, {typedCommand.offset, typedCommand.offset}, typedCommand.text);
        } else if constexpr (std::is_same_v<Command, ReplaceTextRangeCommand>) {
            TextDocument *document = findDocument(store, typedCommand.documentId);
            if (document == nullptr) {
                return TextCommandResult::rejected(TextResultCode::DocumentNotFound, "document does not exist");
            }
            return replaceRange(*document, typedCommand.range, typedCommand.text);
        } else {
            TextDocument *document = findDocument(store, typedCommand.documentId);
            if (document == nullptr) {
                return TextCommandResult::rejected(TextResultCode::DocumentNotFound, "document does not exist");
            }
            return replaceRange(*document, typedCommand.range, {});
        }
    }, command);
}

} // namespace edi::text
