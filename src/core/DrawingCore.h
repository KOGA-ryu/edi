#pragma once

#include <QObject>
#include <QString>
#include <QUrl>
#include <QVariantList>
#include <QVariantMap>
#include <QVector>

#include "io/DrawingDocumentStore.h"

#include "drafting/DraftingArray.h"
#include "drafting/DraftingDocument.h"
#include "drafting/DraftingRoom.h"
#include "drafting/DraftingCalibration.h"
#include "drafting/DraftingCommands.h"
#include "drafting/DraftingGrid.h"
#include "drafting/DraftingGuideOps.h"
#include "drafting/DraftingLayerOps.h"
#include "drafting/DraftingMetadata.h"
#include "drafting/DraftingNudgeOps.h"
#include "drafting/DraftingPhysicalEdit.h"
#include "drafting/DraftingPlotPlan.h"
#include "drafting/DraftingSnap.h"
#include "drafting/DraftingToolCreation.h"

#include <functional>
#include <cstdint>
#include <optional>
#include <vector>

// A "pick a point" capture. When armed, the next canvas click supplies a bare
// point to a waiting consumer (the radial-array centre today; trim's boundary
// next) instead of selecting or creating. The intent is DATA — the controller
// switches on it — not a stored std::function, keeping this variation point in
// the same data-oriented style as the drafting core (CLAUDE.md: "variation
// points are data — enums, kinds, plan structs"). std::optional absence is the
// "not armed" state, so the enum needs no None member.
enum class PointCaptureIntent {
    RadialArrayCenter,
    TrimPoint,        // the click chooses which part of the selected line to trim away
    ExtendPoint,      // the click chooses which end of the selected line to extend
    FilletSecondLine, // the click picks the other line + the corner to round
    ChamferSecondLine, // the click picks the other line + the corner to bevel
    BlockInstance,    // the click is where to stamp the armed block (C3 palette)
    RegionFill,       // the click seeds a room-footprint fill (DM-10)
};

struct PendingPointCapture {
    PointCaptureIntent intent = PointCaptureIntent::RadialArrayCenter;
    QString prompt; // shown to the user while the click is awaited
};

class DrawingDocumentController final : public QObject {
    Q_OBJECT

public:
    explicit DrawingDocumentController(QObject *parent = nullptr);

