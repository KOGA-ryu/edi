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

// The game-style "weapon cross" over a belt grid. The mental model (user
// direction 2026-06-10): each ROW is one tool, its cells are that tool's
// sub-features. The vertical strip lists every non-empty row by its lead
// item; the horizontal strip lays out the active row's items, gaps
// compacted away — no blank box ever renders. Vertical scroll changes tool,
// horizontal scroll walks its sub-features (one cell per wheel notch),
// clicking any visible cell jumps; everything wraps and skips empties
// (BeltState owns that math — this class owns pixels and input only).
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
    // Slot index of the item with this id, or -1. The host syncs with
    // setActiveIndex(indexOfItem(currentId)) when its notion of "current"
    // changes through some other control.
    int indexOfItem(const QString &id) const;
    edi::shell::BeltState beltState() const { return m_state; }

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

signals:
    void selected(const QString &id);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    bool event(QEvent *event) override; // tooltips for hovered cross cells

private:
    // Rect of a cell at a VIEW position (list index), not a grid coordinate:
    // the view compacts gaps away, so screen x/y are positions in the
    // vertical/horizontal strips.
    QRect viewCellRect(int xPosition, int yPosition) const;
    const edi::shell::BeltItem *itemAt(int row, int column) const;
    // The visible cell under `pos` as a grid target, or nullopt for dead space.
    std::optional<edi::shell::BeltState> stateForClick(const QPoint &pos) const;
    void rebuildOccupancy();
    void applyState(const edi::shell::BeltState &next, bool emitSelection);

    edi::shell::BeltState m_state;
    QVector<edi::shell::BeltItem> m_items;
    std::vector<bool> m_occupied; // row-major mask derived from m_items
    QPoint m_wheelRemainder;      // sub-notch wheel deltas, per axis
};
