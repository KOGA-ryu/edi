#pragma once

#include <QObject>
#include <QString>
#include <QVariantList>
#include <QVariantMap>
#include <QVector>

#include "drafting/DraftingDocument.h"
#include "drafting/DraftingGrid.h"
#include "drafting/DraftingSnap.h"
#include "drafting/DraftingToolCreation.h"

#include <optional>

class DrawingDocumentController final : public QObject {
    Q_OBJECT

public:
    explicit DrawingDocumentController(QObject *parent = nullptr);

    QVariantMap modelDocument() const;
    QString selectedToolId() const;
    QString selectedObjectId() const;
    bool gridSnapEnabled() const;
    bool objectSnapEnabled() const;
    bool endpointSnapEnabled() const;
    bool vertexSnapEnabled() const;
    bool midpointSnapEnabled() const;
    bool centerSnapEnabled() const;
    bool objectSnapPriorityBeforeGrid() const;
    QString gridPresetId() const;
    QString objectSnapTolerancePresetId() const;
    void setSelectedToolId(const QString &toolId);
    void setGridSnapEnabled(bool enabled);
    void setObjectSnapEnabled(bool enabled);
    void setEndpointSnapEnabled(bool enabled);
    void setVertexSnapEnabled(bool enabled);
    void setMidpointSnapEnabled(bool enabled);
    void setCenterSnapEnabled(bool enabled);
    void setObjectSnapPriorityBeforeGrid(bool enabled);
    void setObjectSnapTolerancePreset(QString presetId);
    void setGridPresetId(const QString &presetId);
    void updatePointerNormalized(double x, double y);
    bool updateSelectedObjectGeometryField(const QString &fieldId, double value);
    bool nudgeSelection(const QString &direction, const QString &stepMode);
    bool offsetSelectedObject(const QString &sideId);
    bool mirrorSelectedObject(const QString &axisId);
    bool repeatSelectedObject(const QString &axisId);
    bool alignSelection(const QString &modeId);
    bool distributeSelection(const QString &axisId);
    void clickCanvasNormalized(double x, double y);
    void updateCreationPreviewNormalized(double x, double y);
    bool editSelectedHandleNormalized(const QString &handleId, double x, double y);
    bool moveSelectionNormalized(double dx, double dy);
    bool selectObjectsInBoundsNormalized(double x1, double y1, double x2, double y2);

signals:
    void modelChanged();

private:
    QString m_selectedToolId = QStringLiteral("select_move");
    edi::drafting::DraftingDocument m_document;
    edi::drafting::DraftingGridSettings m_gridSettings;
    edi::drafting::DraftingSnapSettings m_snapSettings;
    std::optional<edi::drafting::Point2D> m_pointerRawPoint;
    std::optional<edi::drafting::DraftingToolCreationRequest> m_pendingCreation;
    std::optional<edi::drafting::DraftingObject> m_previewObject;
    int m_nextObjectSerial = 1;
};
