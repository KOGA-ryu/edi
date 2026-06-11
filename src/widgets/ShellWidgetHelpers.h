#pragma once

#include <QPalette>
#include <QString>
#include <QVariantMap>
#include <QVariantList>

#include <optional>

class QFrame;
class QPushButton;
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

struct ShellTheme;

// Palette for widgets that paint themselves through QPalette roles instead
// of QSS (the belt cross): QSS cannot reach a custom paintEvent; the palette
// is the data channel that can. Pure adapter: same theme in, same palette
// out. Lives here, not in ShellTheme — that module stays QtGui-free.
QPalette derivePaintingPalette(const ShellTheme &theme);

QFrame *makeRegionFrame(const QString &objectName);

// A side panel whose content scrolls vertically. Returns the outer frame (for
// the layout) and the inner content layout (for addWidget calls). Without
// this, a panel with more controls than vertical room squashes every child
// toward zero height — buttons become unclickable slivers. The scroll area
// lets the content keep its natural height and offers a scrollbar instead.
// Width is a [min, max] band (max 0 = unbounded) so a splitter can drag-resize
// the panel; the splitter respects these as hard limits.
struct ScrollablePanel {
    QFrame *panel = nullptr;
    QVBoxLayout *content = nullptr;
};

ScrollablePanel makeScrollablePanel(const QString &objectName, int minWidth, int maxWidth);

// A compact checkable button as used by the activity rail and bottom-panel
// tabs. Free function rather than a window method: both the shell and the
// drafting feature build these, and the button needs nothing from either.
QPushButton *makeRailButton(const QString &label, const QString &tooltip, bool active = false, bool enabled = true);

// UPPERCASE section header label (styled by the #sectionLabel QSS rule).
// Free function: every feature builds these, and they need nothing from any.
QLabel *makeSectionLabel(const QString &text);

// A disclosure section: a clickable header that folds its content. The
// inspector's de-bloat tool — every section stays reachable, only the ones
// a context actually needs start open (openInitially is the caller's data).
// Collapsed state is session furniture, not persisted.
QWidget *makeCollapsibleSection(const QString &title, QWidget *content, bool openInitially);

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
