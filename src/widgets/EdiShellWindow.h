#pragma once

#include <QMainWindow>
#include <QMap>
#include <QPair>
#include <QString>
#include <QVariantMap>
#include <QVector>

#include <functional>
#include <optional>

#include "app/AppState.h"

class QAbstractButton;
class QButtonGroup;
class QCheckBox;
class QComboBox;
class QDoubleSpinBox;
class QLabel;
class QLineEdit;
class QPushButton;
class QSpinBox;
class QWidget;

class DrawingCanvasWidget;
class DrawingDocumentController;

class EdiShellWindow final : public QMainWindow {
    Q_OBJECT

public:
    explicit EdiShellWindow(QWidget *parent = nullptr);

    // Path-based drawing I/O seams (the toolbar/shortcut handlers wrap these in
    // QFileDialog; tests drive them directly to avoid modal dialogs).
    bool saveDrawingToPath(const QString &path);
    bool openDrawingFromPath(const QString &path);
    bool exportSvgToPath(const QString &path);
    bool exportHpglToPath(const QString &path);
    QString currentDrawingPath() const { return m_currentDrawingPath; }
    bool isDocumentDirty() const;

private slots:
    void refreshInspector();

private:
    void promptSaveDrawing();
    void promptSaveDrawingAs();
    void promptOpenDrawing();
    void promptExportSvg();
    void promptExportHpgl();
    void updateWindowTitle();
    int currentDocumentRevision() const;

private:
    QWidget *buildActivityRail();
    QWidget *buildLeftPanel();
    QWidget *buildWorkspaceColumn();
    QWidget *buildRightPanel();
    QWidget *buildBottomPanel();
    QWidget *buildGeometryEditor();
    QWidget *buildObjectFlagControls();
    QWidget *buildLayerControls();
    QWidget *buildNudgeControls();
    QWidget *buildAlignControls();
    QWidget *buildOffsetControls();
    QWidget *buildMirrorControls();
    QWidget *buildRepeatControls();
    QWidget *buildCalibrationControls();
    QPushButton *makeActionButton(const QString &objectName, const QString &label, const std::function<void()> &action);
    // Registers the button in m_conditionalButtons; refreshInspector drives its
    // enabled state from enableKey ("has_selection" or a bool key in the selected
    // object projection), so build and refresh cannot drift apart.
    QPushButton *makeConditionalButton(const QString &objectName, const QString &label,
                                       const QString &enableKey, const std::function<void()> &action);
    QCheckBox *makeToggle(const QString &objectName, const QString &label, const std::function<void(bool)> &onToggled,
                          std::optional<bool> initialChecked = std::nullopt);
    QComboBox *makeDataCombo(const QString &objectName,
                             const QVector<QPair<QString, QString>> &items,
                             const std::function<void(const QString &)> &onData,
                             const QString &initialData = QString());
    struct GeometryFieldSpec {
        QString fieldId;
        QString fieldMode;
        int decimals = 4;
        double step = 0.01;
        double minimum = -10.0;
        double maximum = 10.0;
        double value = 0.0;
    };
    QDoubleSpinBox *makeGeometryFieldSpin(const GeometryFieldSpec &spec);
    void applyGeometryFieldEdit(QDoubleSpinBox *spin);
    QPushButton *makeToolButton(const QString &toolId, const QString &label);
    QPushButton *makeRailButton(const QString &label, const QString &tooltip, bool active = false, bool enabled = true);
    QLabel *makeSectionLabel(const QString &text) const;
    QLabel *makeValueLabel(const QString &text = QString()) const;
    void setWorkspaceMode(edi::app::WorkspaceMode mode);
    void rebuildGeometryEditor(const QVariantMap &selectedObject);
    void applyGeometryEditStatus(const QVariantMap &editStatus);
    void setGeometryEditorVisible(bool visible);
    void applyShellStyle();

