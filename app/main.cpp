#include <QGuiApplication>
#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QCryptographicHash>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QImage>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMetaObject>
#include <QObject>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickWindow>
#include <QSaveFile>
#include <QTimer>
#include <QtGlobal>
#include <QUrl>
#include <QVariantList>
#include <QVariantMap>
#include <utility>

#include "core/DrawingCore.h"

class ShellLayoutStore final : public QObject {
    Q_OBJECT

public:
    explicit ShellLayoutStore(QString path, QObject *parent = nullptr)
        : QObject(parent),
          m_path(std::move(path)) {}

    Q_INVOKABLE bool save(const QVariantMap &layout) const {
        QSaveFile file(m_path);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            return false;
        }

        const QJsonObject object = QJsonObject::fromVariantMap(layout);
        const QJsonDocument document(object);
        file.write(document.toJson(QJsonDocument::Indented));
        return file.commit();
    }

    Q_INVOKABLE QString path() const {
        return m_path;
    }

private:
    QString m_path;
};

class DrawingDocumentStore final : public QObject {
    Q_OBJECT

public:
    explicit DrawingDocumentStore(QObject *parent = nullptr)
        : QObject(parent) {}

    Q_INVOKABLE QVariantMap save(const QUrl &url, const QVariantMap &model) const {
        QVariantMap result;
        result.insert(QStringLiteral("ok"), false);
        result.insert(QStringLiteral("message"), QStringLiteral("save unavailable"));

        QString path = localPath(url);
        if (path.trimmed().isEmpty()) {
            result.insert(QStringLiteral("message"), QStringLiteral("drawing path missing"));
            return result;
        }
        if (QFileInfo(path).suffix().isEmpty()) {
            path += QStringLiteral(".json");
        }

        const QFileInfo info(path);
        if (!info.absoluteDir().exists() && !QDir().mkpath(info.absolutePath())) {
            result.insert(QStringLiteral("message"), QStringLiteral("drawing directory unavailable"));
            return result;
        }

        const QJsonObject object = QJsonObject::fromVariantMap(model);
        if (object.isEmpty()) {
            result.insert(QStringLiteral("message"), QStringLiteral("drawing model empty"));
            return result;
        }

        QSaveFile file(path);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
            result.insert(QStringLiteral("message"), QStringLiteral("drawing write failed"));
            return result;
        }
        file.write(QJsonDocument(object).toJson(QJsonDocument::Indented));
        if (!file.commit()) {
            result.insert(QStringLiteral("message"), QStringLiteral("drawing commit failed"));
            return result;
        }

        result.insert(QStringLiteral("ok"), true);
        result.insert(QStringLiteral("message"), QStringLiteral("saved drawing"));
        result.insert(QStringLiteral("path"), path);
        return result;
    }

    Q_INVOKABLE QVariantMap open(const QUrl &url) const {
        QVariantMap result;
        result.insert(QStringLiteral("ok"), false);
        result.insert(QStringLiteral("message"), QStringLiteral("open unavailable"));

        const QString path = localPath(url);
        if (path.trimmed().isEmpty()) {
            result.insert(QStringLiteral("message"), QStringLiteral("drawing path missing"));
            return result;
        }

        QFile file(path);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            result.insert(QStringLiteral("message"), QStringLiteral("drawing read failed"));
            return result;
        }

        const QJsonDocument document = QJsonDocument::fromJson(file.readAll());
        if (!document.isObject()) {
            result.insert(QStringLiteral("message"), QStringLiteral("drawing json invalid"));
            return result;
        }

        const QJsonObject object = document.object();
        if (object.value(QStringLiteral("export_kind")).toString() != QStringLiteral("pattern_lab_2d_native_model_v0")) {
            result.insert(QStringLiteral("message"), QStringLiteral("not a Draftsman drawing"));
            return result;
        }

        result.insert(QStringLiteral("ok"), true);
        result.insert(QStringLiteral("message"), QStringLiteral("opened drawing"));
        result.insert(QStringLiteral("path"), path);
        result.insert(QStringLiteral("model"), object.toVariantMap());
        return result;
    }

    Q_INVOKABLE QVariantMap exportSvg(const QUrl &url, const QString &svg) const {
        QVariantMap result;
        result.insert(QStringLiteral("ok"), false);
        result.insert(QStringLiteral("message"), QStringLiteral("svg export unavailable"));

        QString path = localPath(url);
        if (path.trimmed().isEmpty()) {
            result.insert(QStringLiteral("message"), QStringLiteral("svg path missing"));
            return result;
        }
        if (QFileInfo(path).suffix().isEmpty()) {
            path += QStringLiteral(".svg");
        }

        const QFileInfo info(path);
        if (!info.absoluteDir().exists() && !QDir().mkpath(info.absolutePath())) {
            result.insert(QStringLiteral("message"), QStringLiteral("svg directory unavailable"));
            return result;
        }
        if (svg.trimmed().isEmpty()) {
            result.insert(QStringLiteral("message"), QStringLiteral("svg output empty"));
            return result;
        }

        QSaveFile file(path);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
            result.insert(QStringLiteral("message"), QStringLiteral("svg write failed"));
            return result;
        }
        file.write(svg.toUtf8());
        if (!file.commit()) {
            result.insert(QStringLiteral("message"), QStringLiteral("svg commit failed"));
            return result;
        }

        result.insert(QStringLiteral("ok"), true);
        result.insert(QStringLiteral("message"), QStringLiteral("exported svg"));
        result.insert(QStringLiteral("path"), path);
        return result;
    }

private:
    static QString localPath(const QUrl &url) {
        if (url.isLocalFile()) {
            return url.toLocalFile();
        }
        const QString text = url.toString().trimmed();
        if (text.startsWith(QStringLiteral("file://"))) {
            return QUrl(text).toLocalFile();
        }
        return text;
    }
};

