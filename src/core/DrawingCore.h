#pragma once

#include <QObject>
#include <QString>
#include <QVariantList>
#include <QVariantMap>
#include <QVector>

#include "drafting/DraftingDocument.h"
#include "drafting/DraftingCalibration.h"
#include "drafting/DraftingGrid.h"
#include "drafting/DraftingPlotPlan.h"
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
    QString activeLayerId() const;
    bool gridSnapEnabled() const;
    bool objectSnapEnabled() const;
    bool endpointSnapEnabled() const;
    bool vertexSnapEnabled() const;
    bool midpointSnapEnabled() const;
    bool centerSnapEnabled() const;
    bool objectSnapPriorityBeforeGrid() const;
    QString gridPresetId() const;
    QString objectSnapTolerancePresetId() const;
    QString plotOrderModeId() const;
    QString plotDirectionModeId() const;
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
    void setGridUnitId(const QString &unitId);
    void setGridSize(double width, double height);
    void setGridMargins(double left, double top, double right, double bottom);
    void setGridStep(double minorStep);
    void setGridMajorLineEvery(int majorLineEvery);
    void setGridVisible(bool visible);
    void setPlotOrderModeId(const QString &modeId);
    void setPlotDirectionModeId(const QString &modeId);
    void updatePointerNormalized(double x, double y);
    bool updateSelectedObjectGeometryField(const QString &fieldId, double value);
    bool updateSelectedObjectPhysicalGeometryField(const QString &fieldId, double value);
    bool setSelectedObjectLocked(bool locked);
    bool setSelectedObjectVisible(bool visible);
    bool setDefaultLayerLocked(bool locked);
    bool setDefaultLayerVisible(bool visible);
    bool setActiveLayerLocked(bool locked);
    bool setActiveLayerVisible(bool visible);
    bool setActiveLayerPlotEnabled(bool enabled);
    bool setActiveLayerPenPreset(const QString &presetId);
    bool setActiveLayerStrokeWidthPreset(const QString &presetId);
    bool createLayer();
    bool renameActiveLayer(const QString &name);
    bool setActiveLayerId(const QString &layerId);
    bool moveActiveLayer(const QString &direction);
    bool moveSelectedObjectToLayer(const QString &layerId);
    bool nudgeSelection(const QString &direction, const QString &stepMode);
    bool nudgeSelectionInsideDrawable(const QString &direction, const QString &stepMode);
    bool offsetSelectedObject(const QString &sideId);
    bool mirrorSelectedObject(const QString &axisId);
    bool repeatSelectedObject(const QString &axisId);
    bool alignSelection(const QString &modeId);
    bool distributeSelection(const QString &axisId);
    bool createCalibrationPattern(const QString &patternId);
    bool recordCalibrationMeasurement(double measuredValue);
    bool applyCalibrationCorrection();
    bool fitSelectionToDrawableBounds();
    bool centerSelectionInDrawable();
    bool moveSelectionToDrawableOrigin();
    bool moveSelectedGuideToDrawableOrigin();
    bool centerSelectedGuideInDrawable();
    bool fitSelectedConstructionLineToDrawable();
    bool createGuideFromSelectedBounds(const QString &placementId);
    bool deleteSelectedGuide();
    bool deleteAllGuides();
    bool setAllGuidesVisible(bool visible);
    bool setAllGuidesLocked(bool locked);
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
    edi::drafting::DraftingPlotSettings m_plotSettings;
    std::optional<edi::drafting::DraftingCalibrationMeasurement> m_latestCalibrationMeasurement;
    std::optional<edi::drafting::DraftingCalibrationCorrectionPlan> m_pendingCalibrationCorrection;
    std::optional<edi::drafting::Point2D> m_pointerRawPoint;
    std::optional<edi::drafting::DraftingToolCreationRequest> m_pendingCreation;
    std::optional<edi::drafting::DraftingObject> m_previewObject;
    int m_nextObjectSerial = 1;
};
