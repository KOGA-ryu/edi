#pragma once

#include <QJsonArray>
#include <QJsonObject>
#include <QObject>
#include <QString>
#include <QVariantList>
#include <QVariantMap>
#include <QVector>

struct DrawingCoreResult {
    QJsonObject model;
    QString svg;
};

class DrawingCore final {
public:
    static DrawingCoreResult runScript(const QJsonObject &script);
    static QString modelToJson(const QJsonObject &model);
    static QString modelToSvg(const QJsonObject &model);
};

class DrawingDocumentController final : public QObject {
    Q_OBJECT
    Q_PROPERTY(int revision READ revision NOTIFY modelChanged)

public:
    explicit DrawingDocumentController(QObject *parent = nullptr);

    int revision() const;

    Q_INVOKABLE QVariantMap modelDocument() const;
    Q_INVOKABLE QString exportJson() const;
    Q_INVOKABLE QString exportSvg() const;
    Q_INVOKABLE bool loadModel(const QVariantMap &model);
    Q_INVOKABLE void reset();
    Q_INVOKABLE void selectTool(const QString &toolId);
    Q_INVOKABLE void selectObject(const QString &objectId);
    Q_INVOKABLE void selectObjects(const QVariantList &objectIds);
    Q_INVOKABLE void deleteObject(const QString &objectId);
    Q_INVOKABLE void deleteSelectedObject();
    Q_INVOKABLE void duplicateObject(const QString &objectId, double dx = 0.03125, double dy = 0.03125);
    Q_INVOKABLE void duplicateSelectedObject();
    Q_INVOKABLE void pasteObject(const QVariantMap &object, double dx = 0.03125, double dy = 0.03125);
    Q_INVOKABLE void beginMoveGesture();
    Q_INVOKABLE void endMoveGesture();
    Q_INVOKABLE void moveObjectBy(const QString &objectId, double dx, double dy);
    Q_INVOKABLE void moveSelectedObjectBy(double dx, double dy);
    Q_INVOKABLE void updateObjectField(const QString &objectId, const QString &field, double value);
    Q_INVOKABLE void updateSelectedObjectField(const QString &field, double value);
    Q_INVOKABLE void setToolParameter(const QString &parameter, const QVariant &value);
    Q_INVOKABLE void cancelPending();
    Q_INVOKABLE void setSnap(bool enabled, int gridStepPx);
    Q_INVOKABLE void clickCanvasNormalized(double x, double y);
    Q_INVOKABLE void clickCanvasNormalizedWithSnapStep(double x, double y, int gridStepPx);
    Q_INVOKABLE bool canUndo() const;
    Q_INVOKABLE bool canRedo() const;
    Q_INVOKABLE void undo();
    Q_INVOKABLE void redo();
    Q_INVOKABLE void runScript(const QVariantMap &script);
    Q_INVOKABLE QString selectedToolId() const;
    Q_INVOKABLE QString selectedObjectId() const;

signals:
    void modelChanged();

private:
    void applyCommand(const QJsonObject &command);
    void publish();
    QJsonObject scriptEnvelope() const;

    QJsonArray m_commands;
    QVector<QJsonArray> m_undoSnapshots;
    QVector<QJsonArray> m_redoSnapshots;
    QJsonArray m_lastStableCommands;
    QJsonObject m_model;
    bool m_moveGestureActive = false;
    bool m_moveGestureUndoCaptured = false;
    int m_moveGestureStartCommandCount = 0;
    int m_revision = 0;
};
