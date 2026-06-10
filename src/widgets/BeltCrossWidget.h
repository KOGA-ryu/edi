#pragma once

#include <QString>
#include <QVector>
#include <QWidget>

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

// The game-style "weapon cross" over a belt grid: only the active row
// (horizontal arm) and active column (vertical arm) render, intersecting at
// the live cell. Vertical scroll changes row, horizontal scroll slides along
// the row, clicking any visible cell jumps to it; everything wraps at the
// edges (BeltState owns that math — this class owns pixels and input only).
class BeltCrossWidget final : public QWidget {
    Q_OBJECT

public:
    explicit BeltCrossWidget(QWidget *parent = nullptr);

    // Items fill the grid row-major (slot i = items[i]); missing tail slots
    // render empty and never emit. Resets the active cell if now invalid.
    void setItems(const QVector<edi::shell::BeltItem> &items);
    void setGridSize(int rows, int columns);
    // Programmatic sync (the host's notion of "current" changed elsewhere).
    // Deliberately does NOT emit selected() — echoing a sync back as a user
    // pick is how host<->widget update loops start.
    void setActiveIndex(int index);

    int activeIndex() const;
    QString activeItemId() const;
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
    QRect cellRect(int row, int column) const;
    const edi::shell::BeltItem *itemAt(int row, int column) const;
    void applyState(const edi::shell::BeltState &next, bool emitSelection);

    edi::shell::BeltState m_state;
    QVector<edi::shell::BeltItem> m_items;
};
