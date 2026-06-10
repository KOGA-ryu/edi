#include "widgets/BeltCrossWidget.h"

#include <QHelpEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QToolTip>
#include <QWheelEvent>

#include <algorithm>
#include <cstddef>

using namespace edi::shell;

namespace {

// Mechanics-grade metrics; the look (colors, sizes) is the owner's pass.
constexpr int kCellSize = 34;
constexpr int kCellGap = 4;
// The neighbour rows peek in at half a cell (user feedback: ".5 of the
// vertical choices showing") — enough to read that something is there,
// small enough to keep the carousel one row tall.
constexpr int kPeekSize = kCellSize / 2;
// One physical wheel notch. Trackpads deliver many small angleDelta events;
// stepping per EVENT made the cross fly (user feedback) — accumulate and
// step per full notch instead.
constexpr int kWheelNotch = 120;

} // namespace

BeltCrossWidget::BeltCrossWidget(QWidget *parent)
    : QWidget(parent)
{
    setObjectName(QStringLiteral("beltCross"));
}

void BeltCrossWidget::setItems(const QVector<BeltItem> &items)
{
    m_items = items;
    rebuildOccupancy();
    m_state = beltNormalizeToOccupied(m_state, m_occupied);
    updateGeometry(); // strip lengths derive from occupancy
    update();
}

void BeltCrossWidget::setGridSize(int rows, int columns)
{
    m_state.rows = rows;
    m_state.columns = columns;
    if (!beltStateValid(m_state)) {
        // The previous active cell fell off the resized grid; restart at the
        // origin rather than carry an index no item list can resolve.
        m_state.activeRow = 0;
        m_state.activeColumn = 0;
    }
    rebuildOccupancy();
    m_state = beltNormalizeToOccupied(m_state, m_occupied);
    updateGeometry();
    update();
}

void BeltCrossWidget::setActiveIndex(int index)
{
    if (m_state.columns <= 0) {
        return;
    }
    const int row = index / m_state.columns;
    const int column = index % m_state.columns;
    // No selected() here: this is the host syncing us, not the user picking.
    applyState(beltJumpTo(m_state, row, column), false);
}

int BeltCrossWidget::activeIndex() const
{
    return beltActiveIndex(m_state);
}

QString BeltCrossWidget::activeItemId() const
{
    const int index = beltActiveIndex(m_state);
    return index >= 0 && index < m_items.size() ? m_items.at(index).id : QString();
}

int BeltCrossWidget::indexOfItem(const QString &id) const
{
    if (id.isEmpty()) {
        return -1; // empty slots are interchangeable, never "found"
    }
    for (int i = 0; i < m_items.size(); ++i) {
        if (m_items.at(i).id == id) {
            return i;
        }
    }
    return -1;
}

QRect BeltCrossWidget::activeCellRect(int position) const
{
    return QRect(position * (kCellSize + kCellGap), kPeekSize + kCellGap, kCellSize, kCellSize);
}

int BeltCrossWidget::activeItemPosition() const
{
    const BeltPeekView view = beltPeekView(m_state, m_occupied);
    for (std::size_t position = 0; position < view.items.size(); ++position) {
        if (view.items[position].active) {
            return static_cast<int>(position);
        }
    }
    return 0;
}

QRect BeltCrossWidget::topPeekRect() const
{
    return QRect(activeItemPosition() * (kCellSize + kCellGap), 0, kCellSize, kPeekSize);
}

QRect BeltCrossWidget::bottomPeekRect() const
{
    return QRect(activeItemPosition() * (kCellSize + kCellGap),
                 kPeekSize + kCellGap + kCellSize + kCellGap, kCellSize, kPeekSize);
}

const BeltItem *BeltCrossWidget::itemAt(int row, int column) const
{
    const int index = row * m_state.columns + column;
    return index >= 0 && index < m_items.size() ? &m_items.at(index) : nullptr;
}

std::optional<BeltState> BeltCrossWidget::stateForClick(const QPoint &pos) const
{
    const BeltPeekView view = beltPeekView(m_state, m_occupied);
    for (std::size_t position = 0; position < view.items.size(); ++position) {
        if (activeCellRect(static_cast<int>(position)).contains(pos)) {
            return beltJumpTo(m_state, m_state.activeRow, view.items[position].column);
        }
    }
    // A peek is one vertical step: clicking it goes where scrolling would.
    if (view.previousRow >= 0 && topPeekRect().contains(pos)) {
        return beltJumpTo(m_state, view.previousRow, view.previousLeadColumn);
    }
    if (view.nextRow >= 0 && bottomPeekRect().contains(pos)) {
        return beltJumpTo(m_state, view.nextRow, view.nextLeadColumn);
    }
    return std::nullopt;
}

void BeltCrossWidget::rebuildOccupancy()
{
    const std::size_t slotCount = m_state.rows > 0 && m_state.columns > 0
        ? static_cast<std::size_t>(m_state.rows) * static_cast<std::size_t>(m_state.columns)
        : 0;
    m_occupied.assign(slotCount, false);
    const std::size_t known = std::min(slotCount, static_cast<std::size_t>(m_items.size()));
    for (std::size_t i = 0; i < known; ++i) {
        m_occupied[i] = !m_items.at(static_cast<int>(i)).id.isEmpty();
    }
}

