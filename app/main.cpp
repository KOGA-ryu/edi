#include <QApplication>
#include <QCommandLineParser>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QFileInfo>
#include <QImage>
#include <QProcess>
#include <QSaveFile>
#include <QStandardPaths>
#include <QTextStream>
#include <QTimer>

#include <algorithm>

#include "app/AppState.h"
#include "core/DrawingCore.h"
#include "drafting/DraftingRoom.h"
#include "drafting/DraftingSerialize.h"
#include "io/AssetZooStore.h"
#include "io/CryptGenerator.h"
#include "io/MapToonExport.h"
#include "io/RoomSpecStore.h"
#include "io/SettingsStore.h"
#include "io/ShellLayoutStore.h"
#include "io/TextSessionStore.h"
#include "widgets/DrawingCanvasWidget.h"
#include "widgets/EdiShellWindow.h"
#include "zoo/AssetZoo.h"
#include "zoo/ZooTesterCatalog.h"
#include "zoo/ZooToonExport.h"

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
    // Headless safety (D04): a QApplication aborts at construction if it cannot
    // reach a display platform. But several modes here need no real display —
    // the CLI export termini (--generate-crypt / --export-map / --mint-tester-zoo)
    // write files and exit, and the render-proof (--snapshot / --probe) is
    // explicitly designed for the offscreen plugin. So on a box with no display
    // server — and no platform already chosen — fall back to "offscreen" instead
    // of dying. We only do this where the absence of a display env var actually
    // MEANS headless: X11/Wayland-style Unix. macOS and Windows reach their
    // window server without DISPLAY/WAYLAND_DISPLAY, so forcing offscreen there
    // would wrongly blind a real desktop session. An explicit QT_QPA_PLATFORM
    // always wins (this is how the tests select offscreen), so this only fires
    // when nothing was chosen and nothing is reachable.
#if defined(Q_OS_UNIX) && !defined(Q_OS_MACOS)
    if (qEnvironmentVariableIsEmpty("QT_QPA_PLATFORM")
        && qEnvironmentVariableIsEmpty("DISPLAY")
        && qEnvironmentVariableIsEmpty("WAYLAND_DISPLAY")) {
        qputenv("QT_QPA_PLATFORM", "offscreen");
    }
