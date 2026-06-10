#include <QApplication>

#include "io/SettingsStore.h"
#include "widgets/EdiShellWindow.h"

int main(int argc, char **argv)
{
    QApplication app(argc, argv);

    EdiShellWindow window;
    window.loadSettings(edi::io::defaultSettingsPath());
    window.show();

    return app.exec();
}
