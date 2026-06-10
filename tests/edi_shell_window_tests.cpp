#include "widgets/EdiShellWindow.h"

#include "core/DrawingCore.h"

#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QPushButton>
#include <QString>
#include <QVariantList>
#include <QVariantMap>

#include <cassert>

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

    return 0;
}
