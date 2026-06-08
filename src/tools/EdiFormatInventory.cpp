#include "formats/FormatInventory.h"

#include <QCoreApplication>
#include <QTextStream>

namespace {

QTextStream qout(stdout);
QTextStream qerr(stderr);

struct Args {
    bool help = false;
    bool summary = false;
    QString repo = QStringLiteral(".");
};

QString usage()
{
    return QStringLiteral(
        "Usage:\n"
        "  build/edi_format_inventory --repo .\n"
        "  build/edi_format_inventory --repo . --summary\n");
}

bool parseArgs(const QStringList &tokens, Args *args, QString *error)
{
    for (int i = 0; i < tokens.size(); ++i) {
        const QString token = tokens[i];
        if (token == QStringLiteral("--help") || token == QStringLiteral("-h")) {
            args->help = true;
        } else if (token == QStringLiteral("--summary")) {
            args->summary = true;
        } else if (token == QStringLiteral("--repo")) {
            if (i + 1 >= tokens.size()) {
                *error = QStringLiteral("--repo requires value");
                return false;
            }
            args->repo = tokens[++i];
        } else {
            *error = QStringLiteral("unknown argument: ") + token;
            return false;
        }
    }
    return true;
}

} // namespace

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    Args args;
    QString error;
    if (!parseArgs(app.arguments().mid(1), &args, &error)) {
        qerr << error << '\n' << usage();
        return 1;
    }
    if (args.help) {
        qout << usage();
        return 0;
    }

    const QVector<edi::formats::InventoryRow> rows = edi::formats::inventoryRepoJsonFiles(args.repo);
    if (args.summary) {
        qout << edi::formats::inventorySummary(rows) << '\n';
        return 0;
    }

    qout << edi::formats::inventoryRowHeader() << '\n';
    for (const edi::formats::InventoryRow &row : rows) {
        qout << edi::formats::inventoryRowLine(row) << '\n';
    }
    qout << '\n' << edi::formats::inventorySummary(rows) << '\n';
    return 0;
}
