#include "widgets/EdiShellWindow.h"

#include "core/DrawingCore.h"
#include "io/SettingsStore.h"
#include "io/ShellLayoutStore.h"
#include "widgets/BeltCrossWidget.h"
#include "widgets/DraftingFeature.h"
#include "widgets/FloatingPalette.h"
#include "widgets/ShellTheme.h"

#include <QApplication>
#include <QCheckBox>
#include <QColor>
#include <QCoreApplication>
#include <QImage>
#include <QLabel>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QLineEdit>
#include <QPushButton>
#include <QListWidget>
#include <QMenu>
#include <QMouseEvent>
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

QAction *menuActionWithText(const QWidget &root, const QString &menuName, const QString &text)
{
    auto *menu = root.findChild<QMenu *>(menuName);
    if (menu == nullptr) {
        return nullptr;
    }
    for (QAction *action : menu->actions()) {
        if (action->text() == text) {
            return action;
        }
    }
    return nullptr;
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

    // N3: the inspector's metadata controls drive role/material/group/tags on
    // the selected object, and surface back through the projection.
    {
        QComboBox *roleCombo = window.findChild<QComboBox *>(QStringLiteral("objectRoleCombo"));
        auto *materialField = window.findChild<QLineEdit *>(QStringLiteral("objectMaterialField"));
        auto *groupField = window.findChild<QLineEdit *>(QStringLiteral("objectExportGroupField"));
        auto *tagsField = window.findChild<QLineEdit *>(QStringLiteral("objectTagsField"));
        assert(roleCombo != nullptr && materialField != nullptr && groupField != nullptr && tagsField != nullptr);

        roleCombo->setCurrentIndex(roleCombo->findData(QStringLiteral("wall")));
        assert(activeObject(*controller).value(QStringLiteral("role")).toString() == QStringLiteral("wall"));

        materialField->setText(QStringLiteral("concrete"));
        QMetaObject::invokeMethod(materialField, "editingFinished");
        assert(activeObject(*controller).value(QStringLiteral("material")).toString() == QStringLiteral("concrete"));

        groupField->setText(QStringLiteral("shell"));
        QMetaObject::invokeMethod(groupField, "editingFinished");
        assert(activeObject(*controller).value(QStringLiteral("export_group")).toString() == QStringLiteral("shell"));

        tagsField->setText(QStringLiteral("a, b,c"));
        QMetaObject::invokeMethod(tagsField, "editingFinished");
        assert(activeObject(*controller).value(QStringLiteral("tags")).toString() == QStringLiteral("a, b, c"));

        // The combo mirrors live state after a refresh (the role we set).
        assert(roleCombo->currentData().toString() == QStringLiteral("wall"));
    }

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

    // F2: the inspector is a context-keyed stack — planDraftingInspector
    // decides which groups show; everything else is hidden, never destroyed.
    {
        auto groupVisible = [&window](const QString &groupId) {
            QWidget *group = window.findChild<QWidget *>(QStringLiteral("inspectorGroup_") + groupId);
            assert(group != nullptr);
            // isHidden() reads the group's own flag — the right panel starts
            // collapsed, so ancestor-aware isVisibleTo() would see nothing.
            return !group->isHidden();
        };

        // A freshly drawn line is selected -> shape context.
        controller->setSelectedToolId(QStringLiteral("line_tool"));
        controller->clickCanvasNormalized(0.22, 0.72);
        controller->clickCanvasNormalized(0.42, 0.72);
        assert(activeObject(*controller).value(QStringLiteral("kind")).toString() == QStringLiteral("line"));
        assert(groupVisible(QStringLiteral("selection_summary")));
        assert(groupVisible(QStringLiteral("geometry")));
        assert(groupVisible(QStringLiteral("transform")));
        assert(groupVisible(QStringLiteral("object_guides")));
        assert(!groupVisible(QStringLiteral("guide_position")));
        assert(!groupVisible(QStringLiteral("guide_visuals")));
        assert(!groupVisible(QStringLiteral("dimension")));
        assert(!groupVisible(QStringLiteral("tool_polygon")));
        assert(!groupVisible(QStringLiteral("layers_document")));
        assert(!groupVisible(QStringLiteral("canvas_state")));
        assert(!groupVisible(QStringLiteral("empty_state")));

        // A selected guide swaps in the guide groups.
        controller->setSelectedToolId(QStringLiteral("horizontal_guide_tool"));
        controller->clickCanvasNormalized(0.5, 0.77);
        assert(activeObject(*controller).value(QStringLiteral("kind")).toString() == QStringLiteral("guide"));
        assert(groupVisible(QStringLiteral("guide_position")));
        assert(groupVisible(QStringLiteral("guide_visuals")));
        assert(groupVisible(QStringLiteral("geometry")));
        assert(!groupVisible(QStringLiteral("transform")));
        assert(!groupVisible(QStringLiteral("object_guides")));

        // Tool options ride along with any selection: creation auto-selects,
        // so hiding them would break the set-sides-then-draw loop.
        controller->setSelectedToolId(QStringLiteral("regular_polygon_tool"));
        assert(groupVisible(QStringLiteral("tool_polygon")));
        assert(groupVisible(QStringLiteral("guide_position")));

        // Drawing tool with no options, no selection -> quiet empty state.
        controller->setSelectedToolId(QStringLiteral("line_tool"));
        controller->selectObjectsInBoundsNormalized(0.001, 0.001, 0.002, 0.002);
        assert(controller->modelDocument().value(QStringLiteral("selected_object_ids")).toList().isEmpty());
        assert(groupVisible(QStringLiteral("empty_state")));
        assert(!groupVisible(QStringLiteral("selection_summary")));
        assert(!groupVisible(QStringLiteral("tool_polygon")));
        assert(!groupVisible(QStringLiteral("layers_document")));

        // Neutral select tool, no selection -> document configuration groups
        // (interim home until F4/F5 relocate them).
        controller->setSelectedToolId(QStringLiteral("select_move"));
        assert(groupVisible(QStringLiteral("layers_document")));
        assert(groupVisible(QStringLiteral("guides_document")));
        assert(groupVisible(QStringLiteral("calibration_document")));
        assert(groupVisible(QStringLiteral("document_info")));
        assert(groupVisible(QStringLiteral("canvas_state")));
        assert(!groupVisible(QStringLiteral("empty_state")));
        assert(!groupVisible(QStringLiteral("selection_summary")));
    }

    // Inspector de-bloat: heavy sections are disclosures. Defaults are data
    // (identity open, button armies folded), and the toggle actually folds.
    {
        QPushButton *canvasStateToggle = nullptr;
        QPushButton *selectedObjectToggle = nullptr;
        QPushButton *nudgeToggle = nullptr;
        for (QPushButton *toggle : window.findChildren<QPushButton *>(QStringLiteral("sectionToggle"))) {
            if (toggle->text().endsWith(QStringLiteral("Canvas State"))) {
                canvasStateToggle = toggle;
            } else if (toggle->text().endsWith(QStringLiteral("Selected Object"))) {
                selectedObjectToggle = toggle;
            } else if (toggle->text().endsWith(QStringLiteral("Nudge")) && !toggle->text().contains(QStringLiteral("Guide"))) {
                nudgeToggle = toggle;
            }
        }
        assert(canvasStateToggle != nullptr && !canvasStateToggle->isChecked());  // folded by default
        assert(selectedObjectToggle != nullptr && selectedObjectToggle->isChecked()); // open by default
        assert(nudgeToggle != nullptr && !nudgeToggle->isChecked());

        // Toggling shows the content. The fold hides the CONTENT BOX (the
        // grid panel the buttons live in), not each button — so the hidden
        // flag to read is the parent's.
        QPushButton *nudgeUp = buttonWithText(window, QStringLiteral("Grid Up"));
        assert(nudgeUp != nullptr && nudgeUp->parentWidget() != nullptr);
        assert(nudgeUp->parentWidget()->isHidden());
        nudgeToggle->click();
        assert(!nudgeUp->parentWidget()->isHidden());
        nudgeToggle->click();
        assert(nudgeUp->parentWidget()->isHidden());
    }

    // F3: the weapon-cross tool belt drives the controller and follows it.
    {
        auto *belt = window.findChild<BeltCrossWidget *>(QStringLiteral("beltCross"));
        assert(belt != nullptr);
        // The block above left select_move active; the default belt has it
        // at the origin, so belt and controller agree from the start.
        assert(belt->activeItemId() == controller->selectedToolId());
        // One row per tool, six columns of sub-feature room.
        assert(belt->beltState().rows == 10 && belt->beltState().columns == 6);

        // Belt -> controller: click the bottom peek — one step down the tool
        // carousel, from select_move's row to the point tool's row.
        const QPointF bottomPeek(14.0 + 17.0, 67.0); // peek band centre, right of the 14px nub gutter
        QMouseEvent press(QEvent::MouseButtonPress, bottomPeek, belt->mapToGlobal(bottomPeek),
                          Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
        QApplication::sendEvent(belt, &press);
        assert(controller->selectedToolId() == QStringLiteral("point_tool"));
        assert(belt->activeItemId() == QStringLiteral("point_tool"));

        // Controller -> belt: a tool change from any path re-aims the cross
        // (and lands on the dimension tool's row of the default belt).
        controller->setSelectedToolId(QStringLiteral("distance_dimension_tool"));
        assert(belt->activeItemId() == QStringLiteral("distance_dimension_tool"));
        assert(belt->beltState().activeRow == 9);

        controller->setSelectedToolId(QStringLiteral("select_move"));
        assert(belt->activeItemId() == QStringLiteral("select_move"));
    }

    // F4: the belt floats in a palette over the main area — draggable by its
    // grip, position remembered in the workspace layout, torn down and
    // rebuilt with the workspace.
    {
        window.show();
        QApplication::processEvents();

        auto *palette = window.findChild<FloatingPalette *>(QStringLiteral("floatingPalette"));
        assert(palette != nullptr);
        assert(palette->paletteId() == QStringLiteral("tool_belt"));

        // The belt lives inside the palette frame now, not in the left panel.
        auto *belt = window.findChild<BeltCrossWidget *>(QStringLiteral("beltCross"));
        assert(belt != nullptr && palette->isAncestorOf(belt));
        QWidget *leftPanel = window.findChild<QWidget *>(QStringLiteral("leftPanel"));
        assert(leftPanel != nullptr && !leftPanel->isAncestorOf(belt));

        // Default placement, clamped against a real-sized main area.
        assert(palette->pos() == QPoint(12, 12));

        // Drag by the grab nub: press, move (+30,+20), release.
        QWidget *grip = palette->findChild<QWidget *>(QStringLiteral("paletteGrip"));
        assert(grip != nullptr);
        const QPointF gripPoint(QRectF(grip->geometry()).center());
        QMouseEvent press(QEvent::MouseButtonPress, gripPoint, palette->mapToGlobal(gripPoint),
                          Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
        QApplication::sendEvent(palette, &press);
        const QPointF dragTo = gripPoint + QPointF(30.0, 20.0);
        QMouseEvent drag(QEvent::MouseMove, dragTo, palette->mapToGlobal(dragTo),
                         Qt::NoButton, Qt::LeftButton, Qt::NoModifier);
        QApplication::sendEvent(palette, &drag);
        QMouseEvent release(QEvent::MouseButtonRelease, dragTo, palette->mapToGlobal(dragTo),
                            Qt::LeftButton, Qt::NoButton, Qt::NoModifier);
        QApplication::sendEvent(palette, &release);
        assert(palette->pos() == QPoint(42, 32));

        // The drag landed in the layout: saving writes palette.0.* rows.
        {
            QTemporaryDir tempDir;
            assert(tempDir.isValid());
            const QString path = tempDir.filePath(QStringLiteral("palette_workspace.toml"));
            assert(window.saveWorkspaceLayout(path));
            const edi::io::ShellLayoutData saved = edi::io::loadShellLayoutFromPath(path);
            assert(saved.ok);
            const edi::shell::PalettePlacement stored =
                edi::shell::palettePlacement(saved.layout, QStringLiteral("tool_belt"));
            assert(stored.x == 42 && stored.y == 32);
        }

        // Workspace switch: a layout without the drafting feature has no
        // palettes; switching to a drafting layout that remembers a spot
        // rebuilds the palette there (clamped).
        edi::shell::WorkspaceLayout settingsJob;
        settingsJob.id = QStringLiteral("settings");
        settingsJob.label = QStringLiteral("Settings");
        settingsJob.bindings = {{edi::shell::ShellSlot::Main, QStringLiteral("settings")}};
        window.switchWorkspaceLayout(settingsJob);
        assert(window.findChild<FloatingPalette *>(QStringLiteral("floatingPalette")) == nullptr);

        edi::shell::WorkspaceLayout draftingJob;
        draftingJob.id = QStringLiteral("drafting");
        draftingJob.label = QStringLiteral("Drafting");
        draftingJob.bindings = {
            {edi::shell::ShellSlot::Left, QStringLiteral("drafting")},
            {edi::shell::ShellSlot::Main, QStringLiteral("drafting")},
            {edi::shell::ShellSlot::Right, QStringLiteral("drafting")},
            {edi::shell::ShellSlot::Bottom, QStringLiteral("drafting")},
        };
        draftingJob.belt = DraftingFeature::defaultBeltLayout();
        edi::shell::setPalettePlacement(draftingJob, {QStringLiteral("tool_belt"), 60, 44});
        window.switchWorkspaceLayout(draftingJob);
        QApplication::processEvents();
        auto *rebuilt = window.findChild<FloatingPalette *>(QStringLiteral("floatingPalette"));
        assert(rebuilt != nullptr);
        assert(rebuilt->pos() == QPoint(60, 44));

        // A stored position far off-screen clamps back into the main area.
        edi::shell::WorkspaceLayout farJob = draftingJob;
        edi::shell::setPalettePlacement(farJob, {QStringLiteral("tool_belt"), 99999, 99999});
        window.switchWorkspaceLayout(farJob);
        QApplication::processEvents();
        auto *clamped = window.findChild<FloatingPalette *>(QStringLiteral("floatingPalette"));
        assert(clamped != nullptr);
        QWidget *mainArea = window.findChild<QWidget *>(QStringLiteral("workspaceColumn"));
        assert(mainArea != nullptr);
        assert(clamped->geometry().right() <= mainArea->width());
        assert(clamped->geometry().bottom() <= mainArea->height());
    }

    // Snap settings live behind a chrome "Snap" button now: the popup holds
    // the same controls (the initial-sync block above already proved their
    // wiring), and the button tears down with workspaces that lack drafting.
    {
        auto *snapButton = window.findChild<QPushButton *>(QStringLiteral("chromePanel_snap"));
        assert(snapButton != nullptr);
        auto *snapPopup = window.findChild<QWidget *>(QStringLiteral("chromePopup_snap"));
        assert(snapPopup != nullptr);
        QCheckBox *popupGridSnap = toggleWithLabel(*snapPopup, QStringLiteral("Grid snap"));
        assert(popupGridSnap != nullptr); // the controls moved INTO the popup
        QWidget *leftPanel = window.findChild<QWidget *>(QStringLiteral("leftPanel"));
        assert(leftPanel != nullptr && toggleWithLabel(*leftPanel, QStringLiteral("Grid snap")) == nullptr);

        // The left panel kept only navigation: no undo/redo buttons, no
        // project-file buttons, no placeholder sections.
        assert(leftPanel->findChild<QPushButton *>(QStringLiteral("undoButton")) == nullptr);
        assert(leftPanel->findChild<QPushButton *>(QStringLiteral("openDrawingButton")) == nullptr);

        edi::shell::WorkspaceLayout settingsOnly;
        settingsOnly.id = QStringLiteral("settings");
        settingsOnly.label = QStringLiteral("Settings");
        settingsOnly.bindings = {{edi::shell::ShellSlot::Main, QStringLiteral("settings")}};
        window.switchWorkspaceLayout(settingsOnly);
        assert(window.findChild<QPushButton *>(QStringLiteral("chromePanel_snap")) == nullptr);

        edi::shell::WorkspaceLayout draftingBack;
        draftingBack.id = QStringLiteral("drafting");
        draftingBack.label = QStringLiteral("Drafting");
        draftingBack.bindings = {
            {edi::shell::ShellSlot::Left, QStringLiteral("drafting")},
            {edi::shell::ShellSlot::Main, QStringLiteral("drafting")},
            {edi::shell::ShellSlot::Right, QStringLiteral("drafting")},
            {edi::shell::ShellSlot::Bottom, QStringLiteral("drafting")},
        };
        draftingBack.belt = DraftingFeature::defaultBeltLayout();
        window.switchWorkspaceLayout(draftingBack);
        assert(window.findChild<QPushButton *>(QStringLiteral("chromePanel_snap")) != nullptr);
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
        // Project-file verbs live in the File menu now (left panel slimmed).
        assert(menuActionWithText(window, QStringLiteral("fileMenu"), QStringLiteral("Open…")) != nullptr);
        assert(menuActionWithText(window, QStringLiteral("fileMenu"), QStringLiteral("Save")) != nullptr);
        assert(menuActionWithText(window, QStringLiteral("fileMenu"), QStringLiteral("Save As…")) != nullptr);

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
        QAction *undoButton = window.findChild<QAction *>(QStringLiteral("undoAction"));
        QAction *redoButton = window.findChild<QAction *>(QStringLiteral("redoAction"));
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

        undoButton->trigger();
        assert(controller->modelDocument().value(QStringLiteral("drawing_objects")).toList().size() == before);
        assert(redoButton->isEnabled());

        redoButton->trigger();
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

    // N4 rectangle tool options: radius / inset spins and the aspect-lock
    // toggle drive the controller's rectangle modes.
    {
        auto *radiusSpin = window.findChild<QDoubleSpinBox *>(QStringLiteral("rectCornerRadiusSpin"));
        auto *insetSpin = window.findChild<QDoubleSpinBox *>(QStringLiteral("rectInsetSpin"));
        auto *aspectLock = window.findChild<QCheckBox *>(QStringLiteral("aspectLockCheckbox"));
        assert(radiusSpin != nullptr && insetSpin != nullptr && aspectLock != nullptr);
        radiusSpin->setValue(0.06);
        assert(near(controller->rectCornerRadius(), 0.06));
        insetSpin->setValue(0.03);
        assert(near(controller->rectInset(), 0.03));
        assert(!controller->aspectLockEnabled());
        aspectLock->setChecked(true);
        assert(controller->aspectLockEnabled());
    }

    // Export buttons exist and the path seams write SVG / HPGL files.
    {
        assert(menuActionWithText(window, QStringLiteral("fileMenu"), QStringLiteral("Export SVG…")) != nullptr);
        assert(menuActionWithText(window, QStringLiteral("fileMenu"), QStringLiteral("Export HPGL…")) != nullptr);

        controller->setSelectedToolId(QStringLiteral("line_tool"));
        controller->clickCanvasNormalized(0.2, 0.2);
        controller->clickCanvasNormalized(0.8, 0.8);

        QTemporaryDir tempDir;
        assert(tempDir.isValid());
        const QString svgPath = tempDir.filePath(QStringLiteral("shell.svg"));
        const QString hpglPath = tempDir.filePath(QStringLiteral("shell.hpgl"));
        const QString gcodePath = tempDir.filePath(QStringLiteral("shell.gcode"));
        assert(window.exportSvgToPath(svgPath));
        assert(window.exportHpglToPath(hpglPath));
        assert(window.exportGcodeToPath(gcodePath)); // N5 export seam
        assert(QFile::exists(svgPath));
        assert(QFile::exists(hpglPath));
        assert(QFile::exists(gcodePath));
        {
            QFile f(gcodePath);
            assert(f.open(QIODevice::ReadOnly));
            const QString body = QString::fromUtf8(f.readAll());
            assert(body.startsWith(QStringLiteral("G21\n"))); // real G-code, not empty
            assert(body.contains(QStringLiteral("G1 ")));     // a stroke was emitted
        }
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

    // Recent files: saving a drawing records it and surfaces an Open Recent
    // entry (the File menu took over from the left panel's quick buttons).
    {
        QTemporaryDir tempDir;
        assert(tempDir.isValid());
        const QString drawingPath = tempDir.filePath(QStringLiteral("recent.edidraw"));
        assert(window.saveDrawingToPath(drawingPath));
        assert(window.recentFiles().contains(drawingPath));
        auto *recentMenu = window.findChild<QMenu *>(QStringLiteral("recentFilesMenu"));
        assert(recentMenu != nullptr);
        bool listed = false;
        for (QAction *action : recentMenu->actions()) {
            if (action->data().toString() == drawingPath) {
                listed = true;
            }
        }
        assert(listed);
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

        // Drag limits surface as widget constraints. Bottom has no maximum:
        // the terminal may grow to become the main view.
        assert(leftPanel->minimumWidth() == 180);
        assert(leftPanel->maximumWidth() == 520);
        assert(rightPanel->minimumWidth() == 160);
        assert(bottomPanel->minimumHeight() == 96);
        assert(bottomPanel->maximumHeight() > 100000); // QWIDGETSIZE_MAX, i.e. unbounded

        // Overlay behavior: the grid never resizes when panels come and go —
        // they cover it. The canvas always fills the whole main area.
        window.applyShellPanelPreset(PanelPreset::Full);
        QCoreApplication::processEvents();
        QWidget *canvas = window.findChild<QWidget *>(QStringLiteral("drawingCanvas"));
        QWidget *mainArea = window.findChild<QWidget *>(QStringLiteral("workspaceColumn"));
        assert(canvas != nullptr && mainArea != nullptr);
        const QSize canvasWithPanels = canvas->size();
        assert(canvasWithPanels == mainArea->size());
        window.setPanelCollapsed(ShellSlot::Right, true);
        window.setPanelCollapsed(ShellSlot::Bottom, true);
        assert(canvas->size() == canvasWithPanels); // unchanged: overlays, not siblings
        window.setPanelCollapsed(ShellSlot::Right, false);
        window.setPanelCollapsed(ShellSlot::Bottom, false);
        assert(canvas->size() == canvasWithPanels);
        // The right overlay hugs the right edge; the bottom overlay spans the
        // full width and sits on the bottom edge.
        assert(rightPanel->geometry().right() + 1 == mainArea->width());
        assert(bottomPanel->width() == mainArea->width());
        assert(bottomPanel->geometry().bottom() + 1 == mainArea->height());
        assert(rightPanel->geometry().bottom() < bottomPanel->geometry().top() + 1);

        // The terminal grows over the grid via its grip — up to the whole
        // main area — without the grid moving.
        QWidget *bottomGrip = window.findChild<QWidget *>(QStringLiteral("bottomPanelGrip"));
        assert(bottomGrip != nullptr && bottomGrip->isVisibleTo(&window));
        const int beforeDrag = bottomPanel->height();
        {
            const QPointF local(4.0, 4.0);
            const QPointF global = mainArea->mapToGlobal(QPoint(200, 10)); // drag near the top
            QMouseEvent move(QEvent::MouseMove, local, local, global,
                Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
            QCoreApplication::sendEvent(bottomGrip, &move);
        }
        assert(bottomPanel->height() > beforeDrag);
        assert(bottomPanel->height() >= mainArea->height() - 10); // effectively the main window
        assert(canvas->size() == canvasWithPanels);               // grid untouched

        // Leave everything open and at defaults for later assertions.
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
        // slot's band on the way in. (All four slots bound: this fixture is
        // about geometry import, not about binding changes.)
        EdiShellWindow sized;
        {
            edi::formats::StaticConfig config;
            config["workspace.id"] = "drafting";
            config["binding.0.slot"] = "left";
            config["binding.0.feature"] = "drafting";
            config["binding.1.slot"] = "main";
            config["binding.1.feature"] = "drafting";
            config["binding.2.slot"] = "right";
            config["binding.2.feature"] = "drafting";
            config["binding.3.slot"] = "bottom";
            config["binding.3.feature"] = "drafting";
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

    // Workspace switching: tear down the mounted slots, rebuild from another
    // layout. The document lives in the controller and must be untouched —
    // switching changes the glass, never what is behind it.
    {
        using edi::shell::PanelVisibility;
        using edi::shell::ShellSlot;
        using edi::shell::WorkspaceLayout;

        EdiShellWindow shell;
        shell.show();
        auto *shellController = shell.findChild<DrawingDocumentController *>();
        assert(shellController != nullptr);

        // Draw something so there is a document to preserve.
        shellController->setSelectedToolId(QStringLiteral("point_tool"));
        shellController->clickCanvasNormalized(0.5, 0.5);
        const int objectsBefore = objectCount(*shellController);
        assert(objectsBefore > 0);

        WorkspaceLayout canvasOnly;
        canvasOnly.id = QStringLiteral("canvas_only");
        canvasOnly.label = QStringLiteral("Canvas only");
        canvasOnly.bindings = {{ShellSlot::Main, QStringLiteral("drafting")}};
        shell.switchWorkspaceLayout(canvasOnly);

        // Old panels are gone (immediate delete — no flush needed), the canvas
        // is fresh, and the document survived the teardown.
        assert(shell.findChild<QWidget *>(QStringLiteral("leftPanel")) == nullptr);
        assert(shell.findChild<QWidget *>(QStringLiteral("rightPanel")) == nullptr);
        assert(shell.findChild<QWidget *>(QStringLiteral("bottomPanel")) == nullptr);
        assert(shell.findChild<QWidget *>(QStringLiteral("drawingCanvas")) != nullptr);
        assert(objectCount(*shellController) == objectsBefore);

        // Model changes must not crash the partial-layout inspector (it is
        // wired to modelChanged on the fresh feature instance).
        shellController->clickCanvasNormalized(0.25, 0.25);
        assert(objectCount(*shellController) == objectsBefore + 1);

        // Switch back to the full drafting job: panels return, and the rebuilt
        // inspector tracks the live selection (the click above selected an
        // object, so selection-conditional buttons must be enabled).
        WorkspaceLayout full;
        full.id = QStringLiteral("drafting");
        full.label = QStringLiteral("Drafting");
        full.bindings = {
            {ShellSlot::Left, QStringLiteral("drafting")},
            {ShellSlot::Main, QStringLiteral("drafting")},
            {ShellSlot::Right, QStringLiteral("drafting")},
            {ShellSlot::Bottom, QStringLiteral("drafting")},
        };
        shell.switchWorkspaceLayout(full);
        assert(shell.findChild<QWidget *>(QStringLiteral("leftPanel")) != nullptr);
        assert(shell.findChild<QWidget *>(QStringLiteral("rightPanel")) != nullptr);
        QPushButton *fitButton = buttonNamed(shell, QStringLiteral("fitToDrawableButton"));
        assert(fitButton != nullptr);
        assert(fitButton->isEnabled());
        assert(objectCount(*shellController) == objectsBefore + 1);

        // Persistence round-trips bindings: save the canvas-only job, load it
        // into a fresh window, and the fresh window switches to it.
        shell.switchWorkspaceLayout(canvasOnly);
        QTemporaryDir tempDir;
        assert(tempDir.isValid());
        const QString jobPath = tempDir.filePath(QStringLiteral("job.toml"));
        assert(shell.saveWorkspaceLayout(jobPath));

        EdiShellWindow restored;
        assert(restored.findChild<QWidget *>(QStringLiteral("leftPanel")) != nullptr);
        assert(restored.loadWorkspaceLayout(jobPath));
        assert(restored.findChild<QWidget *>(QStringLiteral("leftPanel")) == nullptr);
        assert(restored.findChild<QWidget *>(QStringLiteral("drawingCanvas")) != nullptr);
    }

    // F1 — the object list: a browsable projection of the document. It
    // mirrors object count, tracks selection both ways, and selectObjectById
    // is selection-only (no undo step, same rule as marquee).
    {
        auto *objectList = window.findChild<QListWidget *>(QStringLiteral("objectList"));
        assert(objectList != nullptr);
        const int documentObjects = objectCount(*controller);
        assert(objectList->count() == documentObjects);
        assert(documentObjects >= 2); // earlier sections created point(s) + guide

        // Select by id through the controller; the list's current row follows.
        const QString firstId = objectList->item(0)->data(Qt::UserRole).toString();
        assert(!firstId.isEmpty());
        assert(window.findChild<DrawingDocumentController *>()->selectObjectById(firstId));
        assert(controller->selectedObjectId() == firstId);
        assert(objectList->currentItem() != nullptr);
        assert(objectList->currentItem()->data(Qt::UserRole).toString() == firstId);

        // A bogus id is rejected and selection is untouched.
        assert(!controller->selectObjectById(QStringLiteral("no-such-object")));
        assert(controller->selectedObjectId() == firstId);

        // Creating an object grows the list (the list is a live projection).
        controller->setSelectedToolId(QStringLiteral("point_tool"));
        controller->clickCanvasNormalized(0.6, 0.6);
        assert(objectList->count() == documentObjects + 1);
    }

    // Live theming: setting the four inputs re-derives the stylesheet, the
    // change survives a workspace switch (fresh canvas gets the live theme),
    // and the inputs round-trip through edi.toml.
    {
        using edi::shell::ShellSlot;
        using edi::shell::ShellThemeInputs;
        using edi::shell::WorkspaceLayout;
        using edi::shell::deriveShellTheme;

        EdiShellWindow themed;
        ShellThemeInputs pink;
        pink.accent = QStringLiteral("#d46ca1");
        pink.uiFontSize = 14;
        themed.setThemeInputs(pink);

        const auto derived = deriveShellTheme(pink);
        assert(themed.styleSheet().contains(derived.selected));      // accent-derived token landed
        assert(themed.styleSheet().contains(QStringLiteral("font-size: 14px")));
        assert(themed.themeInputs().accent == pink.accent);

        // A workspace switch rebuilds the canvas; the live theme must follow.
        WorkspaceLayout canvasOnly;
        canvasOnly.id = QStringLiteral("canvas_only");
        canvasOnly.bindings = {{ShellSlot::Main, QStringLiteral("drafting")}};
        themed.switchWorkspaceLayout(canvasOnly);
        assert(themed.styleSheet().contains(derived.selected));

        // Round trip through the settings file into a fresh window.
        QTemporaryDir tempDir;
        assert(tempDir.isValid());
        const QString settingsPath = tempDir.filePath(QStringLiteral("edi.toml"));
        assert(themed.saveSettings(settingsPath));
        EdiShellWindow reloaded;
        assert(!reloaded.styleSheet().contains(derived.selected)); // stock theme before load
        assert(reloaded.loadSettings(settingsPath));
        assert(reloaded.themeInputs().accent == pink.accent);
        assert(reloaded.themeInputs().uiFontSize == 14);
        assert(reloaded.styleSheet().contains(derived.selected));
    }

    // F5 — the settings pop-out: the rail's S button opens a modeless tool
    // window over the canvas (theme edits stay live-visible); the drafting
    // workspace never unmounts.
    {
        using edi::shell::ShellThemeInputs;
        using edi::shell::deriveShellTheme;

        EdiShellWindow shell;
        shell.show();

        // Find the rail button carrying the settings mode and click it.
        QPushButton *settingsRail = nullptr;
        for (QPushButton *button : shell.findChildren<QPushButton *>(QStringLiteral("railButton"))) {
            if (button->property("modeId").toString() == QStringLiteral("settings")) {
                settingsRail = button;
            }
        }
        assert(settingsRail != nullptr && settingsRail->isEnabled());
        settingsRail->click();

        // The drafting job stays mounted; the settings page floats above it.
        assert(shell.findChild<QWidget *>(QStringLiteral("drawingCanvas")) != nullptr);
        assert(shell.findChild<QWidget *>(QStringLiteral("leftPanel")) != nullptr);
        QWidget *popOut = shell.findChild<QWidget *>(QStringLiteral("settingsWindow"));
        assert(popOut != nullptr && popOut->isVisible());
        QWidget *page = shell.findChild<QWidget *>(QStringLiteral("settingsPanel"));
        assert(page != nullptr && popOut->isAncestorOf(page));
        // The rail check stays on the mounted workspace, not on S.
        assert(!settingsRail->isChecked());

        // Typing a valid accent hex re-themes the window immediately; a
        // half-typed value is ignored.
        auto *accentField = shell.findChild<QLineEdit *>(QStringLiteral("themeAccentField"));
        assert(accentField != nullptr);
        assert(accentField->text() == shell.themeInputs().accent); // page mirrors live state
        accentField->setText(QStringLiteral("#d46c"));             // incomplete: no change
        assert(shell.themeInputs().accent != QStringLiteral("#d46c"));
        accentField->setText(QStringLiteral("#d46ca1"));
        assert(shell.themeInputs().accent == QStringLiteral("#d46ca1"));
        ShellThemeInputs expected = shell.themeInputs();
        assert(shell.styleSheet().contains(deriveShellTheme(expected).selected));

        // Font size flows the same way.
        auto *sizeSpin = shell.findChild<QSpinBox *>(QStringLiteral("uiFontSizeSpin"));
        assert(sizeSpin != nullptr);
        sizeSpin->setValue(15);
        assert(shell.themeInputs().uiFontSize == 15);
        assert(shell.styleSheet().contains(QStringLiteral("font-size: 15px")));

        // F6 — the Tool Belt page: a checklist over the tool inventory that
        // writes the workspace's belt and re-dresses the live belt in place.
        {
            QPushButton *beltPageButton = nullptr;
            for (QPushButton *button : shell.findChildren<QPushButton *>(QStringLiteral("settingsPageButton"))) {
                if (button->property("pageId").toString() == QStringLiteral("tool_belt")) {
                    beltPageButton = button;
                }
            }
            assert(beltPageButton != nullptr);
            beltPageButton->click();

            // Default belt: every tool is on it, so every box starts checked.
            QCheckBox *pointBox = nullptr;
            int checkedCount = 0;
            for (QCheckBox *box : shell.findChildren<QCheckBox *>(QStringLiteral("beltToolCheckbox"))) {
                checkedCount += box->isChecked() ? 1 : 0;
                if (box->property("toolId").toString() == QStringLiteral("point_tool")) {
                    pointBox = box;
                }
            }
            assert(pointBox != nullptr && pointBox->isChecked());
            assert(checkedCount == 19); // the full drafting inventory

            auto *belt = shell.findChild<BeltCrossWidget *>(QStringLiteral("beltCross"));
            assert(belt != nullptr);
            assert(belt->indexOfItem(QStringLiteral("point_tool")) >= 0);

            // Uncheck: the live belt loses the tool, and the workspace TOML
            // would save without it.
            pointBox->setChecked(false);
            assert(belt->indexOfItem(QStringLiteral("point_tool")) == -1);
            assert(belt->indexOfItem(QStringLiteral("line_tool")) >= 0); // others untouched
            {
                QTemporaryDir beltDir;
                assert(beltDir.isValid());
                const QString beltPath = beltDir.filePath(QStringLiteral("belt.toml"));
                assert(shell.saveWorkspaceLayout(beltPath));
                const edi::io::ShellLayoutData saved = edi::io::loadShellLayoutFromPath(beltPath);
                bool hasPoint = false;
                for (const QString &id : saved.layout.belt.itemIds) {
                    hasPoint = hasPoint || id == QStringLiteral("point_tool");
                }
                assert(!hasPoint);
            }

            // Re-check: the tool returns to its row.
            pointBox->setChecked(true);
            assert(belt->indexOfItem(QStringLiteral("point_tool")) >= 0);
        }

        // Closing the pop-out hides it; the theme survives, and reopening
        // through the rail brings the same frame back.
        popOut->close();
        assert(!popOut->isVisible());
        assert(shell.themeInputs().accent == QStringLiteral("#d46ca1"));
        settingsRail->click();
        assert(popOut->isVisible());

        // Profiles: snapshot the current theme under a name via the page's
        // save button, scramble, then load the snapshot back.
        QTemporaryDir profileDir;
        assert(profileDir.isValid());
        shell.setProfilesDirectory(profileDir.path());

        // Switch workspaces (away and back) so the page rebuilds with the
        // profile hooks now that the profiles directory is set — the pop-out
        // frame and its visibility survive the remount.
        edi::shell::WorkspaceLayout reset;
        reset.id = QStringLiteral("blank");
        reset.label = QStringLiteral("Blank");
        reset.bindings = {{edi::shell::ShellSlot::Main, QStringLiteral("drafting")}};
        shell.switchWorkspaceLayout(reset);
        assert(shell.findChild<QWidget *>(QStringLiteral("settingsWindow"))->isVisible());
        assert(shell.findChild<QWidget *>(QStringLiteral("settingsPanel")) != nullptr);
        auto *nameField = shell.findChild<QLineEdit *>(QStringLiteral("profileNameField"));
        auto *saveProfile = shell.findChild<QPushButton *>(QStringLiteral("saveProfileButton"));
        auto *profileCombo = shell.findChild<QComboBox *>(QStringLiteral("profileCombo"));
        assert(nameField != nullptr && saveProfile != nullptr && profileCombo != nullptr);
        assert(profileCombo->count() == 0); // empty dir, no profiles yet

        nameField->setText(QStringLiteral("pink lab"));
        saveProfile->click();
        assert(shell.availableProfiles() == QStringList{QStringLiteral("pink lab")});
        assert(shell.activeProfile() == QStringLiteral("pink lab"));
        assert(profileCombo->count() == 1); // the combo re-listed itself

        // Scramble the theme, then load the profile back through the API.
        ShellThemeInputs scrambled = shell.themeInputs();
        scrambled.accent = QStringLiteral("#11aa22");
        shell.setThemeInputs(scrambled);
        assert(shell.themeInputs().accent == QStringLiteral("#11aa22"));
        assert(shell.loadProfile(QStringLiteral("pink lab")));
        assert(shell.themeInputs().accent == QStringLiteral("#d46ca1"));

        // A hostile name degrades to its cleaned form instead of escaping the
        // profiles directory; a missing profile load keeps the current theme.
        assert(shell.saveProfileAs(QStringLiteral("../evil/../../name")));
        assert(shell.availableProfiles().contains(QStringLiteral("evilname")));
        assert(!shell.loadProfile(QStringLiteral("never-saved")));
        assert(shell.themeInputs().accent == QStringLiteral("#d46ca1"));

        // The active profile name rides along in edi.toml.
        QTemporaryDir settingsDir;
        assert(settingsDir.isValid());
        const QString settingsPath = settingsDir.filePath(QStringLiteral("edi.toml"));
        assert(shell.loadProfile(QStringLiteral("pink lab"))); // make it active again
        assert(shell.saveSettings(settingsPath));
        EdiShellWindow remembered;
        assert(remembered.loadSettings(settingsPath));
        assert(remembered.activeProfile() == QStringLiteral("pink lab"));
    }

    // Title-bar chrome: frameless flag, traffic lights, panel toggles, the
    // back/forward workspace trail, and the File/Edit/Settings menus.
    {
        using edi::shell::PanelVisibility;
        using edi::shell::ShellSlot;
        using edi::shell::WorkspaceLayout;

        EdiShellWindow chrome;
        chrome.show();
        assert(chrome.windowFlags().testFlag(Qt::FramelessWindowHint));

        QWidget *titleBar = chrome.findChild<QWidget *>(QStringLiteral("titleBar"));
        assert(titleBar != nullptr && titleBar->height() == 42);

        // Panel toggles drive the modeled state and mirror it back as checked.
        QPushButton *leftToggle = buttonNamed(chrome, QStringLiteral("toggleLeftPanel"));
        QPushButton *rightToggle = buttonNamed(chrome, QStringLiteral("toggleRightPanel"));
        QPushButton *bottomToggle = buttonNamed(chrome, QStringLiteral("toggleBottomPanel"));
        assert(leftToggle != nullptr && rightToggle != nullptr && bottomToggle != nullptr);
        assert(leftToggle->isChecked());    // left starts open
        assert(!rightToggle->isChecked());  // right starts collapsed
        leftToggle->click();
        assert(chrome.shellPanelVisibility(ShellSlot::Left) == PanelVisibility::Collapsed);
        assert(!leftToggle->isChecked());
        leftToggle->click();
        assert(chrome.shellPanelVisibility(ShellSlot::Left) == PanelVisibility::Visible);
        rightToggle->click();
        assert(chrome.shellPanelVisibility(ShellSlot::Right) == PanelVisibility::Visible);

        // Back/forward walk the workspace trail; a fresh trail has one entry.
        QPushButton *back = buttonNamed(chrome, QStringLiteral("workspaceBack"));
        QPushButton *forward = buttonNamed(chrome, QStringLiteral("workspaceForward"));
        assert(back != nullptr && forward != nullptr);
        assert(!back->isEnabled() && !forward->isEnabled());

        WorkspaceLayout canvasOnly;
        canvasOnly.id = QStringLiteral("canvas_only");
        canvasOnly.bindings = {{ShellSlot::Main, QStringLiteral("drafting")}};
        chrome.switchWorkspaceLayout(canvasOnly);
        assert(back->isEnabled() && !forward->isEnabled());
        assert(chrome.findChild<QWidget *>(QStringLiteral("leftPanel")) == nullptr);

        back->click(); // back to the full drafting job
        assert(chrome.findChild<QWidget *>(QStringLiteral("leftPanel")) != nullptr);
        assert(!back->isEnabled() && forward->isEnabled());
        forward->click();
        assert(chrome.findChild<QWidget *>(QStringLiteral("leftPanel")) == nullptr);
        assert(back->isEnabled() && !forward->isEnabled());

        // Going back and switching somewhere new truncates the forward trail.
        back->click();
        assert(forward->isEnabled());
        WorkspaceLayout third; // any distinct job
        third.id = QStringLiteral("third");
        third.bindings = {{ShellSlot::Main, QStringLiteral("drafting")}, {ShellSlot::Right, QStringLiteral("drafting")}};
        chrome.switchWorkspaceLayout(third);
        assert(!forward->isEnabled() && back->isEnabled());
        // The truncation is observable in where back lands: one step behind
        // "third" must be the full drafting job, not the stale canvas-only
        // entry the truncation discarded.
        back->click();
        assert(chrome.findChild<QWidget *>(QStringLiteral("leftPanel")) != nullptr);
        assert(!back->isEnabled()); // i.e. the trail is exactly two entries deep
        forward->click();           // return to "third" for the sections below

        // Menus: File carries the IO verbs (not triggered — they open
        // dialogs); Edit's Undo really undoes; Settings applies presets.
        auto *fileMenu = chrome.findChild<QMenu *>(QStringLiteral("fileMenu"));
        auto *editMenu = chrome.findChild<QMenu *>(QStringLiteral("editMenu"));
        auto *settingsMenu = chrome.findChild<QMenu *>(QStringLiteral("settingsMenu"));
        assert(fileMenu != nullptr && editMenu != nullptr && settingsMenu != nullptr);
        assert(fileMenu->actions().size() == 8); // 6 verbs + Open Recent + separator

        auto *chromeController = chrome.findChild<DrawingDocumentController *>();
        assert(chromeController != nullptr);
        chromeController->setSelectedToolId(QStringLiteral("point_tool"));
        chromeController->clickCanvasNormalized(0.4, 0.4);
        const int before = objectCount(*chromeController);
        editMenu->actions().at(0)->trigger(); // Undo
        assert(objectCount(*chromeController) == before - 1);
        editMenu->actions().at(1)->trigger(); // Redo
        assert(objectCount(*chromeController) == before);

        settingsMenu->actions().at(1)->trigger(); // Focus layout
        assert(chrome.shellPanelVisibility(ShellSlot::Left) == PanelVisibility::Collapsed);
        assert(chrome.shellPanelVisibility(ShellSlot::Right) == PanelVisibility::Collapsed);

        // Traffic close really closes the window.
        QPushButton *closeButton = buttonNamed(chrome, QStringLiteral("trafficClose"));
        assert(closeButton != nullptr);
        assert(chrome.isVisible());
        closeButton->click();
        assert(!chrome.isVisible());
    }

    // Render proof: panel surfaces and the object list must paint THEME
    // tokens, not the platform palette. QScrollArea::setWidget() force-enables
    // autoFillBackground on the content widget, which once painted macOS light
    // gray edge to edge over the styled frames. Every property read looked
    // correct — only the rendered image catches that class of bug, so this
    // asserts on pixels.
    {
        EdiShellWindow proof;
        proof.resize(1100, 760);
        proof.show();
        QCoreApplication::processEvents();

        const edi::shell::ShellTheme theme =
            edi::shell::deriveShellTheme(edi::shell::ShellThemeInputs{});
        const QImage rendered = proof.grab().toImage();
        assert(rendered.devicePixelRatio() == 1.0); // offscreen: probe == pixel

        QWidget *leftPanel = proof.findChild<QWidget *>(QStringLiteral("leftPanel"));
        assert(leftPanel != nullptr && leftPanel->isVisible());
        // Probe the panel's bottom stretch area — no child widget owns it, so
        // the color is the panel frame's own fill.
        const QPoint panelProbe = leftPanel->mapTo(
            &proof, QPoint(leftPanel->width() / 2, leftPanel->height() - 8));
        assert(QColor(rendered.pixel(panelProbe)).name() == theme.surface);

        auto *objectList = proof.findChild<QListWidget *>(QStringLiteral("objectList"));
        assert(objectList != nullptr);
        const QPoint listProbe = objectList->viewport()->mapTo(
            &proof, QPoint(objectList->viewport()->width() / 2,
                           objectList->viewport()->height() / 2));
        assert(QColor(rendered.pixel(listProbe)).name() == theme.surface);

        // Traffic lights: spec-constant fills, hit area exactly the 14px dot.
        // Guards the QSS-specificity regression — a generic '#titleBar
        // QPushButton' rule once out-ranked the id-only traffic selectors and
        // the lights rendered invisible (transparent, stretched to ~30px)
        // while staying clickable.
        const std::pair<const char *, QString> trafficExpected[] = {
            {"trafficClose", theme.trafficClose},
            {"trafficMinimize", theme.trafficMinimize},
            {"trafficZoom", theme.trafficZoom},
        };
        for (const auto &[name, fill] : trafficExpected) {
            QPushButton *light = buttonNamed(proof, QLatin1String(name));
            assert(light != nullptr);
            assert(light->size() == QSize(14, 14));
            const QPoint center = light->mapTo(&proof, QPoint(7, 7));
            assert(QColor(rendered.pixel(center)).name() == fill);
        }

        // The belt paints through QPalette roles; applyShellStyle must push
        // the theme-derived painting palette onto every mounted belt (the
        // pixel-level proof lives in belt_cross_widget_tests).
        auto *beltWidget = proof.findChild<BeltCrossWidget *>();
        assert(beltWidget != nullptr);
        assert(beltWidget->palette().color(QPalette::Base).name() == theme.control);
        assert(beltWidget->palette().color(QPalette::Highlight).name() == theme.selected);
        assert(beltWidget->palette().color(QPalette::Text).name() == theme.text);

        // Typography: stylesheet fonts do not propagate like setFont — the
        // universal QWidget rule is what carries the theme face/size to every
        // control. A deep child (a panel button) proves the rule reaches it.
        QPushButton *fontProbe = buttonNamed(proof, QStringLiteral("toggleLeftPanel"));
        assert(fontProbe != nullptr);
        assert(fontProbe->font().pixelSize() == theme.fontSizeBody);
        assert(fontProbe->font().family() == theme.uiFont);

        // Auxiliary surfaces paint tokens, not the platform popup gray.
        // Menus: grab the widget directly (offscreen popups render fine) and
        // probe inside the 4px padding ring, clear of border and items.
        auto *menuProbe = proof.findChild<QMenu *>(QStringLiteral("fileMenu"));
        assert(menuProbe != nullptr);
        menuProbe->adjustSize();
        const QImage menuImage = menuProbe->grab().toImage();
        assert(QColor(menuImage.pixel(3, 3)).name() == theme.surfaceRaised);

        // Chrome popups (the Snap panel): QObject children of the window, so
        // they grab without being shown; probe inside the 12px margins.
        QFrame *snapPopup = proof.findChild<QFrame *>(QStringLiteral("chromePopup_snap"));
        assert(snapPopup != nullptr);
        assert(snapPopup->property("chromePopup").toBool());
        const QImage popupImage = snapPopup->grab().toImage();
        assert(QColor(popupImage.pixel(6, 6)).name() == theme.surfaceRaised);

        // Settings pop-out: plain QWidget — without WA_StyledBackground its
        // background rule is silently ignored. Grab works unshown; the frame
        // is a permanent window-owned child.
        QWidget *settingsWindow = proof.findChild<QWidget *>(QStringLiteral("settingsWindow"));
        assert(settingsWindow != nullptr);
        const QImage settingsImage = settingsWindow->grab().toImage();
        assert(QColor(settingsImage.pixel(2, settingsImage.height() - 2)).name() == theme.base);

        // Tooltips are top-level: only the application sheet reaches them.
        assert(qApp->styleSheet().contains(theme.surfaceRaised));

        // Control treatment (spec §4): 30px button boxes (20px QSS content +
        // padding + border) and the pointing-hand cursor, swept on by
        // applyShellStyle since QSS has no cursor property.
        assert(fontProbe->cursor().shape() == Qt::PointingHandCursor);
        // QSS min-height lands as the widget's minimumSize at polish time
        // (not in sizeHint) — assert on the laid-out height.
        assert(fontProbe->height() >= 30);
        // Section headers opt out: spec keeps them a compact ~20px row.
        QPushButton *sectionProbe = nullptr;
        for (QPushButton *candidate : proof.findChildren<QPushButton *>(QStringLiteral("sectionToggle"))) {
            sectionProbe = candidate;
            break;
        }
        assert(sectionProbe != nullptr);
        assert(sectionProbe->height() <= 24);
    }

    // Panel-toggle faces (spec §3): the painted 16x14 frame with its 5x12
    // edge bar answers "which panel" by shape and "what state" by color.
    // Probe the bar pixel through grabbed renders across a state change.
    {
        EdiShellWindow faces;
        faces.resize(1100, 760);
        faces.show();
        QCoreApplication::processEvents();
        const edi::shell::ShellTheme faceTheme =
            edi::shell::deriveShellTheme(edi::shell::ShellThemeInputs{});
        QPushButton *leftToggle = buttonNamed(faces, QStringLiteral("toggleLeftPanel"));
        assert(leftToggle != nullptr);
        assert(leftToggle->size() == QSize(30, 30)); // spec square, sheet-enforced
        // Icon 16x14 centered in 30x30 -> origin (7,8); the left bar's
        // center sits at icon (3,7).
        const QPoint barProbe = leftToggle->mapTo(&faces, QPoint(7 + 3, 8 + 7));
        QImage faceShot = faces.grab().toImage();
        assert(QColor(faceShot.pixel(barProbe)).name() == faceTheme.accent); // visible

        faces.setPanelCollapsed(edi::shell::ShellSlot::Left, true);
        faceShot = faces.grab().toImage();
        assert(QColor(faceShot.pixel(barProbe)).name() == faceTheme.textFaint); // collapsed

        faces.resize(600, 760); // under the auto-hide threshold
        QCoreApplication::processEvents();
        faces.setPanelCollapsed(edi::shell::ShellSlot::Left, false);
        faceShot = faces.grab().toImage();
        assert(QColor(faceShot.pixel(barProbe)).name() == faceTheme.warning); // auto-hidden
    }

    // Spec-minimum window (520x420): every title-bar control must stay
    // inside the bar — chrome that clips at the supported minimum is chrome
    // the user cannot click. Also pins the rail's 34x34 spec squares.
    {
        EdiShellWindow tiny;
        tiny.resize(520, 420);
        tiny.show();
        QCoreApplication::processEvents();
        // The window must actually sit at the claimed minimum — if a layout
        // minimum forced it larger, every containment assert below would
        // pass vacuously at the wrong size.
        assert(tiny.size() == QSize(520, 420));
        QWidget *bar = tiny.findChild<QWidget *>(QStringLiteral("titleBar"));
        assert(bar != nullptr);
        for (QPushButton *control : bar->findChildren<QPushButton *>()) {
            if (!control->isVisibleTo(bar)) {
                continue;
            }
            const QRect inBar(control->mapTo(bar, QPoint(0, 0)), control->size());
            assert(bar->rect().contains(inBar));
        }
        QWidget *rail = tiny.findChild<QWidget *>(QStringLiteral("activityRail"));
        assert(rail != nullptr && rail->width() == 52);
        for (QPushButton *railButton : rail->findChildren<QPushButton *>()) {
            assert(railButton->size() == QSize(34, 34));
        }
    }

    // Workspace restore is not navigation: loading workspace.toml at startup
    // must leave the history a single root — Back used to be born enabled,
    // pointing at a factory default the user never visited. And pinned belt
    // rows ride in the same file: pin -> save -> fresh window -> load
    // restores the frozen quick-bar.
    {
        QTemporaryDir tempDir;
        const QString layoutPath = tempDir.filePath(QStringLiteral("workspace.toml"));
        {
            EdiShellWindow saver;
            saver.resize(1100, 760);
            saver.show();
            QCoreApplication::processEvents();
            auto *saverBelt = saver.findChild<BeltCrossWidget *>();
            assert(saverBelt != nullptr);
            // Freeze the active row by the same gesture a user makes: click
            // the pin nub (gutter, centered on the live row).
            const QPointF pinNub(5.0, 21.0 + 17.0);
            QMouseEvent pinPress(QEvent::MouseButtonPress, pinNub,
                                 saverBelt->mapToGlobal(pinNub.toPoint()), Qt::LeftButton,
                                 Qt::LeftButton, Qt::NoModifier);
            QCoreApplication::sendEvent(saverBelt, &pinPress);
            assert(saverBelt->pinnedRows() == std::vector<int>{0});
            assert(saver.saveWorkspaceLayout(layoutPath));
        }

        EdiShellWindow loader;
        assert(loader.loadWorkspaceLayout(layoutPath));
        QPushButton *back = buttonNamed(loader, QStringLiteral("workspaceBack"));
        QPushButton *forward = buttonNamed(loader, QStringLiteral("workspaceForward"));
        assert(back != nullptr && forward != nullptr);
        assert(!back->isEnabled() && !forward->isEnabled()); // a single-entry trail
        auto *loaderBelt = loader.findChild<BeltCrossWidget *>();
        assert(loaderBelt != nullptr);
        assert(loaderBelt->pinnedRows() == std::vector<int>{0}); // the quick-bar survived restart
    }

    // Review find: a Tool Belt checklist edit must not wipe pinned
    // quick-bars. Rows are tool-stable, so pins survive any toggle whose row
    // still holds a tool; only a row that lost its LAST tool drops its pin.
    {
        EdiShellWindow checklist;
        checklist.resize(1100, 760);
        checklist.show();
        QCoreApplication::processEvents();
        auto *checklistBelt = checklist.findChild<BeltCrossWidget *>();
        assert(checklistBelt != nullptr);
        const QPointF nub(5.0, 21.0 + 17.0);
        QMouseEvent nubPress(QEvent::MouseButtonPress, nub,
                             checklistBelt->mapToGlobal(nub.toPoint()), Qt::LeftButton,
                             Qt::LeftButton, Qt::NoModifier);
        QCoreApplication::sendEvent(checklistBelt, &nubPress);
        assert(checklistBelt->pinnedRows() == std::vector<int>{0}); // select row frozen

        // Drive the same hook the settings checklist uses: open the settings
        // window and uncheck one tool that is NOT on the pinned row.
        QPushButton *settingsRail = nullptr;
        for (QPushButton *button : checklist.findChildren<QPushButton *>(QStringLiteral("railButton"))) {
            if (button->property("modeId").toString() == QStringLiteral("settings")) {
                settingsRail = button;
            }
        }
        assert(settingsRail != nullptr);
        settingsRail->click();
        QCheckBox *circleToggle = nullptr;
        for (QCheckBox *box : checklist.findChildren<QCheckBox *>(QStringLiteral("beltToolCheckbox"))) {
            if (box->property("toolId").toString() == QStringLiteral("circle_tool")) {
                circleToggle = box;
            }
        }
        assert(circleToggle != nullptr && circleToggle->isChecked());
        circleToggle->click();
        // The belt was re-dressed in place; the frozen quick-bar survived.
        auto *redressedBelt = checklist.findChild<BeltCrossWidget *>();
        assert(redressedBelt != nullptr);
        assert(redressedBelt->pinnedRows() == std::vector<int>{0});
    }

    // Panel-toggle tri-state (spec §3): the chrome distinguishes a panel the
    // USER collapsed from one the WINDOW hid (auto-hide below the width
    // threshold) — same projection, three values, carried by the panelState
    // property the sheet colors on.
    {
        EdiShellWindow triState;
        triState.resize(1100, 760);
        triState.show();
        QCoreApplication::processEvents();
        QPushButton *leftToggle = buttonNamed(triState, QStringLiteral("toggleLeftPanel"));
        assert(leftToggle != nullptr);
        assert(leftToggle->property("panelState").toString() == QStringLiteral("visible"));

        triState.setPanelCollapsed(edi::shell::ShellSlot::Left, true);
        assert(leftToggle->property("panelState").toString() == QStringLiteral("collapsed"));
        triState.setPanelCollapsed(edi::shell::ShellSlot::Left, false);
        assert(leftToggle->property("panelState").toString() == QStringLiteral("visible"));

        // Auto-hide reacts to the window, not to clicks: shrink under the
        // 640px threshold and the left panel reports auto_hidden, distinct
        // from the manual collapse above.
        triState.resize(600, 760);
        QCoreApplication::processEvents();
        assert(leftToggle->property("panelState").toString() == QStringLiteral("auto_hidden"));
        triState.resize(1100, 760);
        QCoreApplication::processEvents();
        assert(leftToggle->property("panelState").toString() == QStringLiteral("visible"));
    }

    // Status bar (spec §2/§3): a 28px strip under the body. The left label
    // carries the feature-published mode line; the right label names the
    // document and recolors via the documentDirty property when unsaved
    // edits exist — both driven from the modelChanged funnel.
    {
        EdiShellWindow statusWindow;
        QWidget *statusBar = statusWindow.findChild<QWidget *>(QStringLiteral("statusBar"));
        auto *modeLabel = statusWindow.findChild<QLabel *>(QStringLiteral("statusMode"));
        auto *fileLabel = statusWindow.findChild<QLabel *>(QStringLiteral("statusFile"));
        assert(statusBar != nullptr && modeLabel != nullptr && fileLabel != nullptr);
        assert(statusBar->height() == 28 || statusBar->sizeHint().height() == 28);
        // The drafting feature publishes its mode line into the status bar
        // (it lived in the title bar before — the user's chrome inventory
        // has no status slot there).
        assert(modeLabel->text().contains(QStringLiteral("drafting")));
        assert(statusWindow.findChild<QLabel *>(QStringLiteral("chromeStatus")) == nullptr);

        auto *statusController = statusWindow.findChild<DrawingDocumentController *>();
        assert(statusController != nullptr);
        assert(!fileLabel->property("documentDirty").toBool());
        assert(fileLabel->text() == QStringLiteral("Untitled"));
        statusController->setSelectedToolId(QStringLiteral("point_tool"));
        statusController->clickCanvasNormalized(0.5, 0.5);
        assert(fileLabel->property("documentDirty").toBool());
        assert(fileLabel->text().contains(QStringLiteral("•")));
        statusController->undo();
        assert(!fileLabel->property("documentDirty").toBool());
    }

    // F1 empty state: the object list names its own absence ("No objects
    // yet"), and the label is a projection of the document like every other
    // inspector readout — created objects hide it, an emptied document brings
    // it back.
    {
        EdiShellWindow emptyState;
        auto *stateController = emptyState.findChild<DrawingDocumentController *>();
        auto *emptyLabel = emptyState.findChild<QLabel *>(QStringLiteral("objectListEmpty"));
        assert(stateController != nullptr && emptyLabel != nullptr);
        assert(objectCount(*stateController) == 0);
        assert(emptyLabel->isVisibleTo(&emptyState));

        stateController->setSelectedToolId(QStringLiteral("point_tool"));
        stateController->clickCanvasNormalized(0.5, 0.5);
        assert(objectCount(*stateController) == 1);
        assert(!emptyLabel->isVisibleTo(&emptyState));

        stateController->undo();
        assert(objectCount(*stateController) == 0);
        assert(emptyLabel->isVisibleTo(&emptyState));
    }

    return 0;
}
