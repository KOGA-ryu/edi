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

struct InventoryFamilySummary {
    QString category;
    QString dataFamily;
    QString proposedTargetFormat;
    QString migrationPriority;
    int fileCount = 0;
    qint64 sizeBytes = 0;
    QStringList samplePaths;
};

QString normalizedInventoryPath(const QString &path);
QString extensionForPath(const QString &path);
InventoryRow classifyInventoryPath(const QString &path, qint64 sizeBytes = 0);
QVector<InventoryRow> inventoryRepoJsonFiles(const QString &repoRoot);
bool inventoryRowMatchesFilter(const InventoryRow &row, const InventoryFilter &filter);
QVector<InventoryRow> filterInventoryRows(const QVector<InventoryRow> &rows, const InventoryFilter &filter);
QVector<InventoryFamilySummary> inventoryFamilySummaries(const QVector<InventoryRow> &rows, int sampleLimit = 3);
int inventoryUnknownCount(const QVector<InventoryRow> &rows);
int inventoryBlockedCount(const QVector<InventoryRow> &rows);
QString inventoryRowHeader();
QString inventoryRowLine(const InventoryRow &row);
QString inventoryFamilySummaryHeader();
QString inventoryFamilySummaryLine(const InventoryFamilySummary &summary);
QString inventoryFamilySummaryReport(const QVector<InventoryRow> &rows, int sampleLimit = 3);
QString inventorySummary(const QVector<InventoryRow> &rows);

} // namespace edi::formats
