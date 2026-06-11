#pragma once

#include <QObject>
#include <QPair>
#include <QString>
#include <QStringList>
#include <QVector>

#include <functional>

#include "widgets/ShellHost.h"
#include "widgets/ShellTheme.h"

class QLineEdit;
class QPushButton;
class QWidget;

// The settings workspace as a feature: a registry entry like drafting, so the
// activity rail switches to it through the exact same WorkspaceLayout path.
// It talks to the shell only through callables — read the live theme inputs,
// write them back — and owns no state of its own: the page is a *view* of the
// shell's theme, not a second copy of it.
class SettingsFeature final : public QObject {
    Q_OBJECT

public:
    struct ShellHooks {
        std::function<edi::shell::ShellThemeInputs()> themeInputs;
        std::function<void(const edi::shell::ShellThemeInputs &)> setThemeInputs;
        std::function<QStringList()> profiles;
        std::function<QString()> activeProfile;
        std::function<bool(const QString &)> loadProfile;
        std::function<bool(const QString &)> saveProfile;
        // F6, the belt page. The page is a checklist, nothing more: the
        // shell supplies the vocabulary (id + label), answers which ids are
        // on the belt today, and turns a new id list into an arrangement.
        // The page never sees BeltLayout — ownership stays where it was.
        std::function<QVector<QPair<QString, QString>>()> toolInventory;
        std::function<QStringList()> beltToolIds;
        std::function<void(const QStringList &)> setBeltToolIds;
        // Modular panels page: vocabulary (group id + label), current slot
        // per group, and the single-assignment setter. Same ownership story
        // as the belt page — the page never sees WorkspaceLayout.
        std::function<QVector<QPair<QString, QString>>()> panelGroupInventory;
        std::function<QString(const QString &)> panelSlotForGroup;
        std::function<void(const QString &, const QString &)> setPanelSlotForGroup;
    };

    explicit SettingsFeature(ShellHooks hooks, QObject *parent = nullptr);

    QWidget *buildPanel(edi::shell::ShellSlot slot);

private:
    // F6: the settings surface is a page strip + stack; which pages exist is
    // a table in buildPanel ({id, label, builder} rows), so a new page is an
    // appended row, never new switcher logic. Pages build once and switch by
    // stack index — the F2 lesson: hide, don't rebuild, so live controls
    // never lose state mid-edit.
    QWidget *buildSettingsPage();
    QWidget *buildBeltPage();
    QWidget *buildPanelsPage();
    // One row of the theme section: label + hex field + color-dialog swatch.
    // `member` selects which of the four inputs the row edits — the variation
    // point is data (a member pointer), not four near-identical functions.
    // Returns the field so the page can refresh it after a profile load.
    QLineEdit *addColorRow(QWidget *page, const QString &label, const QString &fieldName,
        QString edi::shell::ShellThemeInputs::*member);
    void pushInputs(const std::function<void(edi::shell::ShellThemeInputs &)> &edit);

    ShellHooks m_hooks;
};
