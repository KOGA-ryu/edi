// E2 — the text editor's SESSION manifest: a strict TOML file that restores the
// open documents and the active one across restarts. Scratch text (incl.
// newlines and quotes) rides in the manifest; file-backed documents reference a
// path whose file holds the content; a missing backing file DEGRADES (skip +
// note) while the rest loads; a manifest that exists but will not parse, or
// names an unknown key/role/gapped index, is a full refusal that names its
// offender. Every path here is a temp path — never the user's real config dir.
#include "io/TextSessionStore.h"

#include "text/TextDocumentStore.h"

#include <QCoreApplication>
#include <QFile>
#include <QTemporaryDir>

#include <cassert>
#include <string>

using namespace edi::io;
using namespace edi::text;

namespace {

void writeFile(const QString &path, const std::string &content)
{
    QFile file(path);
    assert(file.open(QIODevice::WriteOnly));
    file.write(content.data(), static_cast<qint64>(content.size()));
}

} // namespace

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    QTemporaryDir dir;
    assert(dir.isValid());

    // ---- Round trip: scratch text with NEWLINES and QUOTES, two roles, an
    // active document. The writer emits text-form; the load brings it all back
    // through the house TOML escaping (the port-hardened path). ----
    {
        TextDocumentStore store;
        TextDocument notes = makeTextDocument("scratch", "Scratch");
        notes.role = TextDocumentRole::Scratch;
        notes.text = "line one\nline two with a \"quote\"\nthird";
        assert(addDocument(store, notes).ok);
        TextDocument brief = makeTextDocument("brief", "Brief");
        brief.role = TextDocumentRole::Prompt;
        brief.text = "do the thing";
        assert(addDocument(store, brief).ok);
        assert(setActiveDocument(store, "brief").ok);

        const QString path = dir.filePath(QStringLiteral("session.toml"));
        const TextSessionResult saved = saveTextSessionToPath(store, path);
        assert(saved.ok);

        const TextSessionLoad loaded = loadTextSessionFromPath(path);
        assert(loaded.ok);
        assert(loaded.skipped.empty());
        assert(loaded.store.documents.size() == 2);
        assert(loaded.store.activeDocumentId.has_value()
               && *loaded.store.activeDocumentId == "brief");
        const TextDocument *scratchBack = findDocument(loaded.store, "scratch");
        assert(scratchBack != nullptr);
        assert(scratchBack->title == "Scratch");
        assert(scratchBack->role == TextDocumentRole::Scratch);
        // The newlines and the quote survived the manifest round trip.
        assert(scratchBack->text == "line one\nline two with a \"quote\"\nthird");
        const TextDocument *briefBack = findDocument(loaded.store, "brief");
        assert(briefBack != nullptr && briefBack->role == TextDocumentRole::Prompt);
    }

    // ---- File-backed: the content comes from the referenced file at load (the
    // reader form; the writer never emits .path). ----
    {
        const QString contentPath = dir.filePath(QStringLiteral("readme.txt"));
        writeFile(contentPath, "from the file\nsecond line");
        const QString manifest = dir.filePath(QStringLiteral("filebacked.toml"));
        writeFile(manifest,
                  "doc.0.id = \"readme\"\n"
                  "doc.0.title = \"README\"\n"
                  "doc.0.role = \"reference\"\n"
                  "doc.0.path = \"" + contentPath.toStdString() + "\"\n");
        const TextSessionLoad loaded = loadTextSessionFromPath(manifest);
        assert(loaded.ok && loaded.skipped.empty());
        assert(loaded.store.documents.size() == 1);
        const TextDocument *doc = findDocument(loaded.store, "readme");
        assert(doc != nullptr && doc->role == TextDocumentRole::Reference);
        assert(doc->text == "from the file\nsecond line"); // content, not the path
    }

    // ---- Missing backing file DEGRADES: that document is skipped and noted by
    // path; the rest of the session loads. ----
    {
        const QString missing = dir.filePath(QStringLiteral("gone.txt")); // never written
        const QString manifest = dir.filePath(QStringLiteral("degrade.toml"));
        writeFile(manifest,
                  "session.active = \"keep\"\n"
                  "doc.0.id = \"gone\"\n"
                  "doc.0.role = \"scratch\"\n"
                  "doc.0.path = \"" + missing.toStdString() + "\"\n"
                  "doc.1.id = \"keep\"\n"
                  "doc.1.role = \"scratch\"\n"
                  "doc.1.text = \"survivor\"\n");
        const TextSessionLoad loaded = loadTextSessionFromPath(manifest);
        assert(loaded.ok); // degrade, NOT refusal
        assert(loaded.store.documents.size() == 1);
        assert(findDocument(loaded.store, "gone") == nullptr);
        assert(findDocument(loaded.store, "keep") != nullptr);
        assert(loaded.skipped.size() == 1);
        assert(loaded.skipped[0] == "session: could not read " + missing.toStdString());
    }

    // ---- Strict refusals, each naming its offender. ----
    {
        // unknown key (consumption audit)
        const QString p1 = dir.filePath(QStringLiteral("unknown.toml"));
        writeFile(p1, "doc.0.id = \"a\"\ndoc.0.role = \"scratch\"\ndoc.0.text = \"x\"\nstray.key = \"y\"\n");
        const TextSessionLoad r1 = loadTextSessionFromPath(p1);
        assert(!r1.ok);
        assert(r1.message == "unknown session key: stray.key");

        // gapped doc index (doc.0 then doc.2 — doc.1 is missing)
        const QString p2 = dir.filePath(QStringLiteral("gapped.toml"));
        writeFile(p2,
                  "doc.0.id = \"a\"\ndoc.0.role = \"scratch\"\ndoc.0.text = \"x\"\n"
                  "doc.2.id = \"c\"\ndoc.2.role = \"scratch\"\ndoc.2.text = \"z\"\n");
        const TextSessionLoad r2 = loadTextSessionFromPath(p2);
        assert(!r2.ok);
        assert(r2.message == "doc.2.id: gapped or unknown document index");

        // bad role name (FromName silently defaults to Scratch; the reader
        // round-trip-validates and refuses the unknown name).
        const QString p3 = dir.filePath(QStringLiteral("badrole.toml"));
        writeFile(p3, "doc.0.id = \"a\"\ndoc.0.role = \"wizard\"\ndoc.0.text = \"x\"\n");
        const TextSessionLoad r3 = loadTextSessionFromPath(p3);
        assert(!r3.ok);
        assert(r3.message == "doc.0.role: unknown role 'wizard'");

        // both .text and .path (house both-sources rule)
        const QString p4 = dir.filePath(QStringLiteral("both.toml"));
        writeFile(p4, "doc.0.id = \"a\"\ndoc.0.role = \"scratch\"\ndoc.0.text = \"x\"\ndoc.0.path = \"/tmp/x\"\n");
        const TextSessionLoad r4 = loadTextSessionFromPath(p4);
        assert(!r4.ok);
        assert(r4.message == "doc.0: has both .text and .path");
    }

    // ---- Empty manifest: a valid (here, empty) file loads an EMPTY store — not
    // a refusal. The window re-seeds scratch (tested in the shell suite). ----
    {
        const QString empty = dir.filePath(QStringLiteral("empty.toml"));
        writeFile(empty, "");
        const TextSessionLoad loaded = loadTextSessionFromPath(empty);
        assert(loaded.ok);
        assert(loaded.store.documents.empty());
        assert(!loaded.store.activeDocumentId.has_value());
    }

    return 0;
}
