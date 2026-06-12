#include "widgets/TextEditorFeature.h"

#include "widgets/ShellHost.h"
#include "text/TextEncoding.h"
#include "text/TextSearch.h"
#include "widgets/ShellWidgetHelpers.h"

#include <QFile>
#include <QFileInfo>
#include <QFrame>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPointer>
#include <QPushButton>
#include <QSaveFile>
#include <QSplitter>
#include <QTextCursor>
#include <QVBoxLayout>

#include <algorithm>

#include <string>

using namespace edi::text;

TextEditorView::TextEditorView(QWidget *parent)
    : QPlainTextEdit(parent)
{
    setObjectName(QStringLiteral("textEditorView"));
    // Read-only is the mutation firewall (see header); the interaction
    // flags hand back the caret and keyboard/mouse navigation read-only
    // normally takes away.
    setReadOnly(true);
    setTextInteractionFlags(Qt::TextSelectableByKeyboard | Qt::TextSelectableByMouse);
}

void TextEditorView::keyPressEvent(QKeyEvent *event)
{
    // Translate EDITING intents into core commands; navigation falls through
    // to the read-only base. E3 retired E1's ASCII gate: offsets now cross
    // the Qt/core boundary through TextEncoding — the caret arrives in
    // UTF-16 code units, the commands leave in UTF-8 bytes, and deletions
    // respect CHARACTER boundaries, so half-deleting a multi-byte sequence
    // is impossible by construction (the corruption the gate guarded).
    if (applyCommand) {
        const std::string utf8 = toPlainText().toStdString();
        QTextCursor cursor = textCursor();
        const bool hasSelection = cursor.hasSelection();
        // Qt's selectionStart/End are UTF-16 positions; map both ends to
        // bytes ONCE and reuse — every command below speaks bytes.
        const std::size_t selStart = byteOffsetForUtf16(
            utf8, static_cast<std::size_t>(cursor.selectionStart()));
        const std::size_t selEnd = byteOffsetForUtf16(
            utf8, static_cast<std::size_t>(cursor.selectionEnd()));
        const std::size_t caret = byteOffsetForUtf16(
            utf8, static_cast<std::size_t>(cursor.position()));
        const QString typed = event->text();

        if (event->key() == Qt::Key_Backspace || event->key() == Qt::Key_Delete) {
            // With a selection both keys mean the same thing: delete it.
            if (hasSelection) {
                applyCommand(DeleteTextRangeCommand{{}, {selStart, selEnd}});
            } else if (event->key() == Qt::Key_Backspace && caret > 0) {
                // One CHARACTER back, however many bytes that is.
                applyCommand(DeleteTextRangeCommand{{}, {previousCharBoundary(utf8, caret), caret}});
            } else if (event->key() == Qt::Key_Delete && caret < utf8.size()) {
                applyCommand(DeleteTextRangeCommand{{}, {caret, nextCharBoundary(utf8, caret)}});
            }
            // At the edges both are quiet no-ops now — symmetric, like every
            // editor. (E1 let Delete-at-end reach the core for an
            // invalid_range refusal; with boundary math in hand the honest
            // translation is "nothing to delete", and the refusal SURFACE is
            // pinned directly by the tests instead.)
            event->accept();
            return;
        }
        if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
            if (hasSelection) {
                applyCommand(ReplaceTextRangeCommand{{}, {selStart, selEnd}, "\n"});
            } else {
                applyCommand(InsertTextCommand{{}, caret, "\n"});
            }
            event->accept();
            return;
        }
        if (!typed.isEmpty() && typed.at(0).isPrint()) {
            // toStdString IS the UTF-8 encoding — é arrives as its two bytes,
            // an emoji as four; the mapping above keeps the caret honest.
            // Typing over a selection REPLACES it: the user's
            // ReplaceTextRangeCommand existed since the core was built —
            // E3 just supplies the gesture that asks for it.
            if (hasSelection) {
                applyCommand(ReplaceTextRangeCommand{{}, {selStart, selEnd}, typed.toStdString()});
            } else {
                applyCommand(InsertTextCommand{{}, caret, typed.toStdString()});
            }
            event->accept();
            return;
        }
    }
    QPlainTextEdit::keyPressEvent(event);
}

