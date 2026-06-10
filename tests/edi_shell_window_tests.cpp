#include "widgets/EdiShellWindow.h"

#include "core/DrawingCore.h"
#include "io/SettingsStore.h"
#include "io/ShellLayoutStore.h"

#include <QApplication>
#include <QCheckBox>
#include <QCoreApplication>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QLineEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QSplitter>
#include <QString>
#include <QFile>
#include <QTemporaryDir>
#include <QUrl>
#include <QVariantList>
#include <QVariantMap>

#include <cassert>
#include <cmath>

namespace {

QCheckBox *toggleWithLabel(const QWidget &root, const QString &label)
{
    for (QCheckBox *checkbox : root.findChildren<QCheckBox *>()) {
        if (checkbox->text() == label) {
            return checkbox;
        }
    }
    return nullptr;
}

// Several combos share an objectName; the data of their first item is unique.
QComboBox *comboWithFirstItemData(const QWidget &root, const QString &firstItemData)
{
    for (QComboBox *combo : root.findChildren<QComboBox *>()) {
        if (combo->count() > 0 && combo->itemData(0).toString() == firstItemData) {
            return combo;
        }
    }
    return nullptr;
}

QPushButton *buttonNamed(const QWidget &root, const QString &objectName)
{
    return root.findChild<QPushButton *>(objectName);
}

QPushButton *buttonWithText(const QWidget &root, const QString &text)
{
    for (QPushButton *button : root.findChildren<QPushButton *>()) {
        if (button->text() == text) {
            return button;
        }
    }
    return nullptr;
}

QDoubleSpinBox *geometryFieldSpin(const QWidget &root, const QString &fieldId, const QString &fieldMode)
{
    // rebuildGeometryEditor retires old spins with deleteLater(); flush those
    // deletions so the lookup cannot bind a zombie spin from a previous
    // selection, then require exactly one live match.
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    QDoubleSpinBox *match = nullptr;
    for (QDoubleSpinBox *spin : root.findChildren<QDoubleSpinBox *>()) {
        if (spin->property("fieldId").toString() == fieldId
            && spin->property("fieldMode").toString() == fieldMode) {
            assert(match == nullptr);
            match = spin;
        }
    }
    return match;
}

QVariantMap activeObject(const DrawingDocumentController &controller)
{
    const QVariantMap model = controller.modelDocument();
    const QString activeId = model.value(QStringLiteral("active_object_id")).toString();
    for (const QVariant &value : model.value(QStringLiteral("drawing_objects")).toList()) {
        const QVariantMap object = value.toMap();
        if (object.value(QStringLiteral("id")).toString() == activeId) {
            return object;
        }
    }
    return {};
}

int objectCount(const DrawingDocumentController &controller)
{
    return controller.modelDocument().value(QStringLiteral("drawing_objects")).toList().size();
}

bool near(double a, double b, double tolerance = 0.0001)
{
    return std::abs(a - b) <= tolerance;
}

} // namespace

