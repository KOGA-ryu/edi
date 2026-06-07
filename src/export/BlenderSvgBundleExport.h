#pragma once

#include <QJsonObject>
#include <QString>

namespace BlenderSvgBundleExport {
QJsonObject manifest(const QString &bundlePath, const QJsonObject &model);
QJsonObject exportReport(const QString &bundlePath,
                        const QString &svgPath,
                        const QString &manifestPath,
                        const QString &scriptPath,
                        const QString &readmePath,
                        const QString &reportPath,
                        const QString &reportTextPath,
                        const QString &verifyPath,
                        const QJsonObject &manifest);
QString exportReportText(const QJsonObject &report);
}  // namespace BlenderSvgBundleExport
