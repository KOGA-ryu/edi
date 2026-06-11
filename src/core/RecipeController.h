#pragma once

#include "recipe/RecipeDocument.h"

#include <QObject>
#include <QString>

// Thin Qt orchestration over the pure recipe ops, exactly the
// DrawingDocumentController pattern at smaller scale: own the document,
// delegate every edit to a free function, emit one change signal on
// success. The FeatureContext hands this to whichever features compose the
// recipe — they share the document through the bus, never through each
// other.
class RecipeController final : public QObject {
    Q_OBJECT

public:
    explicit RecipeController(QObject *parent = nullptr);

    const edi::recipe::RecipeDocument &document() const { return m_document; }
    // Why the latest edit was refused (empty after a success) — the panel
    // shows it instead of silently ignoring the click.
    QString lastError() const { return m_lastError; }

    bool addStep(const QString &shaperId);
    bool removeStep(int index);
    bool moveStep(int from, int to);
    bool setParamLiteral(int stepIndex, const QString &paramId, double value);
    bool bindParam(int stepIndex, const QString &paramId, const QString &objectId, const QString &field);
    bool setStepProfile(int stepIndex, const QString &objectId);

signals:
    void recipeChanged();

private:
    bool apply(const edi::recipe::RecipeOpResult &result);

    edi::recipe::RecipeDocument m_document;
    QString m_lastError;
};
