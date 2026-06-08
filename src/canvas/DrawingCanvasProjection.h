#pragma once

#include "DrawingCanvasTypes.h"

namespace drawing_canvas {

CanvasBounds emptyBounds();
CanvasBounds includePointInBounds(CanvasBounds bounds, double x, double y);
bool boundsIntersects(const CanvasBounds &bounds, double minX, double minY, double maxX, double maxY);
CanvasBounds normalizedObjectBounds(const CanvasObjectView &object);
bool objectIntersectsBounds(const CanvasObjectView &object, double minX, double minY, double maxX, double maxY);
QStringList selectedObjectIds(const QVariantMap &doc);
bool selectedObject(const QVariantMap &doc, const QString &objectId);
bool selectedLayer(const QVariantMap &doc, const QString &layerId);
CanvasBounds combinedSelectionBounds(const QVariantMap &doc);

} // namespace drawing_canvas
