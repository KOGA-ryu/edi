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

// Mirrors the widget's carousel metrics: 34px cells, 4px gaps, 17px peeks.
// Active row band starts at y=21; peeks sit above (y 0..16) and below
// (y 59..75), horizontally aligned over the active cell.
QPointF activeCellCenter(int position)
{
    return QPointF(position * 38.0 + 17.0, 21.0 + 17.0);
}

QPointF peekCenter(int activePosition, bool top)
{
    return QPointF(activePosition * 38.0 + 17.0, top ? 8.0 : 67.0);
}

BeltItem item(const char *id)
{
    return {QString::fromLatin1(id), QString::fromLatin1(id).toUpper(), QString::fromLatin1(id)};
}

} // namespace

int main(int argc, char **argv)
{
    qputenv("QT_QPA_PLATFORM", "offscreen");
    QApplication app(argc, argv);

    // A 4x4 belt shaped like the drafting arrangement: rows are tools, cells
    // their sub-features. Row 0: a. Row 1: empty. Row 2: b,c,d (with a stored
    // gap at column 2 — the view must compact it away). Row 3: e,f.
    BeltCrossWidget belt;
    belt.setGridSize(4, 4);
    QVector<BeltItem> items(16);
    items[0] = item("a");
    items[8] = item("b");
    items[9] = item("c");
    items[11] = item("d");
    items[12] = item("e");
    items[13] = item("f");
    belt.setItems(items);
    belt.resize(belt.sizeHint());

    QStringList emitted;
    QObject::connect(&belt, &BeltCrossWidget::selected, [&emitted](const QString &id) {
        emitted.push_back(id);
    });

    // Footprint: widest row (3 items) wide; fixed carousel height of
    // peek + active row + peek.
    assert(belt.sizeHint() == QSize(3 * 34 + 2 * 4, 17 * 2 + 34 + 2 * 4));

    // Initial: origin cell occupied, nothing emitted.
    assert(belt.activeItemId() == QStringLiteral("a"));
    assert(emitted.isEmpty());

    // One full notch down: the empty row 1 is skipped, row 2's lead lands.
    sendWheel(belt, QPoint(0, -120));
    assert(belt.activeItemId() == QStringLiteral("b"));
    assert(emitted == QStringList{QStringLiteral("b")});

    // Sub-notch deltas accumulate instead of stepping per event (the
    // scroll-speed feedback): two 60s = one step, not two.
    sendWheel(belt, QPoint(0, -60));
    assert(belt.activeItemId() == QStringLiteral("b"));
    sendWheel(belt, QPoint(0, -60));
    assert(belt.activeItemId() == QStringLiteral("e"));
    assert(emitted.size() == 2);

    // Down again wraps to the top tool.
    sendWheel(belt, QPoint(0, -120));
    assert(belt.activeItemId() == QStringLiteral("a"));

    // Switching axes clears the other axis's remainder: bank 60 vertical,
    // flick horizontal, then 60 vertical again — no vertical step fires.
    sendWheel(belt, QPoint(0, -60));
    sendWheel(belt, QPoint(-120, 0)); // row 0 has one item: column step is a no-op
    assert(belt.activeItemId() == QStringLiteral("a"));
    sendWheel(belt, QPoint(0, -60));
    assert(belt.activeItemId() == QStringLiteral("a"));
    assert(emitted.size() == 3);
    sendWheel(belt, QPoint(0, -60)); // completes a fresh notch
    assert(belt.activeItemId() == QStringLiteral("b"));

    // Horizontal walk skips the stored gap and wraps over the ragged end.
    sendWheel(belt, QPoint(-120, 0));
    assert(belt.activeItemId() == QStringLiteral("c"));
    sendWheel(belt, QPoint(-120, 0));
    assert(belt.activeItemId() == QStringLiteral("d")); // gap at column 2 skipped
    sendWheel(belt, QPoint(-120, 0));
    assert(belt.activeItemId() == QStringLiteral("b")); // wrapped
    sendWheel(belt, QPoint(120, 0));
    assert(belt.activeItemId() == QStringLiteral("d"));

    // Click the bottom peek: one vertical step down. From row 2 (b active,
    // position 0) the next non-empty row is 3; its lead e lands.
    belt.setActiveIndex(8); // b
    sendClick(belt, peekCenter(0, false));
    assert(belt.activeItemId() == QStringLiteral("e"));
    assert(emitted.last() == QStringLiteral("e"));

    // Click an active-row cell: row 3 lays e,f left-to-right; cell 1 is f.
    sendClick(belt, activeCellCenter(1));
    assert(belt.activeItemId() == QStringLiteral("f"));

    // The peeks follow the active cell horizontally: with f active
    // (position 1), the top peek sits over position 1 and steps up to row
    // 2's lead.
    sendClick(belt, peekCenter(1, true));
    assert(belt.activeItemId() == QStringLiteral("b"));

    // Dead space (the gap band between the active row and a peek, and the
    // peek band away from the active column) changes nothing.
    {
        const int emittedBefore = emitted.size();
        sendClick(belt, QPointF(17.0, 57.0)); // gap between row and bottom peek
        sendClick(belt, QPointF(2 * 38.0 + 17.0, 8.0)); // top band, not over active cell
        assert(belt.activeItemId() == QStringLiteral("b"));
        assert(emitted.size() == emittedBefore);
    }

    // Programmatic sync: state follows, no selected() echo.
    {
        const int emittedBefore = emitted.size();
        belt.setActiveIndex(9); // c
        assert(belt.activeItemId() == QStringLiteral("c"));
        assert(emitted.size() == emittedBefore);
    }

    // Shrinking the item list re-normalizes the cursor onto what remains.
    {
        QVector<BeltItem> tail(16);
        tail[12] = item("e");
        belt.setItems(tail);
        assert(belt.activeItemId() == QStringLiteral("e"));
    }

    // An all-empty belt renders nothing and emits nothing.
    {
        const int emittedBefore = emitted.size();
        belt.setItems(QVector<BeltItem>(16));
        sendWheel(belt, QPoint(0, -120));
        sendClick(belt, activeCellCenter(0));
        assert(belt.activeItemId().isEmpty());
        assert(emitted.size() == emittedBefore);
    }

    return 0;
}