int main(int argc, char **argv)
{
    qputenv("QT_QPA_PLATFORM", "offscreen");
    QApplication app(argc, argv);

    EdiShellWindow window;
    auto *controller = window.findChild<DrawingDocumentController *>();
    assert(controller != nullptr);

    // Initial sync: factory-built controls reflect controller state at construction.
    QComboBox *gridPreset = comboWithFirstItemData(window, QStringLiteral("square_art_board"));
    assert(gridPreset != nullptr);
    assert(gridPreset->currentData().toString() == controller->gridPresetId());

    QComboBox *tolerance = comboWithFirstItemData(window, QStringLiteral("tight"));
    assert(tolerance != nullptr);
    assert(tolerance->currentData().toString() == controller->objectSnapTolerancePresetId());

    QCheckBox *gridSnap = toggleWithLabel(window, QStringLiteral("Grid snap"));
    assert(gridSnap != nullptr);
    assert(gridSnap->isChecked() == controller->gridSnapEnabled());
    QCheckBox *guideMoveSnap = toggleWithLabel(window, QStringLiteral("Guide move snap"));
    assert(guideMoveSnap != nullptr);
    assert(guideMoveSnap->isChecked() == controller->guideMoveSnapEnabled());

    // Toggle wiring: flipping the checkbox reaches the controller.
    {
        const bool before = controller->gridSnapEnabled();
        gridSnap->setChecked(!before);
        assert(controller->gridSnapEnabled() == !before);
        gridSnap->setChecked(before);
        assert(controller->gridSnapEnabled() == before);
    }

    // Data-combo wiring: selecting an entry reaches the controller with item data.
    {
        QComboBox *plotOrder = comboWithFirstItemData(window, QStringLiteral("layer_order"));
        assert(plotOrder != nullptr);
        assert(plotOrder->currentData().toString() == controller->plotOrderModeId());
        plotOrder->setCurrentIndex(1);
        assert(controller->plotOrderModeId() == QStringLiteral("nearest_next"));
        plotOrder->setCurrentIndex(0);
        assert(controller->plotOrderModeId() == QStringLiteral("layer_order"));
    }

    // Action-button wiring: a click runs the controller action.
    {
        const int layersBefore = controller->modelDocument().value(QStringLiteral("layers")).toList().size();
        QPushButton *addLayer = buttonNamed(window, QStringLiteral("addLayerButton"));
        assert(addLayer != nullptr);
        addLayer->click();
        const int layersAfter = controller->modelDocument().value(QStringLiteral("layers")).toList().size();
        assert(layersAfter == layersBefore + 1);
    }

    // Conditional buttons: disabled without a selection, enabled by the matching
    // projection flag, and click() on a disabled button is a no-op.
    QPushButton *fitToDrawable = buttonNamed(window, QStringLiteral("fitToDrawableButton"));
    QPushButton *guideToOrigin = buttonNamed(window, QStringLiteral("guideToDrawableOriginButton"));
    QPushButton *deleteSelectedGuide = buttonNamed(window, QStringLiteral("deleteSelectedGuideButton"));
    assert(fitToDrawable != nullptr && guideToOrigin != nullptr && deleteSelectedGuide != nullptr);
    assert(!fitToDrawable->isEnabled());
    assert(!guideToOrigin->isEnabled());

    // Create and select a point object through the controller; refreshInspector
    // runs via the modelChanged connection.
    controller->setSelectedToolId(QStringLiteral("point_tool"));
    controller->clickCanvasNormalized(0.5, 0.5);
    assert(!controller->modelDocument().value(QStringLiteral("active_object_id")).toString().isEmpty());
    assert(fitToDrawable->isEnabled());
    assert(!guideToOrigin->isEnabled());

    // Create and select a guide; guide-conditional buttons flip on.
    controller->setSelectedToolId(QStringLiteral("horizontal_guide_tool"));
    controller->clickCanvasNormalized(0.5, 0.25);
    assert(guideToOrigin->isEnabled());
    assert(deleteSelectedGuide->isEnabled());

    // Registry-driven click path end to end: delete the selected guide.
    {
        const int guidesBefore = controller->modelDocument().value(QStringLiteral("guide_count")).toInt();
        assert(guidesBefore > 0);
        deleteSelectedGuide->click();
        const int guidesAfter = controller->modelDocument().value(QStringLiteral("guide_count")).toInt();
        assert(guidesAfter == guidesBefore - 1);
    }

    // Guide preset button creates guides through the spec-table wiring.
    {
        const int guidesBefore = controller->modelDocument().value(QStringLiteral("guide_count")).toInt();
        QPushButton *presetBounds = buttonNamed(window, QStringLiteral("guidePreset_drawable_bounds"));
        assert(presetBounds != nullptr);
        presetBounds->click();
        const int guidesAfter = controller->modelDocument().value(QStringLiteral("guide_count")).toInt();
        assert(guidesAfter > guidesBefore);
    }

    // Guide visuals: create and select a fresh guide, then drive label, color,
    // dash style, and show-label through the inspector controls.
    controller->setSelectedToolId(QStringLiteral("horizontal_guide_tool"));
    controller->clickCanvasNormalized(0.5, 0.62);
    {
        auto *guideLabel = window.findChild<QLineEdit *>(QStringLiteral("guideLabelField"));
        assert(guideLabel != nullptr && guideLabel->isEnabled());
        guideLabel->setText(QStringLiteral("datum"));
        QMetaObject::invokeMethod(guideLabel, "editingFinished");
        assert(activeObject(*controller).value(QStringLiteral("guide_custom_label")).toString() == QStringLiteral("datum"));

        QComboBox *guideColor = window.findChild<QComboBox *>(QStringLiteral("guideColorCombo"));
        assert(guideColor != nullptr && guideColor->isEnabled());
        guideColor->setCurrentIndex(1);
        assert(activeObject(*controller).value(QStringLiteral("guide_color")).toString() == QStringLiteral("#54d2c6"));

        QComboBox *guideDash = window.findChild<QComboBox *>(QStringLiteral("guideDashStyleCombo"));
        assert(guideDash != nullptr);
        guideDash->setCurrentIndex(1);
        assert(activeObject(*controller).value(QStringLiteral("guide_dash_style")).toString() == QStringLiteral("solid"));

        auto *guideShowLabel = window.findChild<QCheckBox *>(QStringLiteral("guideShowLabelCheckbox"));
        assert(guideShowLabel != nullptr && guideShowLabel->isChecked());
        guideShowLabel->setChecked(false);
        assert(!activeObject(*controller).value(QStringLiteral("guide_show_label"), true).toBool());
    }

    // Dimension controls: create a distance dimension, switch kind and label
    // visibility through the inspector.
    controller->setSelectedToolId(QStringLiteral("distance_dimension_tool"));
    controller->clickCanvasNormalized(0.3, 0.4);
    controller->clickCanvasNormalized(0.6, 0.4);
    {
        assert(activeObject(*controller).value(QStringLiteral("kind")).toString() == QStringLiteral("dimension"));
        QComboBox *dimensionKind = window.findChild<QComboBox *>(QStringLiteral("dimensionKindCombo"));
        assert(dimensionKind != nullptr && dimensionKind->isEnabled());
        dimensionKind->setCurrentIndex(1);
        assert(activeObject(*controller).value(QStringLiteral("dimension_kind")).toString() == QStringLiteral("width"));

        auto *dimensionShowLabel = window.findChild<QCheckBox *>(QStringLiteral("dimensionShowLabelCheckbox"));
        assert(dimensionShowLabel != nullptr && dimensionShowLabel->isChecked());
        dimensionShowLabel->setChecked(false);
        assert(!activeObject(*controller).value(QStringLiteral("dimension_show_label"), true).toBool());
    }

    // Geometry editor spins: select a line; the rebuilt editor carries tagged
    // spins whose edits reach the controller (normalized and physical).
    controller->setSelectedToolId(QStringLiteral("line_tool"));
    controller->clickCanvasNormalized(0.2, 0.2);
    controller->clickCanvasNormalized(0.4, 0.2);
    {
        QDoubleSpinBox *x1 = geometryFieldSpin(window, QStringLiteral("x1"), QStringLiteral("normalized"));
        assert(x1 != nullptr);
        x1->setValue(0.25);
        QMetaObject::invokeMethod(x1, "editingFinished");
        assert(near(activeObject(*controller).value(QStringLiteral("x1")).toDouble(), 0.25));

        QDoubleSpinBox *physicalX1 = geometryFieldSpin(window, QStringLiteral("x1"), QStringLiteral("physical"));
        assert(physicalX1 != nullptr);
        const double gridWidth = controller->modelDocument()
            .value(QStringLiteral("grid")).toMap().value(QStringLiteral("width")).toDouble();
        assert(gridWidth > 0.0);
        // Move the physical X1 to 30% of the bed width; normalized x1 follows.
        physicalX1->setValue(0.3 * gridWidth);
        QMetaObject::invokeMethod(physicalX1, "editingFinished");
        assert(near(activeObject(*controller).value(QStringLiteral("x1")).toDouble(), 0.3, 0.001));
    }

    // Transform buttons: nudge moves the selection, offset/mirror/repeat create
    // objects, align snaps two objects' edges together.
    {
        const QVariantMap before = activeObject(*controller);
        QPushButton *nudgeUp = buttonWithText(window, QStringLiteral("Grid Up"));
        assert(nudgeUp != nullptr);
        nudgeUp->click();
        assert(activeObject(*controller).value(QStringLiteral("y1")).toDouble()
            < before.value(QStringLiteral("y1")).toDouble());
    }
    {
        const int before = objectCount(*controller);
        buttonWithText(window, QStringLiteral("Right +0.05"))->click();
        assert(objectCount(*controller) == before + 1);
    }
    {
        const int before = objectCount(*controller);
        buttonWithText(window, QStringLiteral("Mirror H"))->click();
        assert(objectCount(*controller) == before + 1);
    }
    {
        const int before = objectCount(*controller);
        buttonWithText(window, QStringLiteral("Repeat X"))->click();
        assert(objectCount(*controller) > before);
    }
    {
        // Align Left: select everything, then the left edges should match after.
        controller->selectObjectsInBoundsNormalized(0.0, 0.0, 1.0, 1.0);
        buttonWithText(window, QStringLiteral("Left"))->click();
        const QVariantList objects = controller->modelDocument().value(QStringLiteral("drawing_objects")).toList();
        double minLeft = 2.0;
        for (const QVariant &value : objects) {
            const QVariantMap bounds = value.toMap().value(QStringLiteral("bounds")).toMap();
            minLeft = std::min(minLeft, bounds.value(QStringLiteral("x")).toDouble());
        }
        int aligned = 0;
        for (const QVariant &value : objects) {
            const QVariantMap object = value.toMap();
            if (object.value(QStringLiteral("kind")).toString() == QStringLiteral("guide")) {
                continue;
            }
            const QVariantMap bounds = object.value(QStringLiteral("bounds")).toMap();
            if (near(bounds.value(QStringLiteral("x")).toDouble(), minLeft, 0.0001)) {
                ++aligned;
            }
        }
        assert(aligned >= 2);
    }

    // Calibration row: pattern button creates objects, record captures a
    // measurement, apply-scale consumes it.
    {
        const int before = objectCount(*controller);
        QPushButton *testSquare = buttonWithText(window, QStringLiteral("Test square"));
        assert(testSquare != nullptr);
        testSquare->click();
        assert(objectCount(*controller) > before);

        auto *calibrationPanel = window.findChild<QWidget *>(QStringLiteral("calibrationControls"));
        assert(calibrationPanel != nullptr);
        auto *calibrationValue = calibrationPanel->findChild<QDoubleSpinBox *>();
        assert(calibrationValue != nullptr);
        calibrationValue->setValue(0.25);
        buttonWithText(window, QStringLiteral("Record"))->click();
        assert(!controller->modelDocument().value(QStringLiteral("calibration_measurement")).toMap().isEmpty());
        buttonWithText(window, QStringLiteral("Apply scale"))->click();
        assert(!controller->modelDocument().value(QStringLiteral("calibration_correction")).toMap().isEmpty());
    }

    // Project Files: the buttons exist and the save/open seam round-trips the
    // document while keeping window-title dirty state in sync.
    {
        assert(buttonNamed(window, QStringLiteral("openDrawingButton")) != nullptr);
        assert(buttonNamed(window, QStringLiteral("saveDrawingButton")) != nullptr);
        assert(buttonNamed(window, QStringLiteral("saveDrawingAsButton")) != nullptr);

        QTemporaryDir tempDir;
        assert(tempDir.isValid());
        const QString path = tempDir.filePath(QStringLiteral("shell_roundtrip.edidraw"));

        controller->setSelectedToolId(QStringLiteral("line_tool"));
        controller->clickCanvasNormalized(0.15, 0.25);
        controller->clickCanvasNormalized(0.75, 0.85);
        const QVariantMap savedModel = controller->modelDocument();
        const QVariantList savedObjects = savedModel.value(QStringLiteral("drawing_objects")).toList();
        assert(!savedObjects.isEmpty());

        // Unsaved edits mark the title dirty; saving clears it and adopts the path.
        assert(window.isDocumentDirty());
        assert(window.saveDrawingToPath(path));
        assert(window.currentDrawingPath() == path);
        assert(!window.isDocumentDirty());
        assert(window.windowTitle().contains(QStringLiteral("shell_roundtrip.edidraw")));
        assert(!window.windowTitle().contains(QStringLiteral("•")));

        // Mutate after save: dirty again.
        controller->setSelectedToolId(QStringLiteral("point_tool"));
        controller->clickCanvasNormalized(0.4, 0.4);
        assert(window.isDocumentDirty());
        assert(window.windowTitle().contains(QStringLiteral("•")));

        // Reopen the saved file: projection matches the saved state and title is clean.
        assert(window.openDrawingFromPath(path));
        assert(!window.isDocumentDirty());
        const QVariantList reopenedObjects = controller->modelDocument().value(QStringLiteral("drawing_objects")).toList();
        assert(reopenedObjects.size() == savedObjects.size());
        for (int i = 0; i < savedObjects.size(); ++i) {
            assert(reopenedObjects[i].toMap().value(QStringLiteral("id")).toString()
                   == savedObjects[i].toMap().value(QStringLiteral("id")).toString());
        }

        // Opening a missing path fails and leaves the document untouched.
        assert(!window.openDrawingFromPath(tempDir.filePath(QStringLiteral("missing.edidraw"))));
        assert(controller->modelDocument().value(QStringLiteral("drawing_objects")).toList().size()
               == reopenedObjects.size());
    }

    // Edit section: Undo/Redo buttons exist, enable from canUndo/canRedo, and act.
    {
        QPushButton *undoButton = buttonNamed(window, QStringLiteral("undoButton"));
        QPushButton *redoButton = buttonNamed(window, QStringLiteral("redoButton"));
        assert(undoButton != nullptr);
        assert(redoButton != nullptr);

        const int before = controller->modelDocument().value(QStringLiteral("drawing_objects")).toList().size();
        controller->setSelectedToolId(QStringLiteral("point_tool"));
        controller->clickCanvasNormalized(0.33, 0.33);
        const int afterCreate = controller->modelDocument().value(QStringLiteral("drawing_objects")).toList().size();
        assert(afterCreate == before + 1);
        // The create's modelChanged already refreshed the buttons via canUndo().
        assert(undoButton->isEnabled());
        assert(!redoButton->isEnabled());

        undoButton->click();
        assert(controller->modelDocument().value(QStringLiteral("drawing_objects")).toList().size() == before);
        assert(redoButton->isEnabled());

        redoButton->click();
        assert(controller->modelDocument().value(QStringLiteral("drawing_objects")).toList().size() == afterCreate);
        assert(!redoButton->isEnabled());
    }

    // Tool Options: the Sides spin drives the controller's polygon side count.
    {
        auto *sidesSpin = window.findChild<QSpinBox *>(QStringLiteral("polygonSidesSpin"));
        assert(sidesSpin != nullptr);
        assert(sidesSpin->value() == controller->polygonSides());
        sidesSpin->setValue(8);
        assert(controller->polygonSides() == 8);
    }

    // Export buttons exist and the path seams write SVG / HPGL files.
    {
        assert(buttonNamed(window, QStringLiteral("exportSvgButton")) != nullptr);
        assert(buttonNamed(window, QStringLiteral("exportHpglButton")) != nullptr);

        controller->setSelectedToolId(QStringLiteral("line_tool"));
        controller->clickCanvasNormalized(0.2, 0.2);
        controller->clickCanvasNormalized(0.8, 0.8);

        QTemporaryDir tempDir;
        assert(tempDir.isValid());
        const QString svgPath = tempDir.filePath(QStringLiteral("shell.svg"));
        const QString hpglPath = tempDir.filePath(QStringLiteral("shell.hpgl"));
        assert(window.exportSvgToPath(svgPath));
        assert(window.exportHpglToPath(hpglPath));
        assert(QFile::exists(svgPath));
        assert(QFile::exists(hpglPath));
    }

    // Settings persistence: change a snap toggle + plot mode, save, then rebuild
    // a fresh window from the same file and assert the state survived.
    {
        QTemporaryDir tempDir;
        assert(tempDir.isValid());
        const QString settingsPath = tempDir.filePath(QStringLiteral("edi.toml"));

        // Use an explicit non-default value (grid snap defaults to false) so the
        // assertion cannot pass vacuously against a fresh window's default.
        controller->setGridSnapEnabled(true);
        controller->setPlotOrderModeId(QStringLiteral("nearest_next"));
        controller->setObjectSnapTolerancePreset(QStringLiteral("tight"));
        assert(window.saveSettings(settingsPath));
        assert(QFile::exists(settingsPath));

        EdiShellWindow restored;
        auto *restoredController = restored.findChild<DrawingDocumentController *>();
        assert(restoredController != nullptr);
        assert(!restoredController->gridSnapEnabled()); // fresh default before load
        assert(restored.loadSettings(settingsPath));
        assert(restoredController->gridSnapEnabled()); // loaded the saved true
        assert(restoredController->plotOrderModeId() == QStringLiteral("nearest_next"));
        assert(restoredController->objectSnapTolerancePresetId() == QStringLiteral("tight"));
    }

    // Recent files: saving a drawing records it and surfaces a quick-open button.
    {
        QTemporaryDir tempDir;
        assert(tempDir.isValid());
        const QString drawingPath = tempDir.filePath(QStringLiteral("recent.edidraw"));
        assert(window.saveDrawingToPath(drawingPath));
        assert(window.recentFiles().contains(drawingPath));
        assert(buttonNamed(window, QStringLiteral("recentFileButton")) != nullptr);
    }

    // Panel system (spec §2): collapse, presets, and auto-hide as observable
    // window states. isVisibleTo(&window) reads the panel's own visibility
    // without requiring the offscreen window itself to be shown.
    {
        using edi::shell::PanelPreset;
        using edi::shell::PanelVisibility;
        using edi::shell::ShellSlot;

        // Auto-hide reacts in resizeEvent, and Qt defers resize events for
        // hidden widgets — show the (offscreen) window so resize() delivers
        // them like it does in the real app.
        window.show();
        window.resize(1280, 820);
        QWidget *leftPanel = window.findChild<QWidget *>(QStringLiteral("leftPanel"));
        QWidget *rightPanel = window.findChild<QWidget *>(QStringLiteral("rightPanel"));
        QWidget *bottomPanel = window.findChild<QWidget *>(QStringLiteral("bottomPanel"));
        assert(leftPanel != nullptr && rightPanel != nullptr && bottomPanel != nullptr);

        // Initial state per spec: left open, right and bottom collapsed.
        assert(window.shellPanelVisibility(ShellSlot::Left) == PanelVisibility::Visible);
        assert(window.shellPanelVisibility(ShellSlot::Right) == PanelVisibility::Collapsed);
        assert(window.shellPanelVisibility(ShellSlot::Bottom) == PanelVisibility::Collapsed);
        assert(leftPanel->isVisibleTo(&window));
        assert(!rightPanel->isVisibleTo(&window));

        // Manual collapse toggling reaches the widgets.
        window.setPanelCollapsed(ShellSlot::Right, false);
        assert(window.shellPanelVisibility(ShellSlot::Right) == PanelVisibility::Visible);
        assert(rightPanel->isVisibleTo(&window));

        // Auto-hide: shrink below the left panel's 640px threshold; right
        // never auto-hides. Growing back restores the panel.
        window.resize(600, 820);
        assert(window.shellPanelVisibility(ShellSlot::Left) == PanelVisibility::AutoHidden);
        assert(!leftPanel->isVisibleTo(&window));
        assert(window.shellPanelVisibility(ShellSlot::Right) == PanelVisibility::Visible);
        window.resize(1280, 820);
        assert(window.shellPanelVisibility(ShellSlot::Left) == PanelVisibility::Visible);
        assert(leftPanel->isVisibleTo(&window));

        // Manual collapse is sticky across resizes (it outranks auto-hide).
        window.setPanelCollapsed(ShellSlot::Left, true);
        window.resize(600, 820);
        assert(window.shellPanelVisibility(ShellSlot::Left) == PanelVisibility::Collapsed);
        window.resize(1280, 820);
        assert(window.shellPanelVisibility(ShellSlot::Left) == PanelVisibility::Collapsed);

        // Presets transform the whole state.
        window.applyShellPanelPreset(PanelPreset::Full);
        assert(window.shellPanelVisibility(ShellSlot::Left) == PanelVisibility::Visible);
        assert(window.shellPanelVisibility(ShellSlot::Right) == PanelVisibility::Visible);
        assert(window.shellPanelVisibility(ShellSlot::Bottom) == PanelVisibility::Visible);
        assert(bottomPanel->isVisibleTo(&window));
        window.applyShellPanelPreset(PanelPreset::Focus);
        assert(!leftPanel->isVisibleTo(&window));
        assert(!rightPanel->isVisibleTo(&window));
        assert(!bottomPanel->isVisibleTo(&window));
        window.applyShellPanelPreset(PanelPreset::Review);
        assert(leftPanel->isVisibleTo(&window));
        assert(!rightPanel->isVisibleTo(&window));

        // Drag limits surface as widget constraints the splitter must respect.
        assert(leftPanel->minimumWidth() == 180);
        assert(leftPanel->maximumWidth() == 520);
        assert(rightPanel->minimumWidth() == 160);
        assert(bottomPanel->minimumHeight() == 96);
        assert(bottomPanel->maximumHeight() == 1000);

        // Leave everything open so any assertions added later see a full shell.
        window.applyShellPanelPreset(PanelPreset::Full);
    }

    // Workspace layout persistence: panel geometry survives a "restart"
    // (save from one window, load into a fresh one).
    {
        using edi::shell::PanelPreset;
        using edi::shell::PanelVisibility;
        using edi::shell::ShellSlot;

        QTemporaryDir tempDir;
        assert(tempDir.isValid());
        const QString layoutPath = tempDir.filePath(QStringLiteral("workspace.toml"));

        window.applyShellPanelPreset(PanelPreset::Full);
        window.setPanelCollapsed(ShellSlot::Right, true);
        window.setPanelCollapsed(ShellSlot::Bottom, true);
        assert(window.saveWorkspaceLayout(layoutPath));

        EdiShellWindow restored;
        restored.applyShellPanelPreset(PanelPreset::Full); // scramble away from saved state
        assert(restored.shellPanelVisibility(ShellSlot::Right) == PanelVisibility::Visible);
        assert(restored.loadWorkspaceLayout(layoutPath));
        assert(restored.shellPanelVisibility(ShellSlot::Left) == PanelVisibility::Visible);
        assert(restored.shellPanelVisibility(ShellSlot::Right) == PanelVisibility::Collapsed);
        assert(restored.shellPanelVisibility(ShellSlot::Bottom) == PanelVisibility::Collapsed);

        // A missing file reports false and leaves the built-in defaults alone.
        EdiShellWindow fresh;
        assert(!fresh.loadWorkspaceLayout(tempDir.filePath(QStringLiteral("absent.toml"))));
        assert(fresh.shellPanelVisibility(ShellSlot::Left) == PanelVisibility::Visible);

        // Hand-edited geometry actually lands in the splitter, clamped to the
        // slot's band on the way in.
        EdiShellWindow sized;
        {
            edi::formats::StaticConfig config;
            config["workspace.id"] = "drafting";
            config["binding.0.slot"] = "left";
            config["binding.0.feature"] = "drafting";
            config["panel.left.size"] = "400";
            config["panel.right.collapsed"] = "false";
            const QString sizedPath = tempDir.filePath(QStringLiteral("sized.toml"));
            assert(edi::io::saveSettingsToPath(sizedPath, config));
            sized.show();
            assert(sized.loadWorkspaceLayout(sizedPath));
            auto *bodySplitter = sized.findChild<QSplitter *>(QStringLiteral("bodySplitter"));
            assert(bodySplitter != nullptr);
            assert(bodySplitter->sizes().value(0) == 400);
        }
    }

    return 0;
}
