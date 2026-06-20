#include "widgets/EdiShellWindow.h"
#include "text/TextDocumentStore.h"
#include "widgets/TextEditorFeature.h"

#include "core/DrawingCore.h"
#include "io/SettingsStore.h"
#include "io/ShellLayoutStore.h"
#include "widgets/BeltCrossWidget.h"
#include "widgets/DraftingFeature.h"
#include "widgets/FloatingPalette.h"
#include "widgets/ShellTheme.h"

#include <QApplication>
#include <cstdio>
#include <QCheckBox>
#include <QColor>
#include <QCoreApplication>
#include <QDir>
#include <QImage>
#include <QLabel>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QListWidget>
#include <QMenu>
#include <QSet>
#include <QMouseEvent>
#include <QPlainTextEdit>
#include <QTextCursor>
#include <QSpinBox>
#include <QSplitter>
#include <QTabWidget>
#include <QToolButton>
#include <QString>
#include <QFile>
#include <QFileInfo>
#include <QTemporaryDir>
#include <QUrl>
#include <QVariantList>
#include <QVariantMap>

#include "EdiAssert.h"
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
            EDI_CHECK(match == nullptr);
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
    EDI_CHECK(controller != nullptr);

    // Initial sync: factory-built controls reflect controller state at construction.
    QComboBox *gridPreset = comboWithFirstItemData(window, QStringLiteral("square_art_board"));
    EDI_CHECK(gridPreset != nullptr);
    EDI_CHECK(gridPreset->currentData().toString() == controller->gridPresetId());

    QComboBox *tolerance = comboWithFirstItemData(window, QStringLiteral("tight"));
    EDI_CHECK(tolerance != nullptr);
    EDI_CHECK(tolerance->currentData().toString() == controller->objectSnapTolerancePresetId());

    QCheckBox *gridSnap = toggleWithLabel(window, QStringLiteral("Grid snap"));
    EDI_CHECK(gridSnap != nullptr);
    EDI_CHECK(gridSnap->isChecked() == controller->gridSnapEnabled());
    QCheckBox *guideMoveSnap = toggleWithLabel(window, QStringLiteral("Guide move snap"));
    EDI_CHECK(guideMoveSnap != nullptr);
    EDI_CHECK(guideMoveSnap->isChecked() == controller->guideMoveSnapEnabled());

    // Toggle wiring: flipping the checkbox reaches the controller.
    {
        const bool before = controller->gridSnapEnabled();
        gridSnap->setChecked(!before);
        EDI_CHECK(controller->gridSnapEnabled() == !before);
        gridSnap->setChecked(before);
        EDI_CHECK(controller->gridSnapEnabled() == before);
    }

    // Data-combo wiring: selecting an entry reaches the controller with item data.
    {
        QComboBox *plotOrder = comboWithFirstItemData(window, QStringLiteral("layer_order"));
        EDI_CHECK(plotOrder != nullptr);
        EDI_CHECK(plotOrder->currentData().toString() == controller->plotOrderModeId());
        plotOrder->setCurrentIndex(1);
        EDI_CHECK(controller->plotOrderModeId() == QStringLiteral("nearest_next"));
        plotOrder->setCurrentIndex(0);
        EDI_CHECK(controller->plotOrderModeId() == QStringLiteral("layer_order"));
    }

    // Action-button wiring: a click runs the controller action.
    {
        const int layersBefore = controller->modelDocument().value(QStringLiteral("layers")).toList().size();
        QPushButton *addLayer = buttonNamed(window, QStringLiteral("addLayerButton"));
        EDI_CHECK(addLayer != nullptr);
        addLayer->click();
        const int layersAfter = controller->modelDocument().value(QStringLiteral("layers")).toList().size();
        EDI_CHECK(layersAfter == layersBefore + 1);
    }

    // Conditional buttons: disabled without a selection, enabled by the matching
    // projection flag, and click() on a disabled button is a no-op.
    QPushButton *fitToDrawable = buttonNamed(window, QStringLiteral("fitToDrawableButton"));
    QPushButton *guideToOrigin = buttonNamed(window, QStringLiteral("guideToDrawableOriginButton"));
    QPushButton *deleteSelectedGuide = buttonNamed(window, QStringLiteral("deleteSelectedGuideButton"));
    EDI_CHECK(fitToDrawable != nullptr && guideToOrigin != nullptr && deleteSelectedGuide != nullptr);
    EDI_CHECK(!fitToDrawable->isEnabled());
    EDI_CHECK(!guideToOrigin->isEnabled());

    // Create and select a point object through the controller; refreshInspector
    // runs via the modelChanged connection.
    controller->setSelectedToolId(QStringLiteral("point_tool"));
    controller->clickCanvasNormalized(0.5, 0.5);
    EDI_CHECK(!controller->modelDocument().value(QStringLiteral("active_object_id")).toString().isEmpty());
    EDI_CHECK(fitToDrawable->isEnabled());
    EDI_CHECK(!guideToOrigin->isEnabled());

    // N3: the inspector's metadata controls drive role/material/group/tags on
    // the selected object, and surface back through the projection.
    {
        QComboBox *roleCombo = window.findChild<QComboBox *>(QStringLiteral("objectRoleCombo"));
        auto *materialField = window.findChild<QLineEdit *>(QStringLiteral("objectMaterialField"));
        auto *groupField = window.findChild<QLineEdit *>(QStringLiteral("objectExportGroupField"));
        auto *tagsField = window.findChild<QLineEdit *>(QStringLiteral("objectTagsField"));
        EDI_CHECK(roleCombo != nullptr && materialField != nullptr && groupField != nullptr && tagsField != nullptr);

        roleCombo->setCurrentIndex(roleCombo->findData(QStringLiteral("wall")));
        EDI_CHECK(activeObject(*controller).value(QStringLiteral("role")).toString() == QStringLiteral("wall"));

        materialField->setText(QStringLiteral("concrete"));
        QMetaObject::invokeMethod(materialField, "editingFinished");
        EDI_CHECK(activeObject(*controller).value(QStringLiteral("material")).toString() == QStringLiteral("concrete"));

        groupField->setText(QStringLiteral("shell"));
        QMetaObject::invokeMethod(groupField, "editingFinished");
        EDI_CHECK(activeObject(*controller).value(QStringLiteral("export_group")).toString() == QStringLiteral("shell"));

        tagsField->setText(QStringLiteral("a, b,c"));
        QMetaObject::invokeMethod(tagsField, "editingFinished");
        EDI_CHECK(activeObject(*controller).value(QStringLiteral("tags")).toString() == QStringLiteral("a, b, c"));

        // The combo mirrors live state after a refresh (the role we set).
        EDI_CHECK(roleCombo->currentData().toString() == QStringLiteral("wall"));
    }

    // M1.3: the wall-type combo drives the selected wall's neutral render type,
    // and is gated to walls (disabled for the non-wall selected above).
    {
        QComboBox *wallTypeCombo = window.findChild<QComboBox *>(QStringLiteral("wallTypeCombo"));
        EDI_CHECK(wallTypeCombo != nullptr);
        EDI_CHECK(!wallTypeCombo->isEnabled()); // a point is selected — not a wall

        controller->setSelectedToolId(QStringLiteral("wall_tool"));
        controller->clickCanvasNormalized(0.3, 0.4);
        controller->clickCanvasNormalized(0.7, 0.4);
        EDI_CHECK(activeObject(*controller).value(QStringLiteral("kind")).toString() == QStringLiteral("wall"));
        EDI_CHECK(wallTypeCombo->isEnabled()); // a wall is selected — gate opens
        EDI_CHECK(wallTypeCombo->currentData().toString() == QStringLiteral("solid"));

        wallTypeCombo->setCurrentIndex(wallTypeCombo->findData(QStringLiteral("secret")));
        EDI_CHECK(activeObject(*controller).value(QStringLiteral("wall_type")).toString() == QStringLiteral("secret"));
    }

    // Create and select a guide; guide-conditional buttons flip on.
    controller->setSelectedToolId(QStringLiteral("horizontal_guide_tool"));
    controller->clickCanvasNormalized(0.5, 0.25);
    EDI_CHECK(guideToOrigin->isEnabled());
    EDI_CHECK(deleteSelectedGuide->isEnabled());

    // Registry-driven click path end to end: delete the selected guide.
    {
        const int guidesBefore = controller->modelDocument().value(QStringLiteral("guide_count")).toInt();
        EDI_CHECK(guidesBefore > 0);
        deleteSelectedGuide->click();
        const int guidesAfter = controller->modelDocument().value(QStringLiteral("guide_count")).toInt();
        EDI_CHECK(guidesAfter == guidesBefore - 1);
    }

    // Guide preset button creates guides through the spec-table wiring.
    {
        const int guidesBefore = controller->modelDocument().value(QStringLiteral("guide_count")).toInt();
        QPushButton *presetBounds = buttonNamed(window, QStringLiteral("guidePreset_drawable_bounds"));
        EDI_CHECK(presetBounds != nullptr);
        presetBounds->click();
        const int guidesAfter = controller->modelDocument().value(QStringLiteral("guide_count")).toInt();
        EDI_CHECK(guidesAfter > guidesBefore);
    }

    // Guide visuals: create and select a fresh guide, then drive label, color,
    // dash style, and show-label through the inspector controls.
    controller->setSelectedToolId(QStringLiteral("horizontal_guide_tool"));
    controller->clickCanvasNormalized(0.5, 0.62);
    {
        auto *guideLabel = window.findChild<QLineEdit *>(QStringLiteral("guideLabelField"));
        EDI_CHECK(guideLabel != nullptr && guideLabel->isEnabled());
        guideLabel->setText(QStringLiteral("datum"));
        QMetaObject::invokeMethod(guideLabel, "editingFinished");
        EDI_CHECK(activeObject(*controller).value(QStringLiteral("guide_custom_label")).toString() == QStringLiteral("datum"));

        QComboBox *guideColor = window.findChild<QComboBox *>(QStringLiteral("guideColorCombo"));
        EDI_CHECK(guideColor != nullptr && guideColor->isEnabled());
        guideColor->setCurrentIndex(1);
        EDI_CHECK(activeObject(*controller).value(QStringLiteral("guide_color")).toString() == QStringLiteral("#54d2c6"));

        QComboBox *guideDash = window.findChild<QComboBox *>(QStringLiteral("guideDashStyleCombo"));
        EDI_CHECK(guideDash != nullptr);
        guideDash->setCurrentIndex(1);
        EDI_CHECK(activeObject(*controller).value(QStringLiteral("guide_dash_style")).toString() == QStringLiteral("solid"));

        auto *guideShowLabel = window.findChild<QCheckBox *>(QStringLiteral("guideShowLabelCheckbox"));
        EDI_CHECK(guideShowLabel != nullptr && guideShowLabel->isChecked());
        guideShowLabel->setChecked(false);
        EDI_CHECK(!activeObject(*controller).value(QStringLiteral("guide_show_label"), true).toBool());
    }

    // Dimension controls: create a distance dimension, switch kind and label
    // visibility through the inspector.
    controller->setSelectedToolId(QStringLiteral("distance_dimension_tool"));
    controller->clickCanvasNormalized(0.3, 0.4);
    controller->clickCanvasNormalized(0.6, 0.4);
    {
        EDI_CHECK(activeObject(*controller).value(QStringLiteral("kind")).toString() == QStringLiteral("dimension"));
        QComboBox *dimensionKind = window.findChild<QComboBox *>(QStringLiteral("dimensionKindCombo"));
        EDI_CHECK(dimensionKind != nullptr && dimensionKind->isEnabled());
        dimensionKind->setCurrentIndex(1);
        EDI_CHECK(activeObject(*controller).value(QStringLiteral("dimension_kind")).toString() == QStringLiteral("width"));

        auto *dimensionShowLabel = window.findChild<QCheckBox *>(QStringLiteral("dimensionShowLabelCheckbox"));
        EDI_CHECK(dimensionShowLabel != nullptr && dimensionShowLabel->isChecked());
        dimensionShowLabel->setChecked(false);
        EDI_CHECK(!activeObject(*controller).value(QStringLiteral("dimension_show_label"), true).toBool());
    }

    // Geometry editor spins: select a line; the rebuilt editor carries tagged
    // spins whose edits reach the controller (normalized and physical).
    controller->setSelectedToolId(QStringLiteral("line_tool"));
    controller->clickCanvasNormalized(0.2, 0.2);
    controller->clickCanvasNormalized(0.4, 0.2);
    {
        QDoubleSpinBox *x1 = geometryFieldSpin(window, QStringLiteral("x1"), QStringLiteral("normalized"));
        EDI_CHECK(x1 != nullptr);
        x1->setValue(0.25);
        QMetaObject::invokeMethod(x1, "editingFinished");
        EDI_CHECK(near(activeObject(*controller).value(QStringLiteral("x1")).toDouble(), 0.25));

        QDoubleSpinBox *physicalX1 = geometryFieldSpin(window, QStringLiteral("x1"), QStringLiteral("physical"));
        EDI_CHECK(physicalX1 != nullptr);
        const double gridWidth = controller->modelDocument()
            .value(QStringLiteral("grid")).toMap().value(QStringLiteral("width")).toDouble();
        EDI_CHECK(gridWidth > 0.0);
        // Move the physical X1 to 30% of the bed width; normalized x1 follows.
        physicalX1->setValue(0.3 * gridWidth);
        QMetaObject::invokeMethod(physicalX1, "editingFinished");
        EDI_CHECK(near(activeObject(*controller).value(QStringLiteral("x1")).toDouble(), 0.3, 0.001));
    }

    // Transform buttons: nudge moves the selection, offset/mirror/repeat create
    // objects, align snaps two objects' edges together.
    {
        const QVariantMap before = activeObject(*controller);
        QPushButton *nudgeUp = buttonWithText(window, QStringLiteral("Grid Up"));
        EDI_CHECK(nudgeUp != nullptr);
        nudgeUp->click();
        EDI_CHECK(activeObject(*controller).value(QStringLiteral("y1")).toDouble()
            < before.value(QStringLiteral("y1")).toDouble());
    }
    {
        const int before = objectCount(*controller);
        buttonWithText(window, QStringLiteral("Right +0.05"))->click();
        EDI_CHECK(objectCount(*controller) == before + 1);
    }
    {
        const int before = objectCount(*controller);
        buttonWithText(window, QStringLiteral("Mirror H"))->click();
        EDI_CHECK(objectCount(*controller) == before + 1);
    }
    {
        const int before = objectCount(*controller);
        buttonWithText(window, QStringLiteral("Repeat X"))->click();
        EDI_CHECK(objectCount(*controller) > before);
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
        EDI_CHECK(aligned >= 2);
    }

    // F2: the inspector is a context-keyed stack — planDraftingInspector
    // decides which groups show; everything else is hidden, never destroyed.
    {
        auto groupVisible = [&window](const QString &groupId) {
            QWidget *group = window.findChild<QWidget *>(QStringLiteral("inspectorGroup_") + groupId);
            EDI_CHECK(group != nullptr);
            // isHidden() reads the group's own flag — the right panel starts
            // collapsed, so ancestor-aware isVisibleTo() would see nothing.
            return !group->isHidden();
        };

        // A freshly drawn line is selected -> shape context.
        controller->setSelectedToolId(QStringLiteral("line_tool"));
        controller->clickCanvasNormalized(0.22, 0.72);
        controller->clickCanvasNormalized(0.42, 0.72);
        EDI_CHECK(activeObject(*controller).value(QStringLiteral("kind")).toString() == QStringLiteral("line"));
        EDI_CHECK(groupVisible(QStringLiteral("selection_summary")));
        EDI_CHECK(groupVisible(QStringLiteral("geometry")));
        EDI_CHECK(groupVisible(QStringLiteral("transform")));
        EDI_CHECK(groupVisible(QStringLiteral("object_guides")));
        EDI_CHECK(!groupVisible(QStringLiteral("guide_position")));
        EDI_CHECK(!groupVisible(QStringLiteral("guide_visuals")));
        EDI_CHECK(!groupVisible(QStringLiteral("dimension")));
        EDI_CHECK(!groupVisible(QStringLiteral("tool_polygon")));
        EDI_CHECK(!groupVisible(QStringLiteral("layers_document")));
        EDI_CHECK(!groupVisible(QStringLiteral("canvas_state")));
        EDI_CHECK(!groupVisible(QStringLiteral("empty_state")));

        // A selected guide swaps in the guide groups.
        controller->setSelectedToolId(QStringLiteral("horizontal_guide_tool"));
        controller->clickCanvasNormalized(0.5, 0.77);
        EDI_CHECK(activeObject(*controller).value(QStringLiteral("kind")).toString() == QStringLiteral("guide"));
        EDI_CHECK(groupVisible(QStringLiteral("guide_position")));
        EDI_CHECK(groupVisible(QStringLiteral("guide_visuals")));
        EDI_CHECK(groupVisible(QStringLiteral("geometry")));
        EDI_CHECK(!groupVisible(QStringLiteral("transform")));
        EDI_CHECK(!groupVisible(QStringLiteral("object_guides")));

        // Tool options ride along with any selection: creation auto-selects,
        // so hiding them would break the set-sides-then-draw loop.
        controller->setSelectedToolId(QStringLiteral("regular_polygon_tool"));
        EDI_CHECK(groupVisible(QStringLiteral("tool_polygon")));
        EDI_CHECK(groupVisible(QStringLiteral("guide_position")));

        // Drawing tool with no options, no selection -> quiet empty state.
        controller->setSelectedToolId(QStringLiteral("line_tool"));
        controller->selectObjectsInBoundsNormalized(0.001, 0.001, 0.002, 0.002);
        EDI_CHECK(controller->modelDocument().value(QStringLiteral("selected_object_ids")).toList().isEmpty());
        EDI_CHECK(groupVisible(QStringLiteral("empty_state")));
        EDI_CHECK(!groupVisible(QStringLiteral("selection_summary")));
        EDI_CHECK(!groupVisible(QStringLiteral("tool_polygon")));
        EDI_CHECK(!groupVisible(QStringLiteral("layers_document")));

        // Neutral select tool, no selection -> document configuration groups
        // (interim home until F4/F5 relocate them).
        controller->setSelectedToolId(QStringLiteral("select_move"));
        EDI_CHECK(groupVisible(QStringLiteral("layers_document")));
        EDI_CHECK(groupVisible(QStringLiteral("guides_document")));
        EDI_CHECK(groupVisible(QStringLiteral("calibration_document")));
        EDI_CHECK(groupVisible(QStringLiteral("document_info")));
        EDI_CHECK(groupVisible(QStringLiteral("canvas_state")));
        EDI_CHECK(!groupVisible(QStringLiteral("empty_state")));
        EDI_CHECK(!groupVisible(QStringLiteral("selection_summary")));
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
        EDI_CHECK(canvasStateToggle != nullptr && !canvasStateToggle->isChecked());  // folded by default
        EDI_CHECK(selectedObjectToggle != nullptr && selectedObjectToggle->isChecked()); // open by default
        EDI_CHECK(nudgeToggle != nullptr && !nudgeToggle->isChecked());

        // Toggling shows the content. The fold hides the CONTENT BOX (the
        // grid panel the buttons live in), not each button — so the hidden
        // flag to read is the parent's.
        QPushButton *nudgeUp = buttonWithText(window, QStringLiteral("Grid Up"));
        EDI_CHECK(nudgeUp != nullptr && nudgeUp->parentWidget() != nullptr);
        EDI_CHECK(nudgeUp->parentWidget()->isHidden());
        nudgeToggle->click();
        EDI_CHECK(!nudgeUp->parentWidget()->isHidden());
        nudgeToggle->click();
        EDI_CHECK(nudgeUp->parentWidget()->isHidden());
    }

    // F3: the weapon-cross tool belt drives the controller and follows it.
    {
        auto *belt = window.findChild<BeltCrossWidget *>(QStringLiteral("beltCross"));
        EDI_CHECK(belt != nullptr);
        // The block above left select_move active; the default belt has it
        // at the origin, so belt and controller agree from the start.
        EDI_CHECK(belt->activeItemId() == controller->selectedToolId());
        // One row per tool, six columns of sub-feature room.
        EDI_CHECK(belt->beltState().rows == 10 && belt->beltState().columns == 6);

        // Belt -> controller: click the bottom peek — one step down the tool
        // carousel, from select_move's row to the point tool's row.
        const QPointF bottomPeek(14.0 + 17.0, 67.0); // peek band centre, right of the 14px nub gutter
        QMouseEvent press(QEvent::MouseButtonPress, bottomPeek, belt->mapToGlobal(bottomPeek),
                          Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
        QApplication::sendEvent(belt, &press);
        EDI_CHECK(controller->selectedToolId() == QStringLiteral("point_tool"));
        EDI_CHECK(belt->activeItemId() == QStringLiteral("point_tool"));

        // Controller -> belt: a tool change from any path re-aims the cross
        // (and lands on the dimension tool's row of the default belt).
        controller->setSelectedToolId(QStringLiteral("distance_dimension_tool"));
        EDI_CHECK(belt->activeItemId() == QStringLiteral("distance_dimension_tool"));
        EDI_CHECK(belt->beltState().activeRow == 9);

        controller->setSelectedToolId(QStringLiteral("select_move"));
        EDI_CHECK(belt->activeItemId() == QStringLiteral("select_move"));
    }

    // F4: the belt floats in a palette over the main area — draggable by its
    // grip, position remembered in the workspace layout, torn down and
    // rebuilt with the workspace.
    {
        window.show();
        QApplication::processEvents();

        auto *palette = window.findChild<FloatingPalette *>(QStringLiteral("floatingPalette"));
        EDI_CHECK(palette != nullptr);
        EDI_CHECK(palette->paletteId() == QStringLiteral("tool_belt"));

        // The belt lives inside the palette frame now, not in the left panel.
        auto *belt = window.findChild<BeltCrossWidget *>(QStringLiteral("beltCross"));
        EDI_CHECK(belt != nullptr && palette->isAncestorOf(belt));
        QWidget *leftPanel = window.findChild<QWidget *>(QStringLiteral("leftPanel"));
        EDI_CHECK(leftPanel != nullptr && !leftPanel->isAncestorOf(belt));

        // Default placement, clamped against a real-sized main area.
        EDI_CHECK(palette->pos() == QPoint(12, 12));

        // Drag by the grab nub: press, move (+30,+20), release.
        QWidget *grip = palette->findChild<QWidget *>(QStringLiteral("paletteGrip"));
        EDI_CHECK(grip != nullptr);
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
        EDI_CHECK(palette->pos() == QPoint(42, 32));

        // The drag landed in the layout: saving writes palette.0.* rows.
        {
            QTemporaryDir tempDir;
            EDI_CHECK(tempDir.isValid());
            const QString path = tempDir.filePath(QStringLiteral("palette_workspace.toml"));
            EDI_CHECK(window.saveWorkspaceLayout(path));
            const edi::io::ShellLayoutData saved = edi::io::loadShellLayoutFromPath(path);
            EDI_CHECK(saved.ok);
            const edi::shell::PalettePlacement stored =
                edi::shell::palettePlacement(saved.layout, QStringLiteral("tool_belt"));
            EDI_CHECK(stored.x == 42 && stored.y == 32);
        }

        // Workspace switch: a layout without the drafting feature has no
        // palettes; switching to a drafting layout that remembers a spot
        // rebuilds the palette there (clamped).
        edi::shell::WorkspaceLayout settingsJob;
        settingsJob.id = QStringLiteral("settings");
        settingsJob.label = QStringLiteral("Settings");
        settingsJob.bindings = {{edi::shell::ShellSlot::Main, QStringLiteral("settings")}};
        window.switchWorkspaceLayout(settingsJob);
        EDI_CHECK(window.findChild<FloatingPalette *>(QStringLiteral("floatingPalette")) == nullptr);

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
        EDI_CHECK(rebuilt != nullptr);
        EDI_CHECK(rebuilt->pos() == QPoint(60, 44));

        // A stored position far off-screen clamps back into the main area.
        edi::shell::WorkspaceLayout farJob = draftingJob;
        edi::shell::setPalettePlacement(farJob, {QStringLiteral("tool_belt"), 99999, 99999});
        window.switchWorkspaceLayout(farJob);
        QApplication::processEvents();
        auto *clamped = window.findChild<FloatingPalette *>(QStringLiteral("floatingPalette"));
        EDI_CHECK(clamped != nullptr);
        QWidget *mainArea = window.findChild<QWidget *>(QStringLiteral("workspaceColumn"));
        EDI_CHECK(mainArea != nullptr);
        EDI_CHECK(clamped->geometry().right() <= mainArea->width());
        EDI_CHECK(clamped->geometry().bottom() <= mainArea->height());
    }

    // Snap settings live behind a chrome "Snap" button now: the popup holds
    // the same controls (the initial-sync block above already proved their
    // wiring), and the button tears down with workspaces that lack drafting.
    {
        auto *snapButton = window.findChild<QPushButton *>(QStringLiteral("chromePanel_snap"));
        EDI_CHECK(snapButton != nullptr);
        auto *snapPopup = window.findChild<QWidget *>(QStringLiteral("chromePopup_snap"));
        EDI_CHECK(snapPopup != nullptr);
        QCheckBox *popupGridSnap = toggleWithLabel(*snapPopup, QStringLiteral("Grid snap"));
        EDI_CHECK(popupGridSnap != nullptr); // the controls moved INTO the popup
        QWidget *leftPanel = window.findChild<QWidget *>(QStringLiteral("leftPanel"));
        EDI_CHECK(leftPanel != nullptr && toggleWithLabel(*leftPanel, QStringLiteral("Grid snap")) == nullptr);

        // The left panel kept only navigation: no undo/redo buttons, no
        // project-file buttons, no placeholder sections.
        EDI_CHECK(leftPanel->findChild<QPushButton *>(QStringLiteral("undoButton")) == nullptr);
        EDI_CHECK(leftPanel->findChild<QPushButton *>(QStringLiteral("openDrawingButton")) == nullptr);

        edi::shell::WorkspaceLayout settingsOnly;
        settingsOnly.id = QStringLiteral("settings");
        settingsOnly.label = QStringLiteral("Settings");
        settingsOnly.bindings = {{edi::shell::ShellSlot::Main, QStringLiteral("settings")}};
        window.switchWorkspaceLayout(settingsOnly);
        EDI_CHECK(window.findChild<QPushButton *>(QStringLiteral("chromePanel_snap")) == nullptr);

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
        EDI_CHECK(window.findChild<QPushButton *>(QStringLiteral("chromePanel_snap")) != nullptr);
    }

    // Calibration row: pattern button creates objects, record captures a
    // measurement, apply-scale consumes it.
    {
        const int before = objectCount(*controller);
        QPushButton *testSquare = buttonWithText(window, QStringLiteral("Test square"));
        EDI_CHECK(testSquare != nullptr);
        testSquare->click();
        EDI_CHECK(objectCount(*controller) > before);

        auto *calibrationPanel = window.findChild<QWidget *>(QStringLiteral("calibrationControls"));
        EDI_CHECK(calibrationPanel != nullptr);
        auto *calibrationValue = calibrationPanel->findChild<QDoubleSpinBox *>();
        EDI_CHECK(calibrationValue != nullptr);
        calibrationValue->setValue(0.25);
        buttonWithText(window, QStringLiteral("Record"))->click();
        EDI_CHECK(!controller->modelDocument().value(QStringLiteral("calibration_measurement")).toMap().isEmpty());
        buttonWithText(window, QStringLiteral("Apply scale"))->click();
        EDI_CHECK(!controller->modelDocument().value(QStringLiteral("calibration_correction")).toMap().isEmpty());
    }

    // Project Files: the buttons exist and the save/open seam round-trips the
    // document while keeping window-title dirty state in sync.
    {
        // Project-file verbs live in the File menu now (left panel slimmed).
        EDI_CHECK(menuActionWithText(window, QStringLiteral("fileMenu"), QStringLiteral("Open…")) != nullptr);
        EDI_CHECK(menuActionWithText(window, QStringLiteral("fileMenu"), QStringLiteral("Save")) != nullptr);
        EDI_CHECK(menuActionWithText(window, QStringLiteral("fileMenu"), QStringLiteral("Save As…")) != nullptr);

        QTemporaryDir tempDir;
        EDI_CHECK(tempDir.isValid());
        const QString path = tempDir.filePath(QStringLiteral("shell_roundtrip.edidraw"));

        controller->setSelectedToolId(QStringLiteral("line_tool"));
        controller->clickCanvasNormalized(0.15, 0.25);
        controller->clickCanvasNormalized(0.75, 0.85);
        const QVariantMap savedModel = controller->modelDocument();
        const QVariantList savedObjects = savedModel.value(QStringLiteral("drawing_objects")).toList();
        EDI_CHECK(!savedObjects.isEmpty());

        // Unsaved edits mark the title dirty; saving clears it and adopts the path.
        EDI_CHECK(window.isDocumentDirty());
        EDI_CHECK(window.saveDrawingToPath(path));
        EDI_CHECK(window.currentDrawingPath() == path);
        EDI_CHECK(!window.isDocumentDirty());
        EDI_CHECK(window.windowTitle().contains(QStringLiteral("shell_roundtrip.edidraw")));
        EDI_CHECK(!window.windowTitle().contains(QStringLiteral("•")));

        // Mutate after save: dirty again.
        controller->setSelectedToolId(QStringLiteral("point_tool"));
        controller->clickCanvasNormalized(0.4, 0.4);
        EDI_CHECK(window.isDocumentDirty());
        EDI_CHECK(window.windowTitle().contains(QStringLiteral("•")));

        // Reopen the saved file: projection matches the saved state and title is clean.
        EDI_CHECK(window.openDrawingFromPath(path));
        EDI_CHECK(!window.isDocumentDirty());
        const QVariantList reopenedObjects = controller->modelDocument().value(QStringLiteral("drawing_objects")).toList();
        EDI_CHECK(reopenedObjects.size() == savedObjects.size());
        for (int i = 0; i < savedObjects.size(); ++i) {
            EDI_CHECK(reopenedObjects[i].toMap().value(QStringLiteral("id")).toString()
                   == savedObjects[i].toMap().value(QStringLiteral("id")).toString());
        }

        // Opening a missing path fails and leaves the document untouched.
        EDI_CHECK(!window.openDrawingFromPath(tempDir.filePath(QStringLiteral("missing.edidraw"))));
        EDI_CHECK(controller->modelDocument().value(QStringLiteral("drawing_objects")).toList().size()
               == reopenedObjects.size());
    }

    // Edit section: Undo/Redo buttons exist, enable from canUndo/canRedo, and act.
    {
        QAction *undoButton = window.findChild<QAction *>(QStringLiteral("undoAction"));
        QAction *redoButton = window.findChild<QAction *>(QStringLiteral("redoAction"));
        EDI_CHECK(undoButton != nullptr);
        EDI_CHECK(redoButton != nullptr);

        const int before = controller->modelDocument().value(QStringLiteral("drawing_objects")).toList().size();
        controller->setSelectedToolId(QStringLiteral("point_tool"));
        controller->clickCanvasNormalized(0.33, 0.33);
        const int afterCreate = controller->modelDocument().value(QStringLiteral("drawing_objects")).toList().size();
        EDI_CHECK(afterCreate == before + 1);
        // The create's modelChanged already refreshed the buttons via canUndo().
        EDI_CHECK(undoButton->isEnabled());
        EDI_CHECK(!redoButton->isEnabled());

        undoButton->trigger();
        EDI_CHECK(controller->modelDocument().value(QStringLiteral("drawing_objects")).toList().size() == before);
        EDI_CHECK(redoButton->isEnabled());

        redoButton->trigger();
        EDI_CHECK(controller->modelDocument().value(QStringLiteral("drawing_objects")).toList().size() == afterCreate);
        EDI_CHECK(!redoButton->isEnabled());
    }

    // Tool Options: the Sides spin drives the controller's polygon side count.
    {
        auto *sidesSpin = window.findChild<QSpinBox *>(QStringLiteral("polygonSidesSpin"));
        EDI_CHECK(sidesSpin != nullptr);
        EDI_CHECK(sidesSpin->value() == controller->polygonSides());
        sidesSpin->setValue(8);
        EDI_CHECK(controller->polygonSides() == 8);
    }

    // N4 rectangle tool options: radius / inset spins and the aspect-lock
    // toggle drive the controller's rectangle modes.
    {
        auto *radiusSpin = window.findChild<QDoubleSpinBox *>(QStringLiteral("rectCornerRadiusSpin"));
        auto *insetSpin = window.findChild<QDoubleSpinBox *>(QStringLiteral("rectInsetSpin"));
        auto *aspectLock = window.findChild<QCheckBox *>(QStringLiteral("aspectLockCheckbox"));
        EDI_CHECK(radiusSpin != nullptr && insetSpin != nullptr && aspectLock != nullptr);
        radiusSpin->setValue(0.06);
        EDI_CHECK(near(controller->rectCornerRadius(), 0.06));
        insetSpin->setValue(0.03);
        EDI_CHECK(near(controller->rectInset(), 0.03));
        EDI_CHECK(!controller->aspectLockEnabled());
        aspectLock->setChecked(true);
        EDI_CHECK(controller->aspectLockEnabled());
    }

    // #30 parametric creation + arrays: the radius option group shows for
    // the radius-from-gesture tools, and the array spins/buttons drive the
    // controller's option state and bulk actions.
    {
        auto groupVisible = [&window](const QString &groupId) {
            QWidget *group = window.findChild<QWidget *>(QStringLiteral("inspectorGroup_") + groupId);
            EDI_CHECK(group != nullptr);
            return !group->isHidden();
        };

        // No selection + circle tool -> the radius group shows; a line tool
        // (no options) hides it again.
        controller->setSelectedToolId(QStringLiteral("line_tool"));
        controller->selectObjectsInBoundsNormalized(0.0001, 0.0001, 0.0002, 0.0002);
        EDI_CHECK(controller->modelDocument().value(QStringLiteral("selected_object_ids")).toList().isEmpty());
        EDI_CHECK(!groupVisible(QStringLiteral("tool_radius")));
        controller->setSelectedToolId(QStringLiteral("circle_tool"));
        EDI_CHECK(groupVisible(QStringLiteral("tool_radius")));

        auto *fixedRadiusSpin = window.findChild<QDoubleSpinBox *>(QStringLiteral("fixedRadiusSpin"));
        EDI_CHECK(fixedRadiusSpin != nullptr);
        fixedRadiusSpin->setValue(0.07);
        EDI_CHECK(near(controller->fixedRadius(), 0.07));

        // Array option spins mirror controller state.
        auto *countSpin = window.findChild<QSpinBox *>(QStringLiteral("arrayCountSpin"));
        auto *stepXSpin = window.findChild<QDoubleSpinBox *>(QStringLiteral("arraySpacingXSpin"));
        auto *stepYSpin = window.findChild<QDoubleSpinBox *>(QStringLiteral("arraySpacingYSpin"));
        EDI_CHECK(countSpin != nullptr && stepXSpin != nullptr && stepYSpin != nullptr);
        EDI_CHECK(countSpin->value() == controller->arrayCount());
        countSpin->setValue(2);
        EDI_CHECK(controller->arrayCount() == 2);
        stepXSpin->setValue(0.05);
        EDI_CHECK(near(controller->arraySpacingX(), 0.05));
        stepYSpin->setValue(-0.04); // negative spacing is a legal direction
        EDI_CHECK(near(controller->arraySpacingY(), -0.04));

        // The grid button stamps count x count cells from the selection.
        controller->setSelectedToolId(QStringLiteral("line_tool"));
        controller->clickCanvasNormalized(0.62, 0.62);
        controller->clickCanvasNormalized(0.66, 0.62);
        const int beforeGrid = controller->modelDocument().value(QStringLiteral("drawing_objects")).toList().size();
        QPushButton *gridButton = window.findChild<QPushButton *>(QStringLiteral("gridArrayButton"));
        QPushButton *radialButton = window.findChild<QPushButton *>(QStringLiteral("radialArrayButton"));
        EDI_CHECK(gridButton != nullptr && radialButton != nullptr);
        gridButton->click();
        EDI_CHECK(controller->modelDocument().value(QStringLiteral("drawing_objects")).toList().size()
               == beforeGrid + 3); // 2x2 grid: source + 3 copies

        // Radial now PICKS its centre: the button arms a pick-a-point capture,
        // and the next canvas click sets the ring centre. The fresh copies ring
        // that picked point (away from the source so the arm is non-zero).
        radialButton->click();
        EDI_CHECK(controller->isAwaitingPointCapture());
        controller->clickCanvasNormalized(0.3, 0.3);
        EDI_CHECK(!controller->isAwaitingPointCapture());
        EDI_CHECK(controller->modelDocument().value(QStringLiteral("drawing_objects")).toList().size()
               == beforeGrid + 3 + 2); // arrayCount 2 -> 2 ring copies around the picked centre

        // Reset shared tool-option state so later blocks see defaults.
        fixedRadiusSpin->setValue(0.0);
        countSpin->setValue(3);
        stepXSpin->setValue(0.1);
        stepYSpin->setValue(0.1);
        controller->setSelectedToolId(QStringLiteral("select_move"));
    }

    // Style group: the opacity spin writes through to the selected object and
    // re-reads the object's own value on selection refresh.
    {
        controller->setSelectedToolId(QStringLiteral("line_tool"));
        controller->clickCanvasNormalized(0.72, 0.72);
        controller->clickCanvasNormalized(0.78, 0.72);
        auto *opacitySpin = window.findChild<QDoubleSpinBox *>(QStringLiteral("styleOpacitySpin"));
        EDI_CHECK(opacitySpin != nullptr);
        EDI_CHECK(opacitySpin->value() == 1.0); // fresh object: fully opaque
        opacitySpin->setValue(0.3);
        QMetaObject::invokeMethod(opacitySpin, "editingFinished");
        const QVariantList styledObjects = controller->modelDocument().value(QStringLiteral("drawing_objects")).toList();
        bool foundFaded = false;
        for (const QVariant &value : styledObjects) {
            const QVariantMap object = value.toMap();
            if (object.value(QStringLiteral("id")).toString() == controller->selectedObjectId()) {
                EDI_CHECK(object.value(QStringLiteral("own_stroke_opacity")).toDouble() == 0.3);
                foundFaded = true;
            }
        }
        EDI_CHECK(foundFaded);

        // The spin RE-READS the selected object: a freshly drawn (opaque)
        // line resets it to 1.0, re-selecting the faded one restores 0.3.
        // This is the assertion that dies if the refreshSpinValue line for
        // the opacity spin is removed — the write-through check above
        // passes either way because the test set the spin itself.
        const QString fadedId = controller->selectedObjectId();
        controller->clickCanvasNormalized(0.62, 0.78);
        controller->clickCanvasNormalized(0.68, 0.78);
        EDI_CHECK(controller->selectedObjectId() != fadedId);
        EDI_CHECK(opacitySpin->value() == 1.0);
        EDI_CHECK(controller->selectObjectById(fadedId));
        EDI_CHECK(opacitySpin->value() == 0.3);
        controller->setSelectedToolId(QStringLiteral("select_move"));
    }

    // Export buttons exist and the path seams write SVG / HPGL files.
    {
        EDI_CHECK(menuActionWithText(window, QStringLiteral("fileMenu"), QStringLiteral("Export SVG…")) != nullptr);
        EDI_CHECK(menuActionWithText(window, QStringLiteral("fileMenu"), QStringLiteral("Export HPGL…")) != nullptr);

        controller->setSelectedToolId(QStringLiteral("line_tool"));
        controller->clickCanvasNormalized(0.2, 0.2);
        controller->clickCanvasNormalized(0.8, 0.8);

        QTemporaryDir tempDir;
        EDI_CHECK(tempDir.isValid());
        const QString svgPath = tempDir.filePath(QStringLiteral("shell.svg"));
        const QString hpglPath = tempDir.filePath(QStringLiteral("shell.hpgl"));
        const QString gcodePath = tempDir.filePath(QStringLiteral("shell.gcode"));
        EDI_CHECK(window.exportSvgToPath(svgPath));
        EDI_CHECK(window.exportHpglToPath(hpglPath));
        EDI_CHECK(window.exportGcodeToPath(gcodePath)); // N5 export seam
        EDI_CHECK(QFile::exists(svgPath));
        EDI_CHECK(QFile::exists(hpglPath));
        EDI_CHECK(QFile::exists(gcodePath));
        {
            QFile f(gcodePath);
            EDI_CHECK(f.open(QIODevice::ReadOnly));
            const QString body = QString::fromUtf8(f.readAll());
            EDI_CHECK(body.startsWith(QStringLiteral("G21\n"))); // real G-code, not empty
            EDI_CHECK(body.contains(QStringLiteral("G1 ")));     // a stroke was emitted
        }
    }

    // Settings persistence: change a snap toggle + plot mode, save, then rebuild
    // a fresh window from the same file and assert the state survived.
    {
        QTemporaryDir tempDir;
        EDI_CHECK(tempDir.isValid());
        const QString settingsPath = tempDir.filePath(QStringLiteral("edi.toml"));

        // Use an explicit non-default value (grid snap defaults to false) so the
        // assertion cannot pass vacuously against a fresh window's default.
        controller->setGridSnapEnabled(true);
        controller->setPlotOrderModeId(QStringLiteral("nearest_next"));
        controller->setObjectSnapTolerancePreset(QStringLiteral("tight"));
        EDI_CHECK(window.saveSettings(settingsPath));
        EDI_CHECK(QFile::exists(settingsPath));

        EdiShellWindow restored;
        auto *restoredController = restored.findChild<DrawingDocumentController *>();
        EDI_CHECK(restoredController != nullptr);
        EDI_CHECK(!restoredController->gridSnapEnabled()); // fresh default before load
        EDI_CHECK(restored.loadSettings(settingsPath));
        EDI_CHECK(restoredController->gridSnapEnabled()); // loaded the saved true
        EDI_CHECK(restoredController->plotOrderModeId() == QStringLiteral("nearest_next"));
        EDI_CHECK(restoredController->objectSnapTolerancePresetId() == QStringLiteral("tight"));
    }

    // Recent files: saving a drawing records it and surfaces an Open Recent
    // entry (the File menu took over from the left panel's quick buttons).
    {
        QTemporaryDir tempDir;
        EDI_CHECK(tempDir.isValid());
        const QString drawingPath = tempDir.filePath(QStringLiteral("recent.edidraw"));
        EDI_CHECK(window.saveDrawingToPath(drawingPath));
        EDI_CHECK(window.recentFiles().contains(drawingPath));
        auto *recentMenu = window.findChild<QMenu *>(QStringLiteral("recentFilesMenu"));
        EDI_CHECK(recentMenu != nullptr);
        bool listed = false;
        for (QAction *action : recentMenu->actions()) {
            if (action->data().toString() == drawingPath) {
                listed = true;
            }
        }
        EDI_CHECK(listed);
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
        EDI_CHECK(leftPanel != nullptr && rightPanel != nullptr && bottomPanel != nullptr);

        // Initial state per spec: left open, right and bottom collapsed.
        EDI_CHECK(window.shellPanelVisibility(ShellSlot::Left) == PanelVisibility::Visible);
        EDI_CHECK(window.shellPanelVisibility(ShellSlot::Right) == PanelVisibility::Collapsed);
        EDI_CHECK(window.shellPanelVisibility(ShellSlot::Bottom) == PanelVisibility::Collapsed);
        EDI_CHECK(leftPanel->isVisibleTo(&window));
        EDI_CHECK(!rightPanel->isVisibleTo(&window));

        // Manual collapse toggling reaches the widgets.
        window.setPanelCollapsed(ShellSlot::Right, false);
        EDI_CHECK(window.shellPanelVisibility(ShellSlot::Right) == PanelVisibility::Visible);
        EDI_CHECK(rightPanel->isVisibleTo(&window));

        // Auto-hide: shrink below the left panel's 640px threshold; right
        // never auto-hides. Growing back restores the panel.
        window.resize(600, 820);
        EDI_CHECK(window.shellPanelVisibility(ShellSlot::Left) == PanelVisibility::AutoHidden);
        EDI_CHECK(!leftPanel->isVisibleTo(&window));
        EDI_CHECK(window.shellPanelVisibility(ShellSlot::Right) == PanelVisibility::Visible);
        window.resize(1280, 820);
        EDI_CHECK(window.shellPanelVisibility(ShellSlot::Left) == PanelVisibility::Visible);
        EDI_CHECK(leftPanel->isVisibleTo(&window));

        // Manual collapse is sticky across resizes (it outranks auto-hide).
        window.setPanelCollapsed(ShellSlot::Left, true);
        window.resize(600, 820);
        EDI_CHECK(window.shellPanelVisibility(ShellSlot::Left) == PanelVisibility::Collapsed);
        window.resize(1280, 820);
        EDI_CHECK(window.shellPanelVisibility(ShellSlot::Left) == PanelVisibility::Collapsed);

        // Presets transform the whole state.
        window.applyShellPanelPreset(PanelPreset::Full);
        EDI_CHECK(window.shellPanelVisibility(ShellSlot::Left) == PanelVisibility::Visible);
        EDI_CHECK(window.shellPanelVisibility(ShellSlot::Right) == PanelVisibility::Visible);
        EDI_CHECK(window.shellPanelVisibility(ShellSlot::Bottom) == PanelVisibility::Visible);
        EDI_CHECK(bottomPanel->isVisibleTo(&window));
        window.applyShellPanelPreset(PanelPreset::Focus);
        EDI_CHECK(!leftPanel->isVisibleTo(&window));
        EDI_CHECK(!rightPanel->isVisibleTo(&window));
        EDI_CHECK(!bottomPanel->isVisibleTo(&window));
        window.applyShellPanelPreset(PanelPreset::Review);
        EDI_CHECK(leftPanel->isVisibleTo(&window));
        EDI_CHECK(!rightPanel->isVisibleTo(&window));

        // Drag limits surface as widget constraints. Bottom has no maximum:
        // the terminal may grow to become the main view.
        EDI_CHECK(leftPanel->minimumWidth() == 180);
        EDI_CHECK(leftPanel->maximumWidth() == 520);
        EDI_CHECK(rightPanel->minimumWidth() == 160);
        EDI_CHECK(bottomPanel->minimumHeight() == 96);
        EDI_CHECK(bottomPanel->maximumHeight() > 100000); // QWIDGETSIZE_MAX, i.e. unbounded

        // Overlay behavior: the grid never resizes when panels come and go —
        // they cover it. The canvas always fills the whole main area.
        window.applyShellPanelPreset(PanelPreset::Full);
        QCoreApplication::processEvents();
        QWidget *canvas = window.findChild<QWidget *>(QStringLiteral("drawingCanvas"));
        QWidget *mainArea = window.findChild<QWidget *>(QStringLiteral("workspaceColumn"));
        EDI_CHECK(canvas != nullptr && mainArea != nullptr);
        const QSize canvasWithPanels = canvas->size();
        EDI_CHECK(canvasWithPanels == mainArea->size());
        window.setPanelCollapsed(ShellSlot::Right, true);
        window.setPanelCollapsed(ShellSlot::Bottom, true);
        EDI_CHECK(canvas->size() == canvasWithPanels); // unchanged: overlays, not siblings
        window.setPanelCollapsed(ShellSlot::Right, false);
        window.setPanelCollapsed(ShellSlot::Bottom, false);
        EDI_CHECK(canvas->size() == canvasWithPanels);
        // The right overlay hugs the right edge; the bottom overlay spans the
        // full width and sits on the bottom edge.
        EDI_CHECK(rightPanel->geometry().right() + 1 == mainArea->width());
        EDI_CHECK(bottomPanel->width() == mainArea->width());
        EDI_CHECK(bottomPanel->geometry().bottom() + 1 == mainArea->height());
        EDI_CHECK(rightPanel->geometry().bottom() < bottomPanel->geometry().top() + 1);

        // The terminal grows over the grid via its grip — up to the whole
        // main area — without the grid moving.
        QWidget *bottomGrip = window.findChild<QWidget *>(QStringLiteral("bottomPanelGrip"));
        EDI_CHECK(bottomGrip != nullptr && bottomGrip->isVisibleTo(&window));
        const int beforeDrag = bottomPanel->height();
        {
            const QPointF local(4.0, 4.0);
            const QPointF global = mainArea->mapToGlobal(QPoint(200, 10)); // drag near the top
            QMouseEvent move(QEvent::MouseMove, local, local, global,
                Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
            QCoreApplication::sendEvent(bottomGrip, &move);
        }
        EDI_CHECK(bottomPanel->height() > beforeDrag);
        EDI_CHECK(bottomPanel->height() >= mainArea->height() - 10); // effectively the main window
        EDI_CHECK(canvas->size() == canvasWithPanels);               // grid untouched

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
        EDI_CHECK(tempDir.isValid());
        const QString layoutPath = tempDir.filePath(QStringLiteral("workspace.toml"));

        window.applyShellPanelPreset(PanelPreset::Full);
        window.setPanelCollapsed(ShellSlot::Right, true);
        window.setPanelCollapsed(ShellSlot::Bottom, true);
        EDI_CHECK(window.saveWorkspaceLayout(layoutPath));

        EdiShellWindow restored;
        restored.applyShellPanelPreset(PanelPreset::Full); // scramble away from saved state
        EDI_CHECK(restored.shellPanelVisibility(ShellSlot::Right) == PanelVisibility::Visible);
        EDI_CHECK(restored.loadWorkspaceLayout(layoutPath));
        EDI_CHECK(restored.shellPanelVisibility(ShellSlot::Left) == PanelVisibility::Visible);
        EDI_CHECK(restored.shellPanelVisibility(ShellSlot::Right) == PanelVisibility::Collapsed);
        EDI_CHECK(restored.shellPanelVisibility(ShellSlot::Bottom) == PanelVisibility::Collapsed);

        // A missing file reports false and leaves the built-in defaults alone.
        EdiShellWindow fresh;
        EDI_CHECK(!fresh.loadWorkspaceLayout(tempDir.filePath(QStringLiteral("absent.toml"))));
        EDI_CHECK(fresh.shellPanelVisibility(ShellSlot::Left) == PanelVisibility::Visible);

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
            EDI_CHECK(edi::io::saveSettingsToPath(sizedPath, config));
            sized.show();
            EDI_CHECK(sized.loadWorkspaceLayout(sizedPath));
            auto *bodySplitter = sized.findChild<QSplitter *>(QStringLiteral("bodySplitter"));
            EDI_CHECK(bodySplitter != nullptr);
            EDI_CHECK(bodySplitter->sizes().value(0) == 400);
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
        EDI_CHECK(shellController != nullptr);

        // Draw something so there is a document to preserve.
        shellController->setSelectedToolId(QStringLiteral("point_tool"));
        shellController->clickCanvasNormalized(0.5, 0.5);
        const int objectsBefore = objectCount(*shellController);
        EDI_CHECK(objectsBefore > 0);

        WorkspaceLayout canvasOnly;
        canvasOnly.id = QStringLiteral("canvas_only");
        canvasOnly.label = QStringLiteral("Canvas only");
        canvasOnly.bindings = {{ShellSlot::Main, QStringLiteral("drafting")}};
        shell.switchWorkspaceLayout(canvasOnly);

        // Old panels are gone (immediate delete — no flush needed), the canvas
        // is fresh, and the document survived the teardown.
        EDI_CHECK(shell.findChild<QWidget *>(QStringLiteral("leftPanel")) == nullptr);
        EDI_CHECK(shell.findChild<QWidget *>(QStringLiteral("rightPanel")) == nullptr);
        EDI_CHECK(shell.findChild<QWidget *>(QStringLiteral("bottomPanel")) == nullptr);
        EDI_CHECK(shell.findChild<QWidget *>(QStringLiteral("drawingCanvas")) != nullptr);
        EDI_CHECK(objectCount(*shellController) == objectsBefore);

        // Model changes must not crash the partial-layout inspector (it is
        // wired to modelChanged on the fresh feature instance).
        shellController->clickCanvasNormalized(0.25, 0.25);
        EDI_CHECK(objectCount(*shellController) == objectsBefore + 1);

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
        EDI_CHECK(shell.findChild<QWidget *>(QStringLiteral("leftPanel")) != nullptr);
        EDI_CHECK(shell.findChild<QWidget *>(QStringLiteral("rightPanel")) != nullptr);
        QPushButton *fitButton = buttonNamed(shell, QStringLiteral("fitToDrawableButton"));
        EDI_CHECK(fitButton != nullptr);
        EDI_CHECK(fitButton->isEnabled());
        EDI_CHECK(objectCount(*shellController) == objectsBefore + 1);

        // Persistence round-trips bindings: save the canvas-only job, load it
        // into a fresh window, and the fresh window switches to it.
        shell.switchWorkspaceLayout(canvasOnly);
        QTemporaryDir tempDir;
        EDI_CHECK(tempDir.isValid());
        const QString jobPath = tempDir.filePath(QStringLiteral("job.toml"));
        EDI_CHECK(shell.saveWorkspaceLayout(jobPath));

        EdiShellWindow restored;
        EDI_CHECK(restored.findChild<QWidget *>(QStringLiteral("leftPanel")) != nullptr);
        EDI_CHECK(restored.loadWorkspaceLayout(jobPath));
        EDI_CHECK(restored.findChild<QWidget *>(QStringLiteral("leftPanel")) == nullptr);
        EDI_CHECK(restored.findChild<QWidget *>(QStringLiteral("drawingCanvas")) != nullptr);
    }

    // Map workspace: the rail's mode->layout switch resolves Map to its own
    // job (id "map"). The map is drafting-document content, so the layout
    // reuses the drafting canvas — Drafting and Map are sibling jobs whose
    // bindings match, which is exactly why the id (not the panels) is the
    // observable that proves the new ternary arm fires.
    {
        EdiShellWindow shell;
        shell.show();
        EDI_CHECK(shell.currentWorkspaceId() == QStringLiteral("drafting"));

        shell.setWorkspaceMode(edi::app::WorkspaceMode::Map);
        EDI_CHECK(shell.currentWorkspaceId() == QStringLiteral("map"));
        // The shared canvas mounts under the Map job (no parallel surface).
        EDI_CHECK(shell.findChild<QWidget *>(QStringLiteral("drawingCanvas")) != nullptr);
        EDI_CHECK(shell.findChild<QWidget *>(QStringLiteral("leftPanel")) != nullptr);
        // First-class feel: the Right slot is a distinguishing feature (the map
        // browser), so the switch auto-opened it — the collapse flag is cleared
        // (Visible or, on a tiny window, AutoHidden — never Collapsed).
        EDI_CHECK(shell.shellPanelVisibility(edi::shell::ShellSlot::Right)
               != edi::shell::PanelVisibility::Collapsed);
        // The Right slot is the map browser (not the drafting inspector): the
        // panel mounts and its summary projects the live (empty) document.
        EDI_CHECK(shell.findChild<QWidget *>(QStringLiteral("mapBrowserPanel")) != nullptr);
        auto *summary = shell.findChild<QLabel *>(QStringLiteral("mapBrowserSummary"));
        EDI_CHECK(summary != nullptr);
        EDI_CHECK(summary->text().contains(QStringLiteral("0 rooms")));
        auto *mapList = shell.findChild<QListWidget *>(QStringLiteral("mapBrowserList"));
        EDI_CHECK(mapList != nullptr);
        // An empty document still shows the two STRUCTURAL section headers (Plugs,
        // Connections) — they are part of the browser's fixed shape, not content,
        // so the readout always announces its sections even when graph-empty.
        EDI_CHECK(mapList->count() == 2);
        EDI_CHECK(mapList->item(0)->text() == QStringLiteral("── Plugs ──"));
        EDI_CHECK(mapList->item(1)->text() == QStringLiteral("── Connections ──"));

        // The browser is LIVE: author a tiny two-room graph and the summary +
        // list re-project on modelChanged (the connection bound to the panel).
        // The map is document content, so createMapFromSpec is all it takes; the
        // spec is built inline from the drafting-core types (plugs centered on
        // their edge, exactly as the .map.toml "center" default lands them).
        auto *mapController = shell.findChild<DrawingDocumentController *>();
        EDI_CHECK(mapController != nullptr);
        edi::drafting::MapSpec spec;
        edi::drafting::NamedRoomSpec roomA;
        roomA.name = "a";
        roomA.spec.origin = {0.0, 0.0};
        roomA.spec.width = 8.0;
        roomA.spec.height = 6.0;
        // Give the east plug a neutral type + open-vocabulary flags so the browser
        // must render the trailing flags run (DM-04/06 display shape).
        roomA.spec.plugs.push_back({edi::drafting::RoomEdge::East, 3.0, "to_b", "door",
                                    {"window", "passes_light"}});
        edi::drafting::NamedRoomSpec roomB;
        roomB.name = "b";
        roomB.spec.origin = {12.0, 0.0};
        roomB.spec.width = 8.0;
        roomB.spec.height = 6.0;
        roomB.spec.plugs.push_back({edi::drafting::RoomEdge::West, 3.0, "to_a", "", {}});
        spec.rooms = {roomA, roomB};
        spec.connections.push_back({{"a", "to_b"}, {"b", "to_a"}, "corridor"});
        // Author at 0.25 canvas-per-authored-unit so the browser must DIVIDE to
        // recover authored units: the 8 x 6 (canvas) rooms read as 32 x 24, not
        // the raw stored 8 x 6 — proving the footprint is shown in authored units.
        EDI_CHECK(mapController->createMapFromSpec(spec, 0.25));
        EDI_CHECK(summary->text().contains(QStringLiteral("2 rooms")));
        EDI_CHECK(summary->text().contains(QStringLiteral("1 connection"))); // pluralized: singular
        EDI_CHECK(summary->text().contains(QStringLiteral("2 plugs")));
        // Rows: 2 rooms + "── Plugs ──" + 2 plug rows + "── Connections ──" + 1 conn.
        EDI_CHECK(mapList->count() == 7);
        EDI_CHECK(mapList->item(0)->text().contains(QStringLiteral("32"))); // 8 / 0.25 authored
        EDI_CHECK(mapList->item(0)->text().contains(QStringLiteral("24"))); // 6 / 0.25 authored

        // Collect the list rows by their leading symbol so the assertions don't
        // depend on the exact interleave order beyond the documented sectioning.
        QString plugsHeader;
        QString connectionsHeader;
        QStringList plugRows;
        QStringList connectionRows;
        for (int i = 0; i < mapList->count(); ++i) {
            const QString text = mapList->item(i)->text();
            if (text == QStringLiteral("── Plugs ──")) {
                plugsHeader = text;
            } else if (text == QStringLiteral("── Connections ──")) {
                connectionsHeader = text;
            } else if (text.startsWith(QStringLiteral("◦"))) {
                plugRows << text;
            } else if (text.startsWith(QStringLiteral("⟷"))) {
                connectionRows << text;
            }
        }
        EDI_CHECK(!plugsHeader.isEmpty());
        EDI_CHECK(!connectionsHeader.isEmpty());
        EDI_CHECK(plugRows.size() == 2);
        EDI_CHECK(connectionRows.size() == 1);

        // The east plug (a.to_b) sits on the room's E edge, is named by a
        // connection (linked), and carries its two flags in a `·`-joined bracket.
        QString eastRow;
        for (const QString &row : plugRows) {
            if (row.contains(QStringLiteral("to_b"))) {
                eastRow = row;
            }
        }
        EDI_CHECK(!eastRow.isEmpty());
        EDI_CHECK(eastRow.contains(QStringLiteral("door")));     // neutral type
        EDI_CHECK(eastRow.contains(QStringLiteral("· E ·")));    // derived edge
        EDI_CHECK(eastRow.contains(QStringLiteral("linked")));   // named by a connection
        EDI_CHECK(eastRow.contains(QStringLiteral("[window · passes_light]"))); // flags run

        // The connection row shows its neutral role type ("corridor").
        EDI_CHECK(connectionRows.front().contains(QStringLiteral("corridor")));

        // The switch is reversible; the id tracks the rail and the browser is
        // torn down (its modelChanged connection dies with it).
        shell.setWorkspaceMode(edi::app::WorkspaceMode::Drafting);
        EDI_CHECK(shell.currentWorkspaceId() == QStringLiteral("drafting"));
        EDI_CHECK(shell.findChild<QWidget *>(QStringLiteral("mapBrowserPanel")) == nullptr);
    }

    // The recipe lab's ASCII proof pane: switching to the Blender (lab) job
    // mounts it in the Left slot; it renders the live op stream's projection and
    // re-renders on opsStreamChanged (the editor's Apply path) — the lab's first
    // "one feature reacts to what another produced" coupling.
    {
        EdiShellWindow shell;
        shell.show();
        shell.setWorkspaceMode(edi::app::WorkspaceMode::Blender);
        EDI_CHECK(shell.currentWorkspaceId() == QStringLiteral("blender"));

        // The lab's bottom terminal tabs the editor and the ASCII proof; the
        // proof lives in a tab page (found even while the Editor tab is active,
        // and rendered regardless of tab visibility). Switching auto-opens both
        // the Bottom terminal (a distinguishing slot here) and the Right render
        // preview, so the proof is one tab-click away without a manual expand.
        auto *terminalTabs = shell.findChild<QTabWidget *>(QStringLiteral("recipeTerminal"));
        EDI_CHECK(terminalTabs != nullptr);
        EDI_CHECK(terminalTabs->count() == 3); // Steps + Editor + ASCII Proof
        EDI_CHECK(shell.findChild<QWidget *>(QStringLiteral("asciiPreviewPanel")) != nullptr);
        EDI_CHECK(shell.findChild<QListWidget *>(QStringLiteral("opStepsList")) != nullptr);

        // The Right slot tabs the recipe OUTPUTS: the Blender render (its label
        // still findable, whichever tab is forward) and the Compiled recipe.
        auto *outputTabs = shell.findChild<QTabWidget *>(QStringLiteral("recipeOutput"));
        EDI_CHECK(outputTabs != nullptr);
        EDI_CHECK(outputTabs->count() == 3); // Palette + Render + Compiled
        EDI_CHECK(shell.findChild<QLabel *>(QStringLiteral("blenderPreview")) != nullptr);
        auto *compiledView = shell.findChild<QPlainTextEdit *>(QStringLiteral("compiledRecipeText"));
        EDI_CHECK(compiledView != nullptr);
        EDI_CHECK(compiledView->toPlainText().contains(QStringLiteral("No recipe"))); // empty stream
        EDI_CHECK(shell.shellPanelVisibility(edi::shell::ShellSlot::Bottom)
               != edi::shell::PanelVisibility::Collapsed);
        EDI_CHECK(shell.shellPanelVisibility(edi::shell::ShellSlot::Right)
               != edi::shell::PanelVisibility::Collapsed);
        auto *asciiView = shell.findChild<QPlainTextEdit *>(QStringLiteral("asciiPreviewText"));
        EDI_CHECK(asciiView != nullptr);
        // Empty stream: a proof of nothing says so, never a misleading blank grid.
        EDI_CHECK(asciiView->toPlainText().contains(QStringLiteral("No recipe")));

        // Apply a literal (binding-free) recipe through the editor's hook; the
        // op stream changes, opsStreamChanged fires, and the pane re-renders.
        const QString applyError = shell.applyOpsScript(
            "op.0.type = \"AddCylinder\"\n"
            "op.0.name = \"bare.drum\"\n"
            "op.0.radius = \"1\"\n"
            "op.0.height = \"2\"\n"
            "op.0.z = \"0\"\n");
        EDI_CHECK(applyError.isEmpty()); // the strict reader accepted it
        EDI_CHECK(asciiView->toPlainText().contains(QStringLiteral("FRONT PROJECTION")));
        // The compiled view re-serialized off the same opsStreamChanged: the
        // literal cylinder compiles straight through, so it names its op type.
        EDI_CHECK(compiledView->toPlainText().contains(QStringLiteral("AddCylinder")));

        // The Steps inspector lists the recipe's ops; selecting one shows its
        // numeric fields, and editing one (the verb the spinbox's editingFinished
        // calls) mutates the stream + re-renders the proof — the human's surface.
        auto *steps = shell.findChild<QListWidget *>(QStringLiteral("opStepsList"));
        EDI_CHECK(steps != nullptr);
        EDI_CHECK(steps->count() == 1); // the one AddCylinder we applied
        steps->setCurrentRow(0);
        // Its radius field is present (the cylinder's first editable field) and
        // shows the authored value.
        auto *radiusSpin = shell.findChild<QDoubleSpinBox *>(QStringLiteral("opField_radius"));
        EDI_CHECK(radiusSpin != nullptr);
        EDI_CHECK(radiusSpin->value() == 1.0); // from the applied recipe
        // Tune it through the public edit verb; the stream + proof + compiled all
        // follow, with no manual TOML.
        shell.applyOpFieldEdit(0, QStringLiteral("radius"), 2.5);
        const auto *tunedCylinder = std::get_if<edi::recipe::AddCylinderOp>(&shell.opsStream().ops[0]);
        EDI_CHECK(tunedCylinder != nullptr && tunedCylinder->radius == 2.5);
        EDI_CHECK(compiledView->toPlainText().contains(QStringLiteral("2.5"))); // re-serialized
        EDI_CHECK(radiusSpin->value() == 2.5);                                  // spin followed the change

        // Drive the REAL spinbox commit path (editingFinished) to prove the
        // wiring and that the resulting re-render does not loop back into another
        // edit (a programmatic setValue refresh never re-fires editingFinished).
        radiusSpin->setValue(4.0);
        emit radiusSpin->editingFinished(); // what a focus-out / Enter triggers
        const auto *reCylinder = std::get_if<edi::recipe::AddCylinderOp>(&shell.opsStream().ops[0]);
        EDI_CHECK(reCylinder != nullptr && reCylinder->radius == 4.0);

        // Beyond numbers: the cylinder shows a material combo, a name line-edit,
        // a vertices int-spin, and an entasis checkbox — and the scalar verb sets
        // each kind back onto the op.
        EDI_CHECK(shell.findChild<QComboBox *>(QStringLiteral("opField_material")) != nullptr);
        EDI_CHECK(shell.findChild<QLineEdit *>(QStringLiteral("opField_name")) != nullptr);
        EDI_CHECK(shell.findChild<QSpinBox *>(QStringLiteral("opField_vertices")) != nullptr);
        EDI_CHECK(shell.findChild<QCheckBox *>(QStringLiteral("opField_entasis")) != nullptr);
        shell.applyOpScalarEdit(0, QStringLiteral("material"),
                                edi::recipe::RecipeScalarValue{std::string("marble")});
        shell.applyOpScalarEdit(0, QStringLiteral("vertices"), edi::recipe::RecipeScalarValue{32});
        shell.applyOpScalarEdit(0, QStringLiteral("entasis"), edi::recipe::RecipeScalarValue{true});
        const auto *richCylinder = std::get_if<edi::recipe::AddCylinderOp>(&shell.opsStream().ops[0]);
        EDI_CHECK(richCylinder != nullptr);
        EDI_CHECK(richCylinder->material == "marble" && richCylinder->vertices == 32 && richCylinder->entasis);

        // Binding picker: bind the radius to a drafted measurement. The stream
        // carries the binding and the field rebuilds read-only (its number now
        // comes from the canvas through resolve); unbind returns it to a literal.
        shell.bindOpField(0, QStringLiteral("radius"), QStringLiteral("plank_1"), QStringLiteral("length"));
        EDI_CHECK(shell.opsStream().bindings.size() == 1);
        EDI_CHECK(shell.opsStream().bindings[0].objectId == "plank_1");
        EDI_CHECK(shell.opsStream().bindings[0].field == "length");
        auto *boundRadius = shell.findChild<QDoubleSpinBox *>(QStringLiteral("opField_radius"));
        EDI_CHECK(boundRadius != nullptr && boundRadius->isReadOnly());
        shell.unbindOpField(0, QStringLiteral("radius"));
        EDI_CHECK(shell.opsStream().bindings.empty());
        auto *freeRadius = shell.findChild<QDoubleSpinBox *>(QStringLiteral("opField_radius"));
        EDI_CHECK(freeRadius != nullptr && !freeRadius->isReadOnly());

        // The palette appends a step by CLICK: a new unit cylinder joins the
        // recipe, the Steps list grows, and it lands as a real AddCylinder.
        const int before = static_cast<int>(shell.opsStream().ops.size());
        auto *addCylinder = shell.findChild<QPushButton *>(QStringLiteral("addStep_AddCylinder"));
        EDI_CHECK(addCylinder != nullptr);
        addCylinder->click();
        EDI_CHECK(static_cast<int>(shell.opsStream().ops.size()) == before + 1);
        EDI_CHECK(std::get_if<edi::recipe::AddCylinderOp>(&shell.opsStream().ops.back()) != nullptr);
        EDI_CHECK(steps->count() == before + 1); // the Steps list grew with it

        // Remove/reorder buttons drive the verbs: select the just-added step and
        // remove it; the stream shrinks back. (Move's index fixup is covered in
        // recipe_ops_tests; here we confirm the buttons are wired.)
        EDI_CHECK(shell.findChild<QPushButton *>(QStringLiteral("moveStepUp")) != nullptr);
        EDI_CHECK(shell.findChild<QPushButton *>(QStringLiteral("moveStepDown")) != nullptr);
        auto *removeStepButton = shell.findChild<QPushButton *>(QStringLiteral("removeStep"));
        EDI_CHECK(removeStepButton != nullptr);
        steps->setCurrentRow(before); // the appended op sits at the old size
        removeStepButton->click();
        EDI_CHECK(static_cast<int>(shell.opsStream().ops.size()) == before);

        // The projection selector re-renders the chosen view.
        auto *projection = shell.findChild<QComboBox *>(QStringLiteral("asciiPreviewProjection"));
        EDI_CHECK(projection != nullptr);
        projection->setCurrentIndex(1); // Side
        EDI_CHECK(asciiView->toPlainText().contains(QStringLiteral("SIDE PROJECTION")));

        // Pop-out: the ASCII proof's pop-out button floats a fresh copy into a
        // top-level node window carrying its own live proof view.
        auto *popOutAscii = shell.findChild<QToolButton *>(QStringLiteral("popOutAscii"));
        EDI_CHECK(popOutAscii != nullptr);
        popOutAscii->click();
        auto *node = shell.findChild<QWidget *>(QStringLiteral("floatingNode"));
        EDI_CHECK(node != nullptr && node->isWindow());
        EDI_CHECK(node->findChild<QPlainTextEdit *>(QStringLiteral("asciiPreviewText")) != nullptr);
        node->close(); // WA_DeleteOnClose disposes the float
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);

        // Leaving the lab tears the pane down (its opsStreamChanged connection
        // dies with it — the window signal source outlives the panel).
        shell.setWorkspaceMode(edi::app::WorkspaceMode::Drafting);
        EDI_CHECK(shell.findChild<QWidget *>(QStringLiteral("asciiPreviewPanel")) == nullptr);
    }

    // Custom craftsmen: a fed-in registry adds craftsman buttons to the palette,
    // clicking one composes a Script step, and the inspector renders its params
    // with the widgets the MANIFEST types. The registry is INJECTED (main.cpp
    // runs edi_craft --list-craftsmen; the test feeds a literal) and must be set
    // before the lab mounts so the palette is built with the buttons.
    {
        EdiShellWindow craft;
        craft.setCraftsmanRegistryToml(QStringLiteral(
            "craftsman.0.id = \"twisted_column\"\n"
            "craftsman.0.label = \"Twisted Column\"\n"
            "craftsman.0.param.0.key = \"radius\"\n"
            "craftsman.0.param.0.label = \"Radius\"\n"
            "craftsman.0.param.0.type = \"number\"\n"
            "craftsman.0.param.0.default = \"1.0\"\n"
            "craftsman.0.param.1.key = \"sides\"\n"
            "craftsman.0.param.1.label = \"Sides\"\n"
            "craftsman.0.param.1.type = \"integer\"\n"
            "craftsman.0.param.1.default = \"4\"\n"
            "craftsman.0.param.2.key = \"material\"\n"
            "craftsman.0.param.2.label = \"Material\"\n"
            "craftsman.0.param.2.type = \"material\"\n"
            "craftsman.0.param.2.default = \"stone\"\n"));
        EDI_CHECK(craft.craftsmen().size() == 1);
        craft.show();
        craft.setWorkspaceMode(edi::app::WorkspaceMode::Blender);

        // The palette grew a Craftsmen heading + a button per craftsman.
        EDI_CHECK(craft.findChild<QLabel *>(QStringLiteral("craftsmanPaletteTitle")) != nullptr);
        auto *craftBtn = craft.findChild<QPushButton *>(QStringLiteral("addCraftsman_twisted_column"));
        EDI_CHECK(craftBtn != nullptr);

        // Click it: a Script step joins the recipe, seeded from the manifest.
        craftBtn->click();
        EDI_CHECK(craft.opsStream().ops.size() == 1);
        const auto *script = std::get_if<edi::recipe::ScriptOp>(&craft.opsStream().ops[0]);
        EDI_CHECK(script != nullptr && script->scriptId == "twisted_column");
        EDI_CHECK(script->params.size() == 3);

        // Select the step; the inspector renders each param with the widget its
        // MANIFEST type calls for, plus the placement spins and the read-only id.
        auto *steps = craft.findChild<QListWidget *>(QStringLiteral("opStepsList"));
        EDI_CHECK(steps != nullptr && steps->count() == 1);
        steps->setCurrentRow(0);
        auto *radius = craft.findChild<QDoubleSpinBox *>(QStringLiteral("opField_radius"));
        auto *sides = craft.findChild<QSpinBox *>(QStringLiteral("opField_sides"));
        auto *material = craft.findChild<QComboBox *>(QStringLiteral("opField_material"));
        auto *scriptId = craft.findChild<QLineEdit *>(QStringLiteral("opField_script"));
        EDI_CHECK(radius != nullptr && sides != nullptr && material != nullptr && scriptId != nullptr);
        EDI_CHECK(radius->value() == 1.0 && sides->value() == 4);
        EDI_CHECK(material->currentText() == QStringLiteral("stone"));
        EDI_CHECK(!scriptId->isEnabled()); // the craftsman id is a read-only reference
        // x/y/z are still the bindable placement spins.
        EDI_CHECK(craft.findChild<QDoubleSpinBox *>(QStringLiteral("opField_x")) != nullptr);

        // Edit a number param through the REAL widget commit path: its value is
        // stored back AS A STRING in the bag, formatted as the store would.
        sides->setValue(6);
        emit sides->editingFinished();
        radius->setValue(2.5);
        emit radius->editingFinished();
        const auto *tuned = std::get_if<edi::recipe::ScriptOp>(&craft.opsStream().ops[0]);
        EDI_CHECK(tuned != nullptr);
        const auto paramValue = [tuned](const std::string &key) -> std::string {
            for (const edi::recipe::ScriptParam &p : tuned->params) {
                if (p.key == key) return p.value;
            }
            return "<none>";
        };
        EDI_CHECK(paramValue("sides") == "6");
        EDI_CHECK(paramValue("radius") == "2.5");

        // The material dropdown commits a string too (re-fetch the op afterward).
        craft.applyOpScalarEdit(0, QStringLiteral("material"),
                                edi::recipe::RecipeScalarValue{std::string("marble")});
        const auto *afterMaterial = std::get_if<edi::recipe::ScriptOp>(&craft.opsStream().ops[0]);
        EDI_CHECK(afterMaterial != nullptr);
        bool sawMarble = false;
        for (const edi::recipe::ScriptParam &p : afterMaterial->params) {
            sawMarble = sawMarble || (p.key == "material" && p.value == "marble");
        }
        EDI_CHECK(sawMarble);

        // The inspector renders params in MANIFEST order, not the order they sit
        // in the op. Load a recipe whose params sweep ALPHABETICALLY off disk
        // (the store reads from a sorted map), then force a clean field rebuild:
        // the rows must still read radius → sides → material (the manifest's
        // order), never material → radius → sides (alphabetical).
        const QString applyErr = craft.applyOpsScript(
            "op.0.type = \"Script\"\n"
            "op.0.script = \"twisted_column\"\n"
            "op.0.name = \"t\"\n"
            "op.0.radius = \"1\"\n"
            "op.0.sides = \"4\"\n"
            "op.0.material = \"stone\"\n");
        EDI_CHECK(applyErr.isEmpty());
        const auto *loaded = std::get_if<edi::recipe::ScriptOp>(&craft.opsStream().ops[0]);
        EDI_CHECK(loaded != nullptr && loaded->params.size() == 3);
        EDI_CHECK(loaded->params[0].key == "material"); // proof the store swept alphabetically
        steps->setCurrentRow(-1); // force currentRowChanged so the fields rebuild fresh
        steps->setCurrentRow(0);
        auto *fieldsWidget = craft.findChild<QWidget *>(QStringLiteral("opStepsFields"));
        EDI_CHECK(fieldsWidget != nullptr);
        auto *form = qobject_cast<QFormLayout *>(fieldsWidget->layout());
        EDI_CHECK(form != nullptr);
        QStringList rowLabels;
        for (int row = 0; row < form->rowCount(); ++row) {
            if (auto *item = form->itemAt(row, QFormLayout::LabelRole)) {
                if (auto *label = qobject_cast<QLabel *>(item->widget())) {
                    rowLabels << label->text();
                }
            }
        }
        const int iRadius = rowLabels.indexOf(QStringLiteral("Radius"));
        const int iSides = rowLabels.indexOf(QStringLiteral("Sides"));
        const int iMaterial = rowLabels.indexOf(QStringLiteral("Material"));
        EDI_CHECK(iRadius >= 0 && iSides > iRadius && iMaterial > iSides); // manifest order
    }

    // F1 — the object list: a browsable projection of the document. It
    // mirrors object count, tracks selection both ways, and selectObjectById
    // is selection-only (no undo step, same rule as marquee).
    {
        auto *objectList = window.findChild<QListWidget *>(QStringLiteral("objectList"));
        EDI_CHECK(objectList != nullptr);
        const int documentObjects = objectCount(*controller);
        EDI_CHECK(objectList->count() == documentObjects);
        EDI_CHECK(documentObjects >= 2); // earlier sections created point(s) + guide

        // Select by id through the controller; the list's current row follows.
        const QString firstId = objectList->item(0)->data(Qt::UserRole).toString();
        EDI_CHECK(!firstId.isEmpty());
        EDI_CHECK(window.findChild<DrawingDocumentController *>()->selectObjectById(firstId));
        EDI_CHECK(controller->selectedObjectId() == firstId);
        EDI_CHECK(objectList->currentItem() != nullptr);
        EDI_CHECK(objectList->currentItem()->data(Qt::UserRole).toString() == firstId);

        // A bogus id is rejected and selection is untouched.
        EDI_CHECK(!controller->selectObjectById(QStringLiteral("no-such-object")));
        EDI_CHECK(controller->selectedObjectId() == firstId);

        // Creating an object grows the list (the list is a live projection).
        controller->setSelectedToolId(QStringLiteral("point_tool"));
        controller->clickCanvasNormalized(0.6, 0.6);
        EDI_CHECK(objectList->count() == documentObjects + 1);
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
        EDI_CHECK(themed.styleSheet().contains(derived.selected));      // accent-derived token landed
        EDI_CHECK(themed.styleSheet().contains(QStringLiteral("font-size: 14px")));
        EDI_CHECK(themed.themeInputs().accent == pink.accent);

        // A workspace switch rebuilds the canvas; the live theme must follow.
        WorkspaceLayout canvasOnly;
        canvasOnly.id = QStringLiteral("canvas_only");
        canvasOnly.bindings = {{ShellSlot::Main, QStringLiteral("drafting")}};
        themed.switchWorkspaceLayout(canvasOnly);
        EDI_CHECK(themed.styleSheet().contains(derived.selected));

        // Round trip through the settings file into a fresh window.
        QTemporaryDir tempDir;
        EDI_CHECK(tempDir.isValid());
        const QString settingsPath = tempDir.filePath(QStringLiteral("edi.toml"));
        EDI_CHECK(themed.saveSettings(settingsPath));
        EdiShellWindow reloaded;
        EDI_CHECK(!reloaded.styleSheet().contains(derived.selected)); // stock theme before load
        EDI_CHECK(reloaded.loadSettings(settingsPath));
        EDI_CHECK(reloaded.themeInputs().accent == pink.accent);
        EDI_CHECK(reloaded.themeInputs().uiFontSize == 14);
        EDI_CHECK(reloaded.styleSheet().contains(derived.selected));
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
        EDI_CHECK(settingsRail != nullptr && settingsRail->isEnabled());
        settingsRail->click();

        // The drafting job stays mounted; the settings page floats above it.
        EDI_CHECK(shell.findChild<QWidget *>(QStringLiteral("drawingCanvas")) != nullptr);
        EDI_CHECK(shell.findChild<QWidget *>(QStringLiteral("leftPanel")) != nullptr);
        QWidget *popOut = shell.findChild<QWidget *>(QStringLiteral("settingsWindow"));
        EDI_CHECK(popOut != nullptr && popOut->isVisible());
        QWidget *page = shell.findChild<QWidget *>(QStringLiteral("settingsPanel"));
        EDI_CHECK(page != nullptr && popOut->isAncestorOf(page));
        // The rail check stays on the mounted workspace, not on S.
        EDI_CHECK(!settingsRail->isChecked());

        // Typing a valid accent hex re-themes the window immediately; a
        // half-typed value is ignored.
        auto *accentField = shell.findChild<QLineEdit *>(QStringLiteral("themeAccentField"));
        EDI_CHECK(accentField != nullptr);
        EDI_CHECK(accentField->text() == shell.themeInputs().accent); // page mirrors live state
        accentField->setText(QStringLiteral("#d46c"));             // incomplete: no change
        EDI_CHECK(shell.themeInputs().accent != QStringLiteral("#d46c"));
        accentField->setText(QStringLiteral("#d46ca1"));
        EDI_CHECK(shell.themeInputs().accent == QStringLiteral("#d46ca1"));
        ShellThemeInputs expected = shell.themeInputs();
        EDI_CHECK(shell.styleSheet().contains(deriveShellTheme(expected).selected));

        // Font size flows the same way.
        auto *sizeSpin = shell.findChild<QSpinBox *>(QStringLiteral("uiFontSizeSpin"));
        EDI_CHECK(sizeSpin != nullptr);
        sizeSpin->setValue(15);
        EDI_CHECK(shell.themeInputs().uiFontSize == 15);
        EDI_CHECK(shell.styleSheet().contains(QStringLiteral("font-size: 15px")));

        // F6 — the Tool Belt page: a checklist over the tool inventory that
        // writes the workspace's belt and re-dresses the live belt in place.
        {
            QPushButton *beltPageButton = nullptr;
            for (QPushButton *button : shell.findChildren<QPushButton *>(QStringLiteral("settingsPageButton"))) {
                if (button->property("pageId").toString() == QStringLiteral("tool_belt")) {
                    beltPageButton = button;
                }
            }
            EDI_CHECK(beltPageButton != nullptr);
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
            EDI_CHECK(pointBox != nullptr && pointBox->isChecked());
            // Derived from the inventory so adding a tool never breaks this count
            // (this assertion used to hardcode N and broke on every new tool).
            EDI_CHECK(checkedCount == static_cast<int>(DraftingFeature::toolInventory().size()));

            auto *belt = shell.findChild<BeltCrossWidget *>(QStringLiteral("beltCross"));
            EDI_CHECK(belt != nullptr);
            EDI_CHECK(belt->indexOfItem(QStringLiteral("point_tool")) >= 0);

            // Belt pin (M1.1): wall_tool surfaces on the inventory checklist
            // AND resolves to a real cell on the mounted belt — so "in the
            // kDraftingTools table => on every surface" can't silently break.
            bool wallInInventory = false;
            for (const QPair<QString, QString> &entry : DraftingFeature::toolInventory()) {
                wallInInventory = wallInInventory || entry.first == QStringLiteral("wall_tool");
            }
            EDI_CHECK(wallInInventory);
            EDI_CHECK(belt->indexOfItem(QStringLiteral("wall_tool")) >= 0);

            // Uncheck: the live belt loses the tool, and the workspace TOML
            // would save without it.
            pointBox->setChecked(false);
            EDI_CHECK(belt->indexOfItem(QStringLiteral("point_tool")) == -1);
            EDI_CHECK(belt->indexOfItem(QStringLiteral("line_tool")) >= 0); // others untouched
            {
                QTemporaryDir beltDir;
                EDI_CHECK(beltDir.isValid());
                const QString beltPath = beltDir.filePath(QStringLiteral("belt.toml"));
                EDI_CHECK(shell.saveWorkspaceLayout(beltPath));
                const edi::io::ShellLayoutData saved = edi::io::loadShellLayoutFromPath(beltPath);
                bool hasPoint = false;
                for (const QString &id : saved.layout.belt.itemIds) {
                    hasPoint = hasPoint || id == QStringLiteral("point_tool");
                }
                EDI_CHECK(!hasPoint);
            }

            // Re-check: the tool returns to its row.
            pointBox->setChecked(true);
            EDI_CHECK(belt->indexOfItem(QStringLiteral("point_tool")) >= 0);
        }

        // Closing the pop-out hides it; the theme survives, and reopening
        // through the rail brings the same frame back.
        popOut->close();
        EDI_CHECK(!popOut->isVisible());
        EDI_CHECK(shell.themeInputs().accent == QStringLiteral("#d46ca1"));
        settingsRail->click();
        EDI_CHECK(popOut->isVisible());

        // Profiles: snapshot the current theme under a name via the page's
        // save button, scramble, then load the snapshot back.
        QTemporaryDir profileDir;
        EDI_CHECK(profileDir.isValid());
        shell.setProfilesDirectory(profileDir.path());

        // Switch workspaces (away and back) so the page rebuilds with the
        // profile hooks now that the profiles directory is set — the pop-out
        // frame and its visibility survive the remount.
        edi::shell::WorkspaceLayout reset;
        reset.id = QStringLiteral("blank");
        reset.label = QStringLiteral("Blank");
        reset.bindings = {{edi::shell::ShellSlot::Main, QStringLiteral("drafting")}};
        shell.switchWorkspaceLayout(reset);
        EDI_CHECK(shell.findChild<QWidget *>(QStringLiteral("settingsWindow"))->isVisible());
        EDI_CHECK(shell.findChild<QWidget *>(QStringLiteral("settingsPanel")) != nullptr);
        auto *nameField = shell.findChild<QLineEdit *>(QStringLiteral("profileNameField"));
        auto *saveProfile = shell.findChild<QPushButton *>(QStringLiteral("saveProfileButton"));
        auto *profileCombo = shell.findChild<QComboBox *>(QStringLiteral("profileCombo"));
        EDI_CHECK(nameField != nullptr && saveProfile != nullptr && profileCombo != nullptr);
        EDI_CHECK(profileCombo->count() == 0); // empty dir, no profiles yet

        nameField->setText(QStringLiteral("pink lab"));
        saveProfile->click();
        EDI_CHECK(shell.availableProfiles() == QStringList{QStringLiteral("pink lab")});
        EDI_CHECK(shell.activeProfile() == QStringLiteral("pink lab"));
        EDI_CHECK(profileCombo->count() == 1); // the combo re-listed itself

        // Scramble the theme, then load the profile back through the API.
        ShellThemeInputs scrambled = shell.themeInputs();
        scrambled.accent = QStringLiteral("#11aa22");
        shell.setThemeInputs(scrambled);
        EDI_CHECK(shell.themeInputs().accent == QStringLiteral("#11aa22"));
        EDI_CHECK(shell.loadProfile(QStringLiteral("pink lab")));
        EDI_CHECK(shell.themeInputs().accent == QStringLiteral("#d46ca1"));

        // A hostile name degrades to its cleaned form instead of escaping the
        // profiles directory; a missing profile load keeps the current theme.
        EDI_CHECK(shell.saveProfileAs(QStringLiteral("../evil/../../name")));
        EDI_CHECK(shell.availableProfiles().contains(QStringLiteral("evilname")));
        EDI_CHECK(!shell.loadProfile(QStringLiteral("never-saved")));
        EDI_CHECK(shell.themeInputs().accent == QStringLiteral("#d46ca1"));

        // The active profile name rides along in edi.toml.
        QTemporaryDir settingsDir;
        EDI_CHECK(settingsDir.isValid());
        const QString settingsPath = settingsDir.filePath(QStringLiteral("edi.toml"));
        EDI_CHECK(shell.loadProfile(QStringLiteral("pink lab"))); // make it active again
        EDI_CHECK(shell.saveSettings(settingsPath));
        EdiShellWindow remembered;
        EDI_CHECK(remembered.loadSettings(settingsPath));
        EDI_CHECK(remembered.activeProfile() == QStringLiteral("pink lab"));
    }

    // Title-bar chrome: frameless flag, traffic lights, panel toggles, the
    // back/forward workspace trail, and the File/Edit/Settings menus.
    {
        using edi::shell::PanelVisibility;
        using edi::shell::ShellSlot;
        using edi::shell::WorkspaceLayout;

        EdiShellWindow chrome;
        chrome.show();
        EDI_CHECK(chrome.windowFlags().testFlag(Qt::FramelessWindowHint));

        QWidget *titleBar = chrome.findChild<QWidget *>(QStringLiteral("titleBar"));
        EDI_CHECK(titleBar != nullptr && titleBar->height() == 42);

        // Panel toggles drive the modeled state and mirror it back as checked.
        QPushButton *leftToggle = buttonNamed(chrome, QStringLiteral("toggleLeftPanel"));
        QPushButton *rightToggle = buttonNamed(chrome, QStringLiteral("toggleRightPanel"));
        QPushButton *bottomToggle = buttonNamed(chrome, QStringLiteral("toggleBottomPanel"));
        EDI_CHECK(leftToggle != nullptr && rightToggle != nullptr && bottomToggle != nullptr);
        EDI_CHECK(leftToggle->isChecked());    // left starts open
        EDI_CHECK(!rightToggle->isChecked());  // right starts collapsed
        leftToggle->click();
        EDI_CHECK(chrome.shellPanelVisibility(ShellSlot::Left) == PanelVisibility::Collapsed);
        EDI_CHECK(!leftToggle->isChecked());
        leftToggle->click();
        EDI_CHECK(chrome.shellPanelVisibility(ShellSlot::Left) == PanelVisibility::Visible);
        rightToggle->click();
        EDI_CHECK(chrome.shellPanelVisibility(ShellSlot::Right) == PanelVisibility::Visible);

        // Back/forward walk the workspace trail; a fresh trail has one entry.
        QPushButton *back = buttonNamed(chrome, QStringLiteral("workspaceBack"));
        QPushButton *forward = buttonNamed(chrome, QStringLiteral("workspaceForward"));
        EDI_CHECK(back != nullptr && forward != nullptr);
        EDI_CHECK(!back->isEnabled() && !forward->isEnabled());

        WorkspaceLayout canvasOnly;
        canvasOnly.id = QStringLiteral("canvas_only");
        canvasOnly.bindings = {{ShellSlot::Main, QStringLiteral("drafting")}};
        chrome.switchWorkspaceLayout(canvasOnly);
        EDI_CHECK(back->isEnabled() && !forward->isEnabled());
        EDI_CHECK(chrome.findChild<QWidget *>(QStringLiteral("leftPanel")) == nullptr);

        back->click(); // back to the full drafting job
        EDI_CHECK(chrome.findChild<QWidget *>(QStringLiteral("leftPanel")) != nullptr);
        EDI_CHECK(!back->isEnabled() && forward->isEnabled());
        forward->click();
        EDI_CHECK(chrome.findChild<QWidget *>(QStringLiteral("leftPanel")) == nullptr);
        EDI_CHECK(back->isEnabled() && !forward->isEnabled());

        // Going back and switching somewhere new truncates the forward trail.
        back->click();
        EDI_CHECK(forward->isEnabled());
        WorkspaceLayout third; // any distinct job
        third.id = QStringLiteral("third");
        third.bindings = {{ShellSlot::Main, QStringLiteral("drafting")}, {ShellSlot::Right, QStringLiteral("drafting")}};
        chrome.switchWorkspaceLayout(third);
        EDI_CHECK(!forward->isEnabled() && back->isEnabled());
        // The truncation is observable in where back lands: one step behind
        // "third" must be the full drafting job, not the stale canvas-only
        // entry the truncation discarded.
        back->click();
        EDI_CHECK(chrome.findChild<QWidget *>(QStringLiteral("leftPanel")) != nullptr);
        EDI_CHECK(!back->isEnabled()); // i.e. the trail is exactly two entries deep
        forward->click();           // return to "third" for the sections below

        // Menus: File carries the IO verbs (not triggered — they open
        // dialogs); Edit's Undo really undoes; Settings applies presets.
        auto *fileMenu = chrome.findChild<QMenu *>(QStringLiteral("fileMenu"));
        auto *editMenu = chrome.findChild<QMenu *>(QStringLiteral("editMenu"));
        auto *settingsMenu = chrome.findChild<QMenu *>(QStringLiteral("settingsMenu"));
        EDI_CHECK(fileMenu != nullptr && editMenu != nullptr && settingsMenu != nullptr);
        EDI_CHECK(fileMenu->actions().size() == 13); // 10 verbs + Open Recent + 2 separators (pipeline A's three verbs AND their fence separator retired, R1-B06)

        auto *chromeController = chrome.findChild<DrawingDocumentController *>();
        EDI_CHECK(chromeController != nullptr);
        chromeController->setSelectedToolId(QStringLiteral("point_tool"));
        chromeController->clickCanvasNormalized(0.4, 0.4);
        const int before = objectCount(*chromeController);
        editMenu->actions().at(0)->trigger(); // Undo
        EDI_CHECK(objectCount(*chromeController) == before - 1);
        editMenu->actions().at(1)->trigger(); // Redo
        EDI_CHECK(objectCount(*chromeController) == before);

        settingsMenu->actions().at(1)->trigger(); // Focus layout
        EDI_CHECK(chrome.shellPanelVisibility(ShellSlot::Left) == PanelVisibility::Collapsed);
        EDI_CHECK(chrome.shellPanelVisibility(ShellSlot::Right) == PanelVisibility::Collapsed);

        // Traffic close really closes the window. The document is dirty from
        // the undo/redo dance above, so the #18 guard asks on the way out —
        // proof the traffic light routes through it. Discard via the
        // injected prompt (the real dialog would hang the offscreen run).
        int closeGuardAsks = 0;
        chrome.setDirtyGuardPrompt([&closeGuardAsks]() {
            ++closeGuardAsks;
            return EdiShellWindow::DirtyGuardChoice::Discard;
        });
        QPushButton *closeButton = buttonNamed(chrome, QStringLiteral("trafficClose"));
        EDI_CHECK(closeButton != nullptr);
        EDI_CHECK(chrome.isVisible());
        closeButton->click();
        EDI_CHECK(!chrome.isVisible());
        EDI_CHECK(closeGuardAsks == 1);
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
        EDI_CHECK(rendered.devicePixelRatio() == 1.0); // offscreen: probe == pixel

        QWidget *leftPanel = proof.findChild<QWidget *>(QStringLiteral("leftPanel"));
        EDI_CHECK(leftPanel != nullptr && leftPanel->isVisible());
        // Probe the panel's bottom stretch area — no child widget owns it, so
        // the color is the panel frame's own fill.
        const QPoint panelProbe = leftPanel->mapTo(
            &proof, QPoint(leftPanel->width() / 2, leftPanel->height() - 8));
        EDI_CHECK(QColor(rendered.pixel(panelProbe)).name() == theme.surface);

        auto *objectList = proof.findChild<QListWidget *>(QStringLiteral("objectList"));
        EDI_CHECK(objectList != nullptr);
        const QPoint listProbe = objectList->viewport()->mapTo(
            &proof, QPoint(objectList->viewport()->width() / 2,
                           objectList->viewport()->height() / 2));
        EDI_CHECK(QColor(rendered.pixel(listProbe)).name() == theme.surface);

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
            EDI_CHECK(light != nullptr);
            EDI_CHECK(light->size() == QSize(14, 14));
            const QPoint center = light->mapTo(&proof, QPoint(7, 7));
            EDI_CHECK(QColor(rendered.pixel(center)).name() == fill);
        }

        // The belt paints through QPalette roles; applyShellStyle must push
        // the theme-derived painting palette onto every mounted belt (the
        // pixel-level proof lives in belt_cross_widget_tests).
        auto *beltWidget = proof.findChild<BeltCrossWidget *>();
        EDI_CHECK(beltWidget != nullptr);
        EDI_CHECK(beltWidget->palette().color(QPalette::Base).name() == theme.control);
        EDI_CHECK(beltWidget->palette().color(QPalette::Highlight).name() == theme.selected);
        EDI_CHECK(beltWidget->palette().color(QPalette::Text).name() == theme.text);

        // Typography: stylesheet fonts do not propagate like setFont — the
        // universal QWidget rule is what carries the theme face/size to every
        // control. A deep child (a panel button) proves the rule reaches it.
        QPushButton *fontProbe = buttonNamed(proof, QStringLiteral("toggleLeftPanel"));
        EDI_CHECK(fontProbe != nullptr);
        EDI_CHECK(fontProbe->font().pixelSize() == theme.fontSizeBody);
        EDI_CHECK(fontProbe->font().family() == theme.uiFont);

        // Auxiliary surfaces paint tokens, not the platform popup gray.
        // Menus: grab the widget directly (offscreen popups render fine) and
        // probe inside the 4px padding ring, clear of border and items.
        auto *menuProbe = proof.findChild<QMenu *>(QStringLiteral("fileMenu"));
        EDI_CHECK(menuProbe != nullptr);
        menuProbe->adjustSize();
        const QImage menuImage = menuProbe->grab().toImage();
        EDI_CHECK(QColor(menuImage.pixel(3, 3)).name() == theme.surfaceRaised);

        // Chrome popups (the Snap panel): QObject children of the window, so
        // they grab without being shown; probe inside the 12px margins.
        QFrame *snapPopup = proof.findChild<QFrame *>(QStringLiteral("chromePopup_snap"));
        EDI_CHECK(snapPopup != nullptr);
        EDI_CHECK(snapPopup->property("chromePopup").toBool());
        const QImage popupImage = snapPopup->grab().toImage();
        EDI_CHECK(QColor(popupImage.pixel(6, 6)).name() == theme.surfaceRaised);

        // Settings pop-out: plain QWidget — without WA_StyledBackground its
        // background rule is silently ignored. Grab works unshown; the frame
        // is a permanent window-owned child.
        QWidget *settingsWindow = proof.findChild<QWidget *>(QStringLiteral("settingsWindow"));
        EDI_CHECK(settingsWindow != nullptr);
        const QImage settingsImage = settingsWindow->grab().toImage();
        EDI_CHECK(QColor(settingsImage.pixel(2, settingsImage.height() - 2)).name() == theme.base);

        // Tooltips are top-level: only the application sheet reaches them.
        EDI_CHECK(qApp->styleSheet().contains(theme.surfaceRaised));

        // Control treatment (spec §4): 30px button boxes (20px QSS content +
        // padding + border) and the pointing-hand cursor, swept on by
        // applyShellStyle since QSS has no cursor property.
        EDI_CHECK(fontProbe->cursor().shape() == Qt::PointingHandCursor);
        // QSS min-height lands as the widget's minimumSize at polish time
        // (not in sizeHint) — assert on the laid-out height.
        EDI_CHECK(fontProbe->height() >= 30);
        // Section headers opt out: spec keeps them a compact ~20px row.
        QPushButton *sectionProbe = nullptr;
        for (QPushButton *candidate : proof.findChildren<QPushButton *>(QStringLiteral("sectionToggle"))) {
            sectionProbe = candidate;
            break;
        }
        EDI_CHECK(sectionProbe != nullptr);
        EDI_CHECK(sectionProbe->height() <= 24);
    }

    // Toggle-switch knobs: the pill track shows a knob band at the off/on
    // end (gradient hard stops). Scan the indicator strip of a checked and
    // an unchecked toggle and assert both knob and track colors appear —
    // membership over a strip, not pixel equality, so the radius clip and
    // band boundaries cannot flake the test.
    {
        EdiShellWindow knobs;
        knobs.resize(1100, 760);
        knobs.show();
        // The layer toggles live in the right panel, which starts collapsed
        // (spec initial state) — open it so the indicators render.
        knobs.setPanelCollapsed(edi::shell::ShellSlot::Right, false);
        QCoreApplication::processEvents();
        const edi::shell::ShellTheme knobTheme =
            edi::shell::deriveShellTheme(edi::shell::ShellThemeInputs{});
        const QImage knobShot = knobs.grab().toImage();
        const auto stripColors = [&knobs, &knobShot](QCheckBox *box, int fromX, int toX) {
            QSet<QString> seen;
            const QPoint origin = box->mapTo(&knobs, QPoint(0, box->height() / 2));
            for (int x = fromX; x <= toX; ++x) {
                seen.insert(QColor(knobShot.pixel(origin + QPoint(x, 0))).name());
            }
            return seen;
        };
        // By objectName, not label: the hidden Snap popup also carries a
        // "Visible" checkbox (the grid), and label search finds it first.
        QCheckBox *checkedToggle = nullptr;
        QCheckBox *uncheckedToggle = nullptr;
        for (QCheckBox *box : knobs.findChildren<QCheckBox *>(QStringLiteral("layerFlagCheckbox"))) {
            (box->text() == QStringLiteral("Visible") ? checkedToggle : uncheckedToggle) = box;
        }
        EDI_CHECK(checkedToggle != nullptr && checkedToggle->isChecked());
        EDI_CHECK(checkedToggle->isVisibleTo(&knobs));
        // Ends are asserted separately (review find: a whole-strip membership
        // check passes with the knob on the WRONG end). On: knob right, track
        // left. Off: knob left, track right.
        const QSet<QString> onLeft = stripColors(checkedToggle, 2, 9);
        const QSet<QString> onRight = stripColors(checkedToggle, 19, 26);
        EDI_CHECK(onLeft.contains(knobTheme.accentSoft) && !onLeft.contains(knobTheme.accent));
        EDI_CHECK(onRight.contains(knobTheme.accent));

        EDI_CHECK(uncheckedToggle != nullptr && !uncheckedToggle->isChecked());
        const QSet<QString> offLeft = stripColors(uncheckedToggle, 2, 9);
        const QSet<QString> offRight = stripColors(uncheckedToggle, 19, 26);
        EDI_CHECK(offLeft.contains(knobTheme.textFaint));
        EDI_CHECK(offRight.contains(knobTheme.control) && !offRight.contains(knobTheme.textFaint));
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
        EDI_CHECK(leftToggle != nullptr);
        EDI_CHECK(leftToggle->size() == QSize(30, 30)); // spec square, sheet-enforced
        // Icon 16x14 centered in 30x30 -> origin (7,8); the left bar's
        // center sits at icon (3,7).
        const QPoint barProbe = leftToggle->mapTo(&faces, QPoint(7 + 3, 8 + 7));
        QImage faceShot = faces.grab().toImage();
        EDI_CHECK(QColor(faceShot.pixel(barProbe)).name() == faceTheme.accent); // visible

        // The right and bottom faces get their own probes (review find: the
        // golden's pixel budget is far larger than one icon, so an unprobed
        // face is an unlocked face). Right bar rect(10,1,5,12) -> center
        // (12,7); bottom bar rect(2,8,12,5) -> center (8,10).
        QPushButton *rightToggle = buttonNamed(faces, QStringLiteral("toggleRightPanel"));
        QPushButton *bottomToggle = buttonNamed(faces, QStringLiteral("toggleBottomPanel"));
        EDI_CHECK(rightToggle != nullptr && bottomToggle != nullptr);
        // Right and bottom panels start collapsed (spec): their bars are faint.
        EDI_CHECK(QColor(faceShot.pixel(rightToggle->mapTo(&faces, QPoint(7 + 12, 8 + 7)))).name()
               == faceTheme.textFaint);
        EDI_CHECK(QColor(faceShot.pixel(bottomToggle->mapTo(&faces, QPoint(7 + 8, 8 + 10)))).name()
               == faceTheme.textFaint);

        faces.setPanelCollapsed(edi::shell::ShellSlot::Left, true);
        faceShot = faces.grab().toImage();
        EDI_CHECK(QColor(faceShot.pixel(barProbe)).name() == faceTheme.textFaint); // collapsed

        faces.resize(600, 760); // under the auto-hide threshold
        QCoreApplication::processEvents();
        faces.setPanelCollapsed(edi::shell::ShellSlot::Left, false);
        faceShot = faces.grab().toImage();
        EDI_CHECK(QColor(faceShot.pixel(barProbe)).name() == faceTheme.warning); // auto-hidden
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
        EDI_CHECK(tiny.size() == QSize(520, 420));
        QWidget *bar = tiny.findChild<QWidget *>(QStringLiteral("titleBar"));
        EDI_CHECK(bar != nullptr);
        for (QPushButton *control : bar->findChildren<QPushButton *>()) {
            if (!control->isVisibleTo(bar)) {
                continue;
            }
            const QRect inBar(control->mapTo(bar, QPoint(0, 0)), control->size());
            EDI_CHECK(bar->rect().contains(inBar));
        }
        QWidget *rail = tiny.findChild<QWidget *>(QStringLiteral("activityRail"));
        EDI_CHECK(rail != nullptr && rail->width() == 52);
        for (QPushButton *railButton : rail->findChildren<QPushButton *>()) {
            EDI_CHECK(railButton->size() == QSize(34, 34));
        }
    }

    // Modular panels: the Panels settings page assigns a group to a panel;
    // the group REPARENTS live (no remount — the combo that did it survives),
    // and the assignment rides workspace.toml across a restart.
    {
        QTemporaryDir panelDir;
        const QString panelLayoutPath = panelDir.filePath(QStringLiteral("workspace.toml"));
        {
            EdiShellWindow assigner;
            assigner.resize(1100, 760);
            assigner.show();
            QCoreApplication::processEvents();
            QWidget *layersGroup = assigner.findChild<QWidget *>(QStringLiteral("inspectorGroup_layers_document"));
            QWidget *rightPanel = assigner.findChild<QWidget *>(QStringLiteral("rightPanel"));
            QWidget *leftPanel = assigner.findChild<QWidget *>(QStringLiteral("leftPanel"));
            EDI_CHECK(layersGroup != nullptr && rightPanel != nullptr && leftPanel != nullptr);
            EDI_CHECK(rightPanel->isAncestorOf(layersGroup)); // default home

            // Drive the real settings page combo.
            QComboBox *layersCombo = nullptr;
            for (QComboBox *combo : assigner.findChildren<QComboBox *>(QStringLiteral("panelSlotCombo"))) {
                if (combo->property("groupId").toString() == QStringLiteral("layers_document")) {
                    layersCombo = combo;
                }
            }
            EDI_CHECK(layersCombo != nullptr);
            layersCombo->setCurrentIndex(layersCombo->findData(QStringLiteral("left")));
            EDI_CHECK(leftPanel->isAncestorOf(layersGroup));   // moved live
            EDI_CHECK(!rightPanel->isAncestorOf(layersGroup));
            // The combo survived its own edit (live reparent, no remount).
            EDI_CHECK(layersCombo->currentData().toString() == QStringLiteral("left"));
            EDI_CHECK(assigner.saveWorkspaceLayout(panelLayoutPath));
        }

        EdiShellWindow restorer;
        EDI_CHECK(restorer.loadWorkspaceLayout(panelLayoutPath));
        QWidget *restoredGroup = restorer.findChild<QWidget *>(QStringLiteral("inspectorGroup_layers_document"));
        QWidget *restoredLeft = restorer.findChild<QWidget *>(QStringLiteral("leftPanel"));
        EDI_CHECK(restoredGroup != nullptr && restoredLeft != nullptr);
        EDI_CHECK(restoredLeft->isAncestorOf(restoredGroup)); // the move survived restart
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
            EDI_CHECK(saverBelt != nullptr);
            // Freeze the active row by the same gesture a user makes: click
            // the pin nub (gutter, centered on the live row).
            const QPointF pinNub(5.0, 21.0 + 17.0);
            QMouseEvent pinPress(QEvent::MouseButtonPress, pinNub,
                                 saverBelt->mapToGlobal(pinNub.toPoint()), Qt::LeftButton,
                                 Qt::LeftButton, Qt::NoModifier);
            QCoreApplication::sendEvent(saverBelt, &pinPress);
            EDI_CHECK(saverBelt->pinnedRows() == std::vector<int>{0});
            EDI_CHECK(saver.saveWorkspaceLayout(layoutPath));
        }

        EdiShellWindow loader;
        EDI_CHECK(loader.loadWorkspaceLayout(layoutPath));
        QPushButton *back = buttonNamed(loader, QStringLiteral("workspaceBack"));
        QPushButton *forward = buttonNamed(loader, QStringLiteral("workspaceForward"));
        EDI_CHECK(back != nullptr && forward != nullptr);
        EDI_CHECK(!back->isEnabled() && !forward->isEnabled()); // a single-entry trail
        auto *loaderBelt = loader.findChild<BeltCrossWidget *>();
        EDI_CHECK(loaderBelt != nullptr);
        EDI_CHECK(loaderBelt->pinnedRows() == std::vector<int>{0}); // the quick-bar survived restart
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
        EDI_CHECK(checklistBelt != nullptr);
        const QPointF nub(5.0, 21.0 + 17.0);
        QMouseEvent nubPress(QEvent::MouseButtonPress, nub,
                             checklistBelt->mapToGlobal(nub.toPoint()), Qt::LeftButton,
                             Qt::LeftButton, Qt::NoModifier);
        QCoreApplication::sendEvent(checklistBelt, &nubPress);
        EDI_CHECK(checklistBelt->pinnedRows() == std::vector<int>{0}); // select row frozen

        // Drive the same hook the settings checklist uses: open the settings
        // window and uncheck one tool that is NOT on the pinned row.
        QPushButton *settingsRail = nullptr;
        for (QPushButton *button : checklist.findChildren<QPushButton *>(QStringLiteral("railButton"))) {
            if (button->property("modeId").toString() == QStringLiteral("settings")) {
                settingsRail = button;
            }
        }
        EDI_CHECK(settingsRail != nullptr);
        settingsRail->click();
        QCheckBox *circleToggle = nullptr;
        for (QCheckBox *box : checklist.findChildren<QCheckBox *>(QStringLiteral("beltToolCheckbox"))) {
            if (box->property("toolId").toString() == QStringLiteral("circle_tool")) {
                circleToggle = box;
            }
        }
        EDI_CHECK(circleToggle != nullptr && circleToggle->isChecked());
        circleToggle->click();
        // The belt was re-dressed in place; the frozen quick-bar survived.
        auto *redressedBelt = checklist.findChild<BeltCrossWidget *>();
        EDI_CHECK(redressedBelt != nullptr);
        EDI_CHECK(redressedBelt->pinnedRows() == std::vector<int>{0});
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
        EDI_CHECK(leftToggle != nullptr);
        EDI_CHECK(leftToggle->property("panelState").toString() == QStringLiteral("visible"));

        triState.setPanelCollapsed(edi::shell::ShellSlot::Left, true);
        EDI_CHECK(leftToggle->property("panelState").toString() == QStringLiteral("collapsed"));
        triState.setPanelCollapsed(edi::shell::ShellSlot::Left, false);
        EDI_CHECK(leftToggle->property("panelState").toString() == QStringLiteral("visible"));

        // Auto-hide reacts to the window, not to clicks: shrink under the
        // 640px threshold and the left panel reports auto_hidden, distinct
        // from the manual collapse above.
        triState.resize(600, 760);
        QCoreApplication::processEvents();
        EDI_CHECK(leftToggle->property("panelState").toString() == QStringLiteral("auto_hidden"));
        triState.resize(1100, 760);
        QCoreApplication::processEvents();
        EDI_CHECK(leftToggle->property("panelState").toString() == QStringLiteral("visible"));
    }

    // Blender workspace: the SECOND real layout. Its rail button mounts a
    // distinct named job (canvas in Main + editor in Bottom, like drafting) and
    // records the switch on the back/forward trail — proof the previously-dead
    // multi-workspace path now lives.
    {
        EdiShellWindow blenderShell;
        blenderShell.show();
        auto railButton = [&](const QString &modeId) -> QPushButton * {
            for (QPushButton *button : blenderShell.findChildren<QPushButton *>(QStringLiteral("railButton"))) {
                if (button->property("modeId").toString() == modeId) {
                    return button;
                }
            }
            return nullptr;
        };

        QPushButton *blenderRail = railButton(QStringLiteral("blender"));
        EDI_CHECK(blenderRail != nullptr && blenderRail->isEnabled());
        EDI_CHECK(railButton(QStringLiteral("drafting"))->isChecked());
        EDI_CHECK(blenderShell.findChild<QWidget *>(QStringLiteral("drawingCanvas")) != nullptr);
        EDI_CHECK(blenderShell.findChild<QWidget *>(QStringLiteral("textEditorView")) != nullptr);
        auto *back = blenderShell.findChild<QPushButton *>(QStringLiteral("workspaceBack"));
        EDI_CHECK(back != nullptr && !back->isEnabled()); // no history yet

        blenderRail->click();
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete); // retire old slot widgets first

        // The Blender job mounts the same canvas + editor (its bindings mirror
        // drafting), and the switch pushed a back-history entry.
        EDI_CHECK(blenderShell.findChild<QWidget *>(QStringLiteral("drawingCanvas")) != nullptr);
        EDI_CHECK(blenderShell.findChild<QWidget *>(QStringLiteral("textEditorView")) != nullptr);
        EDI_CHECK(railButton(QStringLiteral("blender"))->isChecked());
        EDI_CHECK(!railButton(QStringLiteral("drafting"))->isChecked());
        EDI_CHECK(blenderShell.findChild<QPushButton *>(QStringLiteral("workspaceBack"))->isEnabled());

        // And back to drafting.
        railButton(QStringLiteral("drafting"))->click();
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        EDI_CHECK(railButton(QStringLiteral("drafting"))->isChecked());
        EDI_CHECK(blenderShell.findChild<QWidget *>(QStringLiteral("drawingCanvas")) != nullptr);
    }

    // Build (the Blender lab): the editor's Build button hands the active .py to
    // the window, which plans a Blender render and calls the runner. Injected
    // runner = the suite NEVER spawns; the plan + subprocess are proven in their
    // own unit tests.
    {
        auto makePythonShell = [](EdiShellWindow &shell, const QString &id) {
            edi::text::TextDocumentStore &store = shell.textDocumentStore();
            edi::text::TextDocument py;
            py.id = id.toStdString();
            py.text = "import bpy\n";
            py.metadata.fields["path"] = ("/tmp/" + id + ".py").toStdString();
            edi::text::addDocument(store, py);
            edi::text::setActiveDocument(store, id.toStdString());
            // Switch to the Blender workspace so the panel rebuilds from the
            // store with the .py document active (which enables Build).
            for (QPushButton *button : shell.findChildren<QPushButton *>(QStringLiteral("railButton"))) {
                if (button->property("modeId").toString() == QStringLiteral("blender")) {
                    button->click();
                }
            }
            QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        };

        // Configured Blender + a .py document: Build reaches the runner with the
        // render argv, and the status shows the "building…" ack.
        EdiShellWindow buildShell;
        buildShell.setBlenderExecutablePath(QStringLiteral("/fake/blender"));
        bool runnerCalled = false;
        edi::scripting::BlenderRunPlan capturedPlan;
        buildShell.setBlenderRunner([&](const edi::scripting::BlenderRunPlan &plan) {
            runnerCalled = true;
            capturedPlan = plan;
        });
        makePythonShell(buildShell, QStringLiteral("scene_py"));

        auto *buildButton = buildShell.findChild<QPushButton *>(QStringLiteral("textEditorBuild"));
        auto *buildStatus = buildShell.findChild<QLabel *>(QStringLiteral("textEditorStatus"));
        EDI_CHECK(buildButton != nullptr && buildStatus != nullptr);
        EDI_CHECK(buildButton->isEnabled()); // the active document is a .py
        buildButton->click();
        EDI_CHECK(runnerCalled && capturedPlan.ok);
        EDI_CHECK(capturedPlan.args.size() >= 5);
        EDI_CHECK(capturedPlan.args[0] == "--background" && capturedPlan.args[1] == "--python");
        EDI_CHECK(buildStatus->text().contains(QStringLiteral("building")));

        // No Blender configured: Build shows the named refusal and never spawns
        // — the executable-empty case routes through the pure plan's message.
        EdiShellWindow noBlenderShell;
        bool stubCalled = false;
        noBlenderShell.setBlenderRunner([&](const edi::scripting::BlenderRunPlan &) { stubCalled = true; });
        makePythonShell(noBlenderShell, QStringLiteral("scene2_py"));
        auto *refuseButton = noBlenderShell.findChild<QPushButton *>(QStringLiteral("textEditorBuild"));
        auto *refuseStatus = noBlenderShell.findChild<QLabel *>(QStringLiteral("textEditorStatus"));
        EDI_CHECK(refuseButton->isEnabled());
        refuseButton->click();
        EDI_CHECK(!stubCalled);
        EDI_CHECK(refuseStatus->text().contains(QStringLiteral("blender.executable_path")));

        // Build is gated to python: on the seeded plain "scratch" document
        // (no .py path), the button is disabled — never a category error.
        EdiShellWindow scratchShell;
        for (QPushButton *button : scratchShell.findChildren<QPushButton *>(QStringLiteral("railButton"))) {
            if (button->property("modeId").toString() == QStringLiteral("blender")) {
                button->click();
            }
        }
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        auto *gatedButton = scratchShell.findChild<QPushButton *>(QStringLiteral("textEditorBuild"));
        EDI_CHECK(gatedButton != nullptr && !gatedButton->isEnabled());
    }

    // Render preview — the app's FIRST raster surface. The Blender profile's
    // Right slot hosts a preview pane (drafting has none); showRenderImage loads
    // a real PNG into it.
    {
        EdiShellWindow previewShell;
        // Drafting profile: no preview pane.
        EDI_CHECK(previewShell.findChild<QLabel *>(QStringLiteral("blenderPreview")) == nullptr);

        for (QPushButton *button : previewShell.findChildren<QPushButton *>(QStringLiteral("railButton"))) {
            if (button->property("modeId").toString() == QStringLiteral("blender")) {
                button->click();
            }
        }
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        auto *preview = previewShell.findChild<QLabel *>(QStringLiteral("blenderPreview"));
        EDI_CHECK(preview != nullptr);          // the pane mounts in the Blender profile
        EDI_CHECK(preview->pixmap().isNull());  // nothing rendered yet

        // A real PNG on disk, loaded into the pane.
        const QString pngPath = QDir::tempPath() + QStringLiteral("/edi_preview_test.png");
        QImage rendered(40, 30, QImage::Format_RGBA8888);
        rendered.fill(Qt::blue);
        EDI_CHECK(rendered.save(pngPath));
        previewShell.showRenderImage(pngPath);
        EDI_CHECK(!preview->pixmap().isNull());
        EDI_CHECK(preview->pixmap().width() > 0);

        // The render survives a workspace switch (drafting and back): the pane is
        // rebuilt, but it re-shows the remembered image.
        for (QPushButton *button : previewShell.findChildren<QPushButton *>(QStringLiteral("railButton"))) {
            if (button->property("modeId").toString() == QStringLiteral("drafting")) {
                button->click();
            }
        }
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        for (QPushButton *button : previewShell.findChildren<QPushButton *>(QStringLiteral("railButton"))) {
            if (button->property("modeId").toString() == QStringLiteral("blender")) {
                button->click();
            }
        }
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        auto *rebuilt = previewShell.findChild<QLabel *>(QStringLiteral("blenderPreview"));
        EDI_CHECK(rebuilt != nullptr && !rebuilt->pixmap().isNull());
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
        EDI_CHECK(statusBar != nullptr && modeLabel != nullptr && fileLabel != nullptr);
        EDI_CHECK(statusBar->height() == 28 || statusBar->sizeHint().height() == 28);
        // The drafting feature publishes its mode line into the status bar
        // (it lived in the title bar before — the user's chrome inventory
        // has no status slot there). The zoom readout rides at the end.
        EDI_CHECK(modeLabel->text().contains(QStringLiteral("drafting")));
        EDI_CHECK(modeLabel->text().contains(QStringLiteral("100%")));
        EDI_CHECK(statusWindow.findChild<QLabel *>(QStringLiteral("chromeStatus")) == nullptr);

        auto *statusController = statusWindow.findChild<DrawingDocumentController *>();
        EDI_CHECK(statusController != nullptr);
        EDI_CHECK(!fileLabel->property("documentDirty").toBool());
        EDI_CHECK(fileLabel->text() == QStringLiteral("Untitled"));
        statusController->setSelectedToolId(QStringLiteral("point_tool"));
        statusController->clickCanvasNormalized(0.5, 0.5);
        EDI_CHECK(fileLabel->property("documentDirty").toBool());
        EDI_CHECK(fileLabel->text().contains(QStringLiteral("•")));
        statusController->undo();
        EDI_CHECK(!fileLabel->property("documentDirty").toBool());
    }

    // F1 empty state: the object list names its own absence ("No objects
    // yet"), and the label is a projection of the document like every other
    // inspector readout — created objects hide it, an emptied document brings
    // it back.
    {
        EdiShellWindow emptyState;
        auto *stateController = emptyState.findChild<DrawingDocumentController *>();
        auto *emptyLabel = emptyState.findChild<QLabel *>(QStringLiteral("objectListEmpty"));
        EDI_CHECK(stateController != nullptr && emptyLabel != nullptr);
        EDI_CHECK(objectCount(*stateController) == 0);
        EDI_CHECK(emptyLabel->isVisibleTo(&emptyState));

        stateController->setSelectedToolId(QStringLiteral("point_tool"));
        stateController->clickCanvasNormalized(0.5, 0.5);
        EDI_CHECK(objectCount(*stateController) == 1);
        EDI_CHECK(!emptyLabel->isVisibleTo(&emptyState));

        stateController->undo();
        EDI_CHECK(objectCount(*stateController) == 0);
        EDI_CHECK(emptyLabel->isVisibleTo(&emptyState));
    }

    // Golden-render lock: the whole default-theme shell against a checked-in
    // reference. Probes prove single pixels; this catches the class they
    // miss — layout shifts, clipped text, z-order, accidental restyles.
    // Machine-local by design (font rasterization differs across machines);
    // after an INTENDED look change, re-bless with:
    //   EDI_BLESS_GOLDEN=1 ./build/edi_shell_window_tests
    {
        EdiShellWindow golden;
        golden.resize(1100, 760);
        golden.show();
        QCoreApplication::processEvents();
        const QImage current = golden.grab().toImage().convertToFormat(QImage::Format_RGB32);
        const QString goldenPath = QFileInfo(QString::fromUtf8(__FILE__))
                                       .dir()
                                       .filePath(QStringLiteral("golden/default_shell_1100x760.png"));
        // Non-empty check, and bless NEVER passes green (review find): a
        // leftover exported EDI_BLESS_GOLDEN would otherwise silently turn
        // every run into a re-bless — the lock self-disabling invisibly
        // while the golden drifts. A red bless run is a visible bless run.
        if (!qEnvironmentVariable("EDI_BLESS_GOLDEN").isEmpty()) {
            QDir().mkpath(QFileInfo(goldenPath).absolutePath());
            const bool saved = current.save(goldenPath); // no side effects inside assert
            fprintf(stderr, saved ? "golden blessed: %s — bless runs exit red by design\n"
                                  : "golden bless FAILED to write: %s\n",
                    qPrintable(goldenPath));
            EDI_CHECK(saved);
            return 1;
        } else {
            QImage reference(goldenPath);
            if (reference.isNull()) {
                fprintf(stderr, "golden missing: %s — bless once with EDI_BLESS_GOLDEN=1\n",
                        qPrintable(goldenPath));
            }
            EDI_CHECK(!reference.isNull());
            reference = reference.convertToFormat(QImage::Format_RGB32);
            if (reference.size() != current.size()) {
                fprintf(stderr, "golden size mismatch: reference %dx%d vs render %dx%d\n",
                        reference.width(), reference.height(), current.width(), current.height());
            }
            EDI_CHECK(reference.size() == current.size());
            // Channel tolerance 8 + a 0.5% pixel budget: tight enough that a
            // 1px chrome shift fails, loose enough that subpixel AA jitter
            // from a Qt patch release does not cry wolf.
            int differing = 0;
            for (int y = 0; y < current.height(); ++y) {
                for (int x = 0; x < current.width(); ++x) {
                    const QRgb ours = current.pixel(x, y);
                    const QRgb theirs = reference.pixel(x, y);
                    if (qAbs(qRed(ours) - qRed(theirs)) > 8
                        || qAbs(qGreen(ours) - qGreen(theirs)) > 8
                        || qAbs(qBlue(ours) - qBlue(theirs)) > 8) {
                        ++differing;
                    }
                }
            }
            const int budget = current.width() * current.height() / 200;
            if (differing > budget) {
                fprintf(stderr, "golden mismatch: %d differing pixels (budget %d) vs %s\n",
                        differing, budget, qPrintable(goldenPath));
            }
            EDI_CHECK(differing <= budget);
        }
    }

    // The OP pipeline's verbs (R1-B05): Open/Save Ops Recipe (strict TOML), and
    // Export Resolved bakes against the LIVE drawing — every stale binding
    // listed at once on refusal, nothing written. Flush DeferredDelete before
    // widget lookups (house gotcha).
    {
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        EDI_CHECK(menuActionWithText(window, QStringLiteral("fileMenu"), QStringLiteral("Open Ops Recipe…")) != nullptr);
        EDI_CHECK(menuActionWithText(window, QStringLiteral("fileMenu"), QStringLiteral("Save Ops Recipe…")) != nullptr);
        EDI_CHECK(menuActionWithText(window, QStringLiteral("fileMenu"), QStringLiteral("Export Resolved…")) != nullptr);
        EDI_CHECK(menuActionWithText(window, QStringLiteral("fileMenu"), QStringLiteral("Export Ops Previews…")) != nullptr);

        QTemporaryDir opsDir;
        EDI_CHECK(opsDir.isValid());

        // Open Ops surfaces the strict reader's offender verbatim.
        const QString badOpsPath = opsDir.filePath(QStringLiteral("bad_ops.toml"));
        {
            QFile bad(badOpsPath);
            EDI_CHECK(bad.open(QIODevice::WriteOnly));
            bad.write("op.0.type = \"AddDodecahedron\"\n");
        }
        EDI_CHECK(!window.openOpsRecipeFromPath(badOpsPath));
        EDI_CHECK(window.lastRecipeError().contains(QStringLiteral("AddDodecahedron")));

        // A fresh drafted line is the measurement source; its physical length
        // feeds a cylinder's bound height.
        controller->setSelectedToolId(QStringLiteral("line_tool"));
        controller->clickCanvasNormalized(0.2, 0.2);
        controller->clickCanvasNormalized(0.5, 0.6);
        const QString lineId = controller->selectedObjectId();
        EDI_CHECK(!lineId.isEmpty());

        const QString opsPath = opsDir.filePath(QStringLiteral("ops.toml"));
        {
            QFile ops(opsPath);
            EDI_CHECK(ops.open(QIODevice::WriteOnly));
            const QString text = QStringLiteral(
                "op.0.type = \"AddCylinder\"\n"
                "op.0.name = \"shaft\"\n"
                "op.0.radius = \"1\"\n"
                "op.0.height.object = \"%1\"\n"
                "op.0.height.field = \"length\"\n"
                "op.0.z = \"0\"\n").arg(lineId);
            ops.write(text.toUtf8());
        }
        EDI_CHECK(window.openOpsRecipeFromPath(opsPath));
        EDI_CHECK(window.lastRecipeError().isEmpty());

        // Export Resolved SUCCEEDS while the line exists: the written file is
        // literals-only — the binding keys are gone.
        const QString resolvedPath = opsDir.filePath(QStringLiteral("resolved.toml"));
        EDI_CHECK(window.exportResolvedOpsToPath(resolvedPath));
        EDI_CHECK(window.lastRecipeError().isEmpty());
        EDI_CHECK(QFile::exists(resolvedPath));
        {
            QFile resolved(resolvedPath);
            EDI_CHECK(resolved.open(QIODevice::ReadOnly));
            const QString text = QString::fromUtf8(resolved.readAll());
            EDI_CHECK(!text.contains(QStringLiteral("height.object"))); // resolved to a literal
        }

        // Delete the line; Export Resolved now REFUSES, naming the binding AND
        // the missing object, and writes NOTHING (the resolve gate, in the UI).
        EDI_CHECK(controller->deleteSelectedObject());
        const QString stalePath = opsDir.filePath(QStringLiteral("stale.toml"));
        EDI_CHECK(!window.exportResolvedOpsToPath(stalePath));
        EDI_CHECK(!QFile::exists(stalePath));
        EDI_CHECK(window.lastRecipeError().contains(QStringLiteral("op.0.height")));
        EDI_CHECK(window.lastRecipeError().contains(QStringLiteral("object not found: ") + lineId));
    }

    // #18 close-without-saving guard: the dialog is an injectable callable,
    // so the offscreen suite drives every choice without a modal. Runs last
    // because the Save case genuinely closes (hides) the window.
    {
        QTemporaryDir guardDir;
        EDI_CHECK(guardDir.isValid());
        const QString guardPath = guardDir.filePath(QStringLiteral("guard.edidraw"));

        // Make the document clean at a known path first.
        EDI_CHECK(window.saveDrawingToPath(guardPath));
        EDI_CHECK(!window.isDocumentDirty());

        int promptCalls = 0;
        window.setDirtyGuardPrompt([&promptCalls]() {
            ++promptCalls;
            return EdiShellWindow::DirtyGuardChoice::Cancel;
        });

        // Clean document: no question asked, the close proceeds.
        EDI_CHECK(window.close());
        EDI_CHECK(promptCalls == 0);
        window.show(); // close() only hides; revive for the rest of the block

        // Dirty + Cancel: the close is refused and the window survives.
        controller->setSelectedToolId(QStringLiteral("point_tool"));
        controller->clickCanvasNormalized(0.41, 0.41);
        controller->clickCanvasNormalized(0.42, 0.42);
        EDI_CHECK(window.isDocumentDirty());
        EDI_CHECK(!window.close());
        EDI_CHECK(promptCalls == 1);

        // Cancel blocks a guarded open the same way — document untouched.
        const int beforeOpen = controller->modelDocument().value(QStringLiteral("drawing_objects")).toList().size();
        EDI_CHECK(!window.openDrawingFromPathGuarded(guardPath));
        EDI_CHECK(promptCalls == 2);
        EDI_CHECK(controller->modelDocument().value(QStringLiteral("drawing_objects")).toList().size() == beforeOpen);

        // Save: the close saves to the current path on its way out.
        window.setDirtyGuardPrompt([&promptCalls]() {
            ++promptCalls;
            return EdiShellWindow::DirtyGuardChoice::Save;
        });
        EDI_CHECK(window.close());
        EDI_CHECK(promptCalls == 3);
        EDI_CHECK(!window.isDocumentDirty()); // the save landed before closing
        window.show();

        // Discard: a guarded open over a dirty document proceeds and loses
        // the changes, exactly as the user chose.
        controller->clickCanvasNormalized(0.43, 0.43);
        controller->clickCanvasNormalized(0.44, 0.44);
        EDI_CHECK(window.isDocumentDirty());
        window.setDirtyGuardPrompt([&promptCalls]() {
            ++promptCalls;
            return EdiShellWindow::DirtyGuardChoice::Discard;
        });
        EDI_CHECK(window.openDrawingFromPathGuarded(guardPath));
        EDI_CHECK(promptCalls == 4);
        EDI_CHECK(!window.isDocumentDirty()); // back to the saved state

        // The recent-files menu routes through the guard too — this is the
        // WIRING test (reverting the menu lambda to the unguarded open
        // passes the direct-method checks above but fails here).
        controller->setSelectedToolId(QStringLiteral("point_tool"));
        controller->clickCanvasNormalized(0.45, 0.45);
        EDI_CHECK(window.isDocumentDirty());
        window.setDirtyGuardPrompt([&promptCalls]() {
            ++promptCalls;
            return EdiShellWindow::DirtyGuardChoice::Cancel;
        });
        auto *recentMenu = window.findChild<QMenu *>(QStringLiteral("recentFilesMenu"));
        EDI_CHECK(recentMenu != nullptr);
        QAction *recentGuardAction = nullptr;
        for (QAction *action : recentMenu->actions()) {
            if (action->data().toString() == guardPath) {
                recentGuardAction = action;
            }
        }
        EDI_CHECK(recentGuardAction != nullptr);
        const int beforeRecent = controller->modelDocument().value(QStringLiteral("drawing_objects")).toList().size();
        recentGuardAction->trigger();
        EDI_CHECK(promptCalls == 5);          // the guard asked...
        EDI_CHECK(window.isDocumentDirty());  // ...and Cancel kept the document
        EDI_CHECK(controller->modelDocument().value(QStringLiteral("drawing_objects")).toList().size() == beforeRecent);

        // The Save choice REFUSES the action when the save cannot land.
        // Removing the directory is NOT enough — the store mkpath()es
        // parents back into existence — so a directory squats on the file
        // path itself, which QSaveFile cannot open. The failure notice is
        // injected (the real one is a modal).
        int saveFailures = 0;
        window.setSaveFailedNotice([&saveFailures](const QString &) { ++saveFailures; });
        window.setDirtyGuardPrompt([&promptCalls]() {
            ++promptCalls;
            return EdiShellWindow::DirtyGuardChoice::Save;
        });
        EDI_CHECK(QFile::remove(guardPath));
        EDI_CHECK(QDir(guardDir.path()).mkdir(QStringLiteral("guard.edidraw")));
        EDI_CHECK(!window.close());
        EDI_CHECK(promptCalls == 6);
        EDI_CHECK(saveFailures == 1);
        EDI_CHECK(window.isDocumentDirty()); // nothing was lost, nothing closed

        // The block-local captures die with this scope: leave capture-free
        // seams installed so the shared window never holds dangling
        // references. Don't CLEAR them instead — an unset prompt falls back
        // to the real modal QMessageBox and would hang the offscreen run.
        window.setDirtyGuardPrompt([]() { return EdiShellWindow::DirtyGuardChoice::Cancel; });
        window.setSaveFailedNotice([](const QString &) {});
    }

    // The text editor host (E1): the bottom terminal hosts the user's text
    // core. Every keystroke routes through TextEditorCommands into the
    // window-owned store — the widget is a PROJECTION; the document is the
    // truth — and a refused command surfaces in the status line.
    {
        // A FRESH window pins the SHIPPED default: the constructor mounts the
        // factory layout, whose Bottom binding is the editor (earlier blocks
        // switched `window` to hand-built jobs, which is exactly the point —
        // bindings are data; this asserts what edi ships, not what tests left).
        EdiShellWindow editorShell;
        QWidget *panel = editorShell.findChild<QWidget *>(QStringLiteral("textEditorPanel"));
        EDI_CHECK(panel != nullptr); // mounted in the bottom terminal slot
        auto *view = editorShell.findChild<TextEditorView *>(QStringLiteral("textEditorView"));
        EDI_CHECK(view != nullptr);
        EDI_CHECK(view->isReadOnly()); // Qt's own mutation paths are dead
        auto *list = editorShell.findChild<QListWidget *>(QStringLiteral("textEditorDocumentList"));
        EDI_CHECK(list != nullptr);
        EDI_CHECK(list->count() == 1); // the seeded scratch document
        auto *editorStatus = editorShell.findChild<QLabel *>(QStringLiteral("textEditorStatus"));
        EDI_CHECK(editorStatus != nullptr);

        edi::text::TextDocumentStore &textStore = editorShell.textDocumentStore();
        EDI_CHECK(textStore.activeDocumentId.has_value());
        EDI_CHECK(*textStore.activeDocumentId == "scratch");
        const std::uint64_t revisionBefore =
            edi::text::findDocument(textStore, "scratch")->revision;

        const auto type = [view](Qt::Key key, const QString &text) {
            QKeyEvent press(QEvent::KeyPress, key, Qt::NoModifier, text);
            QCoreApplication::sendEvent(view, &press);
        };
        // Three keystrokes land as "abc" ONLY if the caret is restored after
        // every re-projection — a caret reset to 0 types "cba". The order IS
        // the caret-restore pin.
        type(Qt::Key_A, QStringLiteral("a"));
        type(Qt::Key_B, QStringLiteral("b"));
        type(Qt::Key_C, QStringLiteral("c"));
        const edi::text::TextDocument *scratch =
            edi::text::findDocument(textStore, "scratch");
        EDI_CHECK(scratch != nullptr);
        EDI_CHECK(scratch->text == "abc");      // the STORE changed, not just pixels
        EDI_CHECK(scratch->dirty);              // the core's own dirty discipline
        EDI_CHECK(scratch->revision > revisionBefore);
        EDI_CHECK(view->toPlainText() == QStringLiteral("abc")); // projection agrees
        EDI_CHECK(editorStatus->text() == QStringLiteral("ok"));
        EDI_CHECK(list->item(0)->text().contains(QStringLiteral("•"))); // dirty marker

        type(Qt::Key_Backspace, QString());
        EDI_CHECK(edi::text::findDocument(textStore, "scratch")->text == "ab");

        // E3: Delete at the end is a quiet no-op — symmetric with Backspace
        // at the start, like every editor. (E1 let it reach the core for an
        // invalid_range refusal; boundary math made the honest translation
        // "nothing to delete".)
        type(Qt::Key_Delete, QString());
        EDI_CHECK(edi::text::findDocument(textStore, "scratch")->text == "ab");
        EDI_CHECK(editorStatus->text() == QStringLiteral("ok"));

        // E3 retired E1's ASCII gate: é now LANDS, as two UTF-8 bytes the
        // core stores and one UTF-16 unit the view counts. The caret math
        // is the proof — typing 'x' immediately after must append, not
        // split the é (which is exactly what E1's review showed happening
        // without the mapping).
        type(Qt::Key_E, QString::fromUtf8("\xc3\xa9")); // é
        EDI_CHECK(edi::text::findDocument(textStore, "scratch")->text == "ab\xc3\xa9");
        type(Qt::Key_X, QStringLiteral("x"));
        EDI_CHECK(edi::text::findDocument(textStore, "scratch")->text == "ab\xc3\xa9x");
        EDI_CHECK(view->toPlainText() == QString::fromUtf8("ab\xc3\xa9x"));
        // Backspace removes the WHOLE character, however many bytes.
        type(Qt::Key_Backspace, QString());
        type(Qt::Key_Backspace, QString());
        EDI_CHECK(edi::text::findDocument(textStore, "scratch")->text == "ab");

        // The core-refusal surface stays pinned DIRECTLY: the host now only
        // translates legal edits, so the defensive branch is exercised by
        // invoking the choke point with a hand-built out-of-range command.
        view->applyCommand(edi::text::DeleteTextRangeCommand{{}, {999, 1000}});
        EDI_CHECK(edi::text::findDocument(textStore, "scratch")->text == "ab");
        EDI_CHECK(editorStatus->text().contains(QStringLiteral("invalid_range")));

        // Selection editing (E3): select all of "ab" and type over it —
        // the host translates the gesture into the user's OWN
        // ReplaceTextRangeCommand, which existed unused since the core was
        // built. One keystroke, one command, selection replaced.
        QTextCursor selectAll = view->textCursor();
        selectAll.setPosition(0);
        selectAll.setPosition(2, QTextCursor::KeepAnchor);
        view->setTextCursor(selectAll);
        type(Qt::Key_Z, QStringLiteral("z"));
        EDI_CHECK(edi::text::findDocument(textStore, "scratch")->text == "z");
        EDI_CHECK(editorStatus->text() == QStringLiteral("ok"));

        // Find (E3): the needle is searched in the CORE's bytes, the match
        // selected in the view; a miss names the needle — never silent.
        type(Qt::Key_E, QStringLiteral("e"));
        type(Qt::Key_D, QStringLiteral("d"));
        type(Qt::Key_I, QStringLiteral("i")); // document: "zedi"
        auto *findInput = editorShell.findChild<QLineEdit *>(QStringLiteral("textEditorFindInput"));
        auto *findButton = editorShell.findChild<QPushButton *>(QStringLiteral("textEditorFindNext"));
        EDI_CHECK(findInput != nullptr && findButton != nullptr);
        findInput->setText(QStringLiteral("ed"));
        findButton->click();
        EDI_CHECK(view->textCursor().hasSelection());
        EDI_CHECK(view->textCursor().selectedText() == QStringLiteral("ed"));
        EDI_CHECK(view->textCursor().selectionStart() == 1);
        findInput->setText(QStringLiteral("zebra"));
        findButton->click();
        EDI_CHECK(editorStatus->text().contains(QStringLiteral("zebra"))); // miss, by name
    }

    // The text editor SESSION (E2): the open documents and the active one
    // survive a restart through a TOML manifest; New/Save move text; the list-
    // selection nit routes through itemClicked. Temp paths only — never the
    // user's real config dir.
    {
        EdiShellWindow editorShell;
        auto *list = editorShell.findChild<QListWidget *>(QStringLiteral("textEditorDocumentList"));
        EDI_CHECK(list != nullptr && list->count() == 1); // conditional seed intact: one scratch
        auto *view = editorShell.findChild<TextEditorView *>(QStringLiteral("textEditorView"));
        EDI_CHECK(view != nullptr);

        QTemporaryDir sessionDir;
        EDI_CHECK(sessionDir.isValid());
        const QString sessionPath = sessionDir.filePath(QStringLiteral("session.toml"));

        // Type a scratch note WITH A NEWLINE (Return -> "\n"), then save the session.
        const auto type = [view](Qt::Key key, const QString &text) {
            QKeyEvent press(QEvent::KeyPress, key, Qt::NoModifier, text);
            QCoreApplication::sendEvent(view, &press);
        };
        type(Qt::Key_H, QStringLiteral("h"));
        type(Qt::Key_I, QStringLiteral("i"));
        type(Qt::Key_Return, QString());
        type(Qt::Key_Y, QStringLiteral("y"));
        edi::text::TextDocumentStore &store = editorShell.textDocumentStore();
        EDI_CHECK(edi::text::findDocument(store, "scratch")->text == "hi\ny");
        EDI_CHECK(editorShell.saveTextSession(sessionPath));
        EDI_CHECK(QFile::exists(sessionPath));

        // A FRESH window restores the open set + active from the manifest, not
        // the bare seed — and the panel re-projects the restored text.
        {
            EdiShellWindow restored;
            EDI_CHECK(restored.loadTextSession(sessionPath));
            const edi::text::TextDocumentStore &rStore = restored.textDocumentStore();
            EDI_CHECK(rStore.activeDocumentId.has_value() && *rStore.activeDocumentId == "scratch");
            const edi::text::TextDocument *scratch = edi::text::findDocument(rStore, "scratch");
            EDI_CHECK(scratch != nullptr && scratch->text == "hi\ny"); // newline survived restart
            auto *rView = restored.findChild<QPlainTextEdit *>(QStringLiteral("textEditorView"));
            EDI_CHECK(rView != nullptr && rView->toPlainText() == QStringLiteral("hi\ny"));
        }

        // New creates + ACTIVATES a second document (makes the switch testable).
        auto *newButton = editorShell.findChild<QPushButton *>(QStringLiteral("textEditorNew"));
        EDI_CHECK(newButton != nullptr);
        newButton->click();
        EDI_CHECK(store.documents.size() == 2);
        EDI_CHECK(store.activeDocumentId.has_value() && *store.activeDocumentId != "scratch");
        const std::string created = *store.activeDocumentId;
        EDI_CHECK(list->count() == 2);

        // itemClicked on the FIRST item (scratch) switches the STORE's active
        // document — the E1 nit fixed (itemActivated was double-click only).
        QListWidgetItem *firstItem = list->item(0);
        EDI_CHECK(firstItem != nullptr);
        EDI_CHECK(firstItem->data(Qt::UserRole).toString() == QStringLiteral("scratch"));
        emit list->itemClicked(firstItem);
        EDI_CHECK(store.activeDocumentId.has_value() && *store.activeDocumentId == "scratch");
        EDI_CHECK(*store.activeDocumentId != created); // switched away from the new doc

        // Save writes the active document's text to a FILE (injected path) and
        // clears dirty — the list's dirty dot disappears.
        edi::text::TextDocument *activeDoc = edi::text::findDocument(store, "scratch");
        EDI_CHECK(activeDoc != nullptr && activeDoc->dirty); // typing left it dirty
        QTemporaryDir saveDir;
        EDI_CHECK(saveDir.isValid());
        const QString savePath = saveDir.filePath(QStringLiteral("note.txt"));
        editorShell.setTextEditorPathProvider(
            [savePath](bool forSave) { return forSave ? savePath : QString(); });
        auto *saveButton = editorShell.findChild<QPushButton *>(QStringLiteral("textEditorSave"));
        EDI_CHECK(saveButton != nullptr);
        saveButton->click();
        EDI_CHECK(QFile::exists(savePath));
        EDI_CHECK(!activeDoc->dirty); // markClean cleared it
        EDI_CHECK(!list->item(0)->text().contains(QStringLiteral("•"))); // dirty dot gone
    }

    // The script view (E4, the R7 loop early): opening an ops recipe puts
    // its CANONICAL TOML in the editor; Apply runs the text back through
    // the strict reader — refusals name the key, success replaces the
    // stream and echoes the canonical form. Text is the cheap preview;
    // the pipeline never re-parses mid-edit.
    {
        EdiShellWindow scriptShell;
        EDI_CHECK(scriptShell.openOpsRecipeFromPath(QStringLiteral(
            EDI_SAMPLES_DIR "/doric_column/doric_column_drafted_ops.toml")));
        auto *view = scriptShell.findChild<TextEditorView *>(QStringLiteral("textEditorView"));
        auto *editorStatus = scriptShell.findChild<QLabel *>(QStringLiteral("textEditorStatus"));
        auto *applyButton = scriptShell.findChild<QPushButton *>(QStringLiteral("textEditorApply"));
        EDI_CHECK(view != nullptr && editorStatus != nullptr && applyButton != nullptr);

        // The script document holds the stream's canonical serialization
        // and is active; Apply is therefore enabled.
        edi::text::TextDocumentStore &textStore = scriptShell.textDocumentStore();
        EDI_CHECK(textStore.activeDocumentId.has_value()
               && *textStore.activeDocumentId == "ops_recipe");
        const edi::text::TextDocument *script =
            edi::text::findDocument(textStore, "ops_recipe");
        EDI_CHECK(script != nullptr);
        EDI_CHECK(script->text.find("op.0.type = \"AddBox\"") != std::string::npos);
        EDI_CHECK(applyButton->isEnabled());

        // Corrupt one key by typing where the caret lands (position 0):
        // "x" prepended makes the first key unknown — the STRICT reader
        // refuses BY NAME and the stream is untouched.
        const std::size_t opsBefore = scriptShell.opsStream().ops.size();
        {
            QTextCursor cursor = view->textCursor();
            cursor.setPosition(0);
            view->setTextCursor(cursor);
        }
        QKeyEvent press(QEvent::KeyPress, Qt::Key_X, Qt::NoModifier, QStringLiteral("x"));
        QCoreApplication::sendEvent(view, &press);
        applyButton->click();
        // (The reader refuses at the first MISSING required key — mangling
        // op.0.depth into xop.0.depth makes depth absent — which is even
        // more pointable than the audit catching the stray key later.)
        EDI_CHECK(editorStatus->text().contains(QStringLiteral("op.0.depth")));
        EDI_CHECK(scriptShell.opsStream().ops.size() == opsBefore); // unchanged

        // Repair: delete the stray byte, Apply again — ok, canonical echo.
        QKeyEvent del(QEvent::KeyPress, Qt::Key_Delete, Qt::NoModifier, QString());
        {
            QTextCursor cursor = view->textCursor();
            cursor.setPosition(0);
            view->setTextCursor(cursor);
        }
        QCoreApplication::sendEvent(view, &del);
        applyButton->click();
        EDI_CHECK(editorStatus->text() == QStringLiteral("ok"));
        EDI_CHECK(scriptShell.opsStream().ops.size() == opsBefore);
        const edi::text::TextDocument *after =
            edi::text::findDocument(textStore, "ops_recipe");
        EDI_CHECK(after->text.find("xop.0") == std::string::npos); // canonical again

        // The canonical ECHO is observable, not assumed: add a trailing
        // blank line (legal TOML, parses identically), Apply, and the echo
        // must NORMALIZE it away — an Apply that "succeeds" without
        // re-projecting the canonical form leaves the extra newline behind.
        {
            QTextCursor cursor = view->textCursor();
            cursor.setPosition(static_cast<int>(view->toPlainText().size()));
            view->setTextCursor(cursor);
        }
        QKeyEvent ret(QEvent::KeyPress, Qt::Key_Return, Qt::NoModifier, QString());
        QCoreApplication::sendEvent(view, &ret);
        EDI_CHECK(edi::text::findDocument(textStore, "ops_recipe")->text.ends_with("\n\n"));
        applyButton->click();
        EDI_CHECK(editorStatus->text() == QStringLiteral("ok"));
        EDI_CHECK(!edi::text::findDocument(textStore, "ops_recipe")->text.ends_with("\n\n"));

        // Apply is a SCRIPT-document gesture: on a scratch note it is
        // disabled, not silently wrong.
        auto *list = scriptShell.findChild<QListWidget *>(QStringLiteral("textEditorDocumentList"));
        EDI_CHECK(list != nullptr);
        for (int i = 0; i < list->count(); ++i) {
            if (list->item(i)->data(Qt::UserRole).toString() == QStringLiteral("scratch")) {
                list->setCurrentItem(list->item(i));
                emit list->itemClicked(list->item(i));
            }
        }
        EDI_CHECK(!applyButton->isEnabled());
    }

    // C3 block palette: save a selection as a named block, see it listed, click
    // the row to arm a placement pick, and stamp it with a canvas click. Fresh
    // window keeps this isolated from the accumulated document above.
    {
        EdiShellWindow blockWindow;
        auto *blockController = blockWindow.findChild<DrawingDocumentController *>();
        EDI_CHECK(blockController != nullptr);

        // Draw two points and marquee-select both.
        blockController->setSelectedToolId(QStringLiteral("point_tool"));
        blockController->clickCanvasNormalized(0.3, 0.3);
        blockController->clickCanvasNormalized(0.6, 0.6);
        EDI_CHECK(objectCount(*blockController) == 2);
        blockController->selectObjectsInBoundsNormalized(0.0, 0.0, 1.0, 1.0);

        auto *nameField = blockWindow.findChild<QLineEdit *>(QStringLiteral("blockNameField"));
        auto *saveButton = buttonNamed(blockWindow, QStringLiteral("saveBlockButton"));
        auto *blockList = blockWindow.findChild<QListWidget *>(QStringLiteral("blockList"));
        EDI_CHECK(nameField != nullptr && saveButton != nullptr && blockList != nullptr);
        EDI_CHECK(blockList->count() == 0); // empty library at first

        // Save the selection as a named block via the palette button.
        nameField->setText(QStringLiteral("table"));
        saveButton->click();

        // The palette now lists the block by name, the row carrying its id.
        EDI_CHECK(blockList->count() == 1);
        EDI_CHECK(blockList->item(0)->text() == QStringLiteral("table"));
        const QString blockId = blockList->item(0)->data(Qt::UserRole).toString();
        EDI_CHECK(blockId.startsWith(QStringLiteral("block_")));

        // Clicking the row (through the real signal wiring) arms a placement pick.
        emit blockList->itemClicked(blockList->item(0));
        EDI_CHECK(blockController->isAwaitingPointCapture());
        EDI_CHECK(blockController->pointCapturePrompt() == QStringLiteral("Click to place the block"));

        // The next canvas click stamps the instance (the definition's two
        // objects) and clears the pick — no point object is created by the tool.
        const int beforeStamp = objectCount(*blockController);
        blockController->clickCanvasNormalized(0.5, 0.5);
        EDI_CHECK(objectCount(*blockController) == beforeStamp + 2);
        EDI_CHECK(!blockController->isAwaitingPointCapture());
    }

    // DM-15 "Block instance" inspector group: the group + transformInstanceButton
    // are visible/enabled when a placed block instance is the active object;
    // hidden for non-instance objects (or when nothing is selected).
    //
    // Design note: the group is gated at two layers —
    //   (1) the plan puts "block_instance" in the object_shape context, so the
    //       group's container widget exists whenever any object is selected;
    //   (2) refreshInspector then reads has_block_instance_selection from the
    //       document-root projection and hides the group for non-instance objects.
    // This test drives both gates through the real controller + projection path.
    {
        EdiShellWindow dm15Window;
        auto *dm15Controller = dm15Window.findChild<DrawingDocumentController *>();
        EDI_CHECK(dm15Controller != nullptr);

        // Build a block: draw a point, select it, save it.
        dm15Controller->setSelectedToolId(QStringLiteral("point_tool"));
        dm15Controller->clickCanvasNormalized(0.4, 0.4);
        EDI_CHECK(objectCount(*dm15Controller) == 1);
        dm15Controller->selectObjectsInBoundsNormalized(0.0, 0.0, 1.0, 1.0);

        auto *blockNameField = dm15Window.findChild<QLineEdit *>(QStringLiteral("blockNameField"));
        auto *saveBlockButton = buttonNamed(dm15Window, QStringLiteral("saveBlockButton"));
        auto *dm15BlockList = dm15Window.findChild<QListWidget *>(QStringLiteral("blockList"));
        EDI_CHECK(blockNameField != nullptr && saveBlockButton != nullptr && dm15BlockList != nullptr);
        blockNameField->setText(QStringLiteral("chair"));
        saveBlockButton->click();
        EDI_CHECK(dm15BlockList->count() == 1);

        // Stamp an instance by picking the block row + clicking the canvas.
        emit dm15BlockList->itemClicked(dm15BlockList->item(0));
        EDI_CHECK(dm15Controller->isAwaitingPointCapture());
        const int beforePlace = objectCount(*dm15Controller);
        dm15Controller->clickCanvasNormalized(0.5, 0.5);
        EDI_CHECK(objectCount(*dm15Controller) == beforePlace + 1); // one point in the definition
        EDI_CHECK(!dm15Controller->isAwaitingPointCapture());

        // The controller auto-selects the stamped objects. Flush DeferredDelete
        // before widget lookups (geometry editor retires spins with deleteLater).
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);

        // --- Gate 1: instance is selected → group + button visible and enabled ---
        auto *transformBtn = buttonNamed(dm15Window, QStringLiteral("transformInstanceButton"));
        EDI_CHECK(transformBtn != nullptr); // widget was built

        // The group's OWN hidden flag is the real gate — isHidden() is
        // ancestor-independent, so it reads correctly even though this offscreen
        // window is never shown (a child's isVisible() is unconditionally false on
        // an unshown top-level and would prove nothing — a tautology). Group
        // objectName follows the inspectorGroup_<id> convention (see F2 above).
        auto blockInstanceVisible = [&dm15Window]() {
            QWidget *group =
                dm15Window.findChild<QWidget *>(QStringLiteral("inspectorGroup_block_instance"));
            EDI_CHECK(group != nullptr); // built once in ensureInspectorGroupsBuilt
            return !group->isHidden();
        };

        // The projection bool: read it directly to confirm the wiring.
        const QVariantMap dm15Doc = dm15Controller->modelDocument();
        EDI_CHECK(dm15Doc.value(QStringLiteral("has_block_instance_selection")).toBool());
        EDI_CHECK(!dm15Doc.value(QStringLiteral("instance_id")).toString().isEmpty());

        // The group is SHOWN for the placed instance, and the action is enabled.
        EDI_CHECK(blockInstanceVisible());
        EDI_CHECK(transformBtn->isEnabled());

        // The two delta spins exist and have the correct identity defaults.
        auto *rotSpin = dm15Window.findChild<QDoubleSpinBox *>(QStringLiteral("instanceRotationSpin"));
        auto *scaleSpin = dm15Window.findChild<QDoubleSpinBox *>(QStringLiteral("instanceScaleSpin"));
        EDI_CHECK(rotSpin != nullptr && scaleSpin != nullptr);
        EDI_CHECK(rotSpin->value() == 0.0);
        EDI_CHECK(scaleSpin->value() == 1.0);

        // --- Gate 2: a non-instance object selected → group hidden ---
        // Draw an ordinary point (not a block definition, not a placed instance).
        dm15Controller->setSelectedToolId(QStringLiteral("point_tool"));
        dm15Controller->clickCanvasNormalized(0.2, 0.2);
        // The new point is auto-selected (it is the active object after creation).
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);

        const QVariantMap dm15DocAfter = dm15Controller->modelDocument();
        // An ordinary point carries no blockPlacement.instanceId.
        EDI_CHECK(!dm15DocAfter.value(QStringLiteral("has_block_instance_selection")).toBool());
        EDI_CHECK(dm15DocAfter.value(QStringLiteral("instance_id")).toString().isEmpty());

        // The group's own hidden flag must now be set (the sub-gate fired).
        EDI_CHECK(!blockInstanceVisible());

        // --- Gate 3: nothing selected → group hidden (plan-level context gate) ---
        dm15Controller->setSelectedToolId(QStringLiteral("select_move"));
        // With the select tool and nothing marquee'd, the context is "document",
        // not "object_shape", so block_instance never enters the plan.
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);

        const QVariantMap dm15DocNoSel = dm15Controller->modelDocument();
        EDI_CHECK(!dm15DocNoSel.value(QStringLiteral("has_block_instance_selection")).toBool());
        EDI_CHECK(!blockInstanceVisible());
    }

    // DR-11 Kaleidoscope chrome: the Duplicate fold's mirror area gains an
    // axisCountSpin (QSpinBox) and a kaleidoscopeButton (QPushButton).
    // The axis-count spin shares m_arrayCount with the Repeat fold (the
    // controller's shared count concept: N axes = N copies, same field as
    // N radial copies). The kaleidoscope button arms a centre pick.
    {
        EdiShellWindow dr11Window;
        auto *dr11Controller = dr11Window.findChild<DrawingDocumentController *>();
        EDI_CHECK(dr11Controller != nullptr);

        // axisCountSpin must be present and seeded from the controller.
        auto *axisCountSpin = dr11Window.findChild<QSpinBox *>(QStringLiteral("axisCountSpin"));
        EDI_CHECK(axisCountSpin != nullptr);
        EDI_CHECK(axisCountSpin->value() == dr11Controller->arrayCount());

        // Changing the spin propagates to the controller.
        axisCountSpin->setValue(4);
        EDI_CHECK(dr11Controller->arrayCount() == 4);

        // The kaleidoscope button must be present.
        auto *kaleidoscopeButton = dr11Window.findChild<QPushButton *>(QStringLiteral("kaleidoscopeButton"));
        EDI_CHECK(kaleidoscopeButton != nullptr);

        // Clicking it with a mirror-supported object selected arms a pick.
        dr11Controller->setSelectedToolId(QStringLiteral("line_tool"));
        dr11Controller->clickCanvasNormalized(0.3, 0.3);
        dr11Controller->clickCanvasNormalized(0.6, 0.6);
        EDI_CHECK(!dr11Controller->selectedObjectId().isEmpty());
        kaleidoscopeButton->click();
        EDI_CHECK(dr11Controller->isAwaitingPointCapture());
        // Cancel so the pick does not bleed into subsequent operations.
        dr11Controller->cancelPendingCreation();
        EDI_CHECK(!dr11Controller->isAwaitingPointCapture());
    }

    // M8 Motif palette: the "Motifs" palette mounts alongside the Blocks
    // palette; defineMotifFromSelection adds a row keyed by name; activating
    // a row arms a MotifPlacement pick-a-point capture.
    {
        EdiShellWindow m8Window;
        m8Window.show();
        auto *m8Controller = m8Window.findChild<DrawingDocumentController *>();
        EDI_CHECK(m8Controller != nullptr);

        // defineMotifButton must exist (the QInputDialog trigger).
        // Flush DeferredDelete before widget lookups (charter rule).
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        auto *defineBtn = buttonNamed(m8Window, QStringLiteral("defineMotifButton"));
        EDI_CHECK(defineBtn != nullptr);

        // motifList must exist, initially empty.
        auto *motifList = m8Window.findChild<QListWidget *>(QStringLiteral("motifList"));
        EDI_CHECK(motifList != nullptr);
        EDI_CHECK(motifList->count() == 0);

        // Draw two objects and marquee-select them — defineMotifFromSelection
        // operates on the active selection.
        m8Controller->setSelectedToolId(QStringLiteral("point_tool"));
        m8Controller->clickCanvasNormalized(0.3, 0.3);
        m8Controller->clickCanvasNormalized(0.7, 0.7);
        EDI_CHECK(objectCount(*m8Controller) == 2);
        m8Controller->selectObjectsInBoundsNormalized(0.0, 0.0, 1.0, 1.0);

        // Define the motif directly through the controller (the button path
        // goes through QInputDialog which is interactive; the controller verb
        // is what the button calls after the prompt). A row for "star" appears.
        const bool defined = m8Controller->defineMotifFromSelection(QStringLiteral("star"));
        EDI_CHECK(defined);

        // refreshInspector runs via the modelChanged connection; flush events
        // so the list repopulation completes before we assert its count.
        QCoreApplication::processEvents();
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        EDI_CHECK(motifList->count() == 1);
        EDI_CHECK(motifList->item(0)->text() == QStringLiteral("star"));

        // Activating the row (USER signal — itemActivated) arms a MotifPlacement
        // pick-a-point capture via beginMotifPlacement. The signal wiring is
        // tested by emitting it directly (same as the block-palette test pattern).
        EDI_CHECK(!m8Controller->isAwaitingPointCapture());
        emit motifList->itemActivated(motifList->item(0));
        EDI_CHECK(m8Controller->isAwaitingPointCapture());

        // Cancel so the pick does not bleed into subsequent operations.
        m8Controller->cancelPendingCreation();
        EDI_CHECK(!m8Controller->isAwaitingPointCapture());
    }

    // DR-13 Angular dimension chrome: the dimension row gains a new belt cell
    // and the dimensionKindCombo gains an "Angular" entry.
    {
        EdiShellWindow dr13Window;
        auto *dr13Controller = dr13Window.findChild<DrawingDocumentController *>();
        EDI_CHECK(dr13Controller != nullptr);

        // The dimensionKindCombo must carry an "Angular" entry (data "angular").
        // Several combos share the objectName; comboWithFirstItemData finds the
        // dimension combo by its first entry's data ("distance").
        QComboBox *dimensionKindCombo = comboWithFirstItemData(dr13Window, QStringLiteral("distance"));
        EDI_CHECK(dimensionKindCombo != nullptr);

        // Check that "Angular" is present in the combo.
        bool foundAngular = false;
        for (int i = 0; i < dimensionKindCombo->count(); ++i) {
            if (dimensionKindCombo->itemData(i).toString() == QStringLiteral("angular")) {
                foundAngular = true;
                break;
            }
        }
        EDI_CHECK(foundAngular);

        // The angular_dimension_tool belt cell must be registered in the default
        // belt layout — it lives on beltRow 9 alongside the other dimension tools.
        const edi::shell::BeltLayout defaultBelt = DraftingFeature::defaultBeltLayout();
        bool foundAngularTool = false;
        for (const QString &id : defaultBelt.itemIds) {
            if (id == QStringLiteral("angular_dimension_tool")) {
                foundAngularTool = true;
                break;
            }
        }
        EDI_CHECK(foundAngularTool);
    }

    return 0;
}
