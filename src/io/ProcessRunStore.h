#pragma once

#include <QObject>
#include <QString>
#include <QStringList>

class QProcess;
class QTimer;

// The result of one process run, surfaced once via ProcessRunStore::finished.
// Plain data (no Qt parentage) so callers copy it freely.
struct ProcessRunResult {
    bool ok = false;          // started, exited 0, within the timeout
    bool started = false;     // the binary actually launched (false = not found / not executable)
    bool timedOut = false;    // killed for exceeding the timeout
    int exitCode = -1;
    QString standardOutput;
    QString standardError;
    QString message;          // a short human summary
};

// The app's FIRST external-process seam: an async wrapper over QProcess that runs
// one command and reports the outcome once. Effect-only — the WHAT-to-run is a
// pure BlenderRunPlan; this is the launch. Kept behind a store (like
// DrawingDocumentStore wraps file IO) and injectable as a callable at the window
// so the offscreen test suite never launches a real Blender.
//
// Lifecycle care (no prior subprocess conventions existed): exactly one finished
// per run; a failed launch (FailedToStart) reports started=false; a run past the
// timeout is killed and reported timedOut; a second run while busy is refused
// rather than racing the first.
class ProcessRunStore final : public QObject {
    Q_OBJECT

public:
    explicit ProcessRunStore(QObject *parent = nullptr);

    // Starts the process and returns immediately; emits finished() exactly once.
    void run(const QString &executablePath, const QStringList &args,
             const QString &workingDir = QString(), int timeoutMs = 120000);
    bool isRunning() const { return m_running; }

signals:
    void finished(const ProcessRunResult &result);

private:
    void emitOnce(ProcessRunResult result);

    QProcess *m_process = nullptr;
    QTimer *m_timeout = nullptr;
    bool m_running = false;
    bool m_emitted = false;
    bool m_timedOut = false;
};
