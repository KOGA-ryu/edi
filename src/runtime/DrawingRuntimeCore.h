#pragma once

#include <QObject>
#include <QVariantList>
#include <QVariantMap>

class DrawingToolCatalog : public QObject {
    Q_OBJECT

public:
    explicit DrawingToolCatalog(QObject *parent = nullptr);

    Q_INVOKABLE QVariantList toolModes() const;
    Q_INVOKABLE QVariantMap toolSettingsById() const;
    Q_INVOKABLE QVariantList precisionTools() const;
    Q_INVOKABLE QVariantList dataTools() const;
    Q_INVOKABLE QVariantList imageTools() const;
    Q_INVOKABLE QVariantMap externalToolSettingsById() const;
    Q_INVOKABLE QVariantList assetSources() const;
    Q_INVOKABLE QVariantList patternFamilies() const;
    Q_INVOKABLE QVariantList toolPresets() const;
    Q_INVOKABLE QVariantList layerStack() const;
    Q_INVOKABLE QVariantList sidebarSections() const;
};

class DrawingRuntimeRows : public QObject {
    Q_OBJECT

public:
    explicit DrawingRuntimeRows(QObject *parent = nullptr);

    Q_INVOKABLE QVariantMap fitTransform(QObject *controller) const;
    Q_INVOKABLE QString editNumber(const QVariant &value) const;
    Q_INVOKABLE QVariantList objectEditRows(QObject *controller) const;
    Q_INVOKABLE QVariantList inspectorRows(QObject *controller) const;
    Q_INVOKABLE QVariantList toolSettingsRows(QObject *controller) const;
    Q_INVOKABLE QVariantList toolParameterEditRows(QObject *controller) const;
    Q_INVOKABLE QVariantList modelValidationRows(QObject *controller) const;
    Q_INVOKABLE QVariantList externalToolRows(QObject *controller) const;
    Q_INVOKABLE QVariantList sidebarRows(QObject *controller, const QVariantMap &section) const;
    Q_INVOKABLE bool sidebarRowSelected(QObject *controller, const QVariantMap &section, const QVariantMap &row) const;
    Q_INVOKABLE bool sidebarRowClickable(const QVariantMap &section) const;
    Q_INVOKABLE QVariantList toolPaletteRows(QObject *controller) const;
    Q_INVOKABLE QVariantList validationRows(QObject *controller) const;
    Q_INVOKABLE QVariantList modelObjectRows(QObject *controller) const;
    Q_INVOKABLE QVariantList logRows(QObject *controller) const;
    Q_INVOKABLE QVariantList exportRows(QObject *controller) const;
    Q_INVOKABLE QVariantList manifestRows(QObject *controller) const;
};

class DrawingInteractionRuntime : public QObject {
    Q_OBJECT

public:
    explicit DrawingInteractionRuntime(QObject *parent = nullptr);

    Q_INVOKABLE QVariantMap initialMetricsState() const;
    Q_INVOKABLE QVariantMap beginMetricsInteraction(const QVariantMap &state, const QString &mode, double timestampMs, const QVariantMap &snapshot) const;
    Q_INVOKABLE QVariantMap recordMetricsPointerMove(const QVariantMap &state, double count = 1) const;
    Q_INVOKABLE QVariantMap recordMetricsControllerMutation(const QVariantMap &state, const QString &kind, double count = 1) const;
    Q_INVOKABLE QVariantMap recordMetricsRenderRequest(const QVariantMap &state, double count = 1) const;
    Q_INVOKABLE QVariantMap recordMetricsHitTest(const QVariantMap &state, double count = 1) const;
    Q_INVOKABLE QVariantMap recordMetricsSnap(const QVariantMap &state, double count = 1) const;
    Q_INVOKABLE QVariantMap recordMetricsHandlePlan(const QVariantMap &state, double count = 1) const;
    Q_INVOKABLE QVariantMap finishMetricsInteraction(const QVariantMap &state, double timestampMs, const QVariantMap &snapshot) const;
    Q_INVOKABLE QVariantMap cancelMetricsInteraction(const QVariantMap &state, double timestampMs, const QVariantMap &snapshot) const;
    Q_INVOKABLE QVariantMap assertWithinBudget(const QVariantMap &record, const QVariantMap &budget) const;

    Q_INVOKABLE QVariantMap initialTelemetryState() const;
    Q_INVOKABLE QVariantMap beginTelemetryInteraction(const QVariantMap &state, const QString &mode, double timestampMs, const QVariantMap &snapshot) const;
    Q_INVOKABLE QVariantMap recordTelemetryPointerMove(const QVariantMap &state, double count = 1) const;
    Q_INVOKABLE QVariantMap recordTelemetryControllerMutation(const QVariantMap &state, const QString &kind, double count = 1) const;
    Q_INVOKABLE QVariantMap recordTelemetryRenderRequest(const QVariantMap &state, double count = 1) const;
    Q_INVOKABLE QVariantMap recordTelemetryHitTest(const QVariantMap &state, double count = 1) const;
    Q_INVOKABLE QVariantMap recordTelemetrySnap(const QVariantMap &state, double count = 1) const;
    Q_INVOKABLE QVariantMap recordTelemetryHandlePlan(const QVariantMap &state, double count = 1) const;
    Q_INVOKABLE QVariantMap finishTelemetryInteraction(const QVariantMap &state, double timestampMs, const QVariantMap &snapshot) const;
    Q_INVOKABLE QVariantMap cancelTelemetryInteraction(const QVariantMap &state, double timestampMs, const QVariantMap &snapshot) const;
};

class DrawingToolScriptRuntime : public QObject {
    Q_OBJECT

public:
    explicit DrawingToolScriptRuntime(QObject *parent = nullptr);

    Q_INVOKABLE QVariantMap validateScript(const QVariantMap &script, const QVariantMap &library = {}) const;
    Q_INVOKABLE QVariantMap expandedSteps(const QVariantMap &script, const QVariantMap &library = {}) const;
    Q_INVOKABLE QVariantMap metricsBudgetsByMode(const QVariantMap &script, const QVariantMap &library = {}) const;
    Q_INVOKABLE QVariantMap executionPlan(const QVariantMap &script, const QVariantMap &library = {}) const;
    Q_INVOKABLE QVariantList stepDriverOps(const QVariantMap &step) const;
    Q_INVOKABLE QVariantMap driverPlan(const QVariantMap &executionPlan) const;
    Q_INVOKABLE QVariantList driverOps(const QVariantMap &executionPlan) const;
};
