#include <QApplication>
#include <QCommandLineParser>
#include <QElapsedTimer>
#include <QImage>
#include <QTextStream>
#include <QTimer>

#include <algorithm>

#include "core/DrawingCore.h"
#include "io/SettingsStore.h"
#include "io/ShellLayoutStore.h"
#include "widgets/EdiShellWindow.h"

namespace {

// Render-proof mode: grab the settled window, optionally save it, optionally
// report what color actually reached given pixels. Exists so "the theme says
// surface is X" can be checked against what was really painted — stylesheet
// rules can be silently ignored (plain QWidgets without WA_StyledBackground,
// palette fills under transparent rules), and only the rendered image tells
// the truth. Run under QT_QPA_PLATFORM=offscreen for deterministic,
// permission-free captures (device pixel ratio 1, so probe coordinates equal
// image pixels).
int runRenderProof(QApplication &app, EdiShellWindow &window,
                   const QString &snapshotPath, const QStringList &probes)
{
    int exitCode = 0;
    const QImage image = window.grab().toImage();
    QTextStream out(stdout);
    QTextStream err(stderr);

    if (!snapshotPath.isEmpty() && !image.save(snapshotPath)) {
        err << "snapshot: could not write " << snapshotPath << '\n';
        exitCode = 1;
    }

    for (const QString &probe : probes) {
        const QStringList parts = probe.split(QLatin1Char(','));
        bool xOk = false;
        bool yOk = false;
        const int x = parts.value(0).toInt(&xOk);
        const int y = parts.value(1).toInt(&yOk);
        if (!xOk || !yOk || parts.size() != 2 || !image.rect().contains(x, y)) {
            err << "probe: bad or out-of-bounds point '" << probe << "' (image "
                << image.width() << 'x' << image.height() << ")\n";
            exitCode = 1;
            continue;
        }
        // #rrggbb, lowercase — the same shape ShellTheme tokens use, so output
        // compares against derived tokens with plain string equality.
        out << probe << ' ' << QColor(image.pixel(x, y)).name();

        // Name the widget that owns the pixel (deepest child first), so a
        // wrong color is immediately attributable to a widget, not hunted
        // through the tree by hand.
        QWidget *hit = &window;
        while (QWidget *child = hit->childAt(hit->mapFrom(&window, QPoint(x, y)))) {
            hit = child;
        }
        for (QWidget *w = hit; w != nullptr && w != &window; w = w->parentWidget()) {
            out << ' ' << (w->objectName().isEmpty()
                               ? QString::fromLatin1(w->metaObject()->className())
                               : w->objectName());
        }
        out << '\n';
    }

    app.exit(exitCode);
    return exitCode;
}

} // namespace

int main(int argc, char **argv)
{
    QApplication app(argc, argv);

    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("edi shell"));
    parser.addHelpOption();
    const QCommandLineOption snapshotOption(
        QStringLiteral("snapshot"),
        QStringLiteral("Save a PNG of the settled shell window, then exit."),
        QStringLiteral("png-path"));
    const QCommandLineOption probeOption(
        QStringLiteral("probe"),
        QStringLiteral("Print the rendered color at x,y (repeatable), then exit."),
        QStringLiteral("x,y"));
    const QCommandLineOption paintBenchOption(
        QStringLiteral("paint-bench"),
        QStringLiteral("Render the settled window N times, print ms/frame, then exit."),
        QStringLiteral("N"));
    const QCommandLineOption benchObjectsOption(
        QStringLiteral("bench-objects"),
        QStringLiteral("Create N synthetic lines before benching (deterministic fan)."),
        QStringLiteral("N"));
    parser.addOption(snapshotOption);
    parser.addOption(probeOption);
    parser.addOption(paintBenchOption);
    parser.addOption(benchObjectsOption);
    parser.process(app);

    EdiShellWindow window;
    window.loadSettings(edi::io::defaultSettingsPath());
    window.loadWorkspaceLayout(edi::io::defaultWorkspaceLayoutPath());
    window.show();

    // Synthetic load for the bench: a deterministic fan of lines through the
    // real creation path (clicks -> commands -> undo bracket), so the bench
    // measures the document the user would actually have. Also times the
    // creation itself — bulk tools will care.
    if (parser.isSet(benchObjectsOption)) {
        auto *controller = window.findChild<DrawingDocumentController *>();
        if (controller != nullptr) {
            const int count = std::max(1, parser.value(benchObjectsOption).toInt());
            QElapsedTimer creation;
            creation.start();
            controller->setSelectedToolId(QStringLiteral("line_tool"));
            for (int i = 0; i < count; ++i) {
                const double t = static_cast<double>(i) / count;
                controller->clickCanvasNormalized(0.05 + 0.9 * t, 0.1);
                controller->clickCanvasNormalized(0.95 - 0.9 * t, 0.9);
            }
            QTextStream(stdout) << "bench-objects: " << count << " lines created in "
                                << creation.elapsed() << " ms\n";
        }
    }

    if (parser.isSet(snapshotOption) || parser.isSet(probeOption) || parser.isSet(paintBenchOption)) {
        // One settle delay instead of "grab immediately": palette placement,
        // deferred deletes and splitter sizing all finish within the first
        // event-loop turns; a fixed delay keeps the capture path dumb and the
        // image deterministic without wiring a "layout done" signal.
        QTimer::singleShot(800, &window, [&app, &window, &parser, snapshotOption, probeOption, paintBenchOption] {
            // The paint bench: render the whole window N times and report the
            // average. grab() runs the real paintEvent pipeline, so this is
            // the honest cost of one frame — the instrument for every canvas
            // performance claim (numbers, not adjectives).
            if (parser.isSet(paintBenchOption)) {
                const int frames = std::max(1, parser.value(paintBenchOption).toInt());
                QElapsedTimer timer;
                timer.start();
                for (int i = 0; i < frames; ++i) {
                    window.grab();
                }
                const qint64 elapsed = timer.elapsed();
                QTextStream(stdout) << "paint-bench: " << frames << " frames, "
                                    << elapsed << " ms total, "
                                    << (static_cast<double>(elapsed) / frames) << " ms/frame\n";
            }
            runRenderProof(app, window,
                           parser.value(snapshotOption), parser.values(probeOption));
        });
    }

    return app.exec();
}
