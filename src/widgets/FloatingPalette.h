#pragma once

#include <QPoint>
#include <QString>
#include <QWidget>

// F4: a floating palette — a movable mini-panel over the main area. The
// frame is chromeless (user feedback: no background, just the content over
// the canvas); a small grab nub on the left is the drag handle, with the
// palette title as its tooltip. The content arrives from a feature; the
// frame knows nothing about it. Dragging moves the frame within its parent
// (clamped by the pure placement math) and reports the final spot, which
// the window persists into the workspace TOML.
class FloatingPalette final : public QWidget {
    Q_OBJECT

public:
    FloatingPalette(const QString &paletteId, const QString &title, QWidget *content, QWidget *parent);

    QString paletteId() const { return m_paletteId; }
    // Clamp-and-move against the current parent size; the one path every
    // position change takes (restore, drag, host resize).
    void applyPlacement(int x, int y);

signals:
    // Fired when a drag ends (not per move event — the TOML cares where the
    // palette settled, not about the journey).
    void placementChanged(const QString &paletteId, int x, int y);

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    bool inDragStrip(const QPoint &pos) const;

    QString m_paletteId;
    QWidget *m_grip = nullptr;
    bool m_dragging = false;
    QPoint m_dragOffset; // press position within the palette
};
