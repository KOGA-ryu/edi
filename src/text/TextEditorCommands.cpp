#include "text/TextEditorCommands.h"

#include <utility>

namespace edi::text {

namespace {

TextCommandResult fromStoreResult(const TextStoreResult &result)
{
    return {result.ok, result.message};
}

TextCommandResult replaceRange(TextDocument &document, TextRange range, const std::string &text)
{
    range = clampRange(range, document.text.size());
    document.text.replace(range.start, range.end - range.start, text);
    markDirty(document);
    return {true, {}};
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
                return {false, "document does not exist"};
            }
            document->title = typedCommand.title;
            markDirty(*document);
            return {true, {}};
        } else if constexpr (std::is_same_v<Command, SetTextDocumentRoleCommand>) {
            return fromStoreResult(updateDocumentRole(store, typedCommand.documentId, typedCommand.role));
        } else if constexpr (std::is_same_v<Command, InsertTextCommand>) {
            TextDocument *document = findDocument(store, typedCommand.documentId);
            if (document == nullptr) {
                return {false, "document does not exist"};
            }
            return replaceRange(*document, {typedCommand.offset, typedCommand.offset}, typedCommand.text);
        } else if constexpr (std::is_same_v<Command, ReplaceTextRangeCommand>) {
            TextDocument *document = findDocument(store, typedCommand.documentId);
            if (document == nullptr) {
                return {false, "document does not exist"};
            }
            return replaceRange(*document, typedCommand.range, typedCommand.text);
        } else {
            TextDocument *document = findDocument(store, typedCommand.documentId);
            if (document == nullptr) {
                return {false, "document does not exist"};
            }
            return replaceRange(*document, typedCommand.range, {});
        }
    }, command);
}

} // namespace edi::text
