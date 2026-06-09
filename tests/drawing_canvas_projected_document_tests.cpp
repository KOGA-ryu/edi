#include "widgets/DrawingCanvasProjectedDocument.h"

#include <QVariantList>
#include <QVariantMap>

#include <cassert>

using namespace drawing_canvas;

namespace {

QVariantMap object(const QString &id, const QString &kind = QStringLiteral("line"))
{
    return QVariantMap{
        {QStringLiteral("id"), id},
        {QStringLiteral("kind"), kind},
    };
}

QVariantMap modelWithObjects(const QVariantList &objects)
{
    return QVariantMap{{QStringLiteral("drawing_objects"), objects}};
}

} // namespace

int main()
{
    const QVariantMap model = modelWithObjects(QVariantList{
        object(QStringLiteral("first")),
        object(QStringLiteral("target"), QStringLiteral("circle")),
        object(QStringLiteral("last")),
    });
    const QVariantMap found = projectedObjectById(model, QStringLiteral("target"));
    assert(found.value(QStringLiteral("id")).toString() == QStringLiteral("target"));
    assert(found.value(QStringLiteral("kind")).toString() == QStringLiteral("circle"));

    assert(projectedObjectById(model, {}).isEmpty());
    assert(projectedObjectById({}, QStringLiteral("target")).isEmpty());
    assert(projectedObjectById(model, QStringLiteral("missing")).isEmpty());

    const QVariantMap malformedModel = modelWithObjects(QVariantList{
        QStringLiteral("ignored"),
        QVariantMap{},
        object(QStringLiteral("target")),
    });
    assert(projectedObjectById(malformedModel, QStringLiteral("target")).value(QStringLiteral("id")).toString() == QStringLiteral("target"));

    const QVariantMap duplicateModel = modelWithObjects(QVariantList{
        object(QStringLiteral("dup"), QStringLiteral("point")),
        object(QStringLiteral("dup"), QStringLiteral("rectangle")),
    });
    const QVariantMap duplicate = projectedObjectById(duplicateModel, QStringLiteral("dup"));
    assert(duplicate.value(QStringLiteral("kind")).toString() == QStringLiteral("point"));

    const QVariantMap fallbackIdModel = modelWithObjects(QVariantList{
        QVariantMap{
            {QStringLiteral("id"), 123},
            {QStringLiteral("kind"), QStringLiteral("point")},
        },
    });
    assert(projectedObjectById(fallbackIdModel, QStringLiteral("123")).value(QStringLiteral("kind")).toString() == QStringLiteral("point"));

    return 0;
}