    QVariantMap modelDocument() const;
    bool saveDocument(const QUrl &url);
    bool openDocument(const QUrl &url);
    bool exportSvgDocument(const QUrl &url);
    bool exportHpglDocument(const QUrl &url);
    bool exportGcodeDocument(const QUrl &url); // N5 plotter G-code
    // Dirty = content differs from the last save/open, ignoring selection (which
    // is excluded from undo too), so merely selecting an object is not "dirty".
    bool isDocumentDirty() const;
    // Monotonic counter bumped by every modelChanged emission — the cache
    // key for anything derived from the document projection (the canvas
    // scene cache keys on it; modelDocument's own cache uses it too).
    quint64 modelGeneration() const { return m_modelGeneration; }
    bool undo();
    bool redo();
    bool canUndo() const;
    bool canRedo() const;
    // Bracket a continuous gesture (object/handle drag) so its many incremental
    // commands collapse into a single undo step. Must be balanced; safe to leave
    // unbalanced (a later begin re-captures).
    void beginInteractiveEdit();
    void endInteractiveEdit();
    QString selectedToolId() const;
    QString selectedObjectId() const;
    // The bus seam feature #2 forced: the recipe lab resolves measurement
    // bindings against the live drafting document and grid. Read-only — the
    // recipe feature consumes drafting data, it never mutates it.
    const edi::drafting::DraftingDocument &draftingDocument() const { return m_document; }
    edi::drafting::DraftingGridProjection draftingGridProjection() const
    {
        return edi::drafting::projectDraftingGrid(m_gridSettings);
    }
    // Select by id (the object list drives this). Selection-only: no undo step.
    bool selectObjectById(const QString &id);
    QString activeLayerId() const;
    bool gridSnapEnabled() const;
    bool objectSnapEnabled() const;
    bool endpointSnapEnabled() const;
    bool vertexSnapEnabled() const;
    bool midpointSnapEnabled() const;
    bool centerSnapEnabled() const;
    bool intersectionSnapEnabled() const;
    bool guideSnapEnabled() const;
    bool guideMoveSnapEnabled() const;
    bool objectSnapPriorityBeforeGrid() const;
    QString gridPresetId() const;
    QString objectSnapTolerancePresetId() const;
    QString plotOrderModeId() const;
    QString plotDirectionModeId() const;
    void setSelectedToolId(const QString &toolId);
    void setPolygonSides(int sides);
    int polygonSides() const;
    // N4 rectangle tool options + aspect-lock. Radius/inset ride into the next
    // rectangle's creation; aspect-lock constrains rectangle corner drags.
    void setRectCornerRadius(double radius);
    double rectCornerRadius() const;
    void setRectInset(double inset);
    double rectInset() const;
    void setAspectLockEnabled(bool enabled);
    bool aspectLockEnabled() const;
    // #30 parametric creation + arrays. fixedRadius rides into the next
    // radius-from-gesture creation (0 = gesture-sized); count/spacing feed
    // repeat (count copies along one axis), grid (count x count cells using
    // both spacings), and radial (count copies ringed around the drawable
    // centre).
    void setFixedRadius(double radius);
    double fixedRadius() const;
    // Wall tool option: the band thickness a freshly drawn wall is born with
    // (today fixed at the 0.1 default, then edited after). Invalid -> 0.1.
    void setWallThickness(double thickness);
    double wallThickness() const;
    void setArrayCount(int count);
    int arrayCount() const;
    void setArraySpacingX(double spacing);
    double arraySpacingX() const;
    void setArraySpacingY(double spacing);
    double arraySpacingY() const;
    void setGridSnapEnabled(bool enabled);
    void setObjectSnapEnabled(bool enabled);
    void setEndpointSnapEnabled(bool enabled);
    void setVertexSnapEnabled(bool enabled);
    void setMidpointSnapEnabled(bool enabled);
    void setCenterSnapEnabled(bool enabled);
    void setIntersectionSnapEnabled(bool enabled);
    void setGuideSnapEnabled(bool enabled);
    void setGuideMoveSnapEnabled(bool enabled);
    void setObjectSnapPriorityBeforeGrid(bool enabled);
    void setObjectSnapTolerancePreset(QString presetId);
    void setGridPresetId(const QString &presetId);
    void setGridUnitId(const QString &unitId);
    void setGridSize(double width, double height);
    void setGridMargins(double left, double top, double right, double bottom);
    void setGridStep(double minorStep);
    void setGridMajorLineEvery(int majorLineEvery);
    void setGridVisible(bool visible);
    void setPlotOrderModeId(const QString &modeId);
    void setPlotDirectionModeId(const QString &modeId);
    void updatePointerNormalized(double x, double y);
    bool updateSelectedObjectGeometryField(const QString &fieldId, double value);
    bool updateSelectedObjectPhysicalGeometryField(const QString &fieldId, double value);
    bool setSelectedObjectLocked(bool locked);
    // Per-object styling (the Style inspector group). Empty color or width
    // <= 0 mean "inherit the layer's plot style" — going back to the layer
    // is itself a styling decision, so the sentinels are legal inputs.
    bool setSelectedObjectStrokeColor(const QString &color);
    bool setSelectedObjectStrokeWidth(double width);
    // Opacity is per-object only (0..1, clamped): layers have no alpha to
    // inherit, so 0 means transparent, not "use the layer's".
    bool setSelectedObjectStrokeOpacity(double opacity);
    // Per-object fill (closed shapes only at paint time): colour + opacity,
    // opacity 0 = no fill (the default that keeps existing drawings unfilled).
    bool setSelectedObjectFillColor(const QString &color);
    bool setSelectedObjectFillOpacity(double opacity);
    // Text-annotation content (the geometry's string) — rides UpdateGeometryCommand.
    bool setSelectedObjectTextContent(const QString &content);
    bool setSelectedObjectLineStyle(const QString &lineStyle);
    bool setSelectedObjectVisible(bool visible);
    bool setSelectedGuideLabel(const QString &label);
    bool setSelectedGuideColor(const QString &color);
    bool setSelectedGuideDashStyle(const QString &dashStyle);
    bool setSelectedGuideLabelVisible(bool visible);
    bool setSelectedDimensionKind(const QString &kindId);
    bool setSelectedDimensionLabelVisible(bool visible);
    // N3 semantic/export metadata — these apply to the selected object of ANY
    // kind (role/material/group/tags are object-wide, not guide/dimension
    // specific). Tags arrive as a list; the shell splits its text field.
    bool setSelectedObjectRole(const QString &roleId);
    // M1.3: set the selected wall's neutral render type (solid/door/window/secret).
    bool setSelectedWallType(const QString &typeId);
    bool setSelectedObjectMaterial(const QString &material);
    bool setSelectedObjectExportGroup(const QString &exportGroup);
    bool setSelectedObjectTags(const QStringList &tags);
    bool setDefaultLayerLocked(bool locked);
    bool setDefaultLayerVisible(bool visible);
    bool setActiveLayerLocked(bool locked);
    bool setActiveLayerVisible(bool visible);
    bool setActiveLayerPlotEnabled(bool enabled);
    bool setActiveLayerPenPreset(const QString &presetId);
    bool setActiveLayerStrokeWidthPreset(const QString &presetId);
    bool createLayer();
    bool renameActiveLayer(const QString &name);
    bool setActiveLayerId(const QString &layerId);
    bool moveActiveLayer(const QString &direction);
    bool moveSelectedObjectToLayer(const QString &layerId);
    bool nudgeSelection(const QString &direction, const QString &stepMode);
    bool nudgeSelectionInsideDrawable(const QString &direction, const QString &stepMode);
    bool offsetSelectedObject(const QString &sideId);
    bool mirrorSelectedObject(const QString &axisId);
    bool repeatSelectedObject(const QString &axisId);
    bool gridArraySelectedObject();
    // Radial array now PICKS its centre: this arms a pick-a-point capture (the
    // next canvas click sets the ring centre), replacing the old hardcoded
    // drawable centre. Returns false (and arms nothing) when no editable source
    // is selected. The captured click runs the array via runRadialArrayAtCenter.
    bool beginRadialArrayCenterPick();
    // True while a pick-a-point capture is armed (the UI can show the prompt /
    // a crosshair cursor); pointCapturePrompt() is the text to display.
    bool isAwaitingPointCapture() const { return m_pointCapture.has_value(); }
    QString pointCapturePrompt() const;
    // Trim verb: arms a pick-a-point capture when a line is selected. The
    // captured click chooses the part of the line to remove, trimming it back
    // to where another line crosses it. False (arms nothing) when no editable
    // line is selected.
    bool beginTrimSelectedLine();
    // Extend verb: the geometric mirror of trim. Arms a pick-a-point capture when a
    // line is selected; the captured click chooses which END to lengthen, extending
    // it to the nearest boundary line it would reach. False when no editable line is
    // selected.
    bool beginExtendSelectedLine();
    // Fillet verb: arms a pick-a-point capture when a line is selected. The
    // captured click picks the OTHER line and the corner; both lines are trimmed
    // to the tangent points and a rounding arc (radius = filletRadius()) joins
    // them, all in one undo step. False when no editable line is selected.
    bool beginFilletSelectedLine();
    void setFilletRadius(double radius);
    double filletRadius() const;
    // Chamfer verb: the angular sibling of fillet. Arms a pick-a-point capture when
    // a line is selected; the captured click picks the OTHER line and the corner,
    // both lines are set back by chamferSetback() and a straight bevel joins them,
    // all in one undo step. False when no editable line is selected.
    bool beginChamferSelectedLine();
    void setChamferSetback(double setback);
    double chamferSetback() const;
    bool alignSelection(const QString &modeId);
    bool distributeSelection(const QString &axisId);
    bool createCalibrationPattern(const QString &patternId);
    // Generate a whole room from a neutral spec (the Seam-A authoring path): mint
    // ids on demand, plan the perimeter walls, create-and-select them atomically.
    // The spec carries geometry + neutral tags only — no game rules.
    bool createRoomFromSpec(const edi::drafting::RoomSpec &spec);
    // Generate a whole MULTI-ROOM map from one neutral spec: plan every room,
    // mint globally-unique ids on a single serial, resolve cross-room connections
    // (by room.plug), and create all rooms + plugs + connections in ONE undo step.
    // `canvasPerAuthoredUnit` is the scale the spec was parsed at (canvas units per
    // authored unit, e.g. 0.02 canvas/ft) — recorded on the document so the engine
    // export can recover authored units. 1.0 = the spec is already in canvas units.
    bool createMapFromSpec(const edi::drafting::MapSpec &spec, double canvasPerAuthoredUnit = 1.0);
    // Generate a map from an ASCII glyph grid (the control-gate authoring path):
    // parse, derive walls/doors/features, auto-fit + centre on the board, and
    // create-and-select atomically. Glyphs are data; tags stay neutral.
    bool createMapFromAscii(const std::string &asciiText);
    bool recordCalibrationMeasurement(double measuredValue);
    bool applyCalibrationCorrection();
    bool fitSelectionToDrawableBounds();
    bool centerSelectionInDrawable();
    bool moveSelectionToDrawableOrigin();
    bool moveSelectedGuideToDrawableOrigin();
    bool centerSelectedGuideInDrawable();
    bool moveSelectedGuideToDrawableMax();
    bool offsetSelectedGuide(const QString &direction, const QString &stepMode);
    bool fitSelectedConstructionLineToDrawable();
    bool createGuideFromSelectedBounds(const QString &placementId);
    bool createOffsetGuideFromSelectedBounds(const QString &placementId, const QString &stepMode);
    bool applyGuidePreset(const QString &presetId);
    bool alignSelectionToNearestGuide(const QString &modeId);
    bool deleteSelectedGuide();
    bool deleteAllGuides();
    bool mergeDuplicateGuides();
    bool setAllGuidesVisible(bool visible);
    bool setAllGuidesLocked(bool locked);
    void clickCanvasNormalized(double x, double y);
    void cancelPendingCreation();
    // Commits the in-flight multi-click creation — polyline OR spline
    // (double-click / Enter). False when nothing valid is pending; a one-point
    // trail dissolves. Shared by both multi-click tools, hence the general name.
    bool finishPendingMultiClick();
    bool deleteSelectedObject();
    bool duplicateSelectedObject();
    // N1 copy/cut/paste. Copy snapshots the current selection into an
    // in-process clipboard (no document change); cut copies then deletes the
    // selection as one undo step; paste creates fresh-id, offset clones of the
    // clipboard, selects them, as one undo step. canPaste gates menu/shortcut
    // state. The clipboard outlives any single selection but not the process.
    bool copySelection();
    bool cutSelection();
    bool paste();
    bool canPaste() const;
    // Phase C block library. defineBlockFromSelection saves the current selection
    // as a named block DEFINITION (normalized to the origin) in one undo step.
    // placeBlockInstance stamps a FLATTEN instance — independent transformed COPIES
    // of the definition centred on a normalized point, via the paste machinery,
    // one undo step, auto-selected. Translate-only (rotation/scale deferred to the
    // transformGeometry slice); a placed object is an ordinary, directly-editable
    // shape with no link back to the definition.
    // `assetRef` (Seam B, optional) links the new block to the Blender asset it
    // depicts — the thread that lets a placement reach the engine.
    bool defineBlockFromSelection(const QString &name, const QString &assetRef = {});
    bool placeBlockInstance(const QString &blockId, double x, double y);
    // C3 palette: arm a pick-a-point capture for placing `blockId` — the next
    // canvas click resolves to placeBlockInstance (same idiom as the radial-array
    // centre pick). Refused if the block does not exist.
    bool beginBlockInstancePick(const QString &blockId);
    // DM-10 region fill: arm a pick-a-point capture whose click seeds a room-footprint
    // fill. Refused (returns false) when the document has no rooms — no dead prompt,
    // the same up-front guard beginBlockInstancePick uses for a vanished block.
    bool beginRegionFillPick();
    // The apply: locate the room containing `seed` (planRegionFill) and mint a filled
    // Polygon of its footprint, auto-selected in one undo step; a seed in open space
    // is a surfaced refusal. Resolved by resolvePointCapture once the click arrives.
    bool fillEnclosedRegion(edi::drafting::Point2D seed);
    void updateCreationPreviewNormalized(double x, double y);
    bool editSelectedHandleNormalized(const QString &handleId, double x, double y);
    bool moveSelectionNormalized(double dx, double dy);
    bool selectObjectsInBoundsNormalized(double x1, double y1, double x2, double y2);

signals:
    void modelChanged();
    // Transient pointer/preview state changed (mouse movement) — the
    // document did NOT. The canvas repaints on this; heavyweight listeners
    // (inspector rebuild, object list, persistence scheduling, undo chrome)
    // subscribe only to modelChanged. Without the split, every mouse move
    // rebuilt the whole right panel.
    void pointerChanged();

private:
    bool applyCommandAndEmit(const edi::drafting::DraftingCommand &command);
    // Undo snapshot bookkeeping. beginEdit() copies the document at the start of
    // a mutating action; commitEdit() (called right before emit, so canUndo() is
    // current when the view refreshes) pushes that snapshot onto the undo stack
    // unless the change was pure selection. Idempotent within one action.
    void beginEdit();
    // selectionOnly: the caller's compile-time knowledge of whether this
    // bracket applied only selection commands (std::holds_alternative over
    // the command variant). Provided, it replaces the encode-and-compare
    // fallback — which serializes the whole document TWICE per commit and
    // made every creation click O(document).
    void commitEdit(std::optional<bool> selectionOnly = std::nullopt);
    // Push a pre-mutation snapshot (with the epoch it held) onto the undo stack,
    // cap, clear redo, and mint a fresh epoch for the now-current state.
    void pushUndoState(const edi::drafting::DraftingDocument &before, std::uint64_t epochBefore);
    bool finishEdit(const QString &mode, const QString &fieldId, bool ok,
                    edi::drafting::DraftingResultCode code, const QString &message);
    bool applyFieldEdit(
        const QString &mode,
        const QString &invalidMessage,
        const QString &fieldId,
        double value,
        const std::function<edi::drafting::DraftingPhysicalGeometryEditPlan(const edi::drafting::DraftingObject &)> &planEdit);
    void setSnapFlag(bool edi::drafting::DraftingSnapSettings::*flag, bool enabled);
    void commitGridSettings(edi::drafting::DraftingGridSettings settings);
    void commitCustomGridSettings(edi::drafting::DraftingGridSettings settings);
    bool applySelectionDrawablePlacement(edi::drafting::DraftingSelectionDrawablePlacement placement);
    bool applyActiveObjectMetadataUpdate(
        edi::drafting::DraftingShapeKind kind,
        const std::function<edi::drafting::DraftingMetadataUpdatePlan(const edi::drafting::ObjectMetadata &)> &planMetadata);
    // Same resolve->plan->apply, but for the active object of ANY kind — the
    // object-wide metadata (N3 role/material/group/tags) is not kind-keyed.
    bool applyActiveObjectMetadataUpdate(
        const std::function<edi::drafting::DraftingMetadataUpdatePlan(const edi::drafting::ObjectMetadata &)> &planMetadata);
    bool applyActiveObjectGeometryUpdate(
        edi::drafting::DraftingShapeKind kind,
        const std::function<std::optional<edi::drafting::DraftingGeometry>(const edi::drafting::DraftingObject &)> &planGeometry);
    // Resolves the active object by the kind matching Geometry and unwraps the
    // variant before invoking the plan; the kind/type pairing is fixed at compile time.
    template <typename Geometry>
    bool applyActiveGeometryPlan(
        const std::function<std::optional<edi::drafting::DraftingGeometry>(const Geometry &)> &planGeometry);
    bool applyGuideDrawablePlacement(edi::drafting::DraftingGuideDrawablePlacement placement);
    bool applyLayerFlagsUpdate(
        const edi::drafting::LayerId &layerId,
        edi::drafting::DraftingLayerFlagsPlan (*planFlags)(const edi::drafting::DraftingLayer &, bool),
        bool value);
    bool applyActiveLayerPlotStyleUpdate(
        const std::function<edi::drafting::LayerPlotStyle(const edi::drafting::DraftingLayer &)> &planPlot);
    bool createTransformedActiveObject(
        const QString &idPrefix,
        const std::function<std::optional<edi::drafting::DraftingObject>(
            const edi::drafting::DraftingObject &source, const std::string &newId)> &transform);
    // By value: callers hand the batch over (std::move) so it rides into the
    // command variant without a second N-object copy. Optional `plugs`,
    // `connections` and `rooms` are map edits declared INSIDE the same edit bracket,
    // in dependency order (objects → plugs anchor to objects → connections reference
    // plugs; rooms are independent), so one undo restores the whole authored map.
    bool createObjectsAndSelect(std::vector<edi::drafting::DraftingObject> objects,
                                std::vector<edi::drafting::DraftingPlug> plugs = {},
                                std::vector<edi::drafting::DraftingDeclaredConnection> connections = {},
                                std::vector<edi::drafting::DraftingMapRoom> rooms = {});
    // Shared resolve->ids->plan->apply tail of every array action (repeat,
    // grid, radial): resolve the editable active object, mint copyCount fresh
    // ids, run the planner, create-and-select. The planner callable is the
    // variation point — same shape as createTransformedActiveObject above.
    bool createArrayFromActiveObject(
        const QString &idPrefix,
        int copyCount,
        const std::function<edi::drafting::DraftingArrayResult(
            const edi::drafting::DraftingObject &source,
            const std::vector<edi::drafting::DraftingObjectId> &newObjectIds)> &plan);
    // Runs the radial array around an explicit centre — shared by the picked
    // centre (the only path today). Separated from the arming so the centre is
    // a plain argument, not controller state.
    bool runRadialArrayAtCenter(edi::drafting::Point2D center);
    // Dispatches a resolved capture click to its waiting consumer (a switch on
    // the intent), then clears the capture. The point is already snapped.
    void resolvePointCapture(edi::drafting::Point2D point);
    // Trims the active line back to the nearest crossing boundary line, removing
    // the side the captured point fell on. Surfaces a status on rejection (no
    // crossing / would-collapse) so a dead trim click is never silent.
    void applyTrimAtPoint(edi::drafting::Point2D point);
    // Extends the active line's nearer end to the nearest boundary line its
    // extension reaches (one UpdateGeometry command). Surfaces a status on a dead
    // click (no reachable boundary). Mirrors applyTrimAtPoint.
    void applyExtendAtPoint(edi::drafting::Point2D point);
    // Fillets the active line against the nearest OTHER line to the captured
    // point: trims both to the tangent points and creates the rounding arc as
    // one atomic edit. Surfaces a status on rejection.
    void applyFilletAtPoint(edi::drafting::Point2D point);
    // Chamfers the active line against the nearest OTHER line to the captured
    // point: sets both back by m_chamferSetback and creates the bevel as one
    // atomic edit. Surfaces a status on rejection. Mirrors applyFilletAtPoint.
    void applyChamferAtPoint(edi::drafting::Point2D point);
    bool createGuideFromActiveBounds(
        const char *sourceTag,
        const std::function<edi::drafting::DraftingGuidePlan(const edi::drafting::Bounds2D &bounds)> &planGuide);

