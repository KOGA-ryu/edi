#include <QApplication>

#include "io/SettingsStore.h"
#include "io/ShellLayoutStore.h"
#include "widgets/EdiShellWindow.h"

int main(int argc, char **argv)
{
    QApplication app(argc, argv);

    EdiShellWindow window;
    window.loadSettings(edi::io::defaultSettingsPath());
    window.loadWorkspaceLayout(edi::io::defaultWorkspaceLayoutPath());
    window.show();

    return app.exec();
}
