#include "widgets/DraftingFeature.h"

#include <QComboBox>
#include <QDoubleSpinBox>
#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QSignalBlocker>
#include <QStyle>
#include <QStringList>
#include <QVariantList>
#include <QVariantMap>

#include <utility>

#include "core/DrawingCore.h"
#include "widgets/DrawingCanvasWidget.h"
#include "widgets/ShellWidgetHelpers.h"

using namespace edi::shell;

void DraftingFeature::rebuildGeometryEditor(const QVariantMap &selectedObject)
{
    if (m_geometryEditor == nullptr) {
        return;
    }

    auto *layout = qobject_cast<QGridLayout *>(m_geometryEditor->layout());
    if (layout == nullptr) {
        return;
    }

    while (QLayoutItem *item = layout->takeAt(0)) {
        if (QWidget *widget = item->widget()) {
            widget->deleteLater();
        }
        delete item;
    }
    m_geometryFields.clear();
    m_physicalGeometryFields.clear();

    const QVariantList fields = selectedObject.value(QStringLiteral("numeric_fields")).toList();
    const QVariantMap physicalGeometry = selectedObject.value(QStringLiteral("physical_geometry")).toMap();
    const QString unitLabel = physicalGeometry.value(QStringLiteral("unit_label")).toString();

    int row = 0;
    for (const QVariant &fieldValue : fields) {
        const QVariantMap field = fieldValue.toMap();
        const QString fieldId = field.value(QStringLiteral("id")).toString();
        if (fieldId.isEmpty() || !selectedObject.contains(fieldId)) {
            continue;
        }

        auto *label = new QLabel(field.value(QStringLiteral("label")).toString());
        label->setObjectName(QStringLiteral("fieldLabel"));
        auto *spin = makeGeometryFieldSpin({
            .fieldId = fieldId,
            .fieldMode = QStringLiteral("normalized"),
            .decimals = field.value(QStringLiteral("decimals"), 4).toInt(),
            .step = field.value(QStringLiteral("step"), 0.01).toDouble(),
            .minimum = field.value(QStringLiteral("minimum"), -10.0).toDouble(),
            .maximum = field.value(QStringLiteral("maximum"), 10.0).toDouble(),
            .value = selectedObject.value(fieldId).toDouble(),
        });
        layout->addWidget(label, row, 0);
        layout->addWidget(spin, row, 1);
        if (field.value(QStringLiteral("physical_editable")).toBool() && physicalGeometry.contains(fieldId)) {
            auto *physicalSpin = makeGeometryFieldSpin({
                .fieldId = fieldId,
                .fieldMode = QStringLiteral("physical"),
                .decimals = field.value(QStringLiteral("physical_decimals"), field.value(QStringLiteral("decimals"), 4)).toInt(),
                .step = field.value(QStringLiteral("physical_step"), field.value(QStringLiteral("step"), 0.01)).toDouble(),
                .minimum = field.value(QStringLiteral("physical_minimum"), -100000.0).toDouble(),
                .maximum = field.value(QStringLiteral("physical_maximum"), 100000.0).toDouble(),
                .value = physicalGeometry.value(fieldId).toDouble(),
            });
            auto *physicalLabel = new QLabel(field.value(QStringLiteral("physical_unit_label"), unitLabel).toString());
            physicalLabel->setObjectName(QStringLiteral("valueLabel"));
            layout->addWidget(physicalSpin, row, 2);
            layout->addWidget(physicalLabel, row, 3);
            m_physicalGeometryFields.insert(fieldId, physicalSpin);
        }
        m_geometryFields.insert(fieldId, spin);
        ++row;
    }

    setGeometryEditorVisible(row > 0);
}