namespace {

// One refresh path: project the active document into the widgets and put
// the caret where the edit left it. setPlainText resets the caret to 0,
// so the offset is restored arithmetically — delete this restore and
// typing "abc" produces "cba", which is exactly what the shell test pins.
void refreshView(TextEditorView *view, const TextDocumentStore &store, std::size_t caretOffset)
{
    const TextDocument *active = store.activeDocumentId.has_value()
        ? findDocument(store, *store.activeDocumentId)
        : nullptr;
    const QString text = active ? QString::fromStdString(active->text) : QString();
    if (view->toPlainText() != text) {
        view->setPlainText(text);
    }
    QTextCursor cursor = view->textCursor();
    const int position = static_cast<int>(
        std::min<std::size_t>(caretOffset, static_cast<std::size_t>(text.size())));
    cursor.setPosition(position);
    view->setTextCursor(cursor);
}

void refreshList(QListWidget *list, const TextDocumentStore &store)
{
    list->clear();
    for (const TextDocument &document : store.documents) {
        auto *item = new QListWidgetItem(
            QStringLiteral("%1  [%2]%3")
                .arg(QString::fromStdString(document.title),
                     QString::fromLatin1(textDocumentRoleName(document.role)),
                     document.dirty ? QStringLiteral(" •") : QString()));
        item->setData(Qt::UserRole, QString::fromStdString(document.id));
        list->addItem(item);
        if (store.activeDocumentId.has_value() && document.id == *store.activeDocumentId) {
            item->setSelected(true);
            list->setCurrentItem(item);
        }
    }
}

} // namespace

