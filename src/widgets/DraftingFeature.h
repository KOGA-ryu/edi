#pragma once

#include <QMap>
#include <QObject>
#include <QPair>
#include <QString>
#include <QStringList>
#include <QVariantMap>
#include <QVector>

#include <functional>
#include <optional>
#include <vector>

#include "widgets/ShellHost.h"

class BeltCrossWidget;
class QAbstractButton;
class QCheckBox;
class QComboBox;
class QDoubleSpinBox;
class QLabel;
class QLineEdit;
class QListWidget;
class QPushButton;
class QSpinBox;
class QVBoxLayout;
class QWidget;

class DrawingCanvasWidget;
class DrawingDocumentController;

// The drafting workspace as a self-contained feature: it owns the drafting
// panels and the inspector refresh, and talks to the shell only through
// ShellActions callables. It never includes EdiShellWindow, so its lifetime
// can later match its widgets when workspace switching lands.
class DraftingFeature final : public QObject {
    Q_OBJECT

public:
    // Everything the feature needs from the shell arrives as data (callables),
    // not as a window pointer — the shell stays swappable.
    struct ShellActions {
        std::function<void()> saveDrawing;
        std::function<void()> saveDrawingAs;
        std::function<void()> openDrawing;
        std::function<void()> exportSvg;
        std::function<void()> exportHpgl;
        std::function<void(const QString &)> openDrawingAtPath;
        std::function<QString()> workspaceModeLabel;
        std::function<QString()> workspaceModeName;
        // The feature publishes its one-line status; the shell decides where
        // it shows (currently the title-bar chrome).
        std::function<void(const QString &)> setStatusText;
        // The belt arrangement is workspace data (which tool sits in which
        // slot is part of the job); the shell owns the layout, the feature
        // only renders it. Absent callable -> the feature's default belt.
        std::function<edi::shell::BeltLayout()> beltLayout;
        // Modular panels: which panel hosts a content group ("left"/"right"/
        // "bottom"/"hidden"; empty -> feature default). The shell owns the
        // assignments (workspace data); the feature only places groups.
        std::function<QString(const QString &)> panelSlotForGroup;
    };

    DraftingFeature(DrawingDocumentController *controller, ShellActions actions, QObject *parent = nullptr);

    QWidget *buildPanel(edi::shell::ShellSlot slot);
    // F4: the feature's floating palettes (currently just the tool belt).
    // Fresh widgets per call; the shell frames, places, and owns them.
    std::vector<edi::shell::FeaturePaletteSpec> buildPalettes();
    // Top-chrome popup panels (currently just the Snap settings). Same
    // contract as buildPalettes: fresh widgets, shell owns the frame.
    std::vector<edi::shell::FeatureChromePanelSpec> buildChromePanels();
    void refreshInspector();
    // The light per-mouse-move sibling: only the pointer/quick-measure/
    // guide-drag/preview readouts. Subscribed to pointerChanged.
    void refreshPointerReadouts();
    // The one-line status publish (mode | counts | zoom%). Zoom changes call
    // only this — never the full inspector rebuild.
    void publishStatus();
    DrawingCanvasWidget *canvas() const { return m_canvas; }

    // The drafting tools arranged on the belt: one row per tool, the row's
    // cells its sub-features. Static: the window bakes it into the built-in
    // drafting workspace layout before any instance exists.
    static edi::shell::BeltLayout defaultBeltLayout();
    // The tool vocabulary for editors (F6): id + label, table order.
    static QVector<QPair<QString, QString>> toolInventory();
    // The row-per-tool arrangement restricted to `enabledIds` — the belt the
    // F6 checklist writes. defaultBeltLayout() is this with everything on.
    static edi::shell::BeltLayout beltLayoutForTools(const QStringList &enabledIds);
    // Re-dress the live belt widget from a changed arrangement, in place —
    // no remount, so the settings checklist edits the belt while both stay
    // on screen (same live-edit contract as theming).
    void refreshBelt(const edi::shell::BeltLayout &belt);
    // Modular panels: the group vocabulary for the settings page, and the
    // live re-place when an assignment changes (groups reparent, not rebuild).
    static QVector<QPair<QString, QString>> panelGroupInventory();
    // The feature-default panel for a group — public because the shell's
    // assignment hook answers with it when the workspace has no opinion.
    static QString defaultPanelSlot(const QString &groupId);
    void applyPanelAssignments();

private:
    QWidget *buildLeftPanel();
    QWidget *buildWorkspaceColumn();
    QWidget *buildRightPanel();
    QWidget *buildBottomPanel();
    // F2: one container per inspector plan group id; refreshInspector toggles
    // their visibility from planDraftingInspector — never rebuilds them.
    QVBoxLayout *beginInspectorGroup(const QString &groupId);
    void ensureInspectorGroupsBuilt();
    void placePanelGroups(const QString &slotName, QVBoxLayout *layout);
    QString assignedPanelSlot(const QString &groupId) const;
    QString placementSlot(const QString &groupId) const;
    void applyInspectorPlan(const QVariantMap &selectedObject);
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
    QLabel *makeValueLabel(const QString &text = QString()) const;
    void rebuildGeometryEditor(const QVariantMap &selectedObject);
    void applyGeometryEditStatus(const QVariantMap &editStatus);
    void setGeometryEditorVisible(bool visible);