    QString m_selectedToolId = QStringLiteral("select_move");
    int m_polygonSides = 6;
    double m_rectCornerRadius = 0.0;
    double m_rectInset = 0.0;
    bool m_aspectLockEnabled = false;
    double m_fixedRadius = 0.0;
    double m_wallThickness = 0.1; // wall tool option: band width at draw time
    double m_filletRadius = 0.05; // default rounding radius for the fillet verb
    double m_chamferSetback = 0.05; // default setback for the chamfer verb
    // Array defaults match the retired hardcoded repeat (3 copies, 0.1
    // spacing) so the buttons behave identically until the spins are touched.
    int m_arrayCount = 3;
    double m_arraySpacingX = 0.1;
    double m_arraySpacingY = 0.1;
    // In-process copy/paste buffer (N1): plain object snapshots, never
    // serialized — copy is "remember these", not a persisted format.
    std::vector<edi::drafting::DraftingObject> m_clipboard;
    edi::drafting::DraftingDocument m_document;
    edi::drafting::DraftingGridSettings m_gridSettings;
    edi::drafting::DraftingSnapSettings m_snapSettings;
    bool m_guideMoveSnapEnabled = true;
    edi::drafting::DraftingPlotSettings m_plotSettings;
    std::optional<edi::drafting::DraftingCalibrationMeasurement> m_latestCalibrationMeasurement;
    std::optional<edi::drafting::DraftingCalibrationCorrectionPlan> m_pendingCalibrationCorrection;
    std::optional<edi::drafting::Point2D> m_pointerRawPoint;
    std::optional<edi::drafting::DraftingToolCreationRequest> m_pendingCreation;
    std::optional<edi::drafting::DraftingObject> m_previewObject;
    std::optional<PendingPointCapture> m_pointCapture;
    // The block awaiting placement while a BlockInstance capture is armed; read
    // by resolvePointCapture once the placement click arrives.
    QString m_pendingBlockId;
    // Projection cache. The document-shaped model (objects, layers, grid,
    // plot plan, safety annotation, selection bounds) rebuilds only when a
    // modelChanged emission bumps the generation; every call then overlays
    // the cheap volatile keys (pointer, preview ghost, quick measure, …).
    // The cache shares the signal's contract exactly: any mutation path
    // that forgot to emit modelChanged was ALREADY leaving every listener
    // stale — the cache adds no new staleness class.
    mutable QVariantMap m_cachedDocumentModel;
    mutable edi::drafting::DraftingGridProjection m_cachedGrid;
    mutable quint64 m_cachedModelGeneration = 0; // 0 = never built
    quint64 m_modelGeneration = 1;
    QVariantMap m_lastGuideDragSnap;
    QVariantMap m_lastEditStatus;
    int m_nextObjectSerial = 1;
    DrawingDocumentStore m_store;

