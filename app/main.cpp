#include <QApplication>
#include <QLabel>
#include <QMainWindow>
#include <QStatusBar>
#include <QVBoxLayout>

int main(int argc, char **argv)
{
    QApplication app(argc, argv);

    QMainWindow window;
    window.setWindowTitle(QStringLiteral("EDI"));

    auto *central = new QWidget;
    auto *layout = new QVBoxLayout(central);

    auto *title = new QLabel(QStringLiteral("EDI"));
    QFont titleFont = title->font();
    titleFont.setPointSize(titleFont.pointSize() + 8);
    titleFont.setBold(true);
    title->setFont(titleFont);

    auto *body = new QLabel(QStringLiteral("C++ Qt Widgets runtime. Ready for the next usable subsystem."));
    body->setWordWrap(true);
    body->setTextInteractionFlags(Qt::TextSelectableByMouse);

    layout->addWidget(title);
    layout->addWidget(body);
    layout->addStretch(1);

    window.setCentralWidget(central);
    window.statusBar()->showMessage(QStringLiteral("ready"));
    window.resize(900, 640);
    window.show();

    return app.exec();
}
