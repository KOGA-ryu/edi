#include "text/TextDocument.h"

#include <utility>

namespace edi::text {

TextDocument makeTextDocument(TextDocumentId id, std::string title)
{
    TextDocument document;
    document.id = std::move(id);
    document.title = title.empty() ? document.id : std::move(title);
    return document;
}

const char *textDocumentRoleName(TextDocumentRole role)
{
    switch (role) {
    case TextDocumentRole::Scratch:
        return "scratch";
    case TextDocumentRole::Prompt:
        return "prompt";
    case TextDocumentRole::Context:
        return "context";
    case TextDocumentRole::Reference:
        return "reference";
    case TextDocumentRole::BuildNote:
        return "build_note";
    }
    return "scratch";
}

TextDocumentRole textDocumentRoleFromName(const std::string &name)
{
    if (name == "prompt") {
        return TextDocumentRole::Prompt;
    }
    if (name == "context") {
        return TextDocumentRole::Context;
    }
    if (name == "reference") {
        return TextDocumentRole::Reference;
    }
    if (name == "build_note") {
        return TextDocumentRole::BuildNote;
    }
    return TextDocumentRole::Scratch;
}

const char *textResultCodeName(TextResultCode code)
{
    switch (code) {
    case TextResultCode::None:
        return "none";
    case TextResultCode::EmptyDocumentId:
        return "empty_document_id";
    case TextResultCode::DuplicateDocumentId:
        return "duplicate_document_id";
    case TextResultCode::DocumentNotFound:
        return "document_not_found";
    case TextResultCode::InvalidRange:
        return "invalid_range";
    case TextResultCode::InvalidTitle:
        return "invalid_title";
    case TextResultCode::InvalidTextPayload:
        return "invalid_text_payload";
    }
    return "unknown";
}

bool isValidTitle(const std::string &title)
{
    return !title.empty();
}

void markDirty(TextDocument &document)
{
    document.dirty = true;
    ++document.revision;
}

void markClean(TextDocument &document)
{
    document.dirty = false;
}

} // namespace edi::text
