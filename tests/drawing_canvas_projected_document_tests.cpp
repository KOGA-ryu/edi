#include "widgets/DrawingCanvasProjectedDocument.h"

#include <QVariantList>
#include <QVariantMap>

#include "EdiAssert.h"

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
    const DrawingCanvasProjectedDocumentSurface missingSurface = projectedDocumentSurface({});
    EDI_CHECK(missingSurface.drawingObjects.empty());
    EDI_CHECK(missingSurface.previewObject.isEmpty());
    EDI_CHECK(missingSurface.plotSummary.isEmpty());

    const DrawingCanvasProjectedDocumentSurface surface = projectedDocumentSurface(QVariantMap{
        {QStringLiteral("drawing_objects"), QVariantList{
            object(QStringLiteral("first")),
            object(QStringLiteral("second")),
        }},
        {QStringLiteral("preview_object"), object(QStringLiteral("preview"), QStringLiteral("rectangle"))},
        {QStringLiteral("plot_summary"), QVariantMap{
            {QStringLiteral("status"), QStringLiteral("ready")},
        }},
    });
    EDI_CHECK(surface.drawingObjects.size() == 2);
    EDI_CHECK(surface.previewObject.value(QStringLiteral("id")).toString() == QStringLiteral("preview"));
    EDI_CHECK(surface.plotSummary.value(QStringLiteral("status")).toString() == QStringLiteral("ready"));

    const DrawingCanvasProjectedDocumentSurface malformedSurface = projectedDocumentSurface(QVariantMap{
        {QStringLiteral("drawing_objects"), QStringLiteral("ignored")},
        {QStringLiteral("preview_object"), QStringLiteral("ignored")},
        {QStringLiteral("plot_summary"), QStringLiteral("ignored")},
    });
    EDI_CHECK(malformedSurface.drawingObjects.empty());
    EDI_CHECK(malformedSurface.previewObject.isEmpty());
    EDI_CHECK(malformedSurface.plotSummary.isEmpty());

    const QVariantMap model = modelWithObjects(QVariantList{
        object(QStringLiteral("first")),
        object(QStringLiteral("target"), QStringLiteral("circle")),
        object(QStringLiteral("last")),
    });
    const QVariantMap found = projectedObjectById(model, QStringLiteral("target"));
    EDI_CHECK(found.value(QStringLiteral("id")).toString() == QStringLiteral("target"));
    EDI_CHECK(found.value(QStringLiteral("kind")).toString() == QStringLiteral("circle"));

    EDI_CHECK(projectedObjectById(model, {}).isEmpty());
    EDI_CHECK(projectedObjectById({}, QStringLiteral("target")).isEmpty());
    EDI_CHECK(projectedObjectById(model, QStringLiteral("missing")).isEmpty());

    const QVariantMap malformedModel = modelWithObjects(QVariantList{
        QStringLiteral("ignored"),
        QVariantMap{},
        object(QStringLiteral("target")),
    });
    EDI_CHECK(projectedObjectById(malformedModel, QStringLiteral("target")).value(QStringLiteral("id")).toString() == QStringLiteral("target"));

    const QVariantMap duplicateModel = modelWithObjects(QVariantList{
        object(QStringLiteral("dup"), QStringLiteral("point")),
        object(QStringLiteral("dup"), QStringLiteral("rectangle")),
    });
    const QVariantMap duplicate = projectedObjectById(duplicateModel, QStringLiteral("dup"));
    EDI_CHECK(duplicate.value(QStringLiteral("kind")).toString() == QStringLiteral("point"));

    const QVariantMap fallbackIdModel = modelWithObjects(QVariantList{
        QVariantMap{
            {QStringLiteral("id"), 123},
            {QStringLiteral("kind"), QStringLiteral("point")},
        },
    });
    EDI_CHECK(projectedObjectById(fallbackIdModel, QStringLiteral("123")).value(QStringLiteral("kind")).toString() == QStringLiteral("point"));

    return 0;
}