void DraftingFeature::applyGeometryEditStatus(const QVariantMap &editStatus)
{
    const bool showFailure = !editStatus.isEmpty() && !editStatus.value(QStringLiteral("ok")).toBool();
    const QString failedFieldId = showFailure ? editStatus.value(QStringLiteral("field_id")).toString() : QString();
    const QString failedMode = showFailure ? editStatus.value(QStringLiteral("mode")).toString() : QString();

    auto updateField = [](QDoubleSpinBox *field, bool invalid) {
        if (field == nullptr || field->property("editInvalid").toBool() == invalid) {
            return;
        }
        field->setProperty("editInvalid", invalid);
        field->style()->unpolish(field);
        field->style()->polish(field);
        field->update();
    };

    for (auto it = m_geometryFields.begin(); it != m_geometryFields.end(); ++it) {
        updateField(it.value(), failedMode == QStringLiteral("normalized") && it.key() == failedFieldId);
    }
    for (auto it = m_physicalGeometryFields.begin(); it != m_physicalGeometryFields.end(); ++it) {
        updateField(it.value(), failedMode == QStringLiteral("physical") && it.key() == failedFieldId);
    }
}

void DraftingFeature::setGeometryEditorVisible(bool visible)
{
    if (m_geometryEditor != nullptr) {
        m_geometryEditor->setVisible(visible);
    }
}

