#include "widgets/FloatingPalette.h"

#include <QHBoxLayout>
#include <QMouseEvent>

#include "widgets/ShellPanels.h"
#include "widgets/ShellTheme.h"

using namespace edi::shell;

FloatingPalette::FloatingPalette(const QString &paletteId, const QString &title, QWidget *content, QWidget *parent)
    : QWidget(parent)
    , m_paletteId(paletteId)
{
    setObjectName(QStringLiteral("floatingPalette"));

    // View metrics from the ShellTheme defaults — see makeScrollablePanel:
    // these dims do not vary with the color inputs, so a default-constructed
    // ShellTheme is the single source of truth without threading a theme in.
    const ShellTheme dims;

    // Chromeless: no frame background or border — the content floats bare
    // over the canvas, and only the nub + cells are visible.
    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(dims.floatingPaletteSpacing);

    // The grab nub: a small handle beside the content, title as tooltip.
    // WA_StyledBackground because plain QWidgets ignore stylesheet
    // backgrounds without it — same lesson as the overlay grips.
    m_grip = new QWidget;
    m_grip->setObjectName(QStringLiteral("paletteGrip"));
    m_grip->setAttribute(Qt::WA_StyledBackground, true);
    m_grip->setCursor(Qt::SizeAllCursor);
    m_grip->setFixedSize(dims.floatingPaletteGripW, dims.floatingPaletteGripH);
    m_grip->setToolTip(title);
    layout->addWidget(m_grip, 0, Qt::AlignVCenter);

    content->setParent(this);
    layout->addWidget(content);

    // The palette is exactly as big as its nub + content; free space in
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
