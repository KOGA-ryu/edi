#include <QCoreApplication>
#include <QFile>
#include <QJSEngine>
#include <QJSValue>
#include <QIODevice>
#include <QTextStream>

#include <cmath>

namespace {

bool expect(bool condition, const QString &message) {
    if (!condition) {
        QTextStream(stderr) << "FAIL: " << message << "\n";
        return false;
    }
    return true;
}

bool expectNear(double actual, double expected, const QString &message) {
    return expect(std::abs(actual - expected) < 0.0001,
                  QStringLiteral("%1; expected %2, got %3").arg(message).arg(expected).arg(actual));
}

QJSValue call(QJSEngine &engine, const QString &name, const QJSValueList &args) {
    return engine.globalObject().property(name).call(args);
}

bool loadViewportModule(QJSEngine &engine) {
    QFile file(QStringLiteral(PROJECT_SOURCE_DIR) + QStringLiteral("/src/features/drawing_tool/DrawingCanvasViewport.js"));
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream(stderr) << "FAIL: could not open DrawingCanvasViewport.js\n";
        return false;
    }
    const QString source = QString::fromUtf8(file.readAll()).replace(QStringLiteral(".pragma library"), QString());
    const QJSValue result = engine.evaluate(source, file.fileName());
    if (result.isError()) {
        QTextStream(stderr) << "FAIL: could not evaluate DrawingCanvasViewport.js: " << result.toString() << "\n";
        return false;
    }
    return true;
}

bool runBoardBoundsContract() {
    QJSEngine engine;
    bool ok = loadViewportModule(engine);
    if (!ok) {
        return false;
    }

    const QJSValue bounds = call(engine, QStringLiteral("boardBounds"), {800, 600, 1.5, 12, -8});
    ok &= expect(bounds.isObject(), QStringLiteral("boardBounds should return an object"));
    ok &= expectNear(bounds.property(QStringLiteral("size")).toNumber(), 876.0,
                     QStringLiteral("boardBounds should use min dimension minus margin times zoom"));
    ok &= expectNear(bounds.property(QStringLiteral("x")).toNumber(), -26.0,
                     QStringLiteral("boardBounds should center and pan x deterministically"));
    ok &= expectNear(bounds.property(QStringLiteral("y")).toNumber(), -146.0,
                     QStringLiteral("boardBounds should center and pan y deterministically"));
    ok &= expect(std::isfinite(bounds.property(QStringLiteral("x")).toNumber())
                     && std::isfinite(bounds.property(QStringLiteral("y")).toNumber())
                     && std::isfinite(bounds.property(QStringLiteral("size")).toNumber())
                     && bounds.property(QStringLiteral("size")).toNumber() > 0.0,
                 QStringLiteral("boardBounds should return finite positive bounds"));

    const QJSValue tinyBounds = call(engine, QStringLiteral("boardBounds"), {4, 3, 0, 0, 0});
    ok &= expectNear(tinyBounds.property(QStringLiteral("size")).toNumber(), 0.0032,
                     QStringLiteral("boardBounds should clamp invalid zoom while preserving minimum board rule"));
    return ok;
}

bool runCoordinateRoundTripContract() {
    QJSEngine engine;
    bool ok = loadViewportModule(engine);
    if (!ok) {
        return false;
    }

    const QJSValue bounds = call(engine, QStringLiteral("boardBounds"), {1024, 768, 2.0, -24, 40});
    const QJSValue screen = call(engine, QStringLiteral("canvasToScreen"), {bounds, 0.375, 0.625});
    const QJSValue normalized = call(engine, QStringLiteral("screenToCanvas"), {
        bounds,
        screen.property(QStringLiteral("x")).toNumber(),
        screen.property(QStringLiteral("y")).toNumber(),
    });

    ok &= expectNear(normalized.property(QStringLiteral("x")).toNumber(), 0.375,
                     QStringLiteral("screenToCanvas should round-trip x with canvasToScreen"));
    ok &= expectNear(normalized.property(QStringLiteral("y")).toNumber(), 0.625,
                     QStringLiteral("screenToCanvas should round-trip y with canvasToScreen"));
    ok &= expectNear(call(engine, QStringLiteral("canvasToScreenX"), {bounds, 0.375}).toNumber(), screen.property(QStringLiteral("x")).toNumber(),
                     QStringLiteral("canvasToScreenX should match canvasToScreen x"));
    ok &= expectNear(call(engine, QStringLiteral("canvasToScreenY"), {bounds, 0.625}).toNumber(), screen.property(QStringLiteral("y")).toNumber(),
                     QStringLiteral("canvasToScreenY should match canvasToScreen y"));
    return ok;
}

} // namespace

int main(int argc, char **argv) {
    QCoreApplication app(argc, argv);
    bool ok = true;
    ok &= runBoardBoundsContract();
    ok &= runCoordinateRoundTripContract();
    return ok ? 0 : 1;
}
