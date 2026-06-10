#pragma once

#include <QObject>
#include <QString>
#include <QUrl>
#include <QVariantList>
#include <QVariantMap>
#include <QVector>

#include "io/DrawingDocumentStore.h"

#include "drafting/DraftingDocument.h"
#include "drafting/DraftingCalibration.h"
#include "drafting/DraftingCommands.h"
#include "drafting/DraftingGrid.h"
#include "drafting/DraftingGuideOps.h"
#include "drafting/DraftingLayerOps.h"
#include "drafting/DraftingMetadata.h"
#include "drafting/DraftingNudgeOps.h"
#include "drafting/DraftingPhysicalEdit.h"
#include "drafting/DraftingPlotPlan.h"
#include "drafting/DraftingSnap.h"
#include "drafting/DraftingToolCreation.h"

#include <functional>
#include <optional>
#include <vector>

class DrawingDocumentController final : public QObject {
    Q_OBJECT

public:
    explicit DrawingDocumentController(QObject *parent = nullptr);

    QVariantMap modelDocument() const;
    bool saveDocument(const QUrl &url);
    bool openDocument(const QUrl &url);
    bool exportSvgDocument(const QUrl &url);
    bool exportHpglDocument(const QUrl &url);
    bool undo();
    bool redo();
    bool canUndo() const;
    bool canRedo() const;
    // Bracket a continuous gesture (object/handle drag) so its many incremental
    // commands collapse into a single undo step. Must be balanced; safe to leave
    // unbalanced (a later begin re-captures).
    void beginInteractiveEdit();
    void endInteractiveEdit();
    QString selectedToolId() const;
    QString selectedObjectId() const;
    QString activeLayerId() const;
    bool gridSnapEnabled() const;
    bool objectSnapEnabled() const;
    bool endpointSnapEnabled() const;
    bool vertexSnapEnabled() const;
    bool midpointSnapEnabled() const;
    bool centerSnapEnabled() const;
    bool guideSnapEnabled() const;
    bool guideMoveSnapEnabled() const;
    bool objectSnapPriorityBeforeGrid() const;
    QString gridPresetId() const;
    QString objectSnapTolerancePresetId() const;
    QString plotOrderModeId() const;
    QString plotDirectionModeId() const;
    void setSelectedToolId(const QString &toolId);
    void setPolygonSides(int sides);
    int polygonSides() const;
    void setGridSnapEnabled(bool enabled);
    void setObjectSnapEnabled(bool enabled);
    void setEndpointSnapEnabled(bool enabled);
    void setVertexSnapEnabled(bool enabled);
    void setMidpointSnapEnabled(bool enabled);
    void setCenterSnapEnabled(bool enabled);
    void setGuideSnapEnabled(bool enabled);
    void setGuideMoveSnapEnabled(bool enabled);
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
    bool setSelectedGuideLabel(const QString &label);
    bool setSelectedGuideColor(const QString &color);
    bool setSelectedGuideDashStyle(const QString &dashStyle);
    bool setSelectedGuideLabelVisible(bool visible);
    bool setSelectedDimensionKind(const QString &kindId);
    bool setSelectedDimensionLabelVisible(bool visible);
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
    bool moveSelectedGuideToDrawableMax();
    bool offsetSelectedGuide(const QString &direction, const QString &stepMode);
    bool fitSelectedConstructionLineToDrawable();
    bool createGuideFromSelectedBounds(const QString &placementId);
    bool createOffsetGuideFromSelectedBounds(const QString &placementId, const QString &stepMode);
    bool applyGuidePreset(const QString &presetId);
    bool alignSelectionToNearestGuide(const QString &modeId);
    bool deleteSelectedGuide();
    bool deleteAllGuides();
    bool mergeDuplicateGuides();
    bool setAllGuidesVisible(bool visible);
    bool setAllGuidesLocked(bool locked);
    void clickCanvasNormalized(double x, double y);
    void cancelPendingCreation();
    bool deleteSelectedObject();
    bool duplicateSelectedObject();
    void updateCreationPreviewNormalized(double x, double y);
    bool editSelectedHandleNormalized(const QString &handleId, double x, double y);
    bool moveSelectionNormalized(double dx, double dy);
    bool selectObjectsInBoundsNormalized(double x1, double y1, double x2, double y2);

signals:
    void modelChanged();

private:
    bool applyCommandAndEmit(const edi::drafting::DraftingCommand &command);
    // Undo snapshot bookkeeping. beginEdit() copies the document at the start of
    // a mutating action; commitEdit() (called right before emit, so canUndo() is
    // current when the view refreshes) pushes that snapshot onto the undo stack
    // unless the change was pure selection. Idempotent within one action.
    void beginEdit();
    void commitEdit();
    bool finishEdit(const QString &mode, const QString &fieldId, bool ok,
                    edi::drafting::DraftingResultCode code, const QString &message);
    bool applyFieldEdit(
        const QString &mode,
        const QString &invalidMessage,
        const QString &fieldId,
        double value,
        const std::function<edi::drafting::DraftingPhysicalGeometryEditPlan(const edi::drafting::DraftingObject &)> &planEdit);
    void setSnapFlag(bool edi::drafting::DraftingSnapSettings::*flag, bool enabled);
    void commitGridSettings(edi::drafting::DraftingGridSettings settings);
    void commitCustomGridSettings(edi::drafting::DraftingGridSettings settings);
    bool applySelectionDrawablePlacement(edi::drafting::DraftingSelectionDrawablePlacement placement);
    bool applyActiveObjectMetadataUpdate(
        edi::drafting::DraftingShapeKind kind,
        const std::function<edi::drafting::DraftingMetadataUpdatePlan(const edi::drafting::ObjectMetadata &)> &planMetadata);
    bool applyActiveObjectGeometryUpdate(
        edi::drafting::DraftingShapeKind kind,
        const std::function<std::optional<edi::drafting::DraftingGeometry>(const edi::drafting::DraftingObject &)> &planGeometry);
    // Resolves the active object by the kind matching Geometry and unwraps the
    // variant before invoking the plan; the kind/type pairing is fixed at compile time.
    template <typename Geometry>
    bool applyActiveGeometryPlan(
        const std::function<std::optional<edi::drafting::DraftingGeometry>(const Geometry &)> &planGeometry);
    bool applyGuideDrawablePlacement(edi::drafting::DraftingGuideDrawablePlacement placement);
    bool applyLayerFlagsUpdate(
        const edi::drafting::LayerId &layerId,
        edi::drafting::DraftingLayerFlagsPlan (*planFlags)(const edi::drafting::DraftingLayer &, bool),
        bool value);
    bool applyActiveLayerPlotStyleUpdate(
        const std::function<edi::drafting::LayerPlotStyle(const edi::drafting::DraftingLayer &)> &planPlot);
    bool createTransformedActiveObject(
        const QString &idPrefix,
        const std::function<std::optional<edi::drafting::DraftingObject>(
            const edi::drafting::DraftingObject &source, const std::string &newId)> &transform);
    bool createObjectsAndSelect(const std::vector<edi::drafting::DraftingObject> &objects);
    bool createGuideFromActiveBounds(
        const char *sourceTag,
        const std::function<edi::drafting::DraftingGuidePlan(const edi::drafting::Bounds2D &bounds)> &planGuide);

