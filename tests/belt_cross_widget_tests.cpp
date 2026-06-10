#include "widgets/BeltCrossWidget.h"

#include <QApplication>
#include <QMouseEvent>
#include <QPoint>
#include <QString>
#include <QStringList>
#include <QVector>
#include <QWheelEvent>

#include <cassert>

using edi::shell::BeltItem;

namespace {

void sendWheel(QWidget &widget, const QPoint &angleDelta)
{
    const QPointF pos(1.0, 1.0);
    QWheelEvent event(pos, widget.mapToGlobal(pos), QPoint(0, 0), angleDelta,
                      Qt::NoButton, Qt::NoModifier, Qt::NoScrollPhase, false);
    QApplication::sendEvent(&widget, &event);
}

void sendClick(QWidget &widget, const QPointF &pos)
{
    QMouseEvent press(QEvent::MouseButtonPress, pos, widget.mapToGlobal(pos),
                      Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
    QApplication::sendEvent(&widget, &press);
}

QVector<BeltItem> numberedItems(int count)
{
    QVector<BeltItem> items;
    for (int i = 0; i < count; ++i) {
        items.push_back({QStringLiteral("tool_%1").arg(i),
                         QString::number(i),
                         QStringLiteral("Tool %1").arg(i)});
    }
    return items;
}

} // namespace

int main(int argc, char **argv)
{
    qputenv("QT_QPA_PLATFORM", "offscreen");
    QApplication app(argc, argv);

    BeltCrossWidget belt;
    belt.setGridSize(6, 6);
    belt.setItems(numberedItems(36));
    belt.resize(belt.sizeHint());

    QStringList emitted;
    QObject::connect(&belt, &BeltCrossWidget::selected, [&emitted](const QString &id) {
        emitted.push_back(id);
    });

    // Initial state: origin cell, item 0 active, nothing emitted.
    assert(belt.activeIndex() == 0);
    assert(belt.activeItemId() == QStringLiteral("tool_0"));
    assert(emitted.isEmpty());

    // Vertical scroll down -> next row; the user's pick is emitted.
    sendWheel(belt, QPoint(0, -120));
    assert(belt.activeIndex() == 6);
    assert(emitted == QStringList{QStringLiteral("tool_6")});

    // Vertical scroll up from row 1 -> row 0; up again wraps to the last row.
    sendWheel(belt, QPoint(0, 120));
    assert(belt.activeIndex() == 0);
    sendWheel(belt, QPoint(0, 120));
    assert(belt.activeIndex() == 30); // row 5, column 0
    assert(emitted.size() == 3);

    // Horizontal scroll right -> next column; left wraps at the row start.
    sendWheel(belt, QPoint(-120, 0));
    assert(belt.activeIndex() == 31);
    sendWheel(belt, QPoint(120, 0));
    sendWheel(belt, QPoint(120, 0));
    assert(belt.activeIndex() == 35); // wrapped to the row's last column
    assert(emitted.size() == 6);

    // Diagonal flicks resolve to the dominant axis (here: vertical).
    sendWheel(belt, QPoint(40, -120));
    assert(belt.activeIndex() == 5); // row wrapped 5 -> 0, column 5 kept
    assert(emitted.size() == 7);

    // Click a visible cell of the vertical arm: jump straight to it.
    // Active is (0,5); cell (3,5) sits on the active column arm.
    {
        const QRect target(5 * 38, 3 * 38, 34, 34); // mirrors cellRect metrics
        sendClick(belt, target.center());
        assert(belt.activeIndex() == 3 * 6 + 5);
        assert(emitted.last() == QStringLiteral("tool_23"));
    }

    // Click in dead space (off both arms): nothing changes, nothing emitted.
    {
        const int before = belt.activeIndex();
        const int emittedBefore = emitted.size();
        sendClick(belt, QPointF(1.0, 1.0)); // cell (0,0) is not on the cross now
        assert(belt.activeIndex() == before);
        assert(emitted.size() == emittedBefore);
    }

    // Programmatic sync: state follows, but no selected() echo.
    {
        const int emittedBefore = emitted.size();
        belt.setActiveIndex(14);
        assert(belt.activeIndex() == 14);
        assert(belt.activeItemId() == QStringLiteral("tool_14"));
        assert(emitted.size() == emittedBefore);
    }

    // Empty slots: a short item list leaves tail cells blank — moving onto
    // one repositions the cross but emits no phantom selection.
    {
        belt.setItems(numberedItems(3));
        belt.setActiveIndex(2);
        const int emittedBefore = emitted.size();
        sendWheel(belt, QPoint(-120, 0)); // -> index 3, an empty slot
        assert(belt.activeIndex() == 3);
        assert(belt.activeItemId().isEmpty());
        assert(emitted.size() == emittedBefore);
    }

    // Grid resize that strands the active cell resets it to the origin.
    {
        belt.setGridSize(2, 2);
        belt.setActiveIndex(3);
        assert(belt.activeIndex() == 3); // (1,1) fits a 2x2 grid
        belt.setGridSize(1, 2);
        assert(belt.beltState().activeRow == 0 && belt.beltState().activeColumn == 0);
    }

    return 0;
}