#endif

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
    const QCommandLineOption demoRoomOption(
        QStringLiteral("demo-room"),
        QStringLiteral("Generate the guard-antechamber room from a spec (Seam-A authoring demo)."));
    const QCommandLineOption roomFileOption(
        QStringLiteral("room-file"),
        QStringLiteral("Generate a room from a .room.toml authoring file (Seam-A: file -> map)."),
        QStringLiteral("path"));
    const QCommandLineOption mapFileOption(
        QStringLiteral("map-file"),
        QStringLiteral("Generate a multi-room map from a .map.toml authoring file (Seam-A: file -> map)."),
        QStringLiteral("path"));
    const QCommandLineOption asciiMapOption(
        QStringLiteral("ascii-map"),
        QStringLiteral("Generate a map from an ASCII glyph grid file (the control-gate path)."),
        QStringLiteral("path"));
    const QCommandLineOption opsFileOption(
        QStringLiteral("ops-file"),
        QStringLiteral("Load a recipe ops TOML into the op stream (Seam A) — pair with "
                       "--workspace blender to see its ASCII proof."),
        QStringLiteral("path"));
    const QCommandLineOption exportMapOption(
        QStringLiteral("export-map"),
        QStringLiteral("Project the --map-file to a TOON map document for the game engine (Seam B), then exit."),
        QStringLiteral("toon-path"));
    const QCommandLineOption workspaceOption(
        QStringLiteral("workspace"),
        QStringLiteral("Switch to a workspace (drafting, blender, map; text/project/planning "
                       "resolve to drafting) before --snapshot/--probe. Ignored by --export-map "
                       "(headless); 'settings' is UI-only and refused here."),
        QStringLiteral("mode"));
    // The single user-facing SCALE knob: generate the hardcoded crypt straight to a
    // Seam-B TOON, dialled by --scale (default 1). Headless terminus of the M0 chain
    // (generator -> TOON -> realizer). --scale defaults to "1" (identity), not a magic
    // dimension — it is the user parameter the whole chain reads.
    const QCommandLineOption generateCryptOption(
        QStringLiteral("generate-crypt"),
        QStringLiteral("Generate the crypt MapSpec straight to a TOON map (Seam B) at --scale, then exit."),
        QStringLiteral("toon-path"));
    const QCommandLineOption scaleOption(
        QStringLiteral("scale"),
        QStringLiteral("Uniform scale S for --generate-crypt (multiplies every base dimension; "
                       "the TOON carries scaled feet + an advisory 'scale: S' header). Default 1."),
        QStringLiteral("S"),
        QStringLiteral("1"));
    // Realize chain R1c: emit the curated tester-zoo fixtures (catalog + manifest)
    // headless, mirroring --generate-crypt. Writes <dir>/tester_zoo.editzoo (the
    // MessagePack catalog) + <dir>/tester_zoo.manifest.toon (the curated-only TOON
    // the realizer reads), then exits — no window. The records come from the pure
    // declarative testerAssetCatalog(); this seam only adds Qt persistence.
    const QCommandLineOption mintTesterZooOption(
        QStringLiteral("mint-tester-zoo"),
        QStringLiteral("Mint the curated tester zoo to <dir>/tester_zoo.editzoo + "
                       "tester_zoo.manifest.toon (realize chain R1c), then exit."),
        QStringLiteral("dir"));
    parser.addOption(snapshotOption);
    parser.addOption(probeOption);
    parser.addOption(paintBenchOption);
    parser.addOption(benchObjectsOption);
    parser.addOption(demoRoomOption);
    parser.addOption(roomFileOption);
    parser.addOption(mapFileOption);
    parser.addOption(asciiMapOption);
    parser.addOption(opsFileOption);
    parser.addOption(exportMapOption);
    parser.addOption(generateCryptOption);
    parser.addOption(scaleOption);
    parser.addOption(mintTesterZooOption);
    parser.addOption(workspaceOption);
    parser.process(app);

    // M0 one-command terminus: generate the crypt -> TOON at a user-dialled scale,
    // headless (no window). Mirrors --export-map, but the SOURCE is the in-repo
    // generator, not a file. The document is driven through a controller so the
    // placed block instances (sarcophagus/brazier/stair) land in the blocks[] array,
    // exactly like --export-map's .edidraw branch.
    if (parser.isSet(generateCryptOption)) {
        QTextStream err(stderr);
        bool scaleOk = false;
        const double scale = parser.value(scaleOption).toDouble(&scaleOk);
        if (!scaleOk || !std::isfinite(scale) || scale <= 0.0) {
            err << "generate-crypt: --scale must be a positive number (got '"
                << parser.value(scaleOption) << "')\n";
            return 2;
        }
        const edi::drafting::MapSpec spec = edi::io::buildCryptMapSpec(scale);
        // canvasPerAuthoredUnit = 1.0 (identity): the document stores AUTHORED FEET
        // directly; exportMapToToon's sceneScale carries S into the advisory header,
        // so nothing is scaled twice (socket contract §0/§5 fence).
        DrawingDocumentController controller;
        if (!controller.createMapFromSpec(spec, 1.0)) {
            err << "generate-crypt: builder rejected the crypt MapSpec\n";
            return 2;
        }
        const std::string toon = edi::io::exportMapToToon(
            controller.draftingDocument(), "crypt", "feet", scale);
        QFile out(parser.value(generateCryptOption));
        if (!out.open(QIODevice::WriteOnly | QIODevice::Text)) {
            err << "generate-crypt: could not write " << parser.value(generateCryptOption) << '\n';
            return 2;
        }
        out.write(toon.data(), static_cast<qint64>(toon.size()));
        QTextStream(stdout) << "generate-crypt: wrote " << parser.value(generateCryptOption)
                            << " (scale " << scale << ", " << spec.rooms.size() << " rooms)\n";
        return 0;
    }

    // Realize chain R1c terminus: mint the curated tester zoo to its sample
    // fixtures, headless. The records are DATA (testerAssetCatalog, edi_zoo_core,
    // Qt-free); this block only does the Qt persistence — the same pure/Qt split
    // the codebase keeps everywhere. Two artifacts land side by side: the
    // MessagePack catalog (round-trips via AssetZooStore) and the curated-only
    // TOON manifest the realizer reads (exportZooToToon).
    if (parser.isSet(mintTesterZooOption)) {
        QTextStream err(stderr);
        const QString dir = parser.value(mintTesterZooOption);

        edi::zoo::AssetZoo zoo;
        zoo.assets = edi::zoo::testerAssetCatalog();

        // PRECONDITION: a record carrying a manifest reserved char would SILENTLY
        // corrupt the TOON round-trip, so refuse to persist before writing anything
        // (the recipeScriptParamKeyProblem precedent — validate at the boundary).
        for (const edi::zoo::AssetRecord &record : zoo.assets) {
            if (const auto problem = edi::zoo::zooManifestFieldProblem(record)) {
                err << "mint-tester-zoo: record '" << QString::fromStdString(record.id)
                    << "' is unfit for the manifest: " << QString::fromStdString(*problem) << '\n';
                return 2;
            }
        }

        const QString catalogPath = QDir(dir).filePath(QStringLiteral("tester_zoo.editzoo"));
        const auto saved = edi::io::saveAssetZoo(catalogPath, zoo);
        if (!saved.ok) {
            err << "mint-tester-zoo: could not save catalog " << catalogPath << ": "
                << QString::fromStdString(saved.message) << '\n';
            return 2;
        }

        // The curated-only TOON manifest. QSaveFile (atomic temp-then-rename) keeps
        // a half-written manifest from ever being committed as a fixture. UTF-8 is
        // explicit — the manifest carries the `·` separator (U+00B7) in texture runs.
        const std::string manifest = edi::zoo::exportZooToToon(zoo);
        const QString manifestPath = QDir(dir).filePath(QStringLiteral("tester_zoo.manifest.toon"));
        QSaveFile manifestFile(manifestPath);
        if (!manifestFile.open(QIODevice::WriteOnly | QIODevice::Text)
            || manifestFile.write(manifest.data(), static_cast<qint64>(manifest.size())) < 0
            || !manifestFile.commit()) {
            err << "mint-tester-zoo: could not write manifest " << manifestPath << '\n';
            return 2;
        }

        QTextStream(stdout) << "mint-tester-zoo: wrote " << catalogPath << " + " << manifestPath
                            << " (" << zoo.assets.size() << " curated assets)\n";
        return 0;
    }

    // Seam B export (Phase D): project a .map.toml straight to a TOON map document
    // the game engine reads, then exit — a headless CLI action that needs no
    // window. Parsed with canvasPerUnit=1.0 so footprints stay in the AUTHORED
    // unit (feet), not canvas units. This is the tool-first author->export
    // terminus: a stable neutral map crosses Seam B to the engine.
    if (parser.isSet(exportMapOption)) {
        QTextStream err(stderr);
        if (!parser.isSet(mapFileOption)) {
            err << "export-map: requires --map-file <path> (.map.toml or .edidraw) as the source\n";
            return 2;
        }
        const QString sourcePath = parser.value(mapFileOption);
        const QString title = QFileInfo(sourcePath).baseName(); // "dungeon.map.toml" -> "dungeon"
        std::string toon;
        std::size_t roomCount = 0;
        std::size_t blockNote = 0; // placed instances, when sourced from a document

        QFile in(sourcePath);
        if (sourcePath.endsWith(QStringLiteral(".edidraw"), Qt::CaseInsensitive)) {
            // A SAVED document carries the placed BLOCK INSTANCES the .map.toml never
            // had (Seam C). Decode it and project the live document, including blocks.
            if (!in.open(QIODevice::ReadOnly)) {
                err << "export-map: could not open " << sourcePath << '\n';
                return 2;
            }
            const QByteArray payload = in.readAll();
            const edi::formats::ByteBuffer bytes(payload.begin(), payload.end());
            const auto decoded = edi::drafting::decodeDraftingDocument(bytes, sourcePath.toStdString());
            if (!decoded.ok || !decoded.value) {
                err << "export-map: could not decode " << sourcePath << '\n';
                return 2;
            }
            toon = edi::io::exportMapToToon(*decoded.value, title.toStdString());
            roomCount = decoded.value->rooms.size();
            for (const auto &object : decoded.value->objects) {
                if (!object.metadata.blockPlacement.instanceId.empty()) {
                    ++blockNote; // counts member objects, not instances — just a hint
                }
            }
        } else {
            // A .map.toml authoring file — parse to a MapSpec (feet) and project.
            if (!in.open(QIODevice::ReadOnly | QIODevice::Text)) {
                err << "export-map: could not open " << sourcePath << '\n';
                return 2;
            }
            const edi::io::MapSpecParseResult parsed = edi::io::parseMapSpecToml(in.readAll().toStdString(), 1.0);
            if (!parsed.ok) {
                err << "export-map: " << QString::fromStdString(parsed.message) << '\n';
                return 2;
            }
            toon = edi::io::exportMapToToon(parsed.spec, title.toStdString());
            roomCount = parsed.spec.rooms.size();
        }

        QFile out(parser.value(exportMapOption));
        if (!out.open(QIODevice::WriteOnly | QIODevice::Text)) {
            err << "export-map: could not write " << parser.value(exportMapOption) << '\n';
            return 2;
        }
        out.write(toon.data(), static_cast<qint64>(toon.size()));
        QTextStream(stdout) << "export-map: wrote " << parser.value(exportMapOption)
                            << " (" << roomCount << " rooms";
        if (blockNote > 0) {
            QTextStream(stdout) << ", with placed blocks";
        }
        QTextStream(stdout) << ")\n";
        return 0;
    }

    EdiShellWindow window;
    window.loadSettings(edi::io::defaultSettingsPath());
    window.loadWorkspaceLayout(edi::io::defaultWorkspaceLayoutPath());
    window.loadTextSession(edi::io::defaultTextSessionPath());
    // D08: hold a LIVE asset zoo on the running shell. Best-effort, beside the
    // other startup loads — a decode failure leaves the window's zoo EMPTY (the
    // loader deliberately does NOT fall back to testerAssetCatalog(): that is
    // fixture data and would pollute D07's controller validation). No visible
    // chrome — this is plumbing for the on-demand asset-ref validation.
    window.loadAssetZoo(edi::io::defaultAssetZooPath());

    // Feed the custom-craftsman registry before any workspace mounts: run
    // edi_craft.py --list-craftsmen under python and hand the window the TOML it
    // prints, so the lab palette offers the installed craftsmen. BEST-EFFORT —
    // a missing python or script (or a non-dev layout) just leaves the registry
    // empty (no craftsman buttons), and the 5s cap keeps a hung python from
    // blocking startup. The widget seam is injected (tests feed a literal), so
    // this is the one place the real subprocess lives. The script sits at
    // <repo>/tools/blender beside the build dir the binary runs from.
    {
        const QString python = QStandardPaths::findExecutable(QStringLiteral("python3"));
        const QString craftScript = QDir::cleanPath(
            QCoreApplication::applicationDirPath()
            + QStringLiteral("/../tools/blender/edi_craft.py"));
        if (!python.isEmpty() && QFileInfo::exists(craftScript)) {
            QProcess listing;
            listing.start(python, {craftScript, QStringLiteral("--list-craftsmen")});
            if (listing.waitForFinished(5000) && listing.exitStatus() == QProcess::NormalExit
                && listing.exitCode() == 0) {
                window.setCraftsmanRegistryToml(QString::fromUtf8(listing.readAllStandardOutput()));
            }
        }
    }

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

    // Seam-A demo: the "a 30x20 stone guard antechamber, oak door south, secret
    // door east, corridor north" prompt, as a neutral RoomSpec, generates the
    // room's mitered walls + doorway gaps in one atomic step. Proves NL -> spec
    // -> geometry: the AI fills the spec, edi builds the layout deterministically.
    if (parser.isSet(demoRoomOption)) {
        auto *controller = window.findChild<DrawingDocumentController *>();
        if (controller != nullptr) {
            edi::drafting::RoomSpec spec;
            spec.origin = {0.30, 0.12};
            spec.width = 0.42;
            spec.height = 0.26;
            spec.wallThickness = 0.016;
            spec.openings = {
                {edi::drafting::RoomEdge::North, 0.21, 0.10, "corridor"},
                {edi::drafting::RoomEdge::East, 0.13, 0.05, "secret"},
                {edi::drafting::RoomEdge::South, 0.21, 0.07, "door"},
            };
            const bool ok = controller->createRoomFromSpec(spec);
            QTextStream(stdout) << "demo-room: guard antechamber "
                                << (ok ? "generated" : "rejected") << '\n';
        }
    }

    // Seam-A, end to end with NO AI in the loop: read a .room.toml authoring
    // file, parse it to a neutral spec, and build the map. Distances are in feet;
    // 0.02 canvas/ft puts a 50 ft (10-square) board across the canvas.
    if (parser.isSet(roomFileOption)) {
        auto *controller = window.findChild<DrawingDocumentController *>();
        QFile file(parser.value(roomFileOption));
        QTextStream err(stderr);
        if (controller == nullptr) {
            err << "room-file: no controller\n";
        } else if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            err << "room-file: could not open " << parser.value(roomFileOption) << '\n';
        } else {
            const std::string text = file.readAll().toStdString();
            const edi::io::RoomSpecParseResult parsed = edi::io::parseRoomSpecToml(text, 0.02);
            if (!parsed.ok) {
                err << "room-file: " << QString::fromStdString(parsed.message) << '\n';
            } else {
                const bool ok = controller->createRoomFromSpec(parsed.spec);
                QTextStream(stdout) << "room-file: " << (ok ? "generated" : "rejected by builder") << '\n';
            }
        }
    }

    // The multi-room authoring path: one .map.toml -> many rooms + cross-room
    // connections, the whole graph in one undo step.
    if (parser.isSet(mapFileOption)) {
        auto *controller = window.findChild<DrawingDocumentController *>();
        QFile file(parser.value(mapFileOption));
        QTextStream err(stderr);
        if (controller == nullptr) {
            err << "map-file: no controller\n";
        } else if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            err << "map-file: could not open " << parser.value(mapFileOption) << '\n';
        } else {
            const std::string text = file.readAll().toStdString();
            constexpr double kCanvasPerFoot = 0.02; // 0.02 canvas/ft puts a 50 ft board across the canvas
            const edi::io::MapSpecParseResult parsed = edi::io::parseMapSpecToml(text, kCanvasPerFoot);
            if (!parsed.ok) {
                err << "map-file: " << QString::fromStdString(parsed.message) << '\n';
            } else {
                // Record the same scale on the document so the engine export recovers feet.
                const bool ok = controller->createMapFromSpec(parsed.spec, kCanvasPerFoot);
                QTextStream(stdout) << "map-file: " << (ok ? "generated" : "rejected by builder") << '\n';
                // DM-01: frame the freshly-loaded map so it renders whole, not
                // clipped. View-only (no model mutation), so it can't loop. The
                // snapshot settle below re-fits after any workspace switch sizes
                // the canvas — this covers the interactive/no-snapshot path.
                if (ok) {
                    if (auto *canvas = window.findChild<DrawingCanvasWidget *>()) {
                        canvas->fitDocumentInView();
                    }
                }
            }
        }
    }

    // The control-gate path: an ASCII glyph grid -> walls/doors/features. The
    // grid IS the proof; the same glyphs the user sees become the geometry.
    if (parser.isSet(asciiMapOption)) {
        auto *controller = window.findChild<DrawingDocumentController *>();
        QFile file(parser.value(asciiMapOption));
        QTextStream err(stderr);
        if (controller == nullptr) {
            err << "ascii-map: no controller\n";
        } else if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            err << "ascii-map: could not open " << parser.value(asciiMapOption) << '\n';
        } else {
            const bool ok = controller->createMapFromAscii(file.readAll().toStdString());
            QTextStream(stdout) << "ascii-map: " << (ok ? "generated" : "rejected") << '\n';
        }
    }

    // Load a recipe into the op stream (Seam A), so --workspace blender renders
    // its ASCII proof. Same non-fatal handler style as the map loaders above: a
    // refusal names the offending key and we still settle the window.
    if (parser.isSet(opsFileOption)) {
        QTextStream err(stderr);
        if (!window.openOpsRecipeFromPath(parser.value(opsFileOption))) {
            err << "ops-file: " << window.lastRecipeError() << '\n';
        } else {
            QTextStream(stdout) << "ops-file: loaded " << parser.value(opsFileOption) << '\n';
        }
    }

    // Eyeball ANY job, not just the shipped drafting one: switch the window to a
    // named workspace mode before the capture settles. Runs after the map/ascii
    // population above, so --workspace map --map-file <dungeon> renders the Map
    // browser over a real dungeon's graph. (Mode resolves by the same names the
    // rail uses; an unknown name is a named refusal, never a silent fallback.)
    if (parser.isSet(workspaceOption)) {
        QTextStream err(stderr);
        const auto mode = edi::app::workspaceModeFromName(parser.value(workspaceOption).toStdString());
        if (!mode) {
            err << "workspace: unknown mode '" << parser.value(workspaceOption) << "'\n";
        } else if (*mode == edi::app::WorkspaceMode::Settings) {
            // Settings is a pop-out the rail opens, not a layout setWorkspaceMode
            // can mount — passing it would silently no-op to drafting. Refuse by
            // name rather than mislead (the same named-refusal rule the seams use).
            err << "workspace: 'settings' is UI-only, not a snapshot workspace\n";
        } else {
            window.setWorkspaceMode(*mode);
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
            // DM-01: re-fit AFTER the layout settles. The canvas only knows its
            // real pixel size once the splitter/workspace has been sized, so the
            // authoritative framing for the captured image happens here, right
            // before the grab. View-only — no model edit, no modelChanged loop.
            if (auto *canvas = window.findChild<DrawingCanvasWidget *>()) {
                canvas->fitDocumentInView();
            }
            runRenderProof(app, window,
                           parser.value(snapshotOption), parser.values(probeOption));
        });
    }

    return app.exec();
}
