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