QWidget *buildTextEditorPanel(edi::shell::FeatureContext &context,
                              TextEditorPathProvider pathProvider)
{
    TextDocumentStore *store = context.textStore;
    if (store == nullptr) {
        return nullptr; // no bus document, no panel — the mount degrades
    }
    edi::shell::FeatureContext *ctx = &context;

    // A FRAME, not a bare QWidget: the bottom slot is an OVERLAY on the
    // canvas, and a plain QWidget ignores QSS backgrounds — the grid would
    // bleed through the margins (the exact trap `edi --probe` exists for).
    QFrame *panel = edi::shell::makeRegionFrame(QStringLiteral("textEditorPanel"));
    auto *layout = new QVBoxLayout(panel);
    layout->setContentsMargins(12, 8, 12, 8);
    layout->setSpacing(6);

    // The toolbar (E2): New makes a document; Open/Save move text to and from
    // FILES through the injectable provider (never a hard-coded QFileDialog the
    // offscreen suite would hang on).
    auto *toolbar = new QHBoxLayout;
    auto *newButton = new QPushButton(QStringLiteral("New"));
    newButton->setObjectName(QStringLiteral("textEditorNew"));
    auto *openButton = new QPushButton(QStringLiteral("Open…"));
    openButton->setObjectName(QStringLiteral("textEditorOpen"));
    auto *saveButton = new QPushButton(QStringLiteral("Save"));
    saveButton->setObjectName(QStringLiteral("textEditorSave"));
    toolbar->addWidget(newButton);
    toolbar->addWidget(openButton);
    toolbar->addWidget(saveButton);
    toolbar->addStretch(1);
    // Find (E3, v1: literal, case-sensitive, wraps): the SEARCH runs in the
    // core over bytes; only the resulting selection touches the widget —
    // and a selection is projection state, not a document mutation, so the
    // read-only firewall stays intact.
    auto *findInput = new QLineEdit;
    findInput->setObjectName(QStringLiteral("textEditorFindInput"));
    findInput->setPlaceholderText(QStringLiteral("Find"));
    findInput->setMaximumWidth(180);
    auto *findButton = new QPushButton(QStringLiteral("Find"));
    findButton->setObjectName(QStringLiteral("textEditorFindNext"));
    toolbar->addWidget(findInput);
    toolbar->addWidget(findButton);
    layout->addLayout(toolbar);

    auto *splitter = new QSplitter(Qt::Horizontal);
    auto *list = new QListWidget;
    list->setObjectName(QStringLiteral("textEditorDocumentList"));
    list->setMaximumWidth(220);
    auto *view = new TextEditorView;
    splitter->addWidget(list);
    splitter->addWidget(view);
    splitter->setStretchFactor(1, 1);

    auto *status = new QLabel(QStringLiteral("ok"));
    status->setObjectName(QStringLiteral("textEditorStatus"));

    layout->addWidget(splitter, 1);
    layout->addWidget(status);

    // The edit loop, one choke point: stamp the active id into the command,
    // apply through the USER'S core, surface the result (a refused command
    // is a status line, never a silently dead keystroke), then re-project.
    // The caret lands after what an insert added, or where a delete began —
    // plain arithmetic on the command itself, no widget state trusted.
    view->applyCommand = [store, view, list, status](const TextEditorCommand &command) {
        if (!store->activeDocumentId.has_value()) {
            status->setText(QStringLiteral("no active document"));
            return;
        }
        const std::string activeId = *store->activeDocumentId;
        TextEditorCommand stamped = command;
        // Caret-after in BYTES, from the command itself: an insert lands
        // after its text, a delete at its start, a replace after what
        // replaced. Converted to UTF-16 against the POST-EDIT document
        // below — the widget's cursor scale, computed exactly once, here.
        std::size_t caretAfterByte = 0;
        if (auto *insert = std::get_if<InsertTextCommand>(&stamped)) {
            insert->documentId = activeId;
            caretAfterByte = insert->offset + insert->text.size();
        } else if (auto *erase = std::get_if<DeleteTextRangeCommand>(&stamped)) {
            erase->documentId = activeId;
            caretAfterByte = erase->range.start;
        } else if (auto *replace = std::get_if<ReplaceTextRangeCommand>(&stamped)) {
            replace->documentId = activeId;
            caretAfterByte = replace->range.start + replace->text.size();
        }
        const TextCommandResult result = applyTextEditorCommand(*store, stamped);
        if (!result.ok) {
            status->setText(QStringLiteral("%1: %2")
                                .arg(QString::fromLatin1(textResultCodeName(result.code)),
                                     QString::fromStdString(result.message)));
            return;
        }
        status->setText(QStringLiteral("ok"));
        const TextDocument *edited = findDocument(*store, activeId);
        const std::size_t caretUtf16 =
            edited ? utf16OffsetForByte(edited->text, caretAfterByte) : 0;
        refreshView(view, *store, caretUtf16);
        refreshList(list, *store); // dirty markers move with the edit
    };

    // Selecting a document activates it in the STORE, the projection follows,
    // caret parked at the end like a fresh focus. E2 nit (decision 7): the
    // trigger is itemClicked (single click) — itemActivated was double-click
    // only, fine while scratch was the sole document; New makes a second. On a
    // refusal the list selection is RE-ASSERTED from the store, so the
    // highlight never drifts from the truth.
    QObject::connect(list, &QListWidget::itemClicked, view, [store, view, list, status](QListWidgetItem *item) {
        const std::string id = item->data(Qt::UserRole).toString().toStdString();
        const TextStoreResult result = setActiveDocument(*store, id);
        if (!result.ok) {
            status->setText(QString::fromLatin1(textResultCodeName(result.code)));
            refreshList(list, *store); // re-assert the selection from the store
            return;
        }
        const TextDocument *active = findDocument(*store, id);
        refreshView(view, *store, active ? active->text.size() : 0);
        refreshList(list, *store);
    });

    // New: a fresh Scratch document, activated. The serial id keeps addDocument
    // happy; the title is what the list shows.
    QObject::connect(newButton, &QPushButton::clicked, view, [store, view, list, status]() {
        std::size_t serial = store->documents.size() + 1;
        std::string id = "note_" + std::to_string(serial);
        while (containsDocument(*store, id)) {
            ++serial;
            id = "note_" + std::to_string(serial);
        }
        const TextStoreResult added =
            addDocument(*store, makeTextDocument(id, "Note " + std::to_string(serial)));
        if (!added.ok) {
            status->setText(QString::fromLatin1(textResultCodeName(added.code)));
            return;
        }
        setActiveDocument(*store, id);
        status->setText(QStringLiteral("ok"));
        refreshView(view, *store, 0);
        refreshList(list, *store);
    });

    // Open: read a FILE into a new document (content from disk; the id is the
    // file name, made unique).
    QObject::connect(openButton, &QPushButton::clicked, view, [store, view, list, status, pathProvider]() {
        if (!pathProvider) {
            return;
        }
        const QString path = pathProvider(false);
        if (path.isEmpty()) {
            return; // cancelled
        }
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly)) {
            status->setText(QStringLiteral("could not read %1").arg(path));
            return;
        }
        const std::string content = QString::fromUtf8(file.readAll()).toStdString();
        const std::string baseName = QFileInfo(path).fileName().toStdString();
        std::string id = baseName;
        std::size_t serial = 2;
        while (id.empty() || containsDocument(*store, id)) {
            id = baseName + "_" + std::to_string(serial++);
        }
        TextDocument document = makeTextDocument(id, baseName);
        document.text = content;
        // The path rides the core's metadata.fields — the extension map the
        // user designed in — so the session can re-read the FILE on restore
        // instead of snapshotting stale text (the E2 review's design note).
        document.metadata.fields["path"] = path.toStdString();
        const TextStoreResult added = addDocument(*store, std::move(document));
        if (!added.ok) {
            status->setText(QString::fromLatin1(textResultCodeName(added.code)));
            return;
        }
        setActiveDocument(*store, id);
        status->setText(QStringLiteral("ok"));
        const TextDocument *active = findDocument(*store, id);
        refreshView(view, *store, active ? active->text.size() : 0);
        refreshList(list, *store);
    });

    // Save: write the active document's text to a FILE and mark it clean — the
    // list's dirty dot clears. (FILE saving stays explicit; the SESSION saves
    // itself on close. E2 does no close-dirty prompt — that is the R7 obligation.)
    QObject::connect(saveButton, &QPushButton::clicked, view, [store, list, status, pathProvider]() {
        if (!store->activeDocumentId.has_value()) {
            status->setText(QStringLiteral("no active document"));
            return;
        }
        TextDocument *active = findDocument(*store, *store->activeDocumentId);
        if (active == nullptr) {
            return;
        }
        // A file-backed document saves to ITS path, no dialog; a scratch
        // document save-as's through the provider and BECOMES file-backed.
        const auto pathField = active->metadata.fields.find("path");
        QString path = (pathField != active->metadata.fields.end())
            ? QString::fromStdString(pathField->second)
            : QString();
        if (path.isEmpty()) {
            if (!pathProvider) {
                return;
            }
            path = pathProvider(true);
            if (path.isEmpty()) {
                return; // cancelled
            }
        }
        QSaveFile file(path);
        bool ok = file.open(QIODevice::WriteOnly | QIODevice::Truncate);
        if (ok) {
            file.write(active->text.data(), static_cast<qint64>(active->text.size()));
            ok = file.commit();
        }
        if (!ok) {
            status->setText(QStringLiteral("could not write %1").arg(path));
            return;
        }
        active->metadata.fields["path"] = path.toStdString(); // save-as adopts the file
        markClean(*active);
        status->setText(QStringLiteral("ok"));
        refreshList(list, *store); // the dirty dot clears
    });

    // Find-next from the caret (or the current match's end, so repeated
    // Find walks every occurrence), wrapping once. A miss names the needle
    // on the status line — the find bar never goes silently dead.
    const auto findNextFromCaret = [store, view, status, findInput]() {
        if (!store->activeDocumentId.has_value()) {
            return;
        }
        const TextDocument *active = findDocument(*store, *store->activeDocumentId);
        if (active == nullptr) {
            return;
        }
        const std::string needle = findInput->text().toStdString();
        const std::size_t fromByte = byteOffsetForUtf16(
            active->text, static_cast<std::size_t>(view->textCursor().selectionEnd()));
        const auto match = edi::text::findNext(active->text, needle, fromByte);
        if (!match.has_value()) {
            status->setText(QStringLiteral("no match for '%1'").arg(findInput->text()));
            return;
        }
        QTextCursor cursor = view->textCursor();
        cursor.setPosition(static_cast<int>(utf16OffsetForByte(active->text, match->start)));
        cursor.setPosition(static_cast<int>(utf16OffsetForByte(active->text, match->end)),
                           QTextCursor::KeepAnchor);
        view->setTextCursor(cursor);
        status->setText(QStringLiteral("ok"));
    };
    QObject::connect(findButton, &QPushButton::clicked, view, findNextFromCaret);
    QObject::connect(findInput, &QLineEdit::returnPressed, view, findNextFromCaret);

    // The session refresh hook (E2): loadTextSession swaps the store, then calls
    // this to re-project the panel and surface any degrade note. QPointers guard
    // the (rare) case where the panel was torn down before the call.
    {
        const QPointer<QListWidget> listPtr = list;
        const QPointer<TextEditorView> viewPtr = view;
        const QPointer<QLabel> statusPtr = status;
        ctx->refreshTextPanel = [store, listPtr, viewPtr, statusPtr, ctx]() {
            if (!listPtr || !viewPtr || !statusPtr) {
                return;
            }
            refreshList(listPtr, *store);
            const TextDocument *active = store->activeDocumentId.has_value()
                ? findDocument(*store, *store->activeDocumentId)
                : nullptr;
            refreshView(viewPtr, *store, active ? active->text.size() : 0);
            statusPtr->setText(ctx->textSessionNote.isEmpty() ? QStringLiteral("ok")
                                                              : ctx->textSessionNote);
        };
    }

    refreshList(list, *store);
    const TextDocument *active = store->activeDocumentId.has_value()
        ? findDocument(*store, *store->activeDocumentId)
        : nullptr;
    refreshView(view, *store, active ? active->text.size() : 0);
    if (!ctx->textSessionNote.isEmpty()) {
        status->setText(ctx->textSessionNote); // a note set before this build survives
    }
    return panel;
}
