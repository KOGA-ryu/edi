#include "io/ProcessRunStore.h"

#include <QCoreApplication>
#include <QEventLoop>
#include <QTimer>

#include <cassert>

namespace {

// Run a command and spin the event loop until the single finished() arrives.
// A watchdog QTimer fails loudly rather than hanging the suite if it never does.
ProcessRunResult runAndWait(ProcessRunStore &store, const QString &exe,
                            const QStringList &args, int timeoutMs = 5000)
{
    ProcessRunResult captured;
    bool done = false;
    QEventLoop loop;
    QObject::connect(&store, &ProcessRunStore::finished, &loop,
                     [&](const ProcessRunResult &result) {
        captured = result;
        done = true;
        loop.quit();
    });
    QTimer watchdog;
    watchdog.setSingleShot(true);
    QObject::connect(&watchdog, &QTimer::timeout, &loop, &QEventLoop::quit);
    watchdog.start(10000);
    store.run(exe, args, QString(), timeoutMs);
    if (!done) {
        loop.exec();
    }
    assert(done && "ProcessRunStore::finished never arrived");
    return captured;
}

} // namespace

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);

    // A clean run: echo exits 0 and its stdout is captured. This is the app's
    // first real subprocess — proven harmlessly, no Blender required.
    {
        ProcessRunStore store;
        const ProcessRunResult result = runAndWait(store, QStringLiteral("/bin/echo"),
                                                    {QStringLiteral("hello")});
        assert(result.ok && result.started && !result.timedOut);
        assert(result.exitCode == 0);
        assert(result.standardOutput.trimmed() == QStringLiteral("hello"));
    }

    // A non-zero exit is reported, not swallowed: ok=false but started=true.
    {
        ProcessRunStore store;
        const ProcessRunResult result = runAndWait(store, QStringLiteral("/bin/sh"),
                                                    {QStringLiteral("-c"), QStringLiteral("exit 3")});
        assert(result.started && !result.ok);
        assert(result.exitCode == 3);
    }

    // A binary that cannot launch: started=false, a named message, no crash.
    {
        ProcessRunStore store;
        const ProcessRunResult result = runAndWait(store,
            QStringLiteral("/no/such/edi/binary/blender"), {});
        assert(!result.started && !result.ok);
        assert(result.message.contains(QStringLiteral("failed to start")));
    }

    // A process past its timeout is killed and reported timedOut (not as a
    // spurious success or an unhandled crash).
    {
        ProcessRunStore store;
        const ProcessRunResult result = runAndWait(store, QStringLiteral("/bin/sleep"),
                                                    {QStringLiteral("5")}, 200);
        assert(result.timedOut && !result.ok);
        assert(result.message == QStringLiteral("timed out"));
    }

    // stderr is captured distinctly from stdout.
    {
        ProcessRunStore store;
        const ProcessRunResult result = runAndWait(store, QStringLiteral("/bin/sh"),
            {QStringLiteral("-c"), QStringLiteral("echo out; echo err 1>&2")});
        assert(result.ok);
        assert(result.standardOutput.trimmed() == QStringLiteral("out"));
        assert(result.standardError.trimmed() == QStringLiteral("err"));
    }

    return 0;
}
