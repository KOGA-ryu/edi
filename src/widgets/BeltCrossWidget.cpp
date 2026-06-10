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

QRect BeltCrossWidget::viewCellRect(int xPosition, int yPosition) const
{
    return QRect(xPosition * (kCellSize + kCellGap), yPosition * (kCellSize + kCellGap), kCellSize, kCellSize);
}

const BeltItem *BeltCrossWidget::itemAt(int row, int column) const
{
    const int index = row * m_state.columns + column;
    return index >= 0 && index < m_items.size() ? &m_items.at(index) : nullptr;
}

std::optional<BeltState> BeltCrossWidget::stateForClick(const QPoint &pos) const
{
    const BeltCrossView view = beltCrossView(m_state, m_occupied);
    // Horizontal strip first: it owns the whole active row's line, including
    // the intersection cell it shares with the vertical strip (same target).
    for (std::size_t position = 0; position < view.items.size(); ++position) {
        if (viewCellRect(static_cast<int>(position), view.activeRowPosition).contains(pos)) {
            return beltJumpTo(m_state, m_state.activeRow, view.items[position].column);
        }
    }
    for (std::size_t position = 0; position < view.rows.size(); ++position) {
        if (viewCellRect(0, static_cast<int>(position)).contains(pos)) {
            return beltJumpTo(m_state, view.rows[position].row, view.rows[position].leadColumn);
        }
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
    const BeltCrossView view = beltCrossView(m_state, m_occupied);

    auto paintCell = [&](const QRect &rect, const BeltItem *item, bool active) {
        painter.fillRect(rect, colors.color(active ? QPalette::Highlight : QPalette::Base));
        painter.setPen(colors.color(QPalette::Mid));
        painter.drawRect(rect.adjusted(0, 0, -1, -1));
        if (item != nullptr && !item->glyph.isEmpty()) {
            painter.setPen(colors.color(active ? QPalette::HighlightedText : QPalette::Text));
            painter.drawText(rect, Qt::AlignCenter, item->glyph);
        }
    };

    // Vertical strip: every tool (non-empty row) by its lead item. The
    // active row's line is painted by the horizontal strip below, which owns
    // the shared intersection cell.
    for (std::size_t position = 0; position < view.rows.size(); ++position) {
        if (static_cast<int>(position) == view.activeRowPosition) {
            continue;
        }
        const BeltRowEntry &entry = view.rows[position];
        paintCell(viewCellRect(0, static_cast<int>(position)), itemAt(entry.row, entry.leadColumn), false);
    }
    // Horizontal strip: the active tool's sub-features, compacted.
    for (std::size_t position = 0; position < view.items.size(); ++position) {
        const BeltItemEntry &entry = view.items[position];
        paintCell(viewCellRect(static_cast<int>(position), view.activeRowPosition),
                  itemAt(m_state.activeRow, entry.column), entry.active);
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
    // Footprint from occupancy, not the raw grid: width fits the longest
    // tool's sub-feature strip, height fits one cell per tool. Stable across
    // active-row changes so the layout never jumps while scrolling.
    int stripRows = 0;
    int widestRow = 1;
    for (int row = 0; row < m_state.rows; ++row) {
        int count = 0;
        for (int column = 0; column < m_state.columns; ++column) {
            if (beltCellOccupied(m_state, m_occupied, row, column)) {
                ++count;
            }
        }
        if (count > 0) {
            ++stripRows;
            widestRow = std::max(widestRow, count);
        }
    }
    stripRows = std::max(stripRows, 1);
    return QSize(widestRow * kCellSize + (widestRow - 1) * kCellGap,
                 stripRows * kCellSize + (stripRows - 1) * kCellGap);
}

QSize BeltCrossWidget::minimumSizeHint() const
{
    return sizeHint();
}
