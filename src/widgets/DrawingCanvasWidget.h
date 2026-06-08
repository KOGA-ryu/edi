#pragma once

#include <QString>
#include <QWidget>

class DrawingDocumentController;

class DrawingCanvasWidget : public QWidget {
    Q_OBJECT

public:
    explicit DrawingCanvasWidget(DrawingDocumentController *controller, QWidget *parent = nullptr);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private slots:
    void refresh();

private:
    QRectF boardRect() const;
    QPointF canvasToScreen(double x, double y) const;
    QPointF screenToCanvas(const QPointF &point) const;
    QVariantMap selectedObjectProjection() const;
    QString hitSelectedHandle(const QPointF &screenPoint) const;
    void drawObject(QPainter &painter, const QVariantMap &object) const;
    void drawSelectedHandles(QPainter &painter, const QVariantMap &object) const;

    DrawingDocumentController *m_controller = nullptr;
    QString m_dragHandleId;
    bool m_dragObjectActive = false;
    QPointF m_lastDragCanvasPoint;
};
