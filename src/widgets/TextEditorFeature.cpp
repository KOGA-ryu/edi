#include "widgets/TextEditorFeature.h"

#include "widgets/ShellHost.h"
#include "widgets/ShellWidgetHelpers.h"

#include <QFile>
#include <QFileInfo>
#include <QFrame>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
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
    // Translate EDITING intents into core commands; let navigation fall
    // through to the base class (arrows, Home/End, PageUp — all read-only
    // safe). The caret offset is the document offset: QPlainTextEdit
    // counts positions in UTF-16 code units, but E1's corpus is the
    // ASCII-leaning script/recipe text, where they coincide with the
    // core's byte offsets. (E3 owns real Unicode mapping — documented
    // here so the simplification is a decision, not an accident.)
    if (applyCommand) {
        const std::size_t offset = static_cast<std::size_t>(textCursor().position());
        const QString typed = event->text();

        if (event->key() == Qt::Key_Backspace) {
            // The offset > 0 guard is unsigned arithmetic, not UX policy:
            // offset - 1 would wrap size_t at the document's start. Its
            // mirror (Delete at the end) is NOT guarded — the range goes to
            // the core, which refuses invalid_range and the status shows it.
            // Asymmetric on purpose: one boundary the host can prove cheaply,
            // one the document model owns. (Tab, likewise, is swallowed by
            // the read-only base until E3 assigns it a meaning.)
            if (offset > 0) {
                applyCommand(DeleteTextRangeCommand{{}, {offset - 1, offset}});
            }
            event->accept();
            return;
        }
        if (event->key() == Qt::Key_Delete) {
            applyCommand(DeleteTextRangeCommand{{}, {offset, offset + 1}});
            event->accept();
            return;
        }
        if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
            applyCommand(InsertTextCommand{{}, offset, "\n"});
            event->accept();
            return;
        }
        // Printable ASCII only — and this is ENFORCED, not assumed: the
        // caret offset is UTF-16 units while the core's offsets are bytes,
        // and the two coincide only in ASCII. A non-ASCII keystroke (é via
        // Option+e, a layout dead key) would silently write garbage bytes
        // into the document while the status said ok — the review proved it
        // empirically. Until E3 builds the real offset mapping, non-ASCII
        // input is a REFUSAL the status line shows, never a corruption.
        if (!typed.isEmpty() && typed.at(0).isPrint()) {
            const bool ascii = std::all_of(typed.begin(), typed.end(),
                [](QChar c) { return c.unicode() >= 0x20 && c.unicode() < 0x7F; });
            if (!ascii) {
                if (reportRefusal) {
                    reportRefusal(QStringLiteral(
                        "non-ASCII input refused until E3's offset mapping"));
                }
                event->accept();
                return;
            }
            applyCommand(InsertTextCommand{{}, offset, typed.toStdString()});
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
        std::size_t caretAfter = 0;
        if (auto *insert = std::get_if<InsertTextCommand>(&stamped)) {
            insert->documentId = activeId;
            caretAfter = insert->offset + insert->text.size();
        } else if (auto *erase = std::get_if<DeleteTextRangeCommand>(&stamped)) {
            erase->documentId = activeId;
            caretAfter = erase->range.start;
        }
        const TextCommandResult result = applyTextEditorCommand(*store, stamped);
        if (!result.ok) {
            status->setText(QStringLiteral("%1: %2")
                                .arg(QString::fromLatin1(textResultCodeName(result.code)),
                                     QString::fromStdString(result.message)));
            return;
        }
        status->setText(QStringLiteral("ok"));
        refreshView(view, *store, caretAfter);
        refreshList(list, *store); // dirty markers move with the edit
    };

    // The view reports its own refusals (the ASCII gate) through the same
    // status line commands use — one surface for every "no".
    view->reportRefusal = [status](const QString &reason) { status->setText(reason); };

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
        if (!pathProvider) {
            return;
        }
        const QString path = pathProvider(true);
        if (path.isEmpty()) {
            return; // cancelled
        }
        TextDocument *active = findDocument(*store, *store->activeDocumentId);
        if (active == nullptr) {
            return;
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
        markClean(*active);
        status->setText(QStringLiteral("ok"));
        refreshList(list, *store); // the dirty dot clears
    });

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
