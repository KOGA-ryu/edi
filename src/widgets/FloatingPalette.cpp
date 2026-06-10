#include "widgets/FloatingPalette.h"

#include <QLabel>
#include <QMouseEvent>
#include <QVBoxLayout>

#include "widgets/ShellPanels.h"

using namespace edi::shell;

FloatingPalette::FloatingPalette(const QString &paletteId, const QString &title, QWidget *content, QWidget *parent)
    : QWidget(parent)
    , m_paletteId(paletteId)
{
    setObjectName(QStringLiteral("floatingPalette"));
    // Plain QWidgets ignore stylesheet backgrounds without this — same
    // lesson as the overlay grips.
    setAttribute(Qt::WA_StyledBackground, true);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(1, 1, 1, 1); // the border is QSS; keep content off it
    layout->setSpacing(0);

    m_grip = new QLabel(title);
    m_grip->setObjectName(QStringLiteral("paletteGrip"));
    m_grip->setCursor(Qt::SizeAllCursor);
    m_grip->setAlignment(Qt::AlignCenter);
    layout->addWidget(m_grip);

    content->setParent(this);
    layout->addWidget(content);

    // The palette is exactly as big as its strip + content; free space in
    // the main area belongs to the canvas, not to palette chrome.
    layout->setSizeConstraint(QLayout::SetFixedSize);
}

void FloatingPalette::applyPlacement(int x, int y)
{
    if (parentWidget() == nullptr) {
        return;
    }
    const PalettePlacement clamped = clampPalettePlacement(
        {m_paletteId, x, y},
        parentWidget()->width(), parentWidget()->height(),
        width(), height());
    move(clamped.x, clamped.y);
}

bool FloatingPalette::inDragStrip(const QPoint &pos) const
{
    return m_grip != nullptr && m_grip->geometry().contains(pos);
}

void FloatingPalette::mousePressEvent(QMouseEvent *event)
{
    raise(); // any grab brings the palette over its siblings
    if (event->button() == Qt::LeftButton && inDragStrip(event->pos())) {
        m_dragging = true;
        m_dragOffset = event->pos();
        event->accept();
        return;
    }
    QWidget::mousePressEvent(event);
}

void FloatingPalette::mouseMoveEvent(QMouseEvent *event)
{
    if (!m_dragging) {
        QWidget::mouseMoveEvent(event);
        return;
    }
    // Parent-relative target: where the palette's origin lands if it follows
    // the cursor while keeping the grab point under the finger.
    const QPoint target = mapToParent(event->pos()) - m_dragOffset;
    applyPlacement(target.x(), target.y());
    event->accept();
}

void FloatingPalette::mouseReleaseEvent(QMouseEvent *event)
{
    if (m_dragging && event->button() == Qt::LeftButton) {
        m_dragging = false;
        emit placementChanged(m_paletteId, x(), y());
        event->accept();
        return;
    }
    QWidget::mouseReleaseEvent(event);
}
