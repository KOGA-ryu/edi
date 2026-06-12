#pragma once

#include "text/TextEditorCommands.h"

#include <QPlainTextEdit>
#include <QWidget>

#include <functional>

class QLabel;
class QListWidget;

namespace edi::shell {
enum class ShellSlot;
struct FeatureContext;
}

// The text editor HOST (E1) — the shell's side of the user's text core.
// The core (src/text: TextDocument/Store/Commands/Selection) was built as
// a learning project and is hosted, never edited: every mutation the
// panel performs goes through applyTextEditorCommand, so the document
// model stays the single authority and the widget is a PROJECTION of it —
// the same relationship the drawing canvas has to the drafting document.
//
// Why the view is a read-only QPlainTextEdit: read-only KILLS every Qt
// built-in mutation path (typing, paste, drag-drop, the context menu's
// edit actions) in one switch, while keyboard/mouse-selectable
// interaction flags keep the caret and navigation alive. Our
// keyPressEvent then translates editing keys into the core's commands.
// The alternative — an editable widget mirrored back into the document —
// would mean two sources of truth and unaudited mutation paths; this way
// the compiler of edits is the user's own command code or nothing.
class TextEditorView final : public QPlainTextEdit {
    Q_OBJECT

public:
    explicit TextEditorView(QWidget *parent = nullptr);

    // The panel installs the translation: an editing key becomes a command
    // for the ACTIVE document at the caret offset. The view never touches
    // the store — it only describes what the keystroke meant.
    std::function<void(const edi::text::TextEditorCommand &)> applyCommand;
    // Host-side refusals (the ASCII input gate) — same status surface the
    // core's refusals use; a blocked keystroke is never silent.
    std::function<void(const QString &)> reportRefusal;

protected:
    void keyPressEvent(QKeyEvent *event) override;
};

// An injectable path chooser (E2) — the drawing Save As precedent, made
// feature-local: New/Open/Save call it at CLICK time, so offscreen tests drive
// the buttons with real temp paths and no QFileDialog ever opens. `forSave`
// picks the dialog; an empty return means the user cancelled.
using TextEditorPathProvider = std::function<QString(bool forSave)>;

// Builds the bottom-terminal panel: a small New/Open/Save toolbar, the document
// list (left), view + status (right). Free-function style on purpose — the
// feature owns no state; the store rides the FeatureContext bus (window-owned,
// so it survives workspace remounts that recreate feature instances).
QWidget *buildTextEditorPanel(edi::shell::FeatureContext &context,
                              TextEditorPathProvider pathProvider);