void BeltCrossWidget::applyState(const BeltState &next, bool emitSelection)
{
    if (next.activeRow == m_state.activeRow && next.activeColumn == m_state.activeColumn
        && next.rows == m_state.rows && next.columns == m_state.columns) {
        return;
    }
    m_state = next;
    update();
    if (emitSelection) {
        const QString id = activeItemId();
        if (!id.isEmpty()) {
            emit selected(id);
        }
    }
}

void BeltCrossWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, false);

    const QPalette colors = palette();
    const BeltPeekView view = beltPeekView(m_state, m_occupied);

    auto paintCell = [&](const QRect &rect, const BeltItem *item, bool active) {
        painter.fillRect(rect, colors.color(active ? QPalette::Highlight : QPalette::Base));
        painter.setPen(colors.color(QPalette::Mid));
        painter.drawRect(rect.adjusted(0, 0, -1, -1));
        if (item != nullptr && !item->glyph.isEmpty()) {
            painter.setPen(colors.color(active ? QPalette::HighlightedText : QPalette::Text));
            painter.drawText(rect, Qt::AlignCenter, item->glyph);
        }
    };

    // The active tool's sub-features, full-size.
    for (std::size_t position = 0; position < view.items.size(); ++position) {
        const BeltItemEntry &entry = view.items[position];
        paintCell(activeCellRect(static_cast<int>(position)),
                  itemAt(m_state.activeRow, entry.column), entry.active);
    }
    // The neighbour peeks: a full cell drawn under a half-cell clip, so the
    // glyph reads as cut off mid-cell — visibly a continuation, not a button.
    if (view.previousRow >= 0) {
        const QRect band = topPeekRect();
        painter.save();
        painter.setClipRect(band);
        paintCell(band.adjusted(0, -(kCellSize - kPeekSize), 0, 0),
                  itemAt(view.previousRow, view.previousLeadColumn), false);
        painter.restore();
    }
    if (view.nextRow >= 0) {
        const QRect band = bottomPeekRect();
        painter.save();
        painter.setClipRect(band);
        paintCell(QRect(band.x(), band.y(), kCellSize, kCellSize),
                  itemAt(view.nextRow, view.nextLeadColumn), false);
        painter.restore();
    }
}

void BeltCrossWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton) {
        QWidget::mousePressEvent(event);
        return;
    }
    if (const std::optional<BeltState> target = stateForClick(event->pos())) {
        applyState(*target, true);
        event->accept();
        return;
    }
    // Clicks in dead space fall through: only visible cells are clickable,
    // exactly as only occupied cells are painted.
    QWidget::mousePressEvent(event);
}

void BeltCrossWidget::wheelEvent(QWheelEvent *event)
{
    const QPoint delta = event->angleDelta();
    if (delta.isNull()) {
        QWidget::wheelEvent(event);
        return;
    }
    // Dominant axis decides, and switching axes clears the other axis's
    // remainder — a diagonal flick must not bank steps for later.
    if (qAbs(delta.y()) >= qAbs(delta.x())) {
        m_wheelRemainder.setX(0);
        m_wheelRemainder.setY(m_wheelRemainder.y() + delta.y());
        const int notches = m_wheelRemainder.y() / kWheelNotch;
        m_wheelRemainder.setY(m_wheelRemainder.y() % kWheelNotch);
        if (notches != 0) {
            // Scroll down (negative delta) -> next tool down the strip.
            applyState(beltStepRowOccupied(m_state, -notches, m_occupied), true);
        }
    } else {
        m_wheelRemainder.setY(0);
        m_wheelRemainder.setX(m_wheelRemainder.x() + delta.x());
        const int notches = m_wheelRemainder.x() / kWheelNotch;
        m_wheelRemainder.setX(m_wheelRemainder.x() % kWheelNotch);
        if (notches != 0) {
            // Scroll right (negative delta) -> next sub-feature along the row.
            applyState(beltStepColumnOccupied(m_state, -notches, m_occupied), true);
        }
    }
    event->accept();
}

bool BeltCrossWidget::event(QEvent *event)
{
    if (event->type() == QEvent::ToolTip) {
        auto *helpEvent = static_cast<QHelpEvent *>(event);
        if (const std::optional<BeltState> target = stateForClick(helpEvent->pos())) {
            const BeltItem *item = itemAt(target->activeRow, target->activeColumn);
            if (item != nullptr && !item->tooltip.isEmpty()) {
                QToolTip::showText(helpEvent->globalPos(), item->tooltip, this);
                return true;
            }
        }
        QToolTip::hideText();
        return true;
    }
    return QWidget::event(event);
}

QSize BeltCrossWidget::sizeHint() const
{
    // Width fits the longest tool's sub-feature strip — stable across row
    // changes so the carousel never jumps while scrolling. Height is fixed:
    // half-cell peek, active row, half-cell peek.
    int widestRow = 1;
    for (int row = 0; row < m_state.rows; ++row) {
        int count = 0;
        for (int column = 0; column < m_state.columns; ++column) {
            if (beltCellOccupied(m_state, m_occupied, row, column)) {
                ++count;
            }
        }
        widestRow = std::max(widestRow, count);
    }
    return QSize(widestRow * kCellSize + (widestRow - 1) * kCellGap,
                 kPeekSize * 2 + kCellSize + 2 * kCellGap);
}

QSize BeltCrossWidget::minimumSizeHint() const
{
    return sizeHint();
}