    QString m_selectedToolId = QStringLiteral("select_move");
    int m_polygonSides = 6;
    edi::drafting::DraftingDocument m_document;
    edi::drafting::DraftingGridSettings m_gridSettings;
    edi::drafting::DraftingSnapSettings m_snapSettings;
    bool m_guideMoveSnapEnabled = true;
    edi::drafting::DraftingPlotSettings m_plotSettings;
    std::optional<edi::drafting::DraftingCalibrationMeasurement> m_latestCalibrationMeasurement;
    std::optional<edi::drafting::DraftingCalibrationCorrectionPlan> m_pendingCalibrationCorrection;
    std::optional<edi::drafting::Point2D> m_pointerRawPoint;
    std::optional<edi::drafting::DraftingToolCreationRequest> m_pendingCreation;
    std::optional<edi::drafting::DraftingObject> m_previewObject;
    QVariantMap m_lastGuideDragSnap;
    QVariantMap m_lastEditStatus;
    int m_nextObjectSerial = 1;
    DrawingDocumentStore m_store;
    std::vector<edi::drafting::DraftingDocument> m_undoStack;
    std::vector<edi::drafting::DraftingDocument> m_redoStack;
    edi::drafting::DraftingDocument m_editBefore;
    bool m_editCommitted = true;
    edi::drafting::DraftingDocument m_interactiveBefore;
    bool m_interactiveEditActive = false;
};
