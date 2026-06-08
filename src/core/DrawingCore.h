#pragma once

#include <QObject>
#include <QString>
#include <QVariantList>
#include <QVariantMap>
#include <QVector>

class DrawingDocumentController final : public QObject {
    Q_OBJECT

public:
    explicit DrawingDocumentController(QObject *parent = nullptr);

    QVariantMap modelDocument() const;
    QString selectedToolId() const;
    QString selectedObjectId() const;
    void clickCanvasNormalized(double x, double y);

signals:
    void modelChanged();

private:
    QVariantMap m_model;
    QString m_selectedToolId = QStringLiteral("select_move");
    QString m_selectedObjectId;
};
