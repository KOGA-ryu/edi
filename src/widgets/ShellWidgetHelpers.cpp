#include "widgets/ShellWidgetHelpers.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFrame>
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QSignalBlocker>
#include <QSpinBox>
#include <QString>
#include <QStringList>
#include <QVariantList>
#include <QVariantMap>
#include <QVBoxLayout>
#include <QWidget>

#include <optional>

namespace edi::shell {

QFrame *makeRegionFrame(const QString &objectName)
{
    auto *frame = new QFrame;
    frame->setObjectName(objectName);
    frame->setFrameShape(QFrame::NoFrame);
    return frame;
}

ScrollablePanel makeScrollablePanel(const QString &objectName, int minWidth, int maxWidth)
{
    auto *panel = makeRegionFrame(objectName);
    panel->setMinimumWidth(minWidth);
    if (maxWidth > 0) {
        panel->setMaximumWidth(maxWidth);
    }

    // Outer layout holds only the scroll area, edge-to-edge.
    auto *outer = new QVBoxLayout(panel);
    outer->setContentsMargins(0, 0, 0, 0);
    outer->setSpacing(0);

    auto *scroll = new QScrollArea;
    scroll->setObjectName(objectName + QStringLiteral("Scroll"));
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setWidgetResizable(true); // content width tracks the viewport
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    outer->addWidget(scroll);

    auto *contentWidget = new QWidget;
    auto *content = new QVBoxLayout(contentWidget);
    content->setContentsMargins(12, 12, 12, 12);
    content->setSpacing(8);
    scroll->setWidget(contentWidget);

    return {panel, content};
}

QPushButton *makeRailButton(const QString &label, const QString &tooltip, bool active, bool enabled)
{
    auto *button = new QPushButton(label);
    button->setObjectName(QStringLiteral("railButton"));
    button->setToolTip(tooltip);
    button->setCheckable(true);
    button->setChecked(active);
    button->setEnabled(enabled);
    button->setMinimumHeight(30);
    return button;
}

QLabel *makeSectionLabel(const QString &text)
{
    auto *label = new QLabel(text);
    label->setObjectName(QStringLiteral("sectionLabel"));
    return label;
}

QWidget *makeCollapsibleSection(const QString &title, QWidget *content, bool openInitially)
{
    auto *section = new QWidget;
    section->setObjectName(QStringLiteral("collapsibleSection"));
    auto *layout = new QVBoxLayout(section);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(4);

    auto *header = new QPushButton((openInitially ? QStringLiteral("▾ ") : QStringLiteral("▸ ")) + title);
    header->setObjectName(QStringLiteral("sectionToggle"));
    header->setCheckable(true);
    header->setChecked(openInitially);
    layout->addWidget(header);

    content->setVisible(openInitially);
    layout->addWidget(content);

    // The glyph is state the user reads at a glance; flip it with the fold.
    QObject::connect(header, &QPushButton::toggled, content, [header, content, title](bool open) {
        content->setVisible(open);
        header->setText((open ? QStringLiteral("▾ ") : QStringLiteral("▸ ")) + title);
    });
    return section;
}

void clearLayoutMargins(QLayout *layout)
{
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
}

void setWidgetEnabled(QWidget *widget, bool enabled)
{
    if (widget != nullptr) {
        widget->setEnabled(enabled);
    }
}

void setLabelText(QLabel *label, const QString &text)
{
    if (label != nullptr) {
        label->setText(text);
    }
}

void refreshToggle(QCheckBox *checkbox, bool checked, std::optional<bool> enabled)
{
    if (checkbox == nullptr) {
        return;
    }
    const QSignalBlocker blocker(checkbox);
    if (enabled) {
        checkbox->setEnabled(*enabled);
    }
    checkbox->setChecked(checked);
}

// fallbackIndex of nullopt keeps the current index when the data is not found.
void refreshComboData(QComboBox *combo, const QString &data, std::optional<int> fallbackIndex, std::optional<bool> enabled)
{
    if (combo == nullptr) {
        return;
    }
    const QSignalBlocker blocker(combo);
    if (enabled) {
        combo->setEnabled(*enabled);
    }
    const int index = combo->findData(data);
    if (index >= 0) {
        combo->setCurrentIndex(index);
    } else if (fallbackIndex) {
        combo->setCurrentIndex(*fallbackIndex);
    }
}

void refreshSpinValue(QDoubleSpinBox *spin, double value)
{
    if (spin == nullptr) {
        return;
    }
    const QSignalBlocker blocker(spin);
    spin->setValue(value);
}

void refreshSpinValue(QSpinBox *spin, int value)
{
    if (spin == nullptr) {
        return;
    }
    const QSignalBlocker blocker(spin);
    spin->setValue(value);
}

ControlGrid makeControlGrid(const QString &objectName)
{
    auto *panel = new QWidget;
    panel->setObjectName(objectName);
    auto *layout = new QGridLayout(panel);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setHorizontalSpacing(6);
    layout->setVerticalSpacing(6);
    return {panel, layout};
}

QString yesNo(bool value)
{
    return value ? QStringLiteral("on") : QStringLiteral("off");
}

QString formatNumber(double value)
{
    return QString::number(value, 'f', 3);
}

QString formatBoundsValue(const QVariantMap &bounds)
{
    if (bounds.isEmpty()) {
        return QStringLiteral("unavailable");
    }
    return QStringLiteral("x %1, y %2, w %3, h %4")
        .arg(formatNumber(bounds.value(QStringLiteral("x")).toDouble()))
        .arg(formatNumber(bounds.value(QStringLiteral("y")).toDouble()))
        .arg(formatNumber(bounds.value(QStringLiteral("width")).toDouble()))
        .arg(formatNumber(bounds.value(QStringLiteral("height")).toDouble()));
}

QString plotBoundsStatus(const QVariantMap &plot)
{
    if (!plot.value(QStringLiteral("has_plot_bounds")).toBool()) {
        return QStringLiteral("unavailable");
    }
    const QString warningKind = plot.value(QStringLiteral("first_warning_kind")).toString();
    if (warningKind == QStringLiteral("raw_out_of_drawable_bounds")
        || warningKind == QStringLiteral("calibrated_plot_out_of_drawable_bounds")) {
        return QStringLiteral("outside");
    }
    return QStringLiteral("inside");
}

QString plotBoundsSummary(const QVariantMap &grid, const QVariantMap &plot)
{
    const QString unit = grid.value(QStringLiteral("unit_label")).toString();
    const QString plotBounds = plot.value(QStringLiteral("has_plot_bounds")).toBool()
        ? formatBoundsValue(plot.value(QStringLiteral("plot_bounds")).toMap())
        : QStringLiteral("unavailable");

    return QStringLiteral("Bounds (%1):\nBed: %2\nDrawable: %3\nPlot: %4\nPlot status: %5")
        .arg(unit.isEmpty() ? QStringLiteral("unit") : unit)
        .arg(formatBoundsValue(grid.value(QStringLiteral("page_bounds")).toMap()))
        .arg(formatBoundsValue(grid.value(QStringLiteral("drawable_bounds")).toMap()))
        .arg(plotBounds)
        .arg(plotBoundsStatus(plot));
}

QString formatPlotReadinessChecklist(const QVariantList &layerStats, const QVariantList &penStats)
{
    int readyLayers = 0;
    int blockedLayers = 0;
    int readyPens = 0;
    int blockedPens = 0;
    QStringList blockers;

    for (const QVariant &statsValue : layerStats) {
        const QVariantMap stats = statsValue.toMap();
        if (stats.value(QStringLiteral("ready")).toBool()) {
            ++readyLayers;
            continue;
        }
        ++blockedLayers;
        if (blockers.size() < 4) {
            blockers.push_back(QStringLiteral("L:%1 %2")
                .arg(stats.value(QStringLiteral("layer_id")).toString())
                .arg(stats.value(QStringLiteral("blocked_reason")).toString()));
        }
    }

    for (const QVariant &statsValue : penStats) {
        const QVariantMap stats = statsValue.toMap();
        if (stats.value(QStringLiteral("ready")).toBool()) {
            ++readyPens;
            continue;
        }
        ++blockedPens;
        if (blockers.size() < 4) {
            const QString penId = stats.value(QStringLiteral("pen_id")).toString();
            blockers.push_back(QStringLiteral("P:%1 %2")
                .arg(penId.isEmpty() ? QStringLiteral("<none>") : penId)
                .arg(stats.value(QStringLiteral("blocked_reason")).toString()));
        }
    }

    QString summary = QStringLiteral("Readiness: layers %1 ready/%2 blocked, pens %3 ready/%4 blocked")
        .arg(readyLayers)
        .arg(blockedLayers)
        .arg(readyPens)
        .arg(blockedPens);
    if (!blockers.isEmpty()) {
        summary.append(QStringLiteral("\nBlocked: %1").arg(blockers.join(QStringLiteral("; "))));
    }
    return summary;
}

QVariantMap plotStatsByField(const QVariantList &statsList, const QString &field, const QString &value)
{
    for (const QVariant &statsValue : statsList) {
        const QVariantMap stats = statsValue.toMap();
        if (stats.value(field).toString() == value) {
            return stats;
        }
    }
    return {};
}

QString readinessText(const QVariantMap &stats)
{
    if (stats.isEmpty()) {
        return QStringLiteral("none");
    }
    return stats.value(QStringLiteral("ready")).toBool()
        ? QStringLiteral("ready")
        : stats.value(QStringLiteral("blocked_reason")).toString();
}

QString selectedPlotSafetySummary(const QVariantMap &object, const QVariantMap &plot)
{
    if (object.isEmpty()) {
        return QStringLiteral("Plot safety: no selection");
    }

    QString objectStatus = QStringLiteral("non-plotting");
    if (object.value(QStringLiteral("plot_blocked")).toBool()) {
        objectStatus = QStringLiteral("blocked");
    } else if (object.value(QStringLiteral("effective_plot_ready")).toBool()) {
        objectStatus = QStringLiteral("ready");
    }

    const QVariantMap layerStats = plotStatsByField(
        plot.value(QStringLiteral("layer_stats")).toList(),
        QStringLiteral("layer_id"),
        object.value(QStringLiteral("layer_id")).toString());
    const QVariantMap penStats = plotStatsByField(
        plot.value(QStringLiteral("pen_stats")).toList(),
        QStringLiteral("pen_id"),
        object.value(QStringLiteral("effective_pen_id")).toString());

    QStringList lines;
    lines.push_back(QStringLiteral("Object: %1").arg(objectStatus));
    lines.push_back(QStringLiteral("Warning: %1")
        .arg(object.value(QStringLiteral("plot_warning_kind")).toString().isEmpty()
                ? QStringLiteral("none")
                : object.value(QStringLiteral("plot_warning_kind")).toString()));
    const QString message = object.value(QStringLiteral("plot_warning_message")).toString();
    if (!message.isEmpty()) {
        lines.push_back(QStringLiteral("Message: %1").arg(message));
    }
    lines.push_back(QStringLiteral("Bounds: raw %1, calibrated %2")
        .arg(yesNo(object.value(QStringLiteral("outside_drawable")).toBool()))
        .arg(yesNo(object.value(QStringLiteral("calibrated_outside_drawable")).toBool())));
    lines.push_back(QStringLiteral("Layer: %1").arg(readinessText(layerStats)));
    lines.push_back(QStringLiteral("Pen: %1").arg(readinessText(penStats)));

    return QStringLiteral("Plot safety:\n%1").arg(lines.join(QLatin1Char('\n')));
}

QString selectionPlotBoundsSummary(const QVariantMap &document)
{
    if (!document.value(QStringLiteral("has_selection_plot_bounds")).toBool()) {
        return QStringLiteral("Selection plot bounds: unavailable");
    }
    return QStringLiteral("Selection plot bounds:\n%1\nw %2, h %3\n%4")
        .arg(formatBoundsValue(document.value(QStringLiteral("selection_plot_bounds")).toMap()))
        .arg(formatNumber(document.value(QStringLiteral("selection_plot_bounds_width")).toDouble()))
        .arg(formatNumber(document.value(QStringLiteral("selection_plot_bounds_height")).toDouble()))
        .arg(document.value(QStringLiteral("selection_drawable_relation")).toString());
}

QVariantMap activeObjectProjection(const QVariantMap &document)
{
    const QString activeId = document.value(QStringLiteral("active_object_id")).toString();
    if (activeId.isEmpty()) {
        return {};
    }

    const QVariantList objects = document.value(QStringLiteral("drawing_objects")).toList();
    for (const QVariant &objectValue : objects) {
        const QVariantMap object = objectValue.toMap();
        if (object.value(QStringLiteral("id")).toString() == activeId) {
            return object;
        }
    }
    return {};
}

QVariantMap layerProjection(const QVariantMap &document, const QString &layerId)
{
    const QVariantList layers = document.value(QStringLiteral("layers")).toList();
    for (const QVariant &layerValue : layers) {
        const QVariantMap layer = layerValue.toMap();
        if (layer.value(QStringLiteral("id")).toString() == layerId) {
            return layer;
        }
    }
    return {};
}

void refreshLayerCombo(QComboBox *combo, const QVariantList &layers, const QString &currentLayerId, bool enabled)
{
    if (combo == nullptr) {
        return;
    }

    {
        const QSignalBlocker blocker(combo);
        combo->clear();
        for (const QVariant &layerValue : layers) {
            const QVariantMap layer = layerValue.toMap();
            const QString id = layer.value(QStringLiteral("id")).toString();
            const QString label = QStringLiteral("%1 (%2%3)")
                .arg(layer.value(QStringLiteral("name")).toString())
                .arg(layer.value(QStringLiteral("visible")).toBool() ? QStringLiteral("V") : QStringLiteral("-"))
                .arg(layer.value(QStringLiteral("locked")).toBool() ? QStringLiteral("L") : QStringLiteral("-"));
            combo->addItem(label, id);
        }
    }
    refreshComboData(combo, currentLayerId, std::nullopt, enabled && !layers.empty());
}

QString strokeWidthPresetId(double width)
{
    if (width <= 1.5) {
        return QStringLiteral("fine");
    }
    if (width >= 2.5) {
        return QStringLiteral("bold");
    }
    return QStringLiteral("normal");
}

QString boundsSummary(const QVariantMap &object)
{
    const QVariantMap bounds = object.value(QStringLiteral("bounds")).toMap();
    if (bounds.isEmpty()) {
        return QStringLiteral("Bounds: unavailable");
    }
    return QStringLiteral("Bounds: x %1, y %2, w %3, h %4")
        .arg(formatNumber(bounds.value(QStringLiteral("x")).toDouble()))
        .arg(formatNumber(bounds.value(QStringLiteral("y")).toDouble()))
        .arg(formatNumber(bounds.value(QStringLiteral("width")).toDouble()))
        .arg(formatNumber(bounds.value(QStringLiteral("height")).toDouble()));
}

QString geometrySummary(const QVariantMap &object)
{
    const QString kind = object.value(QStringLiteral("kind")).toString();
    if (kind == QStringLiteral("point")) {
        return QStringLiteral("Geometry: point (%1, %2)")
            .arg(formatNumber(object.value(QStringLiteral("x")).toDouble()))
            .arg(formatNumber(object.value(QStringLiteral("y")).toDouble()));
    }
    if (kind == QStringLiteral("line")) {
        return QStringLiteral("Geometry: line (%1, %2) -> (%3, %4), len %5, ang %6")
            .arg(formatNumber(object.value(QStringLiteral("x1")).toDouble()))
            .arg(formatNumber(object.value(QStringLiteral("y1")).toDouble()))
            .arg(formatNumber(object.value(QStringLiteral("x2")).toDouble()))
            .arg(formatNumber(object.value(QStringLiteral("y2")).toDouble()))
            .arg(formatNumber(object.value(QStringLiteral("line_length")).toDouble()))
            .arg(formatNumber(object.value(QStringLiteral("line_angle_deg")).toDouble()));
    }
    if (kind == QStringLiteral("construction_line")) {
        return QStringLiteral("Geometry: construction (%1, %2) -> (%3, %4)")
            .arg(formatNumber(object.value(QStringLiteral("x1")).toDouble()))
            .arg(formatNumber(object.value(QStringLiteral("y1")).toDouble()))
            .arg(formatNumber(object.value(QStringLiteral("x2")).toDouble()))
            .arg(formatNumber(object.value(QStringLiteral("y2")).toDouble()));
    }
    if (kind == QStringLiteral("guide")) {
        return QStringLiteral("Geometry: %1 guide at %2")
            .arg(object.value(QStringLiteral("orientation")).toString())
            .arg(formatNumber(object.value(QStringLiteral("position")).toDouble()));
    }
    if (kind == QStringLiteral("dimension")) {
        return QStringLiteral("Geometry: dimension (%1, %2) -> (%3, %4), off %5, %6")
            .arg(formatNumber(object.value(QStringLiteral("x1")).toDouble()))
            .arg(formatNumber(object.value(QStringLiteral("y1")).toDouble()))
            .arg(formatNumber(object.value(QStringLiteral("x2")).toDouble()))
            .arg(formatNumber(object.value(QStringLiteral("y2")).toDouble()))
            .arg(formatNumber(object.value(QStringLiteral("offset")).toDouble()))
            .arg(object.value(QStringLiteral("label")).toString());
    }
    if (kind == QStringLiteral("rectangle")) {
        return QStringLiteral("Geometry: rect x %1, y %2, w %3, h %4, rot %5")
            .arg(formatNumber(object.value(QStringLiteral("x")).toDouble()))
            .arg(formatNumber(object.value(QStringLiteral("y")).toDouble()))
            .arg(formatNumber(object.value(QStringLiteral("width")).toDouble()))
            .arg(formatNumber(object.value(QStringLiteral("height")).toDouble()))
            .arg(formatNumber(object.value(QStringLiteral("rotation_deg")).toDouble()));
    }
    if (kind == QStringLiteral("circle")) {
        return QStringLiteral("Geometry: circle cx %1, cy %2, r %3, d %4")
            .arg(formatNumber(object.value(QStringLiteral("cx")).toDouble()))
            .arg(formatNumber(object.value(QStringLiteral("cy")).toDouble()))
            .arg(formatNumber(object.value(QStringLiteral("radius")).toDouble()))
            .arg(formatNumber(object.value(QStringLiteral("diameter")).toDouble()));
    }

    const QVariantList points = object.value(QStringLiteral("points")).toList();
    return QStringLiteral("Geometry: %1 points").arg(points.size());
}

} // namespace edi::shell
