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
