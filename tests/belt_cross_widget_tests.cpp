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

// Mirrors the widget's carousel metrics: 34px cells, 4px gaps, 17px peeks,
// and a 14px nub gutter on the left. With no pinned rows the active row
// band starts at y=21; peeks sit above (y 0..16) and below (y 59..75),
// horizontally aligned over the active cell. `pins` shifts everything down
// by one cell row per frozen bar.
QPointF activeCellCenter(int position, int pins = 0)
{
    return QPointF(14.0 + position * 38.0 + 17.0, pins * 38.0 + 21.0 + 17.0);
}

QPointF peekCenter(int activePosition, bool top, int pins = 0)
{
    return QPointF(14.0 + activePosition * 38.0 + 17.0, pins * 38.0 + (top ? 8.0 : 67.0));
}

QPointF pinnedCellCenter(int pinPosition, int itemPosition)
{
    return QPointF(14.0 + itemPosition * 38.0 + 17.0, pinPosition * 38.0 + 17.0);
}

QPointF pinNubCenter(int pins)
{
    return QPointF(5.0, pins * 38.0 + 21.0 + 17.0); // gutter, centred on the live row
}

QPointF killNubCenter(int pinPosition)
{
    return QPointF(5.0, pinPosition * 38.0 + 17.0);
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

    // Footprint: nub gutter + widest row (3 items) wide; carousel height of
    // peek + active row + peek, plus the name line (4px gap + one font line),
    // while nothing is pinned.
    assert(belt.sizeHint()
           == QSize(14 + 3 * 34 + 2 * 4,
                    17 * 2 + 34 + 2 * 4 + 4 + belt.fontMetrics().height()));

    // Initial: origin cell occupied, nothing emitted; the name line reads the
    // active cell's tooltip (the fixture's tooltip == id).
    assert(belt.activeItemId() == QStringLiteral("a"));
    assert(belt.activeItemLabel() == QStringLiteral("a"));
    assert(emitted.isEmpty());

    // One full notch down: the empty row 1 is skipped, row 2's lead lands.
    sendWheel(belt, QPoint(0, -120));
    assert(belt.activeItemId() == QStringLiteral("b"));
    assert(belt.activeItemLabel() == QStringLiteral("b")); // the name line follows
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
        sendClick(belt, QPointF(14.0 + 17.0, 57.0)); // gap between row and bottom peek
        sendClick(belt, QPointF(14.0 + 2 * 38.0 + 17.0, 8.0)); // top band, not over active cell
        assert(belt.activeItemId() == QStringLiteral("b"));
        assert(emitted.size() == emittedBefore);
    }

    // Pinning: the + nub freezes the live row into a quick bar above the
    // carousel; selection is untouched and the widget grows by one row.
    {
        const int emittedBefore = emitted.size();
        assert(belt.pinnedRows().empty());
        belt.setActiveIndex(8); // b, row 2
        const QSize before = belt.sizeHint();
        sendClick(belt, pinNubCenter(0));
        assert(belt.pinnedRows() == std::vector<int>{2});
        assert(belt.activeItemId() == QStringLiteral("b")); // selection unchanged
        assert(emitted.size() == emittedBefore);
        assert(belt.sizeHint().height() == before.height() + 38);
        belt.resize(belt.sizeHint());

        // Pinning the same row again is a no-op.
        sendClick(belt, pinNubCenter(1));
        assert(belt.pinnedRows() == std::vector<int>{2});

        // Scroll the carousel away: the frozen copy of row 2 stays put.
        sendWheel(belt, QPoint(0, -120)); // -> row 3 (e)
        assert(belt.activeItemId() == QStringLiteral("e"));
        assert(belt.pinnedRows() == std::vector<int>{2});

        // Freeze row 3 too — "repeating the process": two quick bars now.
        sendClick(belt, pinNubCenter(1));
        assert((belt.pinnedRows() == std::vector<int>{2, 3}));
        belt.resize(belt.sizeHint());

        // A frozen cell is a live control: clicking row 2's second item (c)
        // selects it and the carousel teleports to that row.
        sendClick(belt, pinnedCellCenter(0, 1));
        assert(belt.activeItemId() == QStringLiteral("c"));
        assert(emitted.last() == QStringLiteral("c"));

        // The kill nub removes exactly its row; the other pin survives.
        sendClick(belt, killNubCenter(0)); // kill the row-2 bar
        assert(belt.pinnedRows() == std::vector<int>{3});
        assert(belt.activeItemId() == QStringLiteral("c")); // selection untouched
        sendClick(belt, killNubCenter(0)); // kill the remaining bar
        assert(belt.pinnedRows().empty());
        belt.resize(belt.sizeHint());
    }

    // Programmatic sync: state follows, no selected() echo.
    {
        const int emittedBefore = emitted.size();
        belt.setActiveIndex(9); // c
        assert(belt.activeItemId() == QStringLiteral("c"));
        assert(emitted.size() == emittedBefore);
    }

    // Shrinking the item list re-normalizes the cursor onto what remains —
    // and prunes pins whose rows went empty (no dangling quick bars).
    {
        sendClick(belt, pinNubCenter(0)); // pin the active row (2)
        assert(belt.pinnedRows() == std::vector<int>{2});
        QVector<BeltItem> tail(16);
        tail[12] = item("e");
        belt.setItems(tail); // row 2 is now empty
        assert(belt.activeItemId() == QStringLiteral("e"));
        assert(belt.pinnedRows().empty());
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
