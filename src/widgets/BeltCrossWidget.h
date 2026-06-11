#pragma once

#include <QPoint>
#include <QString>
#include <QVector>
#include <QWidget>

#include <optional>
#include <vector>

#include "widgets/BeltState.h"

namespace edi::shell {

// What a belt slot displays. Pure data: the widget renders glyphs and emits
// ids; it has no idea what an id means (tools here, weapons elsewhere).
struct BeltItem {
    QString id;
    QString glyph;
    QString tooltip;
};

} // namespace edi::shell

// The game-style weapon-belt carousel. The mental model (user direction
// 2026-06-10): each ROW is one tool, its cells are that tool's
// sub-features. Only the active row renders full-size; the neighbouring
// tools peek in as half-cells above and below, hinting what a vertical
// scroll reaches. Vertical scroll changes tool, horizontal scroll walks its
// sub-features (one cell per wheel notch), clicking a peek steps to that
// tool; everything wraps and skips empties (BeltState owns that math — this
// class owns pixels and input only).
class BeltCrossWidget final : public QWidget {
    Q_OBJECT

public:
    explicit BeltCrossWidget(QWidget *parent = nullptr);

    // Items fill the grid row-major (slot i = items[i]); slots holding an
    // empty id are invisible to rendering and navigation. The active cell
    // snaps to the nearest occupied slot.
    void setItems(const QVector<edi::shell::BeltItem> &items);
    void setGridSize(int rows, int columns);
    // Programmatic sync (the host's notion of "current" changed elsewhere).
    // Deliberately does NOT emit selected() — echoing a sync back as a user
    // pick is how host<->widget update loops start.
    void setActiveIndex(int index);

    int activeIndex() const;
    QString activeItemId() const;
    // The active cell's human name (its tooltip), as painted on the name
    // line under the carousel. Exposed so hosts and tests read the same
    // string the user sees.
    QString activeItemLabel() const;
    // Frozen quick-bars (grid row indexes, pin order). Pinning is a user
    // gesture on the widget; the host observes via pinsChanged.
    std::vector<int> pinnedRows() const { return m_pinnedRows; }
    // Programmatic sync (workspace restore). Invalid/duplicate rows drop,
    // empty rows prune — a stale file degrades to fewer pins, never to a
    // broken quick bar. Does NOT emit pinsChanged (same no-echo rule as
    // setActiveIndex: a sync echoed back as a gesture is how loops start).
    void setPinnedRows(const std::vector<int> &rows);
    // Slot index of the item with this id, or -1. The host syncs with
    // setActiveIndex(indexOfItem(currentId)) when its notion of "current"
    // changes through some other control.
    int indexOfItem(const QString &id) const;
    edi::shell::BeltState beltState() const { return m_state; }

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

signals:
    void selected(const QString &id);
    // A user gesture changed the pinned rows (pin or kill nub). Emitted so
    // the host can fold the new pin set into the workspace layout.
    void pinsChanged();

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    bool event(QEvent *event) override; // tooltips for hovered cross cells

private:
    // Geometry of the carousel: the active row's full cells in the middle
    // band, half-cell peeks above and below aligned over the active cell.
    // Positions are list indexes (gaps compacted), not grid columns.
    int carouselTop() const;       // y where the live carousel starts (below pins)
    QRect activeCellRect(int position) const;
    int activeItemPosition() const; // list position of the active cell, or 0
    QRect topPeekRect() const;
    QRect bottomPeekRect() const;
    QRect activeLabelRect() const; // the name line under the carousel
    QRect pinnedCellRect(int pinPosition, int itemPosition) const;
    QRect pinNubRect() const;               // freezes the live row
    QRect killNubRect(int pinPosition) const; // removes a frozen row
    const edi::shell::BeltItem *itemAt(int row, int column) const;
    // The visible cell under `pos` as a grid target, or nullopt for dead space.
    std::optional<edi::shell::BeltState> stateForClick(const QPoint &pos) const;
    void rebuildOccupancy();
    void applyState(const edi::shell::BeltState &next, bool emitSelection);

    edi::shell::BeltState m_state;
    QVector<edi::shell::BeltItem> m_items;
    std::vector<bool> m_occupied; // row-major mask derived from m_items
    std::vector<int> m_pinnedRows;
    QPoint m_wheelRemainder;      // sub-notch wheel deltas, per axis
};
