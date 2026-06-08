#include <QApplication>

#include "widgets/EdiShellWindow.h"

int main(int argc, char **argv)
{
    QApplication app(argc, argv);

    EdiShellWindow window;
    window.show();

    return app.exec();
}