class TextEditorStore final : public QObject {
    Q_OBJECT

public:
    explicit TextEditorStore(QString manifestPath, QObject *parent = nullptr)
        : QObject(parent),
          m_manifestPath(std::move(manifestPath)) {}

    Q_INVOKABLE QVariantMap save(const QVariantList &documents, const QString &activeId, bool saveAll,
                                const QVariantMap &editorState = QVariantMap()) const {
        QVariantMap result;
        result.insert(QStringLiteral("ok"), false);
        result.insert(QStringLiteral("message"), QStringLiteral("storage unavailable"));
        result.insert(QStringLiteral("documents"), documents);

        if (m_manifestPath.isEmpty()) {
            return result;
        }

        const QFileInfo manifestInfo(m_manifestPath);
        QDir manifestDir(manifestInfo.absoluteDir());
        if (!manifestDir.exists() && !QDir().mkpath(manifestDir.absolutePath())) {
            result.insert(QStringLiteral("message"), QStringLiteral("manifest directory unavailable"));
            return result;
        }

        QVariantList savedDocuments;
        QJsonArray manifestDocuments;
        const QVariantMap normalizedState = normalizeEditorState(editorState);
        for (const QVariant &entry : documents) {
            QVariantMap document = entry.toMap();
            const QString id = document.value(QStringLiteral("id")).toString().trimmed();
            if (id.isEmpty()) {
                result.insert(QStringLiteral("message"), QStringLiteral("document id missing"));
                return result;
            }

            QString relativePath = document.value(QStringLiteral("path")).toString().trimmed();
            if (relativePath.isEmpty()) {
                relativePath = QStringLiteral("docs/") + safeFileStem(id) + QStringLiteral(".txt");
                document.insert(QStringLiteral("path"), relativePath);
            }

            const QString cleanPath = cleanRelativePath(relativePath);
            if (cleanPath.isEmpty()) {
                result.insert(QStringLiteral("message"), QStringLiteral("invalid document path"));
                return result;
            }
            document.insert(QStringLiteral("path"), cleanPath);

            const bool shouldWriteText = saveAll || id == activeId;
            if (shouldWriteText) {
                const QString absoluteTextPath = manifestDir.absoluteFilePath(cleanPath);
                const QFileInfo textInfo(absoluteTextPath);
                if (!textInfo.absoluteDir().exists() && !QDir().mkpath(textInfo.absolutePath())) {
                    result.insert(QStringLiteral("message"), QStringLiteral("document directory unavailable"));
                    return result;
                }

                QSaveFile textFile(absoluteTextPath);
                if (!textFile.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
                    result.insert(QStringLiteral("message"), QStringLiteral("document write failed"));
                    return result;
                }
                textFile.write(document.value(QStringLiteral("text")).toString().toUtf8());
                if (!textFile.commit()) {
                    result.insert(QStringLiteral("message"), QStringLiteral("document commit failed"));
                    return result;
                }
                document.insert(QStringLiteral("initialText"), document.value(QStringLiteral("text")).toString());
            }

            const QString name = document.value(QStringLiteral("name")).toString().isEmpty()
                ? QStringLiteral("untitled.txt")
                : document.value(QStringLiteral("name")).toString();
            const QString language = document.value(QStringLiteral("language")).toString().isEmpty()
                ? QStringLiteral("text")
                : document.value(QStringLiteral("language")).toString();
            const QString role = normalizedDocumentRole(document.value(QStringLiteral("role")).toString());
            document.insert(QStringLiteral("role"), role);

            QJsonObject manifestDocument;
            manifestDocument.insert(QStringLiteral("id"), id);
            manifestDocument.insert(QStringLiteral("name"), name);
            manifestDocument.insert(QStringLiteral("language"), language);
            manifestDocument.insert(QStringLiteral("path"), cleanPath);
            manifestDocument.insert(QStringLiteral("role"), role);
            manifestDocuments.append(manifestDocument);
            savedDocuments.append(document);
        }

        QJsonObject manifest;
        manifest.insert(QStringLiteral("schema_version"), 1);
        manifest.insert(QStringLiteral("documents"), manifestDocuments);
        manifest.insert(QStringLiteral("editor_state"), QJsonObject::fromVariantMap(normalizedState));

        QSaveFile manifestFile(m_manifestPath);
        if (!manifestFile.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
            result.insert(QStringLiteral("message"), QStringLiteral("manifest write failed"));
            return result;
        }
        manifestFile.write(QJsonDocument(manifest).toJson(QJsonDocument::Indented));
        if (!manifestFile.commit()) {
            result.insert(QStringLiteral("message"), QStringLiteral("manifest commit failed"));
            return result;
        }

        result.insert(QStringLiteral("ok"), true);
        result.insert(QStringLiteral("message"), saveAll ? QStringLiteral("saved all") : QStringLiteral("saved active"));
        result.insert(QStringLiteral("documents"), savedDocuments);
        return result;
    }

    Q_INVOKABLE QString path() const {
        return m_manifestPath;
    }

