#include "io/TextSessionStore.h"

#include "io/SettingsStore.h" // appConfigFilePath — the one config-root decision
#include "formats/TomlReader.h"
#include "formats/TomlWriter.h"

#include <QByteArray>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSaveFile>

#include <set>
#include <string>
#include <utility>

namespace edi::io {

using edi::formats::StaticConfig;
using edi::text::TextDocument;
using edi::text::TextDocumentRole;
using edi::text::TextDocumentStore;

QString defaultTextSessionPath()
{
    return appConfigFilePath(QStringLiteral("text_session.toml"));
}

TextSessionResult saveTextSessionToPath(const TextDocumentStore &store, const QString &path)
{
    StaticConfig config;
    if (store.activeDocumentId.has_value()) {
        config["session.active"] = *store.activeDocumentId;
    }
    for (std::size_t index = 0; index < store.documents.size(); ++index) {
        const TextDocument &document = store.documents[index];
        const std::string prefix = "doc." + std::to_string(index);
        config[prefix + ".id"] = document.id;
        config[prefix + ".title"] = document.title;
        config[prefix + ".role"] = edi::text::textDocumentRoleName(document.role);
        // E3: a FILE-BACKED document (metadata.fields["path"], the core's own
        // extension point) stores its PATH — the file is the truth, re-read on
        // restore, so externally edited content appears and the manifest never
        // carries a stale snapshot. A scratch document stores its TEXT (the
        // terminal notes survive restart). Exactly one of the two — the same
        // both-sources rule the loader enforces.
        const auto pathField = document.metadata.fields.find("path");
        if (pathField != document.metadata.fields.end() && !pathField->second.empty()) {
            config[prefix + ".path"] = pathField->second;
        } else {
            config[prefix + ".text"] = document.text;
        }
    }

    const auto written = edi::formats::writeTomlStaticConfig(config, path.toStdString());
    if (!written.ok || !written.value) {
        return {false, written.message};
    }
    const QFileInfo info(path);
    if (!info.absoluteDir().exists() && !QDir().mkpath(info.absolutePath())) {
        return {false, "could not create " + info.absolutePath().toStdString()};
    }
    // QSaveFile: atomic rename on commit, the existing manifest untouched on any
    // failure — a half-written session is worse than the previous one.
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        return {false, "could not write " + path.toStdString()};
    }
    file.write(QByteArray::fromStdString(*written.value));
    if (!file.commit()) {
        return {false, "could not write " + path.toStdString()};
    }
    return {true, {}};
}

namespace {

// Consumption-audit reader, the house idiom (the ops store): every key taken is
// recorded; the final sweep refuses anything left over BY NAME.
struct ManifestReader {
    const StaticConfig &config;
    std::set<std::string> consumed;

    const std::string *take(const std::string &key)
    {
        const auto found = config.find(key);
        if (found == config.end()) {
            return nullptr;
        }
        consumed.insert(key);
        return &found->second;
    }
};

// A backing file's content. False if it cannot be opened — the caller degrades
// (skips the document, notes the path) rather than guessing the content.
bool readBackingFile(const QString &path, std::string &out)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }
    out = QString::fromUtf8(file.readAll()).toStdString();
    return true;
}

} // namespace

TextSessionLoad loadTextSessionFromPath(const QString &path)
{
    TextSessionLoad result;

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        result.message = "could not read " + path.toStdString();
        return result;
    }
    const std::string text = QString::fromUtf8(file.readAll()).toStdString();
    // An empty (or whitespace-only) manifest opens an EMPTY session — the host
    // re-seeds the scratch document so the editor is never blank. This is not a
    // refusal: nothing is corrupt, there is simply nothing to restore.
    if (text.find_first_not_of(" \t\r\n") == std::string::npos) {
        result.ok = true;
        return result;
    }
    const auto parsed = edi::formats::readTomlStaticConfig(text, path.toStdString());
    if (!parsed.ok || !parsed.value) {
        // A manifest that EXISTS but will not parse is a FULL refusal, named —
        // corrupt state is not degradable.
        result.message = parsed.message;
        return result;
    }
    const StaticConfig &config = *parsed.value;

    ManifestReader reader{config, {}};
    const std::string *active = reader.take("session.active");

    // Documents are doc.0, doc.1, ... contiguous from zero. Read until the next
    // id is absent; a gap (doc.0 then doc.2) leaves doc.2.* unconsumed and the
    // audit below names it.
    for (std::size_t index = 0;; ++index) {
        const std::string prefix = "doc." + std::to_string(index);
        const std::string *id = reader.take(prefix + ".id");
        if (id == nullptr) {
            break;
        }
        if (id->empty()) {
            result.message = prefix + ".id: a document id must not be empty";
            return result;
        }
        TextDocument document = edi::text::makeTextDocument(*id);
        if (const std::string *title = reader.take(prefix + ".title")) {
            document.title = *title;
        }
        if (const std::string *roleName = reader.take(prefix + ".role")) {
            const TextDocumentRole role = edi::text::textDocumentRoleFromName(*roleName);
            // FromName silently defaults an unknown name to Scratch, so validate
            // by ROUND-TRIP: the name is known iff it survives the round trip.
            if (std::string(edi::text::textDocumentRoleName(role)) != *roleName) {
                result.message = prefix + ".role: unknown role '" + *roleName + "'";
                return result;
            }
            document.role = role;
        }

        const std::string *inlineText = reader.take(prefix + ".text");
        const std::string *backingPath = reader.take(prefix + ".path");
        if (inlineText != nullptr && backingPath != nullptr) {
            // Both sources is a lie in waiting (which one is the truth?) —
            // refuse, the house both-sources rule.
            result.message = prefix + ": has both .text and .path";
            return result;
        }
        if (inlineText != nullptr) {
            document.text = *inlineText;
        } else if (backingPath != nullptr) {
            std::string content;
            if (!readBackingFile(QString::fromStdString(*backingPath), content)) {
                // DEGRADE: skip this document, note it by path, keep loading.
                result.skipped.push_back("session: could not read " + *backingPath);
                continue;
            }
            document.text = std::move(content);
            // The path survives the round trip — without this, the NEXT save
            // would silently demote the document to a text snapshot.
            document.metadata.fields["path"] = *backingPath;
        }
        // (neither key: an empty document — valid, the writer always emits .text
        // but a hand-authored manifest may omit it.)

        const edi::text::TextStoreResult added =
            edi::text::addDocument(result.store, std::move(document));
        if (!added.ok) {
            result.message = prefix + ": " + added.message;
            return result;
        }
    }

    // Consumption audit: any key the reader did not take is unknown or gapped.
    for (const auto &entry : config) {
        if (reader.consumed.find(entry.first) != reader.consumed.end()) {
            continue;
        }
        if (entry.first.rfind("doc.", 0) == 0) {
            result.message = entry.first + ": gapped or unknown document index";
        } else {
            result.message = "unknown session key: " + entry.first;
        }
        return result;
    }

    // The active id must name a document that actually loaded; a manifest whose
    // active was skipped (degraded) simply opens with no active — not fatal.
    if (active != nullptr && edi::text::containsDocument(result.store, *active)) {
        result.store.activeDocumentId = *active;
    }

    result.ok = true;
    return result;
}

} // namespace edi::io