    // Undo/redo + dirty tracking share one mechanism: a monotonic "epoch"
    // labels each distinct document state. Unlike DraftingDocument::revision
    // (bumped with ++, so undo restores an OLD value and the counter aliases —
    // 4→5→undo→4→5), epoch values are never reused, so dirty is a plain
    // inequality and undoing back to the saved state compares exactly equal.
    // Each undo/redo snapshot carries the epoch its document had, so restoring
    // a snapshot restores its epoch too.
    struct DocumentSnapshot {
        edi::drafting::DraftingDocument document;
        std::uint64_t epoch = 0;
    };
    std::vector<DocumentSnapshot> m_undoStack;
    std::vector<DocumentSnapshot> m_redoStack;
    edi::drafting::DraftingDocument m_editBefore;
    std::uint64_t m_editEpochBefore = 0;
    bool m_editCommitted = true;
    edi::drafting::DraftingDocument m_interactiveBefore;
    std::uint64_t m_interactiveEpochBefore = 0;
    bool m_interactiveEditActive = false;
    std::uint64_t m_documentEpoch = 0; // epoch of the current m_document
    std::uint64_t m_nextEpoch = 1;     // never-reused source of new epochs
    std::uint64_t m_savedEpoch = 0;    // epoch at the last save/open (0 = fresh)
};
