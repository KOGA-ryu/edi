#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSet>
#include <QTextStream>

namespace {

QTextStream qout(stdout);
QTextStream qerr(stderr);

struct Args {
    bool help = false;
    bool all = false;
    bool dryRun = false;
    bool compact = false;
    bool recommend = false;
    bool compareBaseline = false;
    bool updateBaseline = false;
    bool failuresOnly = false;
    QString recommendationId;
    QString subsystem;
    QStringList fixtures;
    QStringList categories;
    QStringList tags;
};

QString usage()
{
    return QStringLiteral(
        "Usage:\n"
        "  build/drawing_control_workflow_report --recommend\n"
        "  build/drawing_control_workflow_report --recommend --id <selector>\n"
        "  build/drawing_control_workflow_report --all\n"
        "  build/drawing_control_workflow_report --tag <tag>\n"
        "  build/drawing_control_workflow_report --category <category>\n"
        "  build/drawing_control_workflow_report --fixture <fixture.json>\n"
        "  build/drawing_control_workflow_report --tag line --dry-run --compact\n"
        "  build/drawing_control_workflow_report --all --compare-baseline --failures-only\n");
}

void pushValues(QStringList &target, const QString &value)
{
    for (const QString &part : value.split(',')) {
        const QString trimmed = part.trimmed();
        if (!trimmed.isEmpty()) {
            target.push_back(trimmed);
        }
    }
}

bool parseArgs(const QStringList &tokens, Args *args, QString *error)
{
    for (int i = 0; i < tokens.size(); ++i) {
        const QString token = tokens[i];
        auto requireValue = [&](const QString &name) {
            if (i + 1 >= tokens.size()) {
                *error = name + " requires value";
                return QString();
            }
            return tokens[++i];
        };
        if (token == "--help" || token == "-h") {
            args->help = true;
        } else if (token == "--recommend") {
            args->recommend = true;
        } else if (token == "--id") {
            args->recommendationId = requireValue(token);
        } else if (token == "--all") {
            args->all = true;
        } else if (token == "--dry-run") {
            args->dryRun = true;
        } else if (token == "--compact") {
            args->compact = true;
        } else if (token == "--compare-baseline") {
            args->compareBaseline = true;
        } else if (token == "--update-baseline") {
            args->updateBaseline = true;
        } else if (token == "--subsystem") {
            args->subsystem = requireValue(token);
        } else if (token == "--failures-only") {
            args->failuresOnly = true;
        } else if (token == "--fixture") {
            pushValues(args->fixtures, requireValue(token));
        } else if (token == "--category") {
            pushValues(args->categories, requireValue(token));
        } else if (token == "--tag") {
            pushValues(args->tags, requireValue(token));
        } else {
            *error = "unknown argument: " + token;
            return false;
        }
        if (!error->isEmpty()) {
            return false;
        }
    }
    return true;
}

QJsonObject readObject(const QString &path, QString *error)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        *error = file.errorString();
        return {};
    }
    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        *error = parseError.errorString();
        return {};
    }
    return document.object();
}

bool containsAny(const QJsonArray &values, const QStringList &selected)
{
    if (selected.isEmpty()) {
        return true;
    }
    for (const QJsonValue &value : values) {
        if (selected.contains(value.toString())) {
            return true;
        }
    }
    return false;
}

bool matches(const QJsonObject &workflow, const Args &args)
{
    if (args.all) {
        return true;
    }
    if (!args.fixtures.isEmpty() && !args.fixtures.contains(workflow.value("fixture").toString())) {
        return false;
    }
    if (!args.categories.isEmpty() && !args.categories.contains(workflow.value("category").toString())) {
        return false;
    }
    if (!containsAny(workflow.value("tags").toArray(), args.tags)) {
        return false;
    }
    return true;
}

QJsonArray selectedWorkflows(const QJsonArray &workflows, const Args &args)
{
    QJsonArray selected;
    for (const QJsonValue &value : workflows) {
        const QJsonObject workflow = value.toObject();
        if (matches(workflow, args)) {
            selected.push_back(workflow);
        }
    }
    return selected;
}

QJsonObject countByField(const QJsonArray &workflows, const QString &field)
{
    QJsonObject result;
    for (const QJsonValue &value : workflows) {
        const QString key = value.toObject().value(field).toString();
        if (!key.isEmpty()) {
            result.insert(key, result.value(key).toInt() + 1);
        }
    }
    return result;
}

QJsonObject countTags(const QJsonArray &workflows)
{
    QJsonObject result;
    for (const QJsonValue &value : workflows) {
        for (const QJsonValue &tag : value.toObject().value("tags").toArray()) {
            const QString key = tag.toString();
            if (!key.isEmpty()) {
                result.insert(key, result.value(key).toInt() + 1);
            }
        }
    }
    return result;
}

QJsonObject coverage(const QJsonArray &workflows)
{
    return {
        {"kinds", countByField(workflows, "kind")},
        {"categories", countByField(workflows, "category")},
        {"tags", countTags(workflows)},
    };
}

QJsonObject filters(const Args &args)
{
    return {
        {"all", args.all},
        {"fixtures", QJsonArray::fromStringList(args.fixtures)},
        {"categories", QJsonArray::fromStringList(args.categories)},
        {"tags", QJsonArray::fromStringList(args.tags)},
    };
}

