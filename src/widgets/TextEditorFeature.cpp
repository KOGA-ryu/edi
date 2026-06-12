#include "widgets/TextEditorFeature.h"

#include "widgets/ShellHost.h"
#include "widgets/ShellWidgetHelpers.h"

#include <QFrame>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QListWidget>
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

QWidget *buildTextEditorPanel(edi::shell::FeatureContext &context)
{
    TextDocumentStore *store = context.textStore;
    if (store == nullptr) {
        return nullptr; // no bus document, no panel — the mount degrades
    }

    // A FRAME, not a bare QWidget: the bottom slot is an OVERLAY on the
    // canvas, and a plain QWidget ignores QSS backgrounds — the grid would
    // bleed through the margins (the exact trap `edi --probe` exists for).
    QFrame *panel = edi::shell::makeRegionFrame(QStringLiteral("textEditorPanel"));
    auto *layout = new QVBoxLayout(panel);
    layout->setContentsMargins(12, 8, 12, 8);
    layout->setSpacing(6);

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

    // Selecting a document activates it in the STORE (the projection
    // follows), caret parked at the end like a fresh focus. (itemActivated
    // is double-click/Enter; fine while the seeded scratch is the only
    // document — E2's creation UI must switch to itemClicked and re-assert
    // selection from the store, noted in its bucket.)
    QObject::connect(list, &QListWidget::itemActivated, view, [store, view, list, status](QListWidgetItem *item) {
        const std::string id = item->data(Qt::UserRole).toString().toStdString();
        const TextStoreResult result = setActiveDocument(*store, id);
        if (!result.ok) {
            status->setText(QString::fromLatin1(textResultCodeName(result.code)));
            return;
        }
        const TextDocument *active = findDocument(*store, id);
        refreshView(view, *store, active ? active->text.size() : 0);
        refreshList(list, *store);
    });

    refreshList(list, *store);
    const TextDocument *active = store->activeDocumentId.has_value()
        ? findDocument(*store, *store->activeDocumentId)
        : nullptr;
    refreshView(view, *store, active ? active->text.size() : 0);
    return panel;
}
