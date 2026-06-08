#pragma once

#include <cstdint>
#include <map>
#include <string>

namespace edi::text {

using TextDocumentId = std::string;

enum class TextDocumentRole {
    Scratch,
    Prompt,
    Context,
    Reference,
    BuildNote
};

struct TextDocumentMetadata {
    std::string author;
    std::string source;
    std::map<std::string, std::string> fields;
};

struct TextDocument {
    TextDocumentId id;
    std::string title;
    TextDocumentRole role = TextDocumentRole::Scratch;
    std::string text;
    TextDocumentMetadata metadata;
    bool dirty = false;
    std::uint64_t revision = 0;
};

TextDocument makeTextDocument(TextDocumentId id, std::string title = {});
const char *textDocumentRoleName(TextDocumentRole role);
TextDocumentRole textDocumentRoleFromName(const std::string &name);
void markDirty(TextDocument &document);
void markClean(TextDocument &document);

} // namespace edi::text