    edi::app::AppState m_appState;
    DrawingDocumentController *m_controller = nullptr;
    DrawingCanvasWidget *m_canvas = nullptr;
    QButtonGroup *m_activityGroup = nullptr;
    QButtonGroup *m_toolGroup = nullptr;
    QSpinBox *m_polygonSidesSpin = nullptr;
    QComboBox *m_gridPreset = nullptr;
    QComboBox *m_gridUnit = nullptr;
    QDoubleSpinBox *m_gridWidth = nullptr;
    QDoubleSpinBox *m_gridHeight = nullptr;
    QDoubleSpinBox *m_gridMarginLeft = nullptr;
    QDoubleSpinBox *m_gridMarginTop = nullptr;
    QDoubleSpinBox *m_gridMarginRight = nullptr;
    QDoubleSpinBox *m_gridMarginBottom = nullptr;
    QDoubleSpinBox *m_gridMinorStep = nullptr;
    QSpinBox *m_gridMajorEvery = nullptr;
    QCheckBox *m_gridVisible = nullptr;
    QCheckBox *m_gridSnap = nullptr;
    QCheckBox *m_objectSnap = nullptr;
    QCheckBox *m_endpointSnap = nullptr;
    QCheckBox *m_vertexSnap = nullptr;
    QCheckBox *m_midpointSnap = nullptr;
    QCheckBox *m_centerSnap = nullptr;
    QCheckBox *m_guideSnap = nullptr;
    QCheckBox *m_guideMoveSnap = nullptr;
    QCheckBox *m_objectPrioritySnap = nullptr;
    QComboBox *m_objectTolerance = nullptr;
    QLabel *m_workspaceTitle = nullptr;
    QLabel *m_toolValue = nullptr;
    QLabel *m_selectedValue = nullptr;
    QLabel *m_objectKindValue = nullptr;
    QLabel *m_objectBoundsValue = nullptr;
    QLabel *m_objectGeometryValue = nullptr;
    QLabel *m_objectLayerValue = nullptr;
    QLabel *m_objectMeasurementValue = nullptr;
    QLabel *m_objectPlotSafetyValue = nullptr;
    QLabel *m_selectionPlotBoundsValue = nullptr;
    QVector<QPair<QPushButton *, QString>> m_conditionalButtons;
    QPushButton *m_deleteAllGuidesButton = nullptr;
    QPushButton *m_mergeDuplicateGuidesButton = nullptr;
    QPushButton *m_hideAllGuidesButton = nullptr;
    QPushButton *m_showAllGuidesButton = nullptr;
    QPushButton *m_lockAllGuidesButton = nullptr;
    QPushButton *m_unlockAllGuidesButton = nullptr;
    QLineEdit *m_guideLabel = nullptr;
    QComboBox *m_guideColor = nullptr;
    QComboBox *m_guideDashStyle = nullptr;
    QCheckBox *m_guideShowLabel = nullptr;
    QLabel *m_dimensionReadout = nullptr;
    QComboBox *m_dimensionKind = nullptr;
    QCheckBox *m_dimensionShowLabel = nullptr;
    QMap<QString, QPushButton *> m_guideOffsetButtons;
    QMap<QString, QPushButton *> m_boundsGuideButtons;
    QMap<QString, QPushButton *> m_offsetGuideButtons;
    QMap<QString, QPushButton *> m_alignToGuideButtons;
    QCheckBox *m_selectedLocked = nullptr;
    QCheckBox *m_selectedVisible = nullptr;
    QCheckBox *m_defaultLayerLocked = nullptr;
    QCheckBox *m_defaultLayerVisible = nullptr;
    QComboBox *m_activeLayer = nullptr;
    QComboBox *m_selectedObjectLayer = nullptr;
    QPushButton *m_addLayerButton = nullptr;
    QPushButton *m_layerUpButton = nullptr;
    QPushButton *m_layerDownButton = nullptr;
    QCheckBox *m_activeLayerPlotEnabled = nullptr;
    QComboBox *m_activeLayerPen = nullptr;
    QComboBox *m_activeLayerStrokeWidth = nullptr;
    QWidget *m_geometryEditor = nullptr;
    QLabel *m_geometryEditStatus = nullptr;
    QMap<QString, QDoubleSpinBox *> m_geometryFields;
    QMap<QString, QDoubleSpinBox *> m_physicalGeometryFields;
    QLabel *m_objectsValue = nullptr;
    QLabel *m_guidesValue = nullptr;
    QLabel *m_revisionValue = nullptr;
    QLabel *m_snapValue = nullptr;
    QLabel *m_gridValue = nullptr;
    QLabel *m_plotValue = nullptr;
    QLabel *m_plotBoundsValue = nullptr;
    QLabel *m_plotLayerStatsValue = nullptr;
    QLabel *m_plotPenStatsValue = nullptr;
    QLabel *m_plotReadinessValue = nullptr;
    QComboBox *m_plotOrderMode = nullptr;
    QComboBox *m_plotDirectionMode = nullptr;
    QCheckBox *m_plotPreviewVisible = nullptr;
    QDoubleSpinBox *m_calibrationMeasuredValue = nullptr;
    QLabel *m_calibrationMeasurementValue = nullptr;
    QLabel *m_pointerValue = nullptr;
    QLabel *m_quickMeasureValue = nullptr;
    QLabel *m_guideDragValue = nullptr;
    QLabel *m_previewValue = nullptr;
    QLabel *m_statusValue = nullptr;
    QPushButton *m_undoButton = nullptr;
    QPushButton *m_redoButton = nullptr;
    QString m_currentDrawingPath;
    int m_savedRevision = 0;
};
