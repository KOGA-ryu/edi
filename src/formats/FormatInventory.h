#pragma once

#include <QVector>
#include <QString>

namespace edi::formats {

struct InventoryRow {
    QString path;
    QString extension;
    qint64 sizeBytes = 0;
    QString category;
    QString dataFamily;
    QString proposedTargetFormat;
    QString migrationPriority;
    QString reason;
};

QString normalizedInventoryPath(const QString &path);
QString extensionForPath(const QString &path);
InventoryRow classifyInventoryPath(const QString &path, qint64 sizeBytes = 0);
QVector<InventoryRow> inventoryRepoJsonFiles(const QString &repoRoot);
QString inventoryRowHeader();
QString inventoryRowLine(const InventoryRow &row);
QString inventorySummary(const QVector<InventoryRow> &rows);

} // namespace edi::formats
