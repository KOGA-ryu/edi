#include "widgets/BeltCrossWidget.h"

#include <QHelpEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QToolTip>
#include <QWheelEvent>

using namespace edi::shell;

namespace {

// Mechanics-grade metrics; the look (colors, sizes) is the owner's pass.
constexpr int kCellSize = 34;
constexpr int kCellGap = 4;

} // namespace

BeltCrossWidget::BeltCrossWidget(QWidget *parent)
    : QWidget(parent)
{
    setObjectName(QStringLiteral("beltCross"));
}

void BeltCrossWidget::setItems(const QVector<BeltItem> &items)
{
    m_items = items;
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

QRect BeltCrossWidget::cellRect(int row, int column) const
{
    return QRect(column * (kCellSize + kCellGap), row * (kCellSize + kCellGap), kCellSize, kCellSize);
}

const BeltItem *BeltCrossWidget::itemAt(int row, int column) const
{
    const int index = row * m_state.columns + column;
    return index >= 0 && index < m_items.size() ? &m_items.at(index) : nullptr;
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
            // Empty slots move the cross but select nothing — landing on a
            // blank cell must not fire a phantom tool change.
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
    for (const BeltCrossCell &cell : beltCrossCells(m_state)) {
        const QRect rect = cellRect(cell.row, cell.column);
        const BeltItem *item = itemAt(cell.row, cell.column);

        painter.fillRect(rect, colors.color(cell.active ? QPalette::Highlight : QPalette::Base));
        painter.setPen(colors.color(QPalette::Mid));
        painter.drawRect(rect.adjusted(0, 0, -1, -1));

        if (item != nullptr && !item->glyph.isEmpty()) {
            painter.setPen(colors.color(cell.active ? QPalette::HighlightedText : QPalette::Text));
            painter.drawText(rect, Qt::AlignCenter, item->glyph);
        }
    }
}

void BeltCrossWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton) {
        QWidget::mousePressEvent(event);
        return;
    }
    for (const BeltCrossCell &cell : beltCrossCells(m_state)) {
        if (cellRect(cell.row, cell.column).contains(event->pos())) {
            applyState(beltJumpTo(m_state, cell.row, cell.column), true);
            event->accept();
            return;
        }
    }
    // Clicks in the dead space between arms fall through: only visible cells
    // are clickable, exactly as only visible cells are painted.
    QWidget::mousePressEvent(event);
}

void BeltCrossWidget::wheelEvent(QWheelEvent *event)
{
    const QPoint delta = event->angleDelta();
    // Dominant axis decides: trackpads report both, and treating a diagonal
    // flick as two moves at once makes the cross feel like it slips.
    if (qAbs(delta.y()) >= qAbs(delta.x()) && delta.y() != 0) {
        // Scroll down -> next row (down the grid), wrap at the edges.
        applyState(beltStepRow(m_state, delta.y() < 0 ? 1 : -1), true);
        event->accept();
    } else if (delta.x() != 0) {
        // Scroll right -> next column along the row.
        applyState(beltStepColumn(m_state, delta.x() < 0 ? 1 : -1), true);
        event->accept();
    } else {
        QWidget::wheelEvent(event);
    }
}

bool BeltCrossWidget::event(QEvent *event)
{
    if (event->type() == QEvent::ToolTip) {
        auto *helpEvent = static_cast<QHelpEvent *>(event);
        for (const BeltCrossCell &cell : beltCrossCells(m_state)) {
            if (!cellRect(cell.row, cell.column).contains(helpEvent->pos())) {
                continue;
            }
            const BeltItem *item = itemAt(cell.row, cell.column);
            if (item != nullptr && !item->tooltip.isEmpty()) {
                QToolTip::showText(helpEvent->globalPos(), item->tooltip, this);
            } else {
                QToolTip::hideText();
            }
            return true;
        }
        QToolTip::hideText();
        return true;
    }
    return QWidget::event(event);
}

QSize BeltCrossWidget::sizeHint() const
{
    // The cross can sit on any row/column, so the widget claims the full
    // grid footprint; arms then move inside a stable bounding box.
    return QSize(m_state.columns * kCellSize + (m_state.columns - 1) * kCellGap,
                 m_state.rows * kCellSize + (m_state.rows - 1) * kCellGap);
}

QSize BeltCrossWidget::minimumSizeHint() const
{
    return sizeHint();
}
