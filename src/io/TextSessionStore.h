#pragma once

#include "text/TextDocumentStore.h"

#include <QString>

#include <string>
#include <vector>

namespace edi::io {

// The text editor's SESSION manifest (E2): which documents are open, which is
// active, and — for each — EITHER the text itself (scratch notes ride in the
// manifest so they survive a restart) OR a path whose file holds the content.
// Format law: a text editor edits FILES (no container format for content), but
// the session IS configurable state, so it is TOML through the house
// `edi::formats` StaticConfig reader/writer — flat quoted-string keys, strict
// decode, the escaping path hardened in the port review (newlines round-trip).
//
// Manifest shape:
//   session.active = "<id>"
//   doc.0.id = "<id>"  doc.0.title = "<title>"  doc.0.role = "<role name>"
//   doc.0.text = "<content>"     # scratch: content in the manifest
//   doc.1.path = "<file path>"   # file-backed: content read from the file
//
// Roles serialize by NAME through the core's textDocumentRoleName /
// textDocumentRoleFromName; an unknown role name is refused (the core's
// FromName silently defaults to Scratch, so the reader validates by round-trip).

struct TextSessionResult {
    bool ok = false;
    std::string message;
};

// A load DEGRADES per document: a doc whose backing file is missing/unreadable
// is SKIPPED (not guessed) and noted here ("session: could not read <path>"),
// while the rest of the session loads. A manifest file that EXISTS but fails to
// parse is a FULL refusal (ok=false, named) — corrupt state is not degradable.
struct TextSessionLoad {
    bool ok = false;
    std::string message;
    edi::text::TextDocumentStore store;
    std::vector<std::string> skipped; // per-document degrade notes
};

// <AppConfigLocation>/text_session.toml — derived through the one config-root
// decision point (SettingsStore::appConfigFilePath), beside edi.toml.
QString defaultTextSessionPath();

// Writes the store as a manifest (text-form for every document — the store
// carries no per-document path). Atomic (QSaveFile), creating the dir.
TextSessionResult saveTextSessionToPath(const edi::text::TextDocumentStore &store, const QString &path);

// Reads a manifest. Strict (consumption audit, refusals name their key) with
// per-document degrade for missing backing files. The caller checks file
// existence for the first-run (no manifest) case; this assumes a readable file.
TextSessionLoad loadTextSessionFromPath(const QString &path);

} // namespace edi::io
