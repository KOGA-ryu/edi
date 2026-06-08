#pragma once

#include <QObject>
#include <QString>
#include <QVariantList>
#include <QVariantMap>
#include <QVector>

#include "drafting/DraftingDocument.h"
#include "drafting/DraftingSnap.h"

class DrawingDocumentController final : public QObject {
    Q_OBJECT

public:
    explicit DrawingDocumentController(QObject *parent = nullptr);

    QVariantMap modelDocument() const;
    QString selectedToolId() const;
    QString selectedObjectId() const;
    bool gridSnapEnabled() const;
    bool objectSnapEnabled() const;
    void setSelectedToolId(const QString &toolId);
    void setGridSnapEnabled(bool enabled);
    void setObjectSnapEnabled(bool enabled);
    void clickCanvasNormalized(double x, double y);
    bool editSelectedHandleNormalized(const QString &handleId, double x, double y);
    bool moveSelectionNormalized(double dx, double dy);
    bool selectObjectsInBoundsNormalized(double x1, double y1, double x2, double y2);

signals:
    void modelChanged();

private:
    QString m_selectedToolId = QStringLiteral("select_move");
    edi::drafting::DraftingDocument m_document;
    edi::drafting::DraftingSnapSettings m_snapSettings;
    bool m_hasPendingPoint = false;
    double m_pendingX = 0.0;
    double m_pendingY = 0.0;
    int m_nextObjectSerial = 1;
};
