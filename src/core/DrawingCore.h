#pragma once

#include <QObject>
#include <QString>
#include <QVariantList>
#include <QVariantMap>
#include <QVector>

#include "drafting/DraftingDocument.h"

class DrawingDocumentController final : public QObject {
    Q_OBJECT

public:
    explicit DrawingDocumentController(QObject *parent = nullptr);

    QVariantMap modelDocument() const;
    QString selectedToolId() const;
    QString selectedObjectId() const;
    void setSelectedToolId(const QString &toolId);
    void clickCanvasNormalized(double x, double y);

signals:
    void modelChanged();

private:
    QString m_selectedToolId = QStringLiteral("select_move");
    edi::drafting::DraftingDocument m_document;
    bool m_hasPendingPoint = false;
    double m_pendingX = 0.0;
    double m_pendingY = 0.0;
    int m_nextObjectSerial = 1;
};