    Q_INVOKABLE QVariantMap exportBundle(const QVariantList &documents, const QString &activeId, const QVariantMap &metadata) const {
        QVariantMap result;
        result.insert(QStringLiteral("ok"), false);
        result.insert(QStringLiteral("message"), QStringLiteral("storage unavailable"));

        if (m_manifestPath.isEmpty()) {
            return result;
        }

        const QFileInfo manifestInfo(m_manifestPath);
        QDir manifestDir(manifestInfo.absoluteDir());
        if (!manifestDir.exists() && !QDir().mkpath(manifestDir.absolutePath())) {
            result.insert(QStringLiteral("message"), QStringLiteral("manifest directory unavailable"));
            return result;
        }

        const QString packetType = safeFileStem(metadata.value(QStringLiteral("packet_type")).toString().trimmed().isEmpty()
            ? QStringLiteral("text_editor_bundle")
            : metadata.value(QStringLiteral("packet_type")).toString());
        const bool dexHandoff = packetType == QStringLiteral("dex_handoff");
        const QString timestamp = QDateTime::currentDateTimeUtc().toString(QStringLiteral("yyyyMMdd_HHmmss"));
        const QString exportRoot = manifestDir.absoluteFilePath(QStringLiteral("exports"));
        if (!QDir().mkpath(exportRoot)) {
            result.insert(QStringLiteral("message"), QStringLiteral("export directory unavailable"));
            return result;
        }

        QString packetName = packetType + QStringLiteral("_") + timestamp;
        QString packetPath = QDir(exportRoot).absoluteFilePath(packetName);
        int suffix = 2;
        while (QFileInfo::exists(packetPath)) {
            packetName = packetType + QStringLiteral("_") + timestamp + QStringLiteral("_") + QString::number(suffix);
            packetPath = QDir(exportRoot).absoluteFilePath(packetName);
            suffix += 1;
        }

        QDir packetDir(packetPath);
        if (!QDir().mkpath(packetPath) || !packetDir.mkpath(QStringLiteral("documents"))) {
            result.insert(QStringLiteral("message"), QStringLiteral("packet directory unavailable"));
            return result;
        }

        QJsonArray manifestDocuments;
        QJsonArray packetFiles;
        QString combined;
        QString promptText;
        QString contextText;
        QString promptDocumentId;
        QString promptDocumentName;
        int contextDocumentCount = 0;
        int exportedCount = 0;
        QVariantList pendingPromptDocuments;
        QVariantList exportedHandoffDocuments;
        for (const QVariant &entry : documents) {
            const QVariantMap document = entry.toMap();
            const QString id = document.value(QStringLiteral("id")).toString().trimmed();
            if (id.isEmpty()) {
                continue;
            }
            const QString role = normalizedDocumentRole(document.value(QStringLiteral("role")).toString());
            if (dexHandoff && role == QStringLiteral("scratch")) {
                continue;
            }

            const QString name = document.value(QStringLiteral("name")).toString().trimmed().isEmpty()
                ? id + QStringLiteral(".txt")
                : document.value(QStringLiteral("name")).toString().trimmed();
            const QString fileName = safeExportFileName(name, id);
            const QString relativePath = QStringLiteral("documents/") + fileName;
            const QString text = document.value(QStringLiteral("text")).toString();

            QSaveFile textFile(packetDir.absoluteFilePath(relativePath));
            if (!textFile.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
                result.insert(QStringLiteral("message"), QStringLiteral("document export failed"));
                return result;
            }
            textFile.write(text.toUtf8());
            if (!textFile.commit()) {
                result.insert(QStringLiteral("message"), QStringLiteral("document export commit failed"));
                return result;
            }
            const QString documentHash = sha256Hex(text.toUtf8());
            packetFiles.append(packetFileRecord(relativePath, documentHash, text.toUtf8().size()));

            combined += QStringLiteral("===== ") + name + QStringLiteral(" [") + id + QStringLiteral("] =====\n");
            combined += text;
            if (!combined.endsWith(QLatin1Char('\n'))) {
                combined += QLatin1Char('\n');
            }
            combined += QLatin1Char('\n');

            if (dexHandoff) {
                QVariantMap handoffDocument;
                handoffDocument.insert(QStringLiteral("id"), id);
                handoffDocument.insert(QStringLiteral("name"), name);
                handoffDocument.insert(QStringLiteral("text"), text);
                handoffDocument.insert(QStringLiteral("role"), role);
                exportedHandoffDocuments.append(handoffDocument);
                if (role == QStringLiteral("prompt")) {
                    pendingPromptDocuments.append(handoffDocument);
                }
                if (role == QStringLiteral("context") || role == QStringLiteral("reference")) {
                    contextText += QStringLiteral("===== ") + name + QStringLiteral(" [") + id + QStringLiteral("] =====\n");
                    contextText += text;
                    if (!contextText.endsWith(QLatin1Char('\n'))) {
                        contextText += QLatin1Char('\n');
                    }
                    contextText += QLatin1Char('\n');
                    contextDocumentCount += 1;
                }
            }

            QJsonObject manifestDocument;
            manifestDocument.insert(QStringLiteral("id"), id);
            manifestDocument.insert(QStringLiteral("name"), name);
            const QString language = document.value(QStringLiteral("language")).toString().trimmed().isEmpty()
                ? QStringLiteral("text")
                : document.value(QStringLiteral("language")).toString().trimmed();
            manifestDocument.insert(QStringLiteral("language"), language);
            manifestDocument.insert(QStringLiteral("role"), role);
            manifestDocument.insert(QStringLiteral("source_path"), document.value(QStringLiteral("path")).toString());
            manifestDocument.insert(QStringLiteral("exported_path"), relativePath);
            manifestDocument.insert(QStringLiteral("active"), id == activeId);
            manifestDocument.insert(QStringLiteral("chars"), text.length());
            manifestDocument.insert(QStringLiteral("lines"), text.isEmpty() ? 1 : text.count(QLatin1Char('\n')) + 1);
            manifestDocument.insert(QStringLiteral("sha256"), documentHash);
            manifestDocuments.append(manifestDocument);
            exportedCount += 1;
        }

        if (exportedCount <= 0) {
            result.insert(QStringLiteral("message"), QStringLiteral("no documents to export"));
            return result;
        }

        if (!writeUtf8File(packetDir.absoluteFilePath(QStringLiteral("all_documents.txt")), combined)) {
            result.insert(QStringLiteral("message"), QStringLiteral("combined export failed"));
            return result;
        }
        packetFiles.append(packetFileRecord(QStringLiteral("all_documents.txt"), sha256Hex(combined.toUtf8()), combined.toUtf8().size()));

        if (dexHandoff) {
            QVariantMap promptDocument;
            if (!pendingPromptDocuments.isEmpty()) {
                promptDocument = pendingPromptDocuments.first().toMap();
            } else {
                for (const QVariant &entry : documents) {
                    QVariantMap candidate = entry.toMap();
                    if (candidate.value(QStringLiteral("id")).toString() == activeId
                            && normalizedDocumentRole(candidate.value(QStringLiteral("role")).toString()) != QStringLiteral("scratch")) {
                        promptDocument = candidate;
                        break;
                    }
                }
            }
            if (promptDocument.isEmpty() && !exportedHandoffDocuments.isEmpty()) {
                promptDocument = exportedHandoffDocuments.first().toMap();
            }
            if (promptDocument.isEmpty()) {
                promptDocumentId = QStringLiteral("none");
                promptDocumentName = QStringLiteral("No prompt document");
                promptText = QStringLiteral("No non-scratch prompt document was exported.");
            } else {
                promptDocumentId = promptDocument.value(QStringLiteral("id")).toString();
                promptDocumentName = promptDocument.value(QStringLiteral("name")).toString();
                promptText = promptDocument.value(QStringLiteral("text")).toString();
            }

            QString promptFile;
            promptFile += QStringLiteral("# Active Draftsman Prompt\n\n");
            promptFile += QStringLiteral("Document: ") + promptDocumentName + QStringLiteral("\n");
            promptFile += QStringLiteral("Document id: ") + promptDocumentId + QStringLiteral("\n\n");
            promptFile += promptText;
            if (!promptFile.endsWith(QLatin1Char('\n'))) {
                promptFile += QLatin1Char('\n');
            }

            QString contextFile;
            contextFile += QStringLiteral("# Draftsman Context Documents\n\n");
            contextFile += QStringLiteral("Context documents: ") + QString::number(contextDocumentCount) + QStringLiteral("\n\n");
            contextFile += contextText.isEmpty() ? QStringLiteral("No additional context documents were exported.\n") : contextText;

            QString agentReadme;
            agentReadme += QStringLiteral("Draftsman Dex handoff packet\n");
            agentReadme += QStringLiteral("Created: ") + QDateTime::currentDateTimeUtc().toString(Qt::ISODate) + QStringLiteral("\n\n");
            agentReadme += QStringLiteral("Read order:\n");
            agentReadme += QStringLiteral("1. manifest.json\n");
            agentReadme += QStringLiteral("2. prompt.txt\n");
            agentReadme += QStringLiteral("3. context.txt\n");
            agentReadme += QStringLiteral("4. documents/ for exact source copies\n\n");
            agentReadme += QStringLiteral("Do not assume this packet syncs back to Draftsman. Return changes as a new packet, patch, or explicit instructions.\n");

            if (!writeUtf8File(packetDir.absoluteFilePath(QStringLiteral("prompt.txt")), promptFile)
                    || !writeUtf8File(packetDir.absoluteFilePath(QStringLiteral("context.txt")), contextFile)
                    || !writeUtf8File(packetDir.absoluteFilePath(QStringLiteral("AGENT_README.txt")), agentReadme)) {
                result.insert(QStringLiteral("message"), QStringLiteral("handoff export failed"));
                return result;
            }
            packetFiles.append(packetFileRecord(QStringLiteral("prompt.txt"), sha256Hex(promptFile.toUtf8()), promptFile.toUtf8().size()));
            packetFiles.append(packetFileRecord(QStringLiteral("context.txt"), sha256Hex(contextFile.toUtf8()), contextFile.toUtf8().size()));
            packetFiles.append(packetFileRecord(QStringLiteral("AGENT_README.txt"), sha256Hex(agentReadme.toUtf8()), agentReadme.toUtf8().size()));
        }

        QJsonObject manifest;
        manifest.insert(QStringLiteral("schema_version"), 1);
        manifest.insert(QStringLiteral("packet_type"), packetType);
        manifest.insert(QStringLiteral("created_at_utc"), QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
        manifest.insert(QStringLiteral("app"), QStringLiteral("Draftsman"));
        manifest.insert(QStringLiteral("profile_id"), metadata.value(QStringLiteral("profile_id")).toString());
        manifest.insert(QStringLiteral("profile_label"), metadata.value(QStringLiteral("profile_label")).toString());
        manifest.insert(QStringLiteral("active_document_id"), activeId);
        manifest.insert(QStringLiteral("documents"), manifestDocuments);
        manifest.insert(QStringLiteral("files"), packetFiles);
        if (dexHandoff) {
            QJsonObject handoffFiles;
            handoffFiles.insert(QStringLiteral("agent_readme"), QStringLiteral("AGENT_README.txt"));
            handoffFiles.insert(QStringLiteral("prompt"), QStringLiteral("prompt.txt"));
            handoffFiles.insert(QStringLiteral("context"), QStringLiteral("context.txt"));
            handoffFiles.insert(QStringLiteral("combined"), QStringLiteral("all_documents.txt"));
            manifest.insert(QStringLiteral("handoff_files"), handoffFiles);
        }

        QSaveFile manifestFile(packetDir.absoluteFilePath(QStringLiteral("manifest.json")));
        if (!manifestFile.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
            result.insert(QStringLiteral("message"), QStringLiteral("manifest export failed"));
            return result;
        }
        manifestFile.write(QJsonDocument(manifest).toJson(QJsonDocument::Indented));
        if (!manifestFile.commit()) {
            result.insert(QStringLiteral("message"), QStringLiteral("manifest export commit failed"));
            return result;
        }
        const QByteArray manifestBytes = QJsonDocument(manifest).toJson(QJsonDocument::Indented);
        packetFiles.append(packetFileRecord(QStringLiteral("manifest.json"), sha256Hex(manifestBytes), manifestBytes.size()));

        QString readme;
        readme += QStringLiteral("Draftsman text editor export bundle\n");
        readme += QStringLiteral("Created: ") + manifest.value(QStringLiteral("created_at_utc")).toString() + QStringLiteral("\n");
        readme += QStringLiteral("Documents: ") + QString::number(exportedCount) + QStringLiteral("\n\n");
        readme += QStringLiteral("Read manifest.json first. Document bodies are in documents/. all_documents.txt is a combined plain-text copy.\n");
        readme += QStringLiteral("Use index.txt for a quick packet file list with SHA-256 checksums.\n");
        if (writeUtf8File(packetDir.absoluteFilePath(QStringLiteral("README.txt")), readme)) {
            packetFiles.append(packetFileRecord(QStringLiteral("README.txt"), sha256Hex(readme.toUtf8()), readme.toUtf8().size()));
        }

        QString indexText;
        indexText += QStringLiteral("Draftsman packet index\n");
        indexText += QStringLiteral("Packet: ") + packetName + QStringLiteral("\n");
        indexText += QStringLiteral("Created: ") + manifest.value(QStringLiteral("created_at_utc")).toString() + QStringLiteral("\n\n");
        for (const QJsonValue &value : packetFiles) {
            const QJsonObject file = value.toObject();
            indexText += file.value(QStringLiteral("sha256")).toString()
                + QStringLiteral("  ")
                + file.value(QStringLiteral("path")).toString()
                + QStringLiteral("\n");
        }
        if (!writeUtf8File(packetDir.absoluteFilePath(QStringLiteral("index.txt")), indexText)) {
            result.insert(QStringLiteral("message"), QStringLiteral("index export failed"));
            return result;
        }

        result.insert(QStringLiteral("ok"), true);
        result.insert(QStringLiteral("message"), dexHandoff ? QStringLiteral("exported dex handoff") : QStringLiteral("exported bundle"));
        result.insert(QStringLiteral("path"), packetPath);
        result.insert(QStringLiteral("documents"), exportedCount);
        return result;
    }

    QVariantList load() const {
        QVariantList documents;
        if (m_manifestPath.isEmpty()) {
            return documents;
        }

        QFile manifestFile(m_manifestPath);
        if (!manifestFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            return documents;
        }

        const QJsonDocument manifest = QJsonDocument::fromJson(manifestFile.readAll());
        if (!manifest.isObject()) {
            return documents;
        }

        const QJsonArray entries = manifest.object().value(QStringLiteral("documents")).toArray();
        const QFileInfo manifestInfo(m_manifestPath);
        const QDir manifestDir(manifestInfo.absoluteDir());
        for (const QJsonValue &value : entries) {
            const QJsonObject item = value.toObject();
            const QString cleanPath = cleanRelativePath(item.value(QStringLiteral("path")).toString());
            if (cleanPath.isEmpty()) {
                continue;
            }

            QString text;
            QFile textFile(manifestDir.absoluteFilePath(cleanPath));
            if (textFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
                text = QString::fromUtf8(textFile.readAll());
            }

            QVariantMap document;
            document.insert(QStringLiteral("id"), item.value(QStringLiteral("id")).toString());
            document.insert(QStringLiteral("name"), item.value(QStringLiteral("name")).toString(QStringLiteral("untitled.txt")));
            document.insert(QStringLiteral("language"), item.value(QStringLiteral("language")).toString(QStringLiteral("text")));
            document.insert(QStringLiteral("path"), cleanPath);
            document.insert(QStringLiteral("role"), item.value(QStringLiteral("role")).toString());
            document.insert(QStringLiteral("initialText"), text);
            document.insert(QStringLiteral("text"), text);
            document.insert(QStringLiteral("cursorPosition"), 0);
            document.insert(QStringLiteral("selectionStart"), 0);
            document.insert(QStringLiteral("selectionEnd"), 0);
            if (!document.value(QStringLiteral("id")).toString().isEmpty()) {
                documents.append(document);
            }
        }
        return documents;
    }

    Q_INVOKABLE QVariantMap loadState() const {
        QVariantMap state;
        if (m_manifestPath.isEmpty()) {
            return state;
        }

        QFile manifestFile(m_manifestPath);
        if (!manifestFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            return state;
        }

        const QJsonDocument manifest = QJsonDocument::fromJson(manifestFile.readAll());
        if (!manifest.isObject()) {
            return state;
        }

        const QJsonObject editorState = manifest.object().value(QStringLiteral("editor_state")).toObject();
        if (editorState.isEmpty()) {
            return state;
        }

        state.insert(QStringLiteral("active_document_id"), editorState.value(QStringLiteral("active_document_id")).toString());
        state.insert(QStringLiteral("split_enabled"), editorState.value(QStringLiteral("split_enabled")).toBool());
        state.insert(QStringLiteral("secondary_document_id"), editorState.value(QStringLiteral("secondary_document_id")).toString());
        state.insert(QStringLiteral("wrap_enabled"), editorState.value(QStringLiteral("wrap_enabled")).toBool());
        state.insert(QStringLiteral("line_numbers_visible"), editorState.value(QStringLiteral("line_numbers_visible")).toBool());

        return state;
    }

    Q_INVOKABLE bool saveState(const QVariantMap &editorState) const {
        if (m_manifestPath.isEmpty()) {
            return false;
        }

        const QFileInfo manifestInfo(m_manifestPath);
        QDir manifestDir(manifestInfo.absoluteDir());
        if (!manifestDir.exists() && !QDir().mkpath(manifestDir.absolutePath())) {
            return false;
        }

        QJsonObject manifest;
        QFile manifestFileRead(m_manifestPath);
        if (manifestFileRead.open(QIODevice::ReadOnly | QIODevice::Text)) {
            const QJsonDocument existing = QJsonDocument::fromJson(manifestFileRead.readAll());
            if (existing.isObject()) {
                manifest = existing.object();
            }
        }

        manifest.insert(QStringLiteral("schema_version"), 1);
        manifest.insert(QStringLiteral("editor_state"), QJsonObject::fromVariantMap(normalizeEditorState(editorState)));

        QSaveFile manifestFile(m_manifestPath);
        if (!manifestFile.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
            return false;
        }
        manifestFile.write(QJsonDocument(manifest).toJson(QJsonDocument::Indented));
        return manifestFile.commit();
    }

private:
    static QString cleanRelativePath(const QString &path) {
        if (path.trimmed().isEmpty() || QFileInfo(path).isAbsolute()) {
            return QString();
        }
        const QString clean = QDir::cleanPath(path);
        if (clean == QStringLiteral(".") || clean == QStringLiteral("..") || clean.startsWith(QStringLiteral("../"))) {
            return QString();
        }
        return clean;
    }

    static QString safeFileStem(const QString &text) {
        QString result;
        for (const QChar character : text) {
            if (character.isLetterOrNumber() || character == QLatin1Char('-') || character == QLatin1Char('_')) {
                result.append(character);
            }
        }
        return result.isEmpty() ? QStringLiteral("untitled") : result;
    }

    static QString safeExportFileName(const QString &name, const QString &fallbackId) {
        QString base = QFileInfo(name).completeBaseName();
        QString suffix = QFileInfo(name).suffix();
        if (base.trimmed().isEmpty()) {
            base = fallbackId;
        }
        if (suffix.trimmed().isEmpty()) {
            suffix = QStringLiteral("txt");
        }
        const QString safeId = safeFileStem(fallbackId);
        return safeFileStem(base) + QStringLiteral("_") + safeId + QStringLiteral(".") + safeFileStem(suffix);
    }

    static QString normalizedDocumentRole(const QString &role) {
        const QString clean = role.trimmed().toLower();
        if (clean == QStringLiteral("prompt")
                || clean == QStringLiteral("context")
                || clean == QStringLiteral("reference")
                || clean == QStringLiteral("scratch")
                || clean == QStringLiteral("output")) {
            return clean;
        }
        return QStringLiteral("context");
    }

    static bool writeUtf8File(const QString &path, const QString &text) {
        QSaveFile file(path);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
            return false;
        }
        file.write(text.toUtf8());
        return file.commit();
    }