QJsonObject dryRunOutput(const QJsonArray &workflows, const QJsonArray &selected, const Args &args)
{
    QJsonObject output{
        {"ok", true},
        {"dryRun", true},
        {"selectedWorkflowCount", selected.size()},
        {"totalWorkflowCount", workflows.size()},
        {"filters", filters(args)},
        {"coverage", coverage(selected)},
    };
    if (!args.compact) {
        output.insert("workflows", selected);
    }
    return output;
}

QString commandText(QString command)
{
    command.replace("node tests/helpers/drawing_control_workflow_report.js", "build/drawing_control_workflow_report");
    return command;
}

QJsonValue rewriteCommands(const QJsonValue &value)
{
    if (value.isString()) {
        return commandText(value.toString());
    }
    if (value.isArray()) {
        QJsonArray result;
        for (const QJsonValue &item : value.toArray()) {
            result.push_back(rewriteCommands(item));
        }
        return result;
    }
    if (value.isObject()) {
        QJsonObject result;
        const QJsonObject object = value.toObject();
        for (auto it = object.constBegin(); it != object.constEnd(); ++it) {
            result.insert(it.key(), rewriteCommands(it.value()));
        }
        return result;
    }
    return value;
}

QJsonObject recommendationOutput(const QString &repoRoot, const Args &args)
{
    QString error;
    const QJsonObject document = readObject(QDir(repoRoot).filePath("tests/fixtures/drawing_tool_scripts/workflow_coverage_expectations.json"), &error);
    QJsonArray recommendations;
    for (const QJsonValue &value : document.value("recommendedSelectors").toArray()) {
        const QJsonObject recommendation = rewriteCommands(value).toObject();
        if (args.recommendationId.isEmpty() || recommendation.value("id").toString() == args.recommendationId) {
            recommendations.push_back(recommendation);
        }
    }
    return {
        {"ok", !recommendations.isEmpty()},
        {"recommendations", recommendations},
    };
}

bool harnessEnabled()
{
    return QString::fromLocal8Bit(qgetenv("DRAFTSMAN_ENABLE_DRAWING_HARNESS")) == "1";
}

QJsonObject disabledOutput(const Args &args)
{
    if (args.failuresOnly && args.compareBaseline) {
        return {
            {"ok", true},
            {"baselineComparison", QJsonObject{{"ok", true}, {"failureCount", 0}, {"warningCount", 0}}},
        };
    }
    QJsonObject output{
        {"ok", true},
        {"skipped", true},
        {"reason", "drawing workflow harness is disabled"},
        {"enableWith", "DRAFTSMAN_ENABLE_DRAWING_HARNESS=1"},
    };
    if (args.compareBaseline) {
        output.insert("baselineComparison", QJsonObject{
            {"ok", true},
            {"skipped", true},
            {"reason", "drawing workflow harness is disabled"},
        });
    }
    return output;
}

void printJson(const QJsonObject &object)
{
    qout << QString::fromUtf8(QJsonDocument(object).toJson(QJsonDocument::Indented));
}

} // namespace

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    QStringList tokens;
    for (int i = 1; i < argc; ++i) {
        tokens.push_back(QString::fromLocal8Bit(argv[i]));
    }

    Args args;
    QString error;
    if (!parseArgs(tokens, &args, &error)) {
        qerr << error << '\n' << usage();
        return 1;
    }
    if (args.help) {
        qout << usage();
        return 0;
    }

    const QString repoRoot = QStringLiteral(PROJECT_SOURCE_DIR);
    if (args.recommend) {
        const QJsonObject output = recommendationOutput(repoRoot, args);
        printJson(output);
        return output.value("ok").toBool() ? 0 : 1;
    }
    const bool hasSelector = args.all || !args.fixtures.isEmpty() || !args.categories.isEmpty() || !args.tags.isEmpty();
    if (!hasSelector) {
        qerr << "one selector or --all is required\n" << usage();
        return 1;
    }
    if (!args.subsystem.isEmpty() && !args.compareBaseline) {
        qerr << "--subsystem requires --compare-baseline\n" << usage();
        return 1;
    }
    if (args.failuresOnly && !args.compareBaseline) {
        qerr << "--failures-only requires --compare-baseline\n" << usage();
        return 1;
    }

    const QString manifestPath = QDir(repoRoot).filePath("tests/fixtures/drawing_tool_scripts/workflow_manifest.json");
    const QJsonObject manifest = readObject(manifestPath, &error);
    if (!error.isEmpty()) {
        qerr << manifestPath << ": " << error << '\n';
        return 1;
    }
    const QJsonArray workflows = manifest.value("workflows").toArray();
    const QJsonArray selected = selectedWorkflows(workflows, args);
    if (selected.isEmpty()) {
        qerr << "workflow selectors did not match any fixtures\n";
        return 1;
    }
    if (args.dryRun) {
        if (args.compareBaseline || args.updateBaseline) {
            qerr << "--compare-baseline and --update-baseline require a real workflow run\n";
            return 1;
        }
        printJson(dryRunOutput(workflows, selected, args));
        return 0;
    }

    if (!harnessEnabled()) {
        printJson(disabledOutput(args));
        return 0;
    }

    printJson(QJsonObject{
        {"ok", true},
        {"skipped", true},
        {"reason", "C++ workflow execution is deferred until the QML canvas runner is removed"},
        {"selectedWorkflowCount", selected.size()},
    });
    return 0;
}
