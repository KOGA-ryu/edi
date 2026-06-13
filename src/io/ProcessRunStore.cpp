#include "io/ProcessRunStore.h"

#include <QProcess>
#include <QTimer>

ProcessRunStore::ProcessRunStore(QObject *parent)
    : QObject(parent)
    , m_process(new QProcess(this))
    , m_timeout(new QTimer(this))
{
    m_timeout->setSingleShot(true);

    // The timeout kills a runaway process; the kill drives QProcess::finished
    // (with a Crashed status), where the timedOut flag turns it into a clean
    // "timed out" result rather than a spurious crash.
    connect(m_timeout, &QTimer::timeout, this, [this]() {
        if (m_process->state() != QProcess::NotRunning) {
            m_timedOut = true;
            m_process->kill();
        }
    });

    // FailedToStart is the ONLY error that does not also raise finished() (the
    // process never ran), so it is the one we must terminate here. Other errors
    // (Crashed, etc.) are followed by finished(), which assembles the result.
    connect(m_process, &QProcess::errorOccurred, this, [this](QProcess::ProcessError error) {
        if (error == QProcess::FailedToStart) {
            ProcessRunResult result;
            result.started = false;
            result.message = QStringLiteral("failed to start: %1").arg(m_process->program());
            emitOnce(std::move(result));
        }
    });

    connect(m_process, &QProcess::finished, this,
            [this](int exitCode, QProcess::ExitStatus status) {
        ProcessRunResult result;
        result.started = true;
        result.exitCode = exitCode;
        result.timedOut = m_timedOut;
        result.standardOutput = QString::fromUtf8(m_process->readAllStandardOutput());
        result.standardError = QString::fromUtf8(m_process->readAllStandardError());
        result.ok = !m_timedOut && status == QProcess::NormalExit && exitCode == 0;
        if (m_timedOut) {
            result.message = QStringLiteral("timed out");
        } else if (result.ok) {
            result.message = QStringLiteral("ok");
        } else {
            result.message = QStringLiteral("exited %1").arg(exitCode);
        }
        emitOnce(std::move(result));
    });
}

void ProcessRunStore::run(const QString &executablePath, const QStringList &args,
                          const QString &workingDir, int timeoutMs)
{
    if (m_running) {
        // Refuse a concurrent run rather than racing the first — the result is
        // for THIS call, reported the same way as any other outcome.
        ProcessRunResult busy;
        busy.message = QStringLiteral("a process is already running");
        emit finished(busy);
        return;
    }
    m_running = true;
    m_emitted = false;
    m_timedOut = false;
    if (!workingDir.isEmpty()) {
        m_process->setWorkingDirectory(workingDir);
    }
    if (timeoutMs > 0) {
        m_timeout->start(timeoutMs);
    }
    m_process->start(executablePath, args);
}

void ProcessRunStore::emitOnce(ProcessRunResult result)
{
    // A crashed process raises BOTH errorOccurred and finished; the timeout kill
    // does too. Guard so exactly one finished() leaves the store per run.
    if (m_emitted) {
        return;
    }
    m_emitted = true;
    m_running = false;
    m_timeout->stop();
    emit finished(result);
}
