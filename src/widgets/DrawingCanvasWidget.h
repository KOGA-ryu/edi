#pragma once

#include <QWidget>

class DrawingDocumentController;

class DrawingCanvasWidget : public QWidget {
    Q_OBJECT

public:
    explicit DrawingCanvasWidget(DrawingDocumentController *controller, QWidget *parent = nullptr);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;

private slots:
    void refresh();

private:
    QRectF boardRect() const;
    QPointF canvasToScreen(double x, double y) const;
    QPointF screenToCanvas(const QPointF &point) const;
    void drawObject(QPainter &painter, const QVariantMap &object) const;

    DrawingDocumentController *m_controller = nullptr;
};