    static QString sha256Hex(const QByteArray &data) {
        return QString::fromLatin1(QCryptographicHash::hash(data, QCryptographicHash::Sha256).toHex());
    }

    static QJsonObject packetFileRecord(const QString &path, const QString &sha256, qsizetype bytes) {
        QJsonObject record;
        record.insert(QStringLiteral("path"), path);
        record.insert(QStringLiteral("sha256"), sha256);
        record.insert(QStringLiteral("bytes"), static_cast<double>(bytes));
        return record;
    }

    static QVariantMap normalizeEditorState(const QVariantMap &source) {
        QVariantMap state;
        state.insert(QStringLiteral("active_document_id"), source.value(QStringLiteral("active_document_id")).toString().trimmed());
        state.insert(QStringLiteral("split_enabled"), source.value(QStringLiteral("split_enabled")).toBool());
        state.insert(QStringLiteral("secondary_document_id"), source.value(QStringLiteral("secondary_document_id")).toString().trimmed());
        state.insert(QStringLiteral("wrap_enabled"), source.value(QStringLiteral("wrap_enabled")).toBool());
        state.insert(QStringLiteral("line_numbers_visible"), source.value(QStringLiteral("line_numbers_visible")).toBool());
        return state;
    }

    QString m_manifestPath;
};

