#include "formats/FormatInventory.h"

#include <QCoreApplication>
#include <QTextStream>

namespace {

QTextStream qout(stdout);
QTextStream qerr(stderr);

struct Args {
    bool help = false;
    bool summary = false;
    bool failUnknown = false;
    bool failBlocked = false;
    QString repo = QStringLiteral(".");
    edi::formats::InventoryFilter filter;
};

QString usage()
{
    return QStringLiteral(
        "Usage:\n"
        "  build/edi_format_inventory --repo .\n"
        "  build/edi_format_inventory --repo . --summary\n"
        "  build/edi_format_inventory --repo . --target MessagePack\n"
        "  build/edi_format_inventory --repo . --category internal_authored_json --summary\n"
        "  build/edi_format_inventory --repo . --fail-unknown\n");
}

void pushValues(QStringList *target, const QString &value)
{
    for (const QString &part : value.split(QStringLiteral(","))) {
        const QString trimmed = part.trimmed();
        if (!trimmed.isEmpty()) {
            target->push_back(trimmed);
        }
    }
}

bool parseArgs(const QStringList &tokens, Args *args, QString *error)
{
    for (int i = 0; i < tokens.size(); ++i) {
        const QString token = tokens[i];
        if (token == QStringLiteral("--help") || token == QStringLiteral("-h")) {
            args->help = true;
        } else if (token == QStringLiteral("--summary")) {
            args->summary = true;
        } else if (token == QStringLiteral("--fail-unknown")) {
            args->failUnknown = true;
        } else if (token == QStringLiteral("--fail-blocked")) {
            args->failBlocked = true;
        } else if (token == QStringLiteral("--repo")) {
            if (i + 1 >= tokens.size()) {
                *error = QStringLiteral("--repo requires value");
                return false;
            }
            args->repo = tokens[++i];
        } else if (token == QStringLiteral("--category")) {
            if (i + 1 >= tokens.size()) {
                *error = QStringLiteral("--category requires value");
                return false;
            }
            pushValues(&args->filter.categories, tokens[++i]);
        } else if (token == QStringLiteral("--family")) {
            if (i + 1 >= tokens.size()) {
                *error = QStringLiteral("--family requires value");
                return false;
            }
            pushValues(&args->filter.dataFamilies, tokens[++i]);
        } else if (token == QStringLiteral("--target")) {
            if (i + 1 >= tokens.size()) {
                *error = QStringLiteral("--target requires value");
                return false;
            }
            pushValues(&args->filter.targetFormats, tokens[++i]);
        } else if (token == QStringLiteral("--priority")) {
            if (i + 1 >= tokens.size()) {
                *error = QStringLiteral("--priority requires value");
                return false;
            }
            pushValues(&args->filter.priorities, tokens[++i]);
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

    const QVector<edi::formats::InventoryRow> allRows = edi::formats::inventoryRepoJsonFiles(args.repo);
    const QVector<edi::formats::InventoryRow> rows = edi::formats::filterInventoryRows(allRows, args.filter);
    if (args.summary) {
        qout << edi::formats::inventorySummary(rows) << '\n';
    } else {
        qout << edi::formats::inventoryRowHeader() << '\n';
        for (const edi::formats::InventoryRow &row : rows) {
            qout << edi::formats::inventoryRowLine(row) << '\n';
        }
        qout << '\n' << edi::formats::inventorySummary(rows) << '\n';
    }

    if (args.failUnknown && edi::formats::inventoryUnknownCount(rows) > 0) {
        qerr << "inventory contains unknown JSON rows\n";
        return 2;
    }
    if (args.failBlocked && edi::formats::inventoryBlockedCount(rows) > 0) {
        qerr << "inventory contains blocked migration rows\n";
        return 3;
    }
    return 0;
}
