#pragma once

#include <QString>
#include <QVariantMap>
#include <QVariantList>

#include <optional>

class QFrame;
class QWidget;
class QVBoxLayout;
class QGridLayout;
class QLayout;
class QLabel;
class QCheckBox;
class QComboBox;
class QDoubleSpinBox;
class QSpinBox;

namespace edi::shell {

QFrame *makeRegionFrame(const QString &objectName);

// A fixed-width side panel whose content scrolls vertically. Returns the outer
// frame (for the layout) and the inner content layout (for addWidget calls).
// Without this, a panel with more controls than vertical room squashes every
// child toward zero height — buttons become unclickable slivers. The scroll
// area lets the content keep its natural height and offers a scrollbar instead.
struct ScrollablePanel {
    QFrame *panel = nullptr;
    QVBoxLayout *content = nullptr;
};

ScrollablePanel makeScrollablePanel(const QString &objectName, int fixedWidth);

void clearLayoutMargins(QLayout *layout);

void setWidgetEnabled(QWidget *widget, bool enabled);

void setLabelText(QLabel *label, const QString &text);

void refreshToggle(QCheckBox *checkbox, bool checked, std::optional<bool> enabled = std::nullopt);

// fallbackIndex of nullopt keeps the current index when the data is not found.
void refreshComboData(QComboBox *combo, const QString &data, std::optional<int> fallbackIndex, std::optional<bool> enabled = std::nullopt);

void refreshSpinValue(QDoubleSpinBox *spin, double value);

void refreshSpinValue(QSpinBox *spin, int value);

struct ControlGrid {
    QWidget *panel = nullptr;
    QGridLayout *layout = nullptr;
};

ControlGrid makeControlGrid(const QString &objectName);

QString yesNo(bool value);

QString formatNumber(double value);

QString formatBoundsValue(const QVariantMap &bounds);

QString plotBoundsStatus(const QVariantMap &plot);

QString plotBoundsSummary(const QVariantMap &grid, const QVariantMap &plot);

QString formatPlotReadinessChecklist(const QVariantList &layerStats, const QVariantList &penStats);

QVariantMap plotStatsByField(const QVariantList &statsList, const QString &field, const QString &value);

QString readinessText(const QVariantMap &stats);

QString selectedPlotSafetySummary(const QVariantMap &object, const QVariantMap &plot);

QString selectionPlotBoundsSummary(const QVariantMap &document);

QVariantMap activeObjectProjection(const QVariantMap &document);

QVariantMap layerProjection(const QVariantMap &document, const QString &layerId);

void refreshLayerCombo(QComboBox *combo, const QVariantList &layers, const QString &currentLayerId, bool enabled);

QString strokeWidthPresetId(double width);

QString boundsSummary(const QVariantMap &object);

QString geometrySummary(const QVariantMap &object);

} // namespace edi::shell