int main(int argc, char *argv[]) {
    if (qEnvironmentVariableIsEmpty("QT_QUICK_CONTROLS_STYLE")) {
        qputenv("QT_QUICK_CONTROLS_STYLE", "Basic");
    }

    QGuiApplication app(argc, argv);
    app.setOrganizationName("KOGA-ryu");
    app.setApplicationName("Qt QML Draftsman Shell");

    QCommandLineParser parser;
    parser.setApplicationDescription("Editable Draftsman Qt/QML shell");
    parser.addHelpOption();
    const QCommandLineOption screenshotOption(
        QStringList() << "s" << "screenshot",
        "Save a screenshot of the app window to <path>, then quit.",
        "path");
    const QCommandLineOption routeOption(
        QStringList() << "route",
        "Select a review route before screenshot capture.",
        "route_id");
    const QCommandLineOption widthOption(
        QStringList() << "width",
        "Set the app window width before capture.",
        "pixels");
    const QCommandLineOption heightOption(
        QStringList() << "height",
        "Set the app window height before capture.",
        "pixels");
    const QCommandLineOption tabOption(
        QStringList() << "tab",
        "Select a local review tab before screenshot capture.",
        "tab_name");
    const QCommandLineOption noteOption(
        QStringList() << "note",
        "Add one in-memory note before screenshot capture.",
        "text");
    const QCommandLineOption noteStatusOption(
        QStringList() << "note-status",
        "Status to use with --note.",
        "status");
    const QCommandLineOption reviewSubjectOption(
        QStringList() << "review-subject",
        "Load review subject JSON from <path>.",
        "path");
    const QCommandLineOption themeOption(
        QStringList() << "theme",
        "Load UI theme JSON from <path>.",
        "path");
    const QCommandLineOption projectProfileOption(
        QStringList() << "project-profile",
        "Load project profile JSON from <path>.",
        "path");
    const QCommandLineOption shellLayoutOption(
        QStringList() << "shell-layout",
        "Load shell layout JSON from <path>.",
        "path");
    const QCommandLineOption activityOption(
        QStringList() << "activity",
        "Select an activity mode before screenshot capture.",
        "mode_id");
    const QCommandLineOption settingsPageOption(
        QStringList() << "settings-page",
        "Select a settings page before screenshot capture.",
        "page_id");
    const QCommandLineOption actionOption(
        QStringList() << "action",
        "Run a profile custom action after startup.",
        "action_id");
    parser.addOption(screenshotOption);
    parser.addOption(routeOption);
    parser.addOption(widthOption);
    parser.addOption(heightOption);
    parser.addOption(tabOption);
    parser.addOption(noteOption);
    parser.addOption(noteStatusOption);
    parser.addOption(reviewSubjectOption);
    parser.addOption(themeOption);
    parser.addOption(projectProfileOption);
    parser.addOption(shellLayoutOption);
    parser.addOption(activityOption);
    parser.addOption(settingsPageOption);
    parser.addOption(actionOption);
    parser.process(app);

    auto absolutePath = [](const QString &path) {
        if (path.isEmpty()) {
            return QString();
        }
        if (QFileInfo(path).isRelative()) {
            return QDir::current().absoluteFilePath(path);
        }
        return path;
    };

    auto projectSourcePath = [](const QString &path) {
        if (path.isEmpty()) {
            return QString();
        }
        if (QFileInfo(path).isRelative()) {
            return QDir(QStringLiteral(PROJECT_SOURCE_DIR)).absoluteFilePath(path);
        }
        return path;
    };

    auto loadJsonObject = [](const QString &path) {
        QVariant result = QVariantMap();
        QFile file(path);
        if (file.open(QIODevice::ReadOnly)) {
            const QJsonDocument document = QJsonDocument::fromJson(file.readAll());
            if (document.isObject()) {
                result = document.object().toVariantMap();
            }
        }
        return result;
    };

    auto loadTextFile = [](const QString &path) {
        QFile file(path);
        if (!path.isEmpty() && file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            return QString::fromUtf8(file.readAll());
        }
        return QString();
    };

    QString projectProfilePath = parser.isSet(projectProfileOption)
        ? parser.value(projectProfileOption)
        : QStringLiteral(PROJECT_SOURCE_DIR) + QStringLiteral("/data/project_profiles/draftsman_blank.json");
    projectProfilePath = absolutePath(projectProfilePath);
    const QVariant projectProfile = loadJsonObject(projectProfilePath);
    const QVariantMap projectProfileMap = projectProfile.toMap();
    const QVariantMap dataSources = projectProfileMap.value(QStringLiteral("data_sources")).toMap();
    const QString profileReviewSubjectPath = dataSources.value(QStringLiteral("review_subject")).toString().trimmed();

    QString reviewSubjectPath;
    QVariant reviewSubject = QVariantMap();
    if (parser.isSet(reviewSubjectOption)) {
        reviewSubjectPath = absolutePath(parser.value(reviewSubjectOption));
        reviewSubject = loadJsonObject(reviewSubjectPath);
    } else if (!profileReviewSubjectPath.isEmpty()) {
        reviewSubjectPath = projectSourcePath(profileReviewSubjectPath);
        reviewSubject = loadJsonObject(reviewSubjectPath);
    }

    const QString mapCsvPath = projectSourcePath(dataSources.value(QStringLiteral("map_csv")).toString().trimmed());
    const QString cellMetadataPath = projectSourcePath(dataSources.value(QStringLiteral("cell_metadata")).toString().trimmed());
    const QString mapCsvText = loadTextFile(mapCsvPath);
    const QString cellMetadataText = loadTextFile(cellMetadataPath);
    const QString textEditorManifestPath = projectSourcePath(dataSources.value(QStringLiteral("text_documents")).toString().trimmed());
    TextEditorStore textEditorStore(textEditorManifestPath);
    const QVariantList textEditorDocuments = textEditorStore.load();
    const QVariantMap textEditorState = textEditorStore.loadState();
    const QString drawingToolRegistryPath = QStringLiteral(PROJECT_SOURCE_DIR)
        + QStringLiteral("/data/features/drawing_tool/tool_registry.json");
    const QVariant drawingToolRegistry = loadJsonObject(drawingToolRegistryPath);
    DrawingDocumentController drawingController;

    QString themePath = parser.isSet(themeOption)
        ? parser.value(themeOption)
        : QStringLiteral(PROJECT_SOURCE_DIR) + QStringLiteral("/data/ui_theme.json");
    themePath = absolutePath(themePath);
    const QVariant uiTheme = loadJsonObject(themePath);

    QString shellLayoutPath = parser.isSet(shellLayoutOption)
        ? parser.value(shellLayoutOption)
        : QStringLiteral(PROJECT_SOURCE_DIR) + QStringLiteral("/data/shell_layout.json");
    shellLayoutPath = absolutePath(shellLayoutPath);
    const QVariant shellLayout = loadJsonObject(shellLayoutPath);
    ShellLayoutStore shellLayoutStore(shellLayoutPath);
    DrawingDocumentStore drawingDocumentStore;

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty(QStringLiteral("initialReviewSubject"), reviewSubject);
    engine.rootContext()->setContextProperty(QStringLiteral("initialReviewSubjectPath"), reviewSubjectPath);
    engine.rootContext()->setContextProperty(QStringLiteral("initialUiTheme"), uiTheme);
    engine.rootContext()->setContextProperty(QStringLiteral("initialUiThemePath"), themePath);
    engine.rootContext()->setContextProperty(QStringLiteral("initialProjectProfile"), projectProfile);
    engine.rootContext()->setContextProperty(QStringLiteral("initialProjectProfilePath"), projectProfilePath);
    engine.rootContext()->setContextProperty(QStringLiteral("initialMapCsvPath"), mapCsvPath);
    engine.rootContext()->setContextProperty(QStringLiteral("initialMapCsvText"), mapCsvText);
    engine.rootContext()->setContextProperty(QStringLiteral("initialCellMetadataPath"), cellMetadataPath);
    engine.rootContext()->setContextProperty(QStringLiteral("initialCellMetadataText"), cellMetadataText);
    engine.rootContext()->setContextProperty(QStringLiteral("initialTextEditorDocuments"), textEditorDocuments);
    engine.rootContext()->setContextProperty(QStringLiteral("initialTextEditorState"), textEditorState);
    engine.rootContext()->setContextProperty(QStringLiteral("initialTextEditorManifestPath"), textEditorManifestPath);
    engine.rootContext()->setContextProperty(QStringLiteral("initialDrawingToolRegistry"), drawingToolRegistry);
    engine.rootContext()->setContextProperty(QStringLiteral("initialDrawingToolRegistryPath"), drawingToolRegistryPath);
    engine.rootContext()->setContextProperty(QStringLiteral("initialDrawingModel"), drawingController.modelDocument());
    engine.rootContext()->setContextProperty(QStringLiteral("nativeDrawingController"), &drawingController);
    engine.rootContext()->setContextProperty(QStringLiteral("initialShellLayout"), shellLayout);
    engine.rootContext()->setContextProperty(QStringLiteral("initialShellLayoutPath"), shellLayoutPath);
    engine.rootContext()->setContextProperty(QStringLiteral("shellLayoutStore"), &shellLayoutStore);
    engine.rootContext()->setContextProperty(QStringLiteral("textEditorStore"), &textEditorStore);
    engine.rootContext()->setContextProperty(QStringLiteral("drawingDocumentStore"), &drawingDocumentStore);
    const QUrl mainUrl = QUrl::fromLocalFile(QStringLiteral(QML_SOURCE_DIR) + QStringLiteral("/Main.qml"));
    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);
    engine.load(mainUrl);

    if (engine.rootObjects().isEmpty()) {
        return -1;
    }

    auto *window = qobject_cast<QQuickWindow *>(engine.rootObjects().constFirst());
    if (window == nullptr) {
        return -1;
    }

    bool widthOk = false;
    bool heightOk = false;
    const int requestedWidth = parser.value(widthOption).toInt(&widthOk);
    const int requestedHeight = parser.value(heightOption).toInt(&heightOk);
    if (widthOk && requestedWidth > 0) {
        window->setWidth(requestedWidth);
    }
    if (heightOk && requestedHeight > 0) {
        window->setHeight(requestedHeight);
    }

    if (parser.isSet(routeOption)) {
        if (auto *runtime = window->findChild<QObject *>(QStringLiteral("runtimeController"))) {
            QMetaObject::invokeMethod(
                runtime,
                "selectRoute",
                Q_ARG(QVariant, QVariant(parser.value(routeOption))));
        }
    }

    if (auto *runtime = window->findChild<QObject *>(QStringLiteral("runtimeController"))) {
        if (parser.isSet(activityOption)) {
            QMetaObject::invokeMethod(
                runtime,
                "setActivityMode",
                Q_ARG(QVariant, QVariant(parser.value(activityOption))));
        }

        if (parser.isSet(settingsPageOption)) {
            QMetaObject::invokeMethod(
                runtime,
                "setSettingsPage",
                Q_ARG(QVariant, QVariant(parser.value(settingsPageOption))));
        }

        if (parser.isSet(noteOption)) {
            const QString routeId = parser.isSet(routeOption)
                ? parser.value(routeOption)
                : runtime->property("rootRouteId").toString();
            const QString status = parser.isSet(noteStatusOption)
                ? parser.value(noteStatusOption)
                : QStringLiteral("pending");
            QMetaObject::invokeMethod(
                runtime,
                "addNote",
                Q_ARG(QVariant, QVariant(routeId)),
                Q_ARG(QVariant, QVariant(status)),
                Q_ARG(QVariant, QVariant(parser.value(noteOption))));
        }

        if (parser.isSet(tabOption)) {
            QMetaObject::invokeMethod(
                runtime,
                "setLocalTab",
                Q_ARG(QVariant, QVariant(parser.value(tabOption))));
        }

        if (parser.isSet(actionOption)) {
            QMetaObject::invokeMethod(
                runtime,
                "runCustomAction",
                Q_ARG(QVariant, QVariant(parser.value(actionOption))));
        }
    }

    if (parser.isSet(screenshotOption)) {
        const QString path = parser.value(screenshotOption);
        QTimer::singleShot(700, window, [window, path]() {
            const QFileInfo info(path);
            if (!info.absoluteDir().exists()) {
                QDir().mkpath(info.absolutePath());
            }
            const QImage image = window->grabWindow();
            const bool saved = image.save(path);
            QCoreApplication::exit(saved ? 0 : 2);
        });
    }

    return app.exec();
}

#include "main.moc"
