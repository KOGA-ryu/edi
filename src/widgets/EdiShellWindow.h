#pragma once

#include <QMainWindow>
#include <QMap>
#include <QString>
#include <QVariantMap>

#include "app/AppState.h"

class QAbstractButton;
class QButtonGroup;
class QCheckBox;
class QComboBox;
class QDoubleSpinBox;
class QLabel;
class QPushButton;
class QWidget;

class DrawingCanvasWidget;
class DrawingDocumentController;

class EdiShellWindow final : public QMainWindow {
    Q_OBJECT

public:
    explicit EdiShellWindow(QWidget *parent = nullptr);

private slots:
    void refreshInspector();

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
    QPushButton *makeToolButton(const QString &toolId, const QString &label);
    QPushButton *makeRailButton(const QString &label, const QString &tooltip, bool active = false, bool enabled = true);
    QLabel *makeSectionLabel(const QString &text) const;
    QLabel *makeValueLabel(const QString &text = QString()) const;
    void setWorkspaceMode(edi::app::WorkspaceMode mode);
    void rebuildGeometryEditor(const QVariantMap &selectedObject);
    void setGeometryEditorVisible(bool visible);
    void applyShellStyle();

    edi::app::AppState m_appState;
    DrawingDocumentController *m_controller = nullptr;
    DrawingCanvasWidget *m_canvas = nullptr;
    QButtonGroup *m_activityGroup = nullptr;
    QButtonGroup *m_toolGroup = nullptr;
    QComboBox *m_gridPreset = nullptr;
    QCheckBox *m_gridSnap = nullptr;
    QCheckBox *m_objectSnap = nullptr;
    QCheckBox *m_endpointSnap = nullptr;
    QCheckBox *m_vertexSnap = nullptr;
    QCheckBox *m_midpointSnap = nullptr;
    QCheckBox *m_centerSnap = nullptr;
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
    QMap<QString, QDoubleSpinBox *> m_geometryFields;
    QLabel *m_objectsValue = nullptr;
    QLabel *m_revisionValue = nullptr;
    QLabel *m_snapValue = nullptr;
    QLabel *m_gridValue = nullptr;
    QLabel *m_plotValue = nullptr;
    QComboBox *m_plotOrderMode = nullptr;
    QCheckBox *m_plotPreviewVisible = nullptr;
    QLabel *m_pointerValue = nullptr;
    QLabel *m_previewValue = nullptr;
    QLabel *m_statusValue = nullptr;
};