void DraftingFeature::refreshInspector()
{
    const QVariantMap document = m_controller->modelDocument();
    const QVariantList objects = document.value(QStringLiteral("drawing_objects")).toList();
    const QVariantList selected = document.value(QStringLiteral("selected_object_ids")).toList();
    const QVariantMap snap = document.value(QStringLiteral("snap")).toMap();
    const QVariantMap grid = document.value(QStringLiteral("grid")).toMap();
    const QVariantMap plot = document.value(QStringLiteral("plot_summary")).toMap();
    const QVariantMap pointer = document.value(QStringLiteral("pointer")).toMap();
    const QVariantMap quickMeasurement = document.value(QStringLiteral("quick_measurement")).toMap();
    const QVariantMap guideDragSnap = document.value(QStringLiteral("guide_drag_snap")).toMap();
    const QVariantMap calibrationMeasurement = document.value(QStringLiteral("calibration_measurement")).toMap();
    const QVariantMap calibrationCorrection = document.value(QStringLiteral("calibration_correction")).toMap();
    const QVariantMap editStatus = document.value(QStringLiteral("edit_status")).toMap();
    const QVariantMap selectedObject = activeObjectProjection(document);
    const QVariantList layers = document.value(QStringLiteral("layers")).toList();
    const QString activeLayerId = document.value(QStringLiteral("active_layer_id")).toString();
    const QVariantMap activeLayer = layerProjection(document, activeLayerId);
    const bool hasPreview = document.contains(QStringLiteral("preview_object"));

    if (m_objectList != nullptr) {
        // Rebuild the object list as a projection of the document — same
        // recompute-whole discipline as everything else here. The blocker
        // keeps programmatic rebuilds from re-entering selection.
        const QSignalBlocker blocker(*m_objectList);
        m_objectList->clear();
        const QString activeId = document.value(QStringLiteral("active_object_id")).toString();
        for (const QVariant &value : objects) {
            const QVariantMap object = value.toMap();
            const QString id = object.value(QStringLiteral("id")).toString();
            auto *item = new QListWidgetItem(
                QStringLiteral("%1 — %2").arg(object.value(QStringLiteral("kind")).toString(), id));
            item->setData(Qt::UserRole, id);
            m_objectList->addItem(item);
            if (id == activeId) {
                m_objectList->setCurrentItem(item);
            }
        }
    }

    setLabelText(m_toolValue, QStringLiteral("Tool: %1").arg(m_controller->selectedToolId()));
    if (m_selectedValue != nullptr) {
        const QString activeObject = document.value(QStringLiteral("active_object_id")).toString();
        m_selectedValue->setText(activeObject.isEmpty()
            ? QStringLiteral("Selected: none")
            : QStringLiteral("Selected: %1").arg(activeObject));
    }
    if (m_objectKindValue != nullptr) {
        m_objectKindValue->setText(selectedObject.isEmpty()
            ? QStringLiteral("Kind: none")
            : QStringLiteral("Kind: %1   Id: %2")
                .arg(selectedObject.value(QStringLiteral("kind")).toString())
                .arg(selectedObject.value(QStringLiteral("id")).toString()));
    }
    setLabelText(m_objectBoundsValue, selectedObject.isEmpty() ? QStringLiteral("Bounds: none") : boundsSummary(selectedObject));
    setLabelText(m_objectGeometryValue, selectedObject.isEmpty() ? QStringLiteral("Geometry: none") : geometrySummary(selectedObject));
    if (m_geometryEditStatus != nullptr) {
        const bool showFailure = !editStatus.isEmpty() && !editStatus.value(QStringLiteral("ok")).toBool();
        m_geometryEditStatus->setVisible(showFailure);
        m_geometryEditStatus->setText(showFailure
            ? QStringLiteral("Edit rejected: %1").arg(editStatus.value(QStringLiteral("message")).toString())
            : QString());
    }
    if (m_objectLayerValue != nullptr) {
        m_objectLayerValue->setText(selectedObject.isEmpty()
            ? QStringLiteral("Layer: none")
            : QStringLiteral("Layer: %1   Obj L/V: %2/%3   Effective L/V: %4/%5   Pen: %6")
                .arg(selectedObject.value(QStringLiteral("layer_id")).toString())
                .arg(yesNo(selectedObject.value(QStringLiteral("locked")).toBool()))
                .arg(yesNo(selectedObject.value(QStringLiteral("visible")).toBool()))
                .arg(yesNo(selectedObject.value(QStringLiteral("effective_locked")).toBool()))
                .arg(yesNo(selectedObject.value(QStringLiteral("effective_visible")).toBool()))
                .arg(selectedObject.value(QStringLiteral("effective_pen_id")).toString()));
    }
    refreshToggle(m_selectedLocked, selectedObject.value(QStringLiteral("locked")).toBool(), !selectedObject.isEmpty());
    refreshToggle(m_selectedVisible, selectedObject.isEmpty() ? false : selectedObject.value(QStringLiteral("visible")).toBool(), !selectedObject.isEmpty());
    refreshToggle(m_defaultLayerLocked, activeLayer.value(QStringLiteral("locked")).toBool(), !activeLayer.isEmpty());
    refreshToggle(m_defaultLayerVisible, activeLayer.isEmpty() ? false : activeLayer.value(QStringLiteral("visible")).toBool(), !activeLayer.isEmpty());
    setWidgetEnabled(m_layerDownButton, !activeLayer.isEmpty() && activeLayer.value(QStringLiteral("order")).toInt() > 0);
    setWidgetEnabled(m_layerUpButton, !activeLayer.isEmpty() && activeLayer.value(QStringLiteral("order")).toInt() + 1 < layers.size());
    refreshToggle(m_activeLayerPlotEnabled, activeLayer.value(QStringLiteral("plot_enabled")).toBool(), !activeLayer.isEmpty());
    refreshComboData(m_activeLayerPen, activeLayer.value(QStringLiteral("pen_id")).toString(), 0, !activeLayer.isEmpty());
    refreshComboData(m_activeLayerStrokeWidth, strokeWidthPresetId(activeLayer.value(QStringLiteral("stroke_width")).toDouble()), 1, !activeLayer.isEmpty());
    refreshLayerCombo(m_activeLayer, layers, activeLayerId, true);
    refreshLayerCombo(
        m_selectedObjectLayer,
        layers,
        selectedObject.value(QStringLiteral("layer_id")).toString(),
        !selectedObject.isEmpty());
    refreshComboData(m_gridPreset, grid.value(QStringLiteral("preset")).toString(), 0);
    refreshComboData(m_gridUnit, grid.value(QStringLiteral("unit")).toString(), 0);
    refreshSpinValue(m_gridWidth, grid.value(QStringLiteral("width")).toDouble());
    refreshSpinValue(m_gridHeight, grid.value(QStringLiteral("height")).toDouble());
    refreshSpinValue(m_gridMarginLeft, grid.value(QStringLiteral("margin_left")).toDouble());
    refreshSpinValue(m_gridMarginTop, grid.value(QStringLiteral("margin_top")).toDouble());
    refreshSpinValue(m_gridMarginRight, grid.value(QStringLiteral("margin_right")).toDouble());
    refreshSpinValue(m_gridMarginBottom, grid.value(QStringLiteral("margin_bottom")).toDouble());
    refreshSpinValue(m_gridMinorStep, grid.value(QStringLiteral("minor_step")).toDouble());
    refreshSpinValue(m_gridMajorEvery, grid.value(QStringLiteral("major_line_every")).toInt());
    refreshToggle(m_gridVisible, grid.value(QStringLiteral("visible")).toBool());
    if (m_objectMeasurementValue != nullptr) {
        const QVariantList lines = selectedObject.value(QStringLiteral("measurement_lines")).toList();
        QStringList formatted;
        for (const QVariant &line : lines) {
            formatted.push_back(line.toString());
        }
        m_objectMeasurementValue->setText(formatted.isEmpty()
            ? QStringLiteral("Measurement: none")
            : QStringLiteral("Measurement:\n%1").arg(formatted.join(QLatin1Char('\n'))));
    }
    setLabelText(m_objectPlotSafetyValue, selectedPlotSafetySummary(selectedObject, plot));
    setLabelText(m_selectionPlotBoundsValue, selectionPlotBoundsSummary(document));
    for (const auto &conditional : std::as_const(m_conditionalButtons)) {
        conditional.first->setEnabled(conditional.second == QStringLiteral("has_selection")
            ? !selectedObject.isEmpty()
            : selectedObject.value(conditional.second).toBool());
    }
    const bool boundsGuideControlsEnabled = selectedObject.value(QStringLiteral("bounds_guide_controls")).toBool();
    for (QPushButton *button : std::as_const(m_boundsGuideButtons)) {
        button->setEnabled(boundsGuideControlsEnabled);
    }
    for (QPushButton *button : std::as_const(m_offsetGuideButtons)) {
        button->setEnabled(boundsGuideControlsEnabled);
    }
    const bool alignToGuideControlsEnabled = selectedObject.value(QStringLiteral("align_to_guide_controls")).toBool();
    for (QPushButton *button : std::as_const(m_alignToGuideButtons)) {
        button->setEnabled(alignToGuideControlsEnabled);
    }
    const bool selectedGuideControlsEnabled = selectedObject.value(QStringLiteral("guide_drawable_controls")).toBool();
    for (QPushButton *button : std::as_const(m_guideOffsetButtons)) {
        button->setEnabled(selectedGuideControlsEnabled);
    }
    const bool guideVisualControlsEnabled = selectedObject.value(QStringLiteral("guide_visual_controls")).toBool();
    if (m_guideLabel != nullptr) {
        const QSignalBlocker blocker(m_guideLabel);
        m_guideLabel->setEnabled(guideVisualControlsEnabled);
        m_guideLabel->setText(guideVisualControlsEnabled ? selectedObject.value(QStringLiteral("guide_custom_label")).toString() : QString());
    }
    refreshComboData(m_guideColor, selectedObject.value(QStringLiteral("guide_color")).toString(), 0, guideVisualControlsEnabled);
    refreshComboData(m_guideDashStyle, selectedObject.value(QStringLiteral("guide_dash_style")).toString(), 0, guideVisualControlsEnabled);
    refreshToggle(m_guideShowLabel, selectedObject.value(QStringLiteral("guide_show_label"), true).toBool(), guideVisualControlsEnabled);
    const bool dimensionControlsEnabled = selectedObject.value(QStringLiteral("dimension_visual_controls")).toBool();
    if (m_dimensionReadout != nullptr) {
        const QVariantMap physical = selectedObject.value(QStringLiteral("physical_geometry")).toMap();
        m_dimensionReadout->setText(dimensionControlsEnabled
                ? QStringLiteral("Dimension: %1   Physical: %2   Kind: %3")
                    .arg(selectedObject.value(QStringLiteral("label")).toString(),
                        physical.value(QStringLiteral("dimension_label")).toString(),
                        selectedObject.value(QStringLiteral("dimension_kind")).toString())
                : QStringLiteral("Dimension: none"));
    }
    refreshComboData(m_dimensionKind, selectedObject.value(QStringLiteral("dimension_kind")).toString(), 0, dimensionControlsEnabled);
    refreshToggle(m_dimensionShowLabel, selectedObject.value(QStringLiteral("dimension_show_label"), true).toBool(), dimensionControlsEnabled);
    rebuildGeometryEditor(selectedObject);
    applyGeometryEditStatus(editStatus);
    setLabelText(m_objectsValue, QStringLiteral("Objects: %1").arg(objects.size()));
    if (m_guidesValue != nullptr) {
        m_guidesValue->setText(QStringLiteral("Guides: %1 visible / %2 total / %3 duplicate")
            .arg(document.value(QStringLiteral("visible_guide_count")).toInt())
            .arg(document.value(QStringLiteral("guide_count")).toInt())
            .arg(document.value(QStringLiteral("duplicate_guide_count")).toInt()));
    }
    setLabelText(m_revisionValue, QStringLiteral("Revision: %1").arg(document.value(QStringLiteral("revision")).toInt()));
    if (m_snapValue != nullptr) {
        m_snapValue->setText(QStringLiteral("Snap grid: %1   Object: %2   Guide: %3   Move: %4   Priority: %5")
            .arg(yesNo(snap.value(QStringLiteral("grid_enabled")).toBool()))
            .arg(yesNo(snap.value(QStringLiteral("object_enabled")).toBool()))
            .arg(yesNo(snap.value(QStringLiteral("guide_enabled")).toBool()))
            .arg(yesNo(snap.value(QStringLiteral("guide_move_enabled")).toBool()))
            .arg(snap.value(QStringLiteral("object_priority_before_grid")).toBool() ? QStringLiteral("object") : QStringLiteral("grid")));
    }
    if (m_gridValue != nullptr) {
        m_gridValue->setText(QStringLiteral("Bed: %1, %2 x %3 %4, step %5")
            .arg(grid.value(QStringLiteral("preset_label")).toString())
            .arg(formatNumber(grid.value(QStringLiteral("width")).toDouble()))
            .arg(formatNumber(grid.value(QStringLiteral("height")).toDouble()))
            .arg(grid.value(QStringLiteral("unit_label")).toString())
            .arg(formatNumber(grid.value(QStringLiteral("minor_step")).toDouble())));
    }
    if (m_plotValue != nullptr) {
        const bool blocked = plot.value(QStringLiteral("blocked")).toBool();
        const int travelSegmentCount = plot.value(QStringLiteral("travel_segment_count")).toInt();
        const QString travelDistance = formatNumber(plot.value(QStringLiteral("travel_distance")).toDouble());
        m_plotValue->setText(blocked
            ? QStringLiteral("Plot: %1 objects, %2 strokes, %3 travel (%4), %5 warnings, first %6")
                .arg(plot.value(QStringLiteral("plot_object_count")).toInt())
                .arg(plot.value(QStringLiteral("segment_count")).toInt())
                .arg(travelSegmentCount)
                .arg(travelDistance)
                .arg(plot.value(QStringLiteral("warning_count")).toInt())
                .arg(plot.value(QStringLiteral("first_warning_object_id")).toString())
            : QStringLiteral("Plot: %1 objects, %2 strokes, %3 travel (%4), ready")
                .arg(plot.value(QStringLiteral("plot_object_count")).toInt())
                .arg(plot.value(QStringLiteral("segment_count")).toInt())
                .arg(travelSegmentCount)
                .arg(travelDistance));
    }
    setLabelText(m_plotBoundsValue, plotBoundsSummary(grid, plot));
    if (m_plotLayerStatsValue != nullptr) {
        QVariantMap activeLayerStats;
        const QVariantList layerStats = plot.value(QStringLiteral("layer_stats")).toList();
        for (const QVariant &statsValue : layerStats) {
            const QVariantMap stats = statsValue.toMap();
            if (stats.value(QStringLiteral("layer_id")).toString() == activeLayerId) {
                activeLayerStats = stats;
                break;
            }
        }
        m_plotLayerStatsValue->setText(activeLayerStats.isEmpty()
            ? QStringLiteral("Layer plot: none")
            : QStringLiteral("Layer plot: %1, %2 obj, %3 strokes, draw %4, travel %5")
                .arg(activeLayerStats.value(QStringLiteral("ready")).toBool()
                    ? QStringLiteral("ready")
                    : activeLayerStats.value(QStringLiteral("blocked_reason")).toString())
                .arg(activeLayerStats.value(QStringLiteral("object_count")).toInt())
                .arg(activeLayerStats.value(QStringLiteral("segment_count")).toInt())
                .arg(formatNumber(activeLayerStats.value(QStringLiteral("stroke_distance")).toDouble()))
                .arg(formatNumber(activeLayerStats.value(QStringLiteral("travel_distance")).toDouble())));
    }
    if (m_plotPenStatsValue != nullptr) {
        QVariantMap activePenStats;
        const QString activePenId = activeLayer.value(QStringLiteral("pen_id")).toString();
        const QVariantList penStats = plot.value(QStringLiteral("pen_stats")).toList();
        for (const QVariant &statsValue : penStats) {
            const QVariantMap stats = statsValue.toMap();
            if (stats.value(QStringLiteral("pen_id")).toString() == activePenId) {
                activePenStats = stats;
                break;
            }
        }
        m_plotPenStatsValue->setText(activePenStats.isEmpty()
            ? QStringLiteral("Pen plot: none")
            : QStringLiteral("Pen plot: %1, %2, %3 obj, %4 strokes, draw %5, travel %6")
                .arg(activePenStats.value(QStringLiteral("pen_id")).toString())
                .arg(activePenStats.value(QStringLiteral("ready")).toBool()
                    ? QStringLiteral("ready")
                    : activePenStats.value(QStringLiteral("blocked_reason")).toString())
                .arg(activePenStats.value(QStringLiteral("object_count")).toInt())
                .arg(activePenStats.value(QStringLiteral("segment_count")).toInt())
                .arg(formatNumber(activePenStats.value(QStringLiteral("stroke_distance")).toDouble()))
                .arg(formatNumber(activePenStats.value(QStringLiteral("travel_distance")).toDouble())));
    }
    if (m_plotReadinessValue != nullptr) {
        m_plotReadinessValue->setText(formatPlotReadinessChecklist(
            plot.value(QStringLiteral("layer_stats")).toList(),
            plot.value(QStringLiteral("pen_stats")).toList()));
    }
    if (m_calibrationMeasurementValue != nullptr) {
        m_calibrationMeasurementValue->setText(calibrationMeasurement.isEmpty()
            ? QStringLiteral("Calibration measurement: none")
            : QStringLiteral("Calibration: %1 expected %2 measured %3 error %4 (%5%)\nApply scale: %6   active scale: %7")
                .arg(calibrationMeasurement.value(QStringLiteral("pattern_id")).toString())
                .arg(formatNumber(calibrationMeasurement.value(QStringLiteral("expected_value")).toDouble()))
                .arg(formatNumber(calibrationMeasurement.value(QStringLiteral("measured_value")).toDouble()))
                .arg(formatNumber(calibrationMeasurement.value(QStringLiteral("error_value")).toDouble()))
                .arg(formatNumber(calibrationMeasurement.value(QStringLiteral("percent_error")).toDouble()))
                .arg(calibrationCorrection.isEmpty()
                    ? QStringLiteral("none")
                    : formatNumber(calibrationCorrection.value(QStringLiteral("scale_factor")).toDouble()))
                .arg(formatNumber(plot.value(QStringLiteral("calibration_scale")).toDouble())));
    }
    refreshComboData(m_plotOrderMode, plot.value(QStringLiteral("order_mode")).toString(), 0);
    refreshComboData(m_plotDirectionMode, plot.value(QStringLiteral("direction_mode")).toString(), 0);
    if (m_pointerValue != nullptr) {
        if (pointer.isEmpty()) {
            m_pointerValue->setText(QStringLiteral("Pointer: none"));
        } else {
            const QVariantMap raw = pointer.value(QStringLiteral("raw")).toMap();
            const QVariantMap snapped = pointer.value(QStringLiteral("snapped")).toMap();
            const QString sourceObjectId = pointer.value(QStringLiteral("source_object_id")).toString();
            m_pointerValue->setText(QStringLiteral("Pointer: raw %1,%2  snap %3,%4 %5/%6  source %7  unit %8,%9 %10  %11")
                .arg(formatNumber(raw.value(QStringLiteral("x")).toDouble()))
                .arg(formatNumber(raw.value(QStringLiteral("y")).toDouble()))
                .arg(formatNumber(snapped.value(QStringLiteral("x")).toDouble()))
                .arg(formatNumber(snapped.value(QStringLiteral("y")).toDouble()))
                .arg(pointer.value(QStringLiteral("kind")).toString())
                .arg(pointer.value(QStringLiteral("source")).toString())
                .arg(sourceObjectId.isEmpty() ? QStringLiteral("none") : sourceObjectId)
                .arg(formatNumber(pointer.value(QStringLiteral("snapped_unit_x")).toDouble()))
                .arg(formatNumber(pointer.value(QStringLiteral("snapped_unit_y")).toDouble()))
                .arg(pointer.value(QStringLiteral("unit_label")).toString())
                .arg(pointer.value(QStringLiteral("inside_drawable")).toBool() ? QStringLiteral("inside") : QStringLiteral("outside")));
        }
    }
    if (m_quickMeasureValue != nullptr) {
        if (quickMeasurement.isEmpty() || !quickMeasurement.value(QStringLiteral("ok")).toBool()) {
            m_quickMeasureValue->setText(QStringLiteral("Quick measure: none"));
        } else {
            m_quickMeasureValue->setText(QStringLiteral("Quick measure: %1  target %2")
                .arg(quickMeasurement.value(QStringLiteral("label")).toString())
                .arg(quickMeasurement.value(QStringLiteral("object_id")).toString()));
        }
    }
    if (m_guideDragValue != nullptr) {
        if (guideDragSnap.isEmpty()) {
            m_guideDragValue->setText(QStringLiteral("Guide drag: none"));
        } else {
            const QString sourceObjectId = guideDragSnap.value(QStringLiteral("source_object_id")).toString();
            m_guideDragValue->setText(QStringLiteral("Guide drag: %1 -> %2 %3 dist %4")
                .arg(guideDragSnap.value(QStringLiteral("anchor_label")).toString())
                .arg(sourceObjectId.isEmpty() ? QStringLiteral("guide") : sourceObjectId)
                .arg(guideDragSnap.value(QStringLiteral("intersection")).toBool() ? QStringLiteral("intersection") : QStringLiteral("axis"))
                .arg(formatNumber(guideDragSnap.value(QStringLiteral("distance")).toDouble())));
        }
    }
    setLabelText(m_previewValue, QStringLiteral("Preview: %1").arg(hasPreview ? QStringLiteral("active") : QStringLiteral("none")));
    if (m_actions.setStatusText) {
        // The status line is shell chrome now; the feature publishes, the
        // shell decides where it shows.
        m_actions.setStatusText(QStringLiteral("%1 | %2 selected | %3 objects")
            .arg(m_actions.workspaceModeName())
            .arg(selected.size())
            .arg(objects.size()));
    }

    if (m_undoButton != nullptr) {
        m_undoButton->setEnabled(m_controller->canUndo());
    }
    if (m_redoButton != nullptr) {
        m_redoButton->setEnabled(m_controller->canRedo());
    }
}