    // C3 block palette: repopulate the block list from the document's blocks
    // (name + id), called on build and on every model change (like m_objectList).
    void refreshBlockPalette();

    DrawingDocumentController *m_controller = nullptr;
    ShellActions m_actions;
    DrawingCanvasWidget *m_canvas = nullptr;
    QListWidget *m_objectList = nullptr;
    QLabel *m_objectListEmpty = nullptr;
    QListWidget *m_blockList = nullptr;
    QLineEdit *m_blockNameField = nullptr;
    // DM-14: the per-placement transform for the NEXT stamped block. They bind to
    // controller setters (setBlockPlacementRotation/Scale); the controller's next
    // placeBlockInstance consumes them. Defaults 0 deg / 1.0 keep identity placement.
    QDoubleSpinBox *m_blockRotationSpin = nullptr;
    QDoubleSpinBox *m_blockScaleSpin = nullptr;
    // DM-15: inspector delta-transform spins for a selected placed instance.
    // These are WRITE-ONLY-ON-CLICK deltas — they do not bind a controller setter;
    // only transformInstanceButton reads them at click time. Defaulted to identity
    // (0.0 deg / 1.0 scale) on build. No programmatic write-back: a refresh never
    // touches them, so the group can never loop back into the controller.
    QDoubleSpinBox *m_instanceRotationSpin = nullptr;
    QDoubleSpinBox *m_instanceScaleSpin = nullptr;
    // The transform button is stored so refreshInspector can gate it alongside the
    // group — redundant when the group hides, but explicit for test discoverability.
    QPushButton *m_transformInstanceButton = nullptr;
    BeltCrossWidget *m_beltWidget = nullptr;
    QSpinBox *m_polygonSidesSpin = nullptr;
    QCheckBox *m_aspectLockToggle = nullptr;
    // DR-10: when the Repeat fold's "Rotate copies" toggle is on, the shared
    // Radial button arms the rotate-copies rosette instead of the placement-only
    // radial array. The toggle state is a pure UI choice (which op the one button
    // runs), so it lives here as data, not as a widget subclass.
    bool m_rotateCopies = false;
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
    QCheckBox *m_intersectionSnap = nullptr;
    QCheckBox *m_guideSnap = nullptr;
    QCheckBox *m_guideMoveSnap = nullptr;
    QCheckBox *m_objectPrioritySnap = nullptr;
    QComboBox *m_objectTolerance = nullptr;
    QLineEdit *m_styleColorField = nullptr;
    QDoubleSpinBox *m_styleWidthSpin = nullptr;
    QDoubleSpinBox *m_styleOpacitySpin = nullptr;
    QLineEdit *m_styleFillColorField = nullptr;
    QDoubleSpinBox *m_styleFillOpacitySpin = nullptr;
    QLineEdit *m_textContentField = nullptr;
    QWidget *m_textContentRow = nullptr;
    QComboBox *m_styleLineCombo = nullptr;
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
    QComboBox *m_objectRole = nullptr;
    QComboBox *m_wallType = nullptr;
    QLineEdit *m_objectMaterial = nullptr;
    QLineEdit *m_objectExportGroup = nullptr;
    QLineEdit *m_objectTags = nullptr;
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
    QMap<QString, QWidget *> m_inspectorGroups;
    QStringList m_groupBuildOrder; // canonical top-to-bottom order
    QMap<QString, QVBoxLayout *> m_panelGroupHosts; // slot name -> content layout
};
