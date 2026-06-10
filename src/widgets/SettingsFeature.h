#pragma once

#include <QObject>
#include <QString>
#include <QStringList>

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
    };

    explicit SettingsFeature(ShellHooks hooks, QObject *parent = nullptr);

    QWidget *buildPanel(edi::shell::ShellSlot slot);

private:
    QWidget *buildSettingsPage();
    // One row of the theme section: label + hex field + color-dialog swatch.
    // `member` selects which of the four inputs the row edits — the variation
    // point is data (a member pointer), not four near-identical functions.
    // Returns the field so the page can refresh it after a profile load.
    QLineEdit *addColorRow(QWidget *page, const QString &label, const QString &fieldName,
        QString edi::shell::ShellThemeInputs::*member);
    void pushInputs(const std::function<void(edi::shell::ShellThemeInputs &)> &edit);

    ShellHooks m_hooks;
};
