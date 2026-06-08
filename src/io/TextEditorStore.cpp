#include "TextEditorStore.h"

#include <QCryptographicHash>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSaveFile>
#include <QtGlobal>

#include <QStringList>
#include <utility>

TextEditorStore::TextEditorStore(QString manifestPath, QObject *parent)
    : QObject(parent),
      m_manifestPath(std::move(manifestPath)) {}

QVariantMap TextEditorStore::save(const QVariantList &documents, const QString &activeId, bool saveAll,
                                 const QVariantMap &editorState) const {
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

    }

    QSaveFile manifestFile(m_manifestPath);
    if (!manifestFile.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        result.insert(QStringLiteral("message"), QStringLiteral("manifest write failed"));
        return result;
    }

    result.insert(QStringLiteral("ok"), true);
    result.insert(QStringLiteral("message"), saveAll ? QStringLiteral("saved all") : QStringLiteral("saved active"));
    result.insert(QStringLiteral("documents"), savedDocuments);
    return result;
}

QString TextEditorStore::path() const {
    return m_manifestPath;
}

QVariantMap TextEditorStore::exportBundle(const QVariantList &documents, const QString &activeId, const QVariantMap &metadata) const {
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
}

QVariantList TextEditorStore::load() const {
    QVariantList documents;
    if (m_manifestPath.isEmpty()) {
        return documents;
    }

    QFile manifestFile(m_manifestPath);
    if (!manifestFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return documents;
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

QVariantMap TextEditorStore::loadState() const {
    QVariantMap state;
    if (m_manifestPath.isEmpty()) {
        return state;
    }

    QFile manifestFile(m_manifestPath);
    if (!manifestFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return state;
    }

    state.insert(QStringLiteral("active_document_id"), editorState.value(QStringLiteral("active_document_id")).toString());
    state.insert(QStringLiteral("split_enabled"), editorState.value(QStringLiteral("split_enabled")).toBool());
    state.insert(QStringLiteral("secondary_document_id"), editorState.value(QStringLiteral("secondary_document_id")).toString());
    state.insert(QStringLiteral("wrap_enabled"), editorState.value(QStringLiteral("wrap_enabled")).toBool());
    state.insert(QStringLiteral("line_numbers_visible"), editorState.value(QStringLiteral("line_numbers_visible")).toBool());

    return state;
}

bool TextEditorStore::saveState(const QVariantMap &editorState) const {
    if (m_manifestPath.isEmpty()) {
        return false;
    }

    const QFileInfo manifestInfo(m_manifestPath);
    QDir manifestDir(manifestInfo.absoluteDir());
    if (!manifestDir.exists() && !QDir().mkpath(manifestDir.absolutePath())) {
        return false;
    }

QString TextEditorStore::cleanRelativePath(const QString &path) {
    if (path.trimmed().isEmpty() || QFileInfo(path).isAbsolute()) {
        return QString();
    }
    const QString clean = QDir::cleanPath(path);
    if (clean == QStringLiteral(".") || clean == QStringLiteral("..") || clean.startsWith(QStringLiteral("../"))) {
        return QString();
    }
    return clean;
}

QString TextEditorStore::safeFileStem(const QString &text) {
    QString result;
    for (const QChar character : text) {
        if (character.isLetterOrNumber() || character == QLatin1Char('-') || character == QLatin1Char('_')) {
            result.append(character);
        }
    }
    return result.isEmpty() ? QStringLiteral("untitled") : result;
}

QString TextEditorStore::safeExportFileName(const QString &name, const QString &fallbackId) {
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

QString TextEditorStore::normalizedDocumentRole(const QString &role) {
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

bool TextEditorStore::writeUtf8File(const QString &path, const QString &text) {
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        return false;
    }
    file.write(text.toUtf8());
    return file.commit();
}

QString TextEditorStore::sha256Hex(const QByteArray &data) {
    return QString::fromLatin1(QCryptographicHash::hash(data, QCryptographicHash::Sha256).toHex());
}

QVariantMap TextEditorStore::normalizeEditorState(const QVariantMap &source) {
    QVariantMap state;
    state.insert(QStringLiteral("active_document_id"), source.value(QStringLiteral("active_document_id")).toString().trimmed());
    state.insert(QStringLiteral("split_enabled"), source.value(QStringLiteral("split_enabled")).toBool());
    state.insert(QStringLiteral("secondary_document_id"), source.value(QStringLiteral("secondary_document_id")).toString().trimmed());
    state.insert(QStringLiteral("wrap_enabled"), source.value(QStringLiteral("wrap_enabled")).toBool());
    state.insert(QStringLiteral("line_numbers_visible"), source.value(QStringLiteral("line_numbers_visible")).toBool());
    return state;
}
