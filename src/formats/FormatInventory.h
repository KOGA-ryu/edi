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

struct InventoryFilter {
    QStringList categories;
    QStringList dataFamilies;
    QStringList targetFormats;
    QStringList priorities;
};

QString normalizedInventoryPath(const QString &path);
QString extensionForPath(const QString &path);
InventoryRow classifyInventoryPath(const QString &path, qint64 sizeBytes = 0);
QVector<InventoryRow> inventoryRepoJsonFiles(const QString &repoRoot);
bool inventoryRowMatchesFilter(const InventoryRow &row, const InventoryFilter &filter);
QVector<InventoryRow> filterInventoryRows(const QVector<InventoryRow> &rows, const InventoryFilter &filter);
int inventoryUnknownCount(const QVector<InventoryRow> &rows);
int inventoryBlockedCount(const QVector<InventoryRow> &rows);
QString inventoryRowHeader();
QString inventoryRowLine(const InventoryRow &row);
QString inventorySummary(const QVector<InventoryRow> &rows);

} // namespace edi::formats
