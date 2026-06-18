#include "widgets/DraftingFeature.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFileInfo>
#include <QFrame>
#include <QInputDialog>
#include <QScrollArea>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPair>
#include <QPushButton>
#include <QShortcut>
#include <QSizePolicy>
#include <QSpinBox>
#include <QStyle>
#include <QStringList>
#include <QVariantList>
#include <QVariantMap>
#include <QVBoxLayout>
#include <QVector>

#include <cstddef>
#include <optional>
#include <utility>
#include <vector>

#include "core/DrawingCore.h"
#include "widgets/BeltCrossWidget.h"
#include "widgets/DrawingCanvasWidget.h"
#include "widgets/ShellPanels.h"
#include "widgets/ShellWidgetHelpers.h"

using namespace edi::shell;

namespace {

// The drafting tool vocabulary in one table: id (the controller's language),
// label (belt tooltip), glyph (belt cell face), and the default belt row.
// Belt model (user direction 2026-06-10): each ROW is one tool, its cells
// are that tool's sub-features — guides, construction lines, and dimensions
// are one tool each with variants along the row; future variants (polyline,
// rectangle kinds) extend their tool's row. The belt rendering AND the
// default arrangement both read this — two hand-maintained lists would drift.
struct DraftingToolSpec {
    const char *id;
    const char *label;
    const char *glyph;
    int beltRow;
};

constexpr DraftingToolSpec kDraftingTools[] = {
    {"select_move", "Select / Move", "Se", 0},
    {"point_tool", "Point", "Pt", 1},
    {"text_tool", "Text", "Tx", 1},
    {"line_tool", "Line", "Ln", 2},
    {"polyline_tool", "Polyline", "Py", 2},
    {"spline_tool", "Spline", "Sp", 2},
    {"arrow_tool", "Arrow", "→", 2},
    {"double_arrow_tool", "Double Arrow", "↔", 2},
    {"wall_tool", "Wall", "Wl", 2},
    {"rectangle_tool", "Rectangle", "Rc", 3},
    {"circle_tool", "Circle", "Ci", 4},
    {"ellipse_tool", "Ellipse", "El", 4},
    {"arc_tool", "Arc", "Ar", 5},
    {"regular_polygon_tool", "Polygon", "Pg", 6},
    {"horizontal_guide_tool", "Horizontal Guide", "Gh", 7},
    {"vertical_guide_tool", "Vertical Guide", "Gv", 7},
    {"horizontal_construction_line_tool", "Horizontal Construction Line", "Ch", 8},
    {"vertical_construction_line_tool", "Vertical Construction Line", "Cv", 8},
    {"angled_construction_line_tool", "Angled Construction Line", "Ca", 8},
    {"distance_dimension_tool", "Dimension: Distance", "Dd", 9},
    {"width_dimension_tool", "Dimension: Width", "Dw", 9},
    {"height_dimension_tool", "Dimension: Height", "Dh", 9},
    {"radius_dimension_tool", "Dimension: Radius", "Dr", 9},
    {"diameter_dimension_tool", "Dimension: Diameter", "Di", 9},
    // DR-13: angular dimension — two-line pick, plans via planAngularDimension.
    // The controller arm (DraftingToolKind::AngularDimension + the two-line
    // sequencing capture) is a CROSS-DEPT GAP flagged to edi-drafting. The
    // belt cell and combo entry land here so the chrome is ready when the
    // controller arm ships; clicking the cell sets the tool id without crashing
    // (draftingToolKindFromId returns Unknown → reject on second click).
    {"angular_dimension_tool", "Dimension: Angular", "Da", 9},
};

const DraftingToolSpec *draftingToolSpec(const QString &toolId)
{
    for (const DraftingToolSpec &spec : kDraftingTools) {
        if (toolId == QLatin1String(spec.id)) {
            return &spec;
        }
    }
    return nullptr;
}

// Drawn cell faces, authored as unit-space coordinates ([0,1]^2, y down).
// Each face is a tiny diagram of what the tool MAKES — the name line under
// the carousel teaches the words, so faces only need to be told apart at a
// glance, not read. Geometry-as-data keeps the belt widget drafting-blind.
BeltFace draftingToolFace(const QString &toolId)
{
    using P = QPointF;
    BeltFace face;
    if (toolId == QLatin1String("select_move")) {
        face.polylines = {QPolygonF({P(0.25, 0.05), P(0.25, 0.8), P(0.45, 0.6), P(0.6, 0.95),
                                     P(0.72, 0.88), P(0.57, 0.55), P(0.82, 0.55), P(0.25, 0.05)})};
    } else if (toolId == QLatin1String("point_tool")) {
        face.dots = {P(0.5, 0.5)};
    } else if (toolId == QLatin1String("text_tool")) {
        face.polylines = {QPolygonF({P(0.2, 0.25), P(0.8, 0.25)}), QPolygonF({P(0.5, 0.25), P(0.5, 0.8)})};
    } else if (toolId == QLatin1String("line_tool")) {
        face.polylines = {QPolygonF({P(0.1, 0.9), P(0.9, 0.1)})};
    } else if (toolId == QLatin1String("wall_tool")) {
        // A thick bar along the same diagonal as the line face — the four
        // corners of an oriented band, so the wall reads as a fat line at a
        // glance (BeltFace has no fill primitive, so the band is its closed
        // outline polyline).
        face.polylines = {QPolygonF({P(0.06, 0.78), P(0.78, 0.06), P(0.94, 0.22), P(0.22, 0.94), P(0.06, 0.78)})};
    } else if (toolId == QLatin1String("polyline_tool")) {
        face.polylines = {QPolygonF({P(0.05, 0.85), P(0.35, 0.3), P(0.6, 0.7), P(0.95, 0.15)})};
    } else if (toolId == QLatin1String("spline_tool")) {
        // A smooth S — same span as the polyline face but curved, so the two
        // multi-click tools read as kin yet tell apart at a glance.
        face.polylines = {QPolygonF({P(0.05, 0.8), P(0.2, 0.4), P(0.4, 0.3), P(0.5, 0.5),
                                     P(0.6, 0.7), P(0.8, 0.6), P(0.95, 0.2)})};
    } else if (toolId == QLatin1String("arrow_tool")) {
        face.polylines = {QPolygonF({P(0.1, 0.9), P(0.85, 0.15)}),
                          QPolygonF({P(0.5, 0.15), P(0.85, 0.15), P(0.85, 0.5)})};
    } else if (toolId == QLatin1String("double_arrow_tool")) {
        face.polylines = {QPolygonF({P(0.1, 0.5), P(0.9, 0.5)}),
                          QPolygonF({P(0.25, 0.35), P(0.1, 0.5), P(0.25, 0.65)}),
                          QPolygonF({P(0.75, 0.35), P(0.9, 0.5), P(0.75, 0.65)})};
    } else if (toolId == QLatin1String("rectangle_tool")) {
        face.polylines = {QPolygonF({P(0.15, 0.25), P(0.85, 0.25), P(0.85, 0.75), P(0.15, 0.75), P(0.15, 0.25)})};
    } else if (toolId == QLatin1String("circle_tool")) {
        face.ellipses = {QRectF(0.15, 0.15, 0.7, 0.7)};
    } else if (toolId == QLatin1String("ellipse_tool")) {
        face.ellipses = {QRectF(0.1, 0.3, 0.8, 0.4)};
    } else if (toolId == QLatin1String("arc_tool")) {
        face.polylines = {QPolygonF({P(0.1, 0.9), P(0.13, 0.55), P(0.3, 0.27), P(0.58, 0.12), P(0.9, 0.1)})};
    } else if (toolId == QLatin1String("regular_polygon_tool")) {
        face.polylines = {QPolygonF({P(0.5, 0.08), P(0.9, 0.4), P(0.74, 0.9), P(0.26, 0.9), P(0.1, 0.4), P(0.5, 0.08)})};
    } else if (toolId == QLatin1String("horizontal_guide_tool")) {
        face.polylines = {QPolygonF({P(0.05, 0.5), P(0.95, 0.5)}), QPolygonF({P(0.5, 0.3), P(0.5, 0.7)})};
    } else if (toolId == QLatin1String("vertical_guide_tool")) {
        face.polylines = {QPolygonF({P(0.5, 0.05), P(0.5, 0.95)}), QPolygonF({P(0.3, 0.5), P(0.7, 0.5)})};
    } else if (toolId == QLatin1String("horizontal_construction_line_tool")) {
        face.polylines = {QPolygonF({P(0.05, 0.35), P(0.95, 0.35)}), QPolygonF({P(0.05, 0.65), P(0.95, 0.65)})};
    } else if (toolId == QLatin1String("vertical_construction_line_tool")) {
        face.polylines = {QPolygonF({P(0.35, 0.05), P(0.35, 0.95)}), QPolygonF({P(0.65, 0.05), P(0.65, 0.95)})};
    } else if (toolId == QLatin1String("angled_construction_line_tool")) {
        face.polylines = {QPolygonF({P(0.05, 0.75), P(0.75, 0.05)}), QPolygonF({P(0.25, 0.95), P(0.95, 0.25)})};
    } else if (toolId == QLatin1String("distance_dimension_tool")) {
        face.polylines = {QPolygonF({P(0.1, 0.5), P(0.9, 0.5)}),
                          QPolygonF({P(0.1, 0.3), P(0.1, 0.7)}), QPolygonF({P(0.9, 0.3), P(0.9, 0.7)})};
    } else if (toolId == QLatin1String("width_dimension_tool")) {
        face.polylines = {QPolygonF({P(0.2, 0.1), P(0.8, 0.1), P(0.8, 0.4), P(0.2, 0.4), P(0.2, 0.1)}),
                          QPolygonF({P(0.2, 0.75), P(0.8, 0.75)}),
                          QPolygonF({P(0.2, 0.6), P(0.2, 0.9)}), QPolygonF({P(0.8, 0.6), P(0.8, 0.9)})};
    } else if (toolId == QLatin1String("height_dimension_tool")) {
        face.polylines = {QPolygonF({P(0.1, 0.2), P(0.4, 0.2), P(0.4, 0.8), P(0.1, 0.8), P(0.1, 0.2)}),
                          QPolygonF({P(0.75, 0.2), P(0.75, 0.8)}),
                          QPolygonF({P(0.6, 0.2), P(0.9, 0.2)}), QPolygonF({P(0.6, 0.8), P(0.9, 0.8)})};
    } else if (toolId == QLatin1String("radius_dimension_tool")) {
        face.ellipses = {QRectF(0.15, 0.15, 0.7, 0.7)};
        face.polylines = {QPolygonF({P(0.5, 0.5), P(0.82, 0.3)})};
    } else if (toolId == QLatin1String("diameter_dimension_tool")) {
        face.ellipses = {QRectF(0.15, 0.15, 0.7, 0.7)};
        face.polylines = {QPolygonF({P(0.18, 0.7), P(0.82, 0.3)})};
    } else if (toolId == QLatin1String("angular_dimension_tool")) {
        // Two lines meeting at a vertex near the bottom-left; a small arc
        // across the opening angle tells apart from the linear siblings.
        face.polylines = {
            QPolygonF({P(0.15, 0.85), P(0.85, 0.15)}), // ray 1 (diagonal)
            QPolygonF({P(0.15, 0.85), P(0.85, 0.65)}), // ray 2 (shallower)
            // Arc approximation (3 segments) spanning the angle between the rays:
            QPolygonF({P(0.38, 0.62), P(0.44, 0.53), P(0.53, 0.50)}),
        };
    }
    return face; // unknown tools return an empty face -> text fallback
}

BeltItem beltItemForTool(const QString &toolId)
{
    if (toolId.isEmpty()) {
        return {}; // an empty slot stays an empty cell
    }
    const DraftingToolSpec *spec = draftingToolSpec(toolId);
    if (spec == nullptr) {
        // A layout file can name a tool this build doesn't have. Render the
        // id behind a "?" instead of a silent blank, so the user sees what
        // the file asked for.
        return {toolId, QStringLiteral("?"), toolId, {}};
    }
    return {toolId, QLatin1String(spec->glyph), QLatin1String(spec->label), draftingToolFace(toolId)};
}

// Content box for a foldable inspector section: add widgets to .layout,
// then hand .box to makeCollapsibleSection. Margins stay zero so folded
// sections cost no whitespace.
struct FoldBox {
    QWidget *box = nullptr;
    QVBoxLayout *layout = nullptr;
};

FoldBox makeFoldBox()
{
    FoldBox fold;
    fold.box = new QWidget;
    fold.layout = new QVBoxLayout(fold.box);
    fold.layout->setContentsMargins(0, 0, 0, 0);
    fold.layout->setSpacing(4);
    return fold;
}

// A 2-column grid of buttons replaces the old full-width stacks — half the
// vertical cost for the same reach.
QWidget *makeButtonGrid(const QVector<QPushButton *> &buttons)
{
    auto *panel = new QWidget;
    auto *grid = new QGridLayout(panel);
    grid->setContentsMargins(0, 0, 0, 0);
    grid->setHorizontalSpacing(4);
    grid->setVerticalSpacing(2);
    for (int i = 0; i < buttons.size(); ++i) {
        grid->addWidget(buttons[i], i / 2, i % 2);
    }
    return panel;
}

} // namespace

DraftingFeature::DraftingFeature(DrawingDocumentController *controller, ShellActions actions, QObject *parent)
    : QObject(parent)
    , m_controller(controller)
    , m_actions(std::move(actions))
{
    connect(m_controller, &DrawingDocumentController::modelChanged, this, &DraftingFeature::refreshInspector);
    // Mouse movement updates only the pointer readouts — rebuilding the
    // whole inspector (and its object list) per move was a dominant lag.
    connect(m_controller, &DrawingDocumentController::pointerChanged, this, &DraftingFeature::refreshPointerReadouts);
}

BeltLayout DraftingFeature::beltLayoutForTools(const QStringList &enabledIds)
{
    BeltLayout belt;
    // One row per tool (ten tools today); six columns leave room for the
    // sub-features the feature backlog adds (polyline/arrow on the line row,
    // rectangle variants). Empty cells cost nothing — the cross renders and
    // navigates occupancy, never the raw rectangle.
    belt.rows = 10;
    belt.columns = 6;
    belt.itemIds.assign(static_cast<std::size_t>(belt.rows) * static_cast<std::size_t>(belt.columns), QString());
    // Fill each enabled tool into the next free column of its row. Derived
    // from the spec table, not a second hand-written grid; a tool left off
    // the checklist simply never lands, and its row stays empty (the
    // carousel skips empty rows by design).
    std::vector<int> nextColumn(static_cast<std::size_t>(belt.rows), 0);
    for (const DraftingToolSpec &spec : kDraftingTools) {
        if (!enabledIds.contains(QLatin1String(spec.id))) {
            continue;
        }
        const int row = spec.beltRow;
        if (row < 0 || row >= belt.rows || nextColumn[row] >= belt.columns) {
            continue; // a row overflow drops the tool rather than corrupting a neighbour row
        }
        belt.itemIds[static_cast<std::size_t>(row) * belt.columns + nextColumn[row]] = QLatin1String(spec.id);
        ++nextColumn[row];
    }
    return belt;
}

BeltLayout DraftingFeature::defaultBeltLayout()
{
    QStringList all;
    for (const DraftingToolSpec &spec : kDraftingTools) {
        all.push_back(QLatin1String(spec.id));
    }
    return beltLayoutForTools(all);
}

QVector<QPair<QString, QString>> DraftingFeature::toolInventory()
{
    QVector<QPair<QString, QString>> inventory;
    for (const DraftingToolSpec &spec : kDraftingTools) {
        inventory.push_back({QLatin1String(spec.id), QLatin1String(spec.label)});
    }
    return inventory;
}

void DraftingFeature::refreshBelt(const BeltLayout &belt)
{
    if (m_beltWidget == nullptr) {
        return;
    }
    m_beltWidget->setGridSize(belt.rows, belt.columns);
    QVector<BeltItem> items;
    items.reserve(static_cast<int>(belt.itemIds.size()));
    for (const QString &toolId : belt.itemIds) {
        items.push_back(beltItemForTool(toolId));
    }
    m_beltWidget->setItems(items);
    m_beltWidget->setPinnedRows(belt.pinnedRows); // restore frozen quick-bars with the arrangement
    const int activeIndex = m_beltWidget->indexOfItem(m_controller->selectedToolId());
    if (activeIndex >= 0) {
        m_beltWidget->setActiveIndex(activeIndex);
    }
}

std::vector<edi::shell::FeaturePaletteSpec> DraftingFeature::buildPalettes()
{
    // F4: the belt floats over the canvas ("tools float"). The arrangement
    // is workspace data (the shell supplies it via callable); this feature
    // only dresses the ids with glyphs/tooltips from its tool table.
    m_beltWidget = new BeltCrossWidget;
    refreshBelt(m_actions.beltLayout ? m_actions.beltLayout() : defaultBeltLayout());
    connect(m_beltWidget, &BeltCrossWidget::selected, m_controller, [this](const QString &toolId) {
        m_controller->setSelectedToolId(toolId);
    });

    // C3 block palette (the "flash sheet"). Minimal, unstyled wiring — the shell
    // frames it in a FloatingPalette like the belt; the look is the user's to
    // polish later. A name field + a save action turn the current selection into
    // a block; the list stamps a block by id when a row is clicked (arming a
    // pick-a-point capture, the radial-array/fillet idiom).
    auto *blockPanel = new QWidget;
    blockPanel->setObjectName(QStringLiteral("blockPalette"));
    auto *blockLayout = new QVBoxLayout(blockPanel);
    clearLayoutMargins(blockLayout);
    blockLayout->setSpacing(6);

    m_blockNameField = new QLineEdit;
    m_blockNameField->setObjectName(QStringLiteral("blockNameField"));
    m_blockNameField->setPlaceholderText(QStringLiteral("Block name"));
    blockLayout->addWidget(m_blockNameField);

    auto *saveBlockButton = new QPushButton(QStringLiteral("Save selection as block"));
    saveBlockButton->setObjectName(QStringLiteral("saveBlockButton"));
    connect(saveBlockButton, &QPushButton::clicked, m_controller, [this]() {
        QString name = m_blockNameField->text().trimmed();
        if (name.isEmpty()) {
            name = QStringLiteral("block"); // a blank name still saves something usable
        }
        m_controller->defineBlockFromSelection(name);
    });
    blockLayout->addWidget(saveBlockButton);

    m_blockList = new QListWidget;
    m_blockList->setObjectName(QStringLiteral("blockList"));
    // A sane default height so an empty library is a small box, not a giant black
    // rectangle over the canvas (mirrors the object list's fixed height). Visual
    // polish/placement beyond this is the user's to restyle.
    m_blockList->setMaximumHeight(160);
    connect(m_blockList, &QListWidget::itemClicked, m_controller, [this](QListWidgetItem *item) {
        m_controller->beginBlockInstancePick(item->data(Qt::UserRole).toString());
    });
    blockLayout->addWidget(m_blockList);
    refreshBlockPalette();

    // DM-14 placement transform: rotation + scale spins directly under the block
    // list (the user-settled fork: Left "Blocks" palette, co-located with the
    // click-to-stamp list, NOT the Right inspector). They are the placement
    // parameters consumed by the next placeBlockInstance — the author dials the
    // transform, then clicks a block row to stamp it rotated/scaled. The
    // fillet-radius-spin pattern verbatim: value set from the controller at build,
    // pushed back on the USER valueChanged signal. Defaults 0 deg / 1.0 preserve
    // today's identity placement exactly.
    {
        auto *row = new QWidget;
        auto *rowLayout = new QHBoxLayout(row);
        clearLayoutMargins(rowLayout);
        rowLayout->setSpacing(6);
        rowLayout->addWidget(new QLabel(QStringLiteral("Rotation")));
        m_blockRotationSpin = new QDoubleSpinBox;
        m_blockRotationSpin->setObjectName(QStringLiteral("blockRotationSpin"));
        m_blockRotationSpin->setDecimals(1);
        m_blockRotationSpin->setSingleStep(1.0);
        m_blockRotationSpin->setRange(-360.0, 360.0);
        m_blockRotationSpin->setValue(m_controller->blockPlacementRotation());
        connect(m_blockRotationSpin, &QDoubleSpinBox::valueChanged, m_controller, [this](double deg) {
            m_controller->setBlockPlacementRotation(deg);
        });
        rowLayout->addWidget(m_blockRotationSpin, 1);
        blockLayout->addWidget(row);
    }
    {
        auto *row = new QWidget;
        auto *rowLayout = new QHBoxLayout(row);
        clearLayoutMargins(rowLayout);
        rowLayout->setSpacing(6);
        rowLayout->addWidget(new QLabel(QStringLiteral("Scale")));
        m_blockScaleSpin = new QDoubleSpinBox;
        m_blockScaleSpin->setObjectName(QStringLiteral("blockScaleSpin"));
        m_blockScaleSpin->setDecimals(3);
        m_blockScaleSpin->setSingleStep(0.1);
        m_blockScaleSpin->setRange(0.01, 100.0);
        m_blockScaleSpin->setValue(m_controller->blockPlacementScale());
        connect(m_blockScaleSpin, &QDoubleSpinBox::valueChanged, m_controller, [this](double factor) {
            m_controller->setBlockPlacementScale(factor);
        });
        rowLayout->addWidget(m_blockScaleSpin, 1);
        blockLayout->addWidget(row);
    }

    // M8 motif palette: a sibling of the Blocks palette, directly below it.
    // Pattern: same section/label/list treatment as the block palette —
    // one "Define from selection" button (prompts via QInputDialog) + a list
    // whose rows arm beginMotifPlacement on activation. The list is seeded
    // before the activation signal is connected so the seed loop cannot fire
    // a placement (read-only projection; signal-safety rule: seed before
    // connect). Name stored by the item text — motifs are name-keyed (no id).
    auto *motifPanel = new QWidget;
    motifPanel->setObjectName(QStringLiteral("motifPalette"));
    auto *motifLayout = new QVBoxLayout(motifPanel);
    clearLayoutMargins(motifLayout);
    motifLayout->setSpacing(6);

    auto *defineMotifButton = new QPushButton(QStringLiteral("Define from selection"));
    defineMotifButton->setObjectName(QStringLiteral("defineMotifButton"));
    // Clicked (USER signal) → prompt for a name → defineMotifFromSelection.
    // QInputDialog::getText is the shell-standard one-line prompt: same modal
    // flow a user expects, no new dialog class, no stale state between clicks.
    connect(defineMotifButton, &QPushButton::clicked, m_controller, [this]() {
        bool ok = false;
        const QString name = QInputDialog::getText(
            nullptr,
            QStringLiteral("Define Motif"),
            QStringLiteral("Motif name:"),
            QLineEdit::Normal,
            QString(),
            &ok);
        if (ok && !name.trimmed().isEmpty()) {
            m_controller->defineMotifFromSelection(name.trimmed());
        }
    });
    motifLayout->addWidget(defineMotifButton);

    m_motifList = new QListWidget;
    m_motifList->setObjectName(QStringLiteral("motifList"));
    // Same sane default height as the block list — prevents a giant black
    // rectangle over the canvas when the library is empty.
    m_motifList->setMaximumHeight(160);
    // Seed the list BEFORE connecting the activation signal so the
    // programmatic clear+populate below cannot fire beginMotifPlacement.
    // The signal is connected ONCE at build time; at fire time we read the
    // item's text (the name) — no per-item connection that could leak across
    // a refresh (signal-safety: single connect, read name at fire time).
    refreshMotifPalette();
    connect(m_motifList, &QListWidget::itemActivated, m_controller, [this](QListWidgetItem *item) {
        m_controller->beginMotifPlacement(item->text());
    });
    motifLayout->addWidget(m_motifList);

    return {
        {QStringLiteral("tool_belt"), QStringLiteral("Tools"), m_beltWidget},
        {QStringLiteral("block_palette"), QStringLiteral("Blocks"), blockPanel},
        {QStringLiteral("motif_palette"), QStringLiteral("Motifs"), motifPanel},
    };
}

void DraftingFeature::refreshBlockPalette()
{
    if (m_blockList == nullptr) {
        return; // palette not built yet (refreshInspector can run first)
    }
    // Rebuild the block list as a projection of the document's blocks — the same
    // recompute-whole discipline as the object list. The blocker keeps a
    // programmatic rebuild from re-emitting itemClicked.
    const QSignalBlocker blocker(*m_blockList);
    m_blockList->clear();
    for (const edi::drafting::DraftingBlock &block : m_controller->draftingDocument().blocks) {
        const QString id = QString::fromStdString(block.id);
        const QString name = QString::fromStdString(block.name);
        auto *item = new QListWidgetItem(name.isEmpty() ? id : name);
        item->setData(Qt::UserRole, id); // the row carries the block id to stamp
        m_blockList->addItem(item);
    }
}

void DraftingFeature::refreshMotifPalette()
{
    if (m_motifList == nullptr) {
        return; // palette not built yet (refreshInspector can run first)
    }
    // Rebuild the motif list as a read-only projection of the document's motifs.
    // QSignalBlocker prevents the programmatic clear+add from re-firing
    // itemActivated and accidentally arming a placement capture — the same
    // write-back-loop guard the block palette and the object list use.
    // Motifs are name-keyed (no id field): the row TEXT is the lookup key
    // for beginMotifPlacement, and the QSignalBlocker guarantees this loop
    // never calls it (no mutation path through a model-changed refresh).
    const QSignalBlocker blocker(*m_motifList);
    m_motifList->clear();
    for (const edi::drafting::DraftingMotif &motif : m_controller->draftingDocument().motifs) {
        const QString name = QString::fromStdString(motif.name);
        m_motifList->addItem(name);
    }
}

QWidget *DraftingFeature::buildPanel(ShellSlot slot)
{
    switch (slot) {
    case ShellSlot::Main: return buildWorkspaceColumn();
    case ShellSlot::Left: return buildLeftPanel();
    case ShellSlot::Right: return buildRightPanel();
    case ShellSlot::Bottom: return buildBottomPanel();
    }
    return nullptr;
}

std::vector<edi::shell::FeatureChromePanelSpec> DraftingFeature::buildChromePanels()
{
    // The snap/grid settings, relocated from the left panel to a top-chrome
    // popup (user direction 2026-06-10): they configure how input behaves,
    // not what the document contains, so they belong with the window's
    // controls rather than in the navigation panel. Widgets and wiring are
    // unchanged — refreshInspector still syncs these members on every model
    // change, wherever they happen to be hosted.
    auto *panel = new QWidget;
    panel->setObjectName(QStringLiteral("snapControls"));
    auto *layout = new QVBoxLayout(panel);
    clearLayoutMargins(layout);
    layout->setSpacing(8);

    m_gridPreset = makeDataCombo(QStringLiteral("controlInput"), {
        {QStringLiteral("Square art board"), QStringLiteral("square_art_board")},
        {QStringLiteral("Letter"), QStringLiteral("letter")},
        {QStringLiteral("A4"), QStringLiteral("a4")},
        {QStringLiteral("Custom"), QStringLiteral("custom")},
    }, [this](const QString &presetId) {
        m_controller->setGridPresetId(presetId);
    }, m_controller->gridPresetId());
    layout->addWidget(m_gridPreset);

    auto *gridSettingsLayout = new QGridLayout;
    gridSettingsLayout->setContentsMargins(0, 0, 0, 0);
    gridSettingsLayout->setHorizontalSpacing(6);
    gridSettingsLayout->setVerticalSpacing(6);

    m_gridUnit = makeDataCombo(QStringLiteral("controlInput"), {
        {QStringLiteral("cu"), QStringLiteral("canvas_unit")},
        {QStringLiteral("mm"), QStringLiteral("millimeter")},
        {QStringLiteral("cm"), QStringLiteral("centimeter")},
        {QStringLiteral("in"), QStringLiteral("inch")},
        {QStringLiteral("ft"), QStringLiteral("foot")},
    }, [this](const QString &unitId) {
        m_controller->setGridUnitId(unitId);
    });

    auto makeGridSpin = []() {
        auto *spin = new QDoubleSpinBox;
        spin->setObjectName(QStringLiteral("geometryField"));
        spin->setDecimals(4);
        spin->setSingleStep(0.25);
        spin->setRange(0.0, 100000.0);
        return spin;
    };
    m_gridWidth = makeGridSpin();
    m_gridHeight = makeGridSpin();
    m_gridMarginLeft = makeGridSpin();
    m_gridMarginTop = makeGridSpin();
    m_gridMarginRight = makeGridSpin();
    m_gridMarginBottom = makeGridSpin();
    m_gridMinorStep = makeGridSpin();
    m_gridMinorStep->setMinimum(0.0001);
    m_gridMajorEvery = new QSpinBox;
    m_gridMajorEvery->setObjectName(QStringLiteral("geometryField"));
    m_gridMajorEvery->setRange(1, 1000);
    m_gridVisible = makeToggle(QStringLiteral("gridVisibleCheckbox"), QStringLiteral("Visible"), [this](bool visible) {
        m_controller->setGridVisible(visible);
    });

    gridSettingsLayout->addWidget(new QLabel(QStringLiteral("Unit")), 0, 0);
    gridSettingsLayout->addWidget(m_gridUnit, 0, 1);
    gridSettingsLayout->addWidget(new QLabel(QStringLiteral("W")), 1, 0);
    gridSettingsLayout->addWidget(m_gridWidth, 1, 1);
    gridSettingsLayout->addWidget(new QLabel(QStringLiteral("H")), 2, 0);
    gridSettingsLayout->addWidget(m_gridHeight, 2, 1);
    gridSettingsLayout->addWidget(new QLabel(QStringLiteral("ML")), 3, 0);
    gridSettingsLayout->addWidget(m_gridMarginLeft, 3, 1);
    gridSettingsLayout->addWidget(new QLabel(QStringLiteral("MT")), 4, 0);
    gridSettingsLayout->addWidget(m_gridMarginTop, 4, 1);
    gridSettingsLayout->addWidget(new QLabel(QStringLiteral("MR")), 5, 0);
    gridSettingsLayout->addWidget(m_gridMarginRight, 5, 1);
    gridSettingsLayout->addWidget(new QLabel(QStringLiteral("MB")), 6, 0);
    gridSettingsLayout->addWidget(m_gridMarginBottom, 6, 1);
    gridSettingsLayout->addWidget(new QLabel(QStringLiteral("Step")), 7, 0);
    gridSettingsLayout->addWidget(m_gridMinorStep, 7, 1);
    gridSettingsLayout->addWidget(new QLabel(QStringLiteral("Major")), 8, 0);
    gridSettingsLayout->addWidget(m_gridMajorEvery, 8, 1);
    gridSettingsLayout->addWidget(m_gridVisible, 9, 0, 1, 2);
    layout->addLayout(gridSettingsLayout);

    m_gridSnap = makeToggle(QStringLiteral("snapToggle"), QStringLiteral("Grid snap"), [this](bool enabled) {
        m_controller->setGridSnapEnabled(enabled);
    }, m_controller->gridSnapEnabled());
    m_objectSnap = makeToggle(QStringLiteral("snapToggle"), QStringLiteral("Object snap"), [this](bool enabled) {
        m_controller->setObjectSnapEnabled(enabled);
    }, m_controller->objectSnapEnabled());
    m_endpointSnap = makeToggle(QStringLiteral("snapToggle"), QStringLiteral("Endpoint"), [this](bool enabled) {
        m_controller->setEndpointSnapEnabled(enabled);
    }, m_controller->endpointSnapEnabled());
    m_vertexSnap = makeToggle(QStringLiteral("snapToggle"), QStringLiteral("Vertex"), [this](bool enabled) {
        m_controller->setVertexSnapEnabled(enabled);
    }, m_controller->vertexSnapEnabled());
    m_midpointSnap = makeToggle(QStringLiteral("snapToggle"), QStringLiteral("Midpoint"), [this](bool enabled) {
        m_controller->setMidpointSnapEnabled(enabled);
    }, m_controller->midpointSnapEnabled());
    m_centerSnap = makeToggle(QStringLiteral("snapToggle"), QStringLiteral("Center"), [this](bool enabled) {
        m_controller->setCenterSnapEnabled(enabled);
    }, m_controller->centerSnapEnabled());
    m_intersectionSnap = makeToggle(QStringLiteral("snapToggle"), QStringLiteral("Intersection"), [this](bool enabled) {
        m_controller->setIntersectionSnapEnabled(enabled);
    }, m_controller->intersectionSnapEnabled());
    m_guideSnap = makeToggle(QStringLiteral("snapToggle"), QStringLiteral("Guide"), [this](bool enabled) {
        m_controller->setGuideSnapEnabled(enabled);
    }, m_controller->guideSnapEnabled());
    m_guideMoveSnap = makeToggle(QStringLiteral("snapToggle"), QStringLiteral("Guide move snap"), [this](bool enabled) {
        m_controller->setGuideMoveSnapEnabled(enabled);
    }, m_controller->guideMoveSnapEnabled());
    m_objectPrioritySnap = makeToggle(QStringLiteral("snapToggle"), QStringLiteral("Object before grid"), [this](bool enabled) {
        m_controller->setObjectSnapPriorityBeforeGrid(enabled);
    }, m_controller->objectSnapPriorityBeforeGrid());
    m_objectTolerance = makeDataCombo(QStringLiteral("controlInput"), {
        {QStringLiteral("Tight tolerance"), QStringLiteral("tight")},
        {QStringLiteral("Normal tolerance"), QStringLiteral("normal")},
        {QStringLiteral("Loose tolerance"), QStringLiteral("loose")},
    }, [this](const QString &presetId) {
        m_controller->setObjectSnapTolerancePreset(presetId);
    }, m_controller->objectSnapTolerancePresetId());
    layout->addWidget(m_gridSnap);
    layout->addWidget(m_objectSnap);
    layout->addWidget(m_endpointSnap);
    layout->addWidget(m_vertexSnap);
    layout->addWidget(m_midpointSnap);
    layout->addWidget(m_centerSnap);
    layout->addWidget(m_intersectionSnap);
    layout->addWidget(m_guideSnap);
    layout->addWidget(m_guideMoveSnap);
    layout->addWidget(m_objectPrioritySnap);
    layout->addWidget(m_objectTolerance);

    connect(m_gridWidth, &QDoubleSpinBox::editingFinished, m_controller, [this]() {
        m_controller->setGridSize(m_gridWidth->value(), m_gridHeight->value());
    });
    connect(m_gridHeight, &QDoubleSpinBox::editingFinished, m_controller, [this]() {
        m_controller->setGridSize(m_gridWidth->value(), m_gridHeight->value());
    });
    const auto applyGridMargins = [this]() {
        m_controller->setGridMargins(
            m_gridMarginLeft->value(),
            m_gridMarginTop->value(),
            m_gridMarginRight->value(),
            m_gridMarginBottom->value());
    };
    connect(m_gridMarginLeft, &QDoubleSpinBox::editingFinished, m_controller, applyGridMargins);
    connect(m_gridMarginTop, &QDoubleSpinBox::editingFinished, m_controller, applyGridMargins);
    connect(m_gridMarginRight, &QDoubleSpinBox::editingFinished, m_controller, applyGridMargins);
    connect(m_gridMarginBottom, &QDoubleSpinBox::editingFinished, m_controller, applyGridMargins);
    connect(m_gridMinorStep, &QDoubleSpinBox::editingFinished, m_controller, [this]() {
        m_controller->setGridStep(m_gridMinorStep->value());
    });
    connect(m_gridMajorEvery, &QSpinBox::editingFinished, m_controller, [this]() {
        m_controller->setGridMajorLineEvery(m_gridMajorEvery->value());
    });

    return {{QStringLiteral("snap"), QStringLiteral("Snap"), panel}};
}

QWidget *DraftingFeature::buildLeftPanel()
{
    const PanelSpec leftSpec = panelSpec(ShellSlot::Left);
    auto [panel, layout] = makeScrollablePanel(QStringLiteral("leftPanel"), leftSpec.minSize, leftSpec.maxSize);
    // Modular panels: this panel hosts whatever groups the user assigned
    // "left" (default: the object list). Same placement pass as the right
    // panel — left and right share formatting by construction.
    ensureInspectorGroupsBuilt();
    placePanelGroups(QStringLiteral("left"), layout);
    layout->addStretch(1);
    return panel;
}

QWidget *DraftingFeature::buildWorkspaceColumn()
{
    // The main slot is the grid and nothing else (user decision 2026-06-10):
    // no header, no title — "the thing tells me what it is by the shape of
    // it". The status line lives in the shell chrome, fed via ShellActions.
    m_canvas = new DrawingCanvasWidget(m_controller);
    m_canvas->setObjectName(QStringLiteral("drawingCanvas"));
    // Zoom changes republish the status line (the % readout) without a
    // document mutation — same light-touch path as the pointer readouts.
    connect(m_canvas, &DrawingCanvasWidget::zoomChanged, this, &DraftingFeature::publishStatus);
    // Window-scope view shortcuts: the canvas key handler needs click-focus,
    // but Cmd+0 / Cmd+= / Cmd+- should work from anywhere in the window
    // (review find). The canvas keyPress path stays for focused use.
    const auto addViewShortcut = [this](const QKeySequence &keys, const std::function<void()> &action) {
        auto *shortcut = new QShortcut(keys, m_canvas);
        shortcut->setContext(Qt::WindowShortcut);
        connect(shortcut, &QShortcut::activated, m_canvas, action);
    };
    addViewShortcut(QKeySequence(Qt::CTRL | Qt::Key_0), [this]() { m_canvas->resetView(); });
    addViewShortcut(QKeySequence::ZoomIn, [this]() { m_canvas->zoomAtCenter(1.25); });
    addViewShortcut(QKeySequence::ZoomOut, [this]() { m_canvas->zoomAtCenter(0.8); });
    return m_canvas;
}

QWidget *DraftingFeature::buildRightPanel()
{
    const PanelSpec rightSpec = panelSpec(ShellSlot::Right);
    auto [panel, layout] = makeScrollablePanel(QStringLiteral("rightPanel"), rightSpec.minSize, rightSpec.maxSize);
    ensureInspectorGroupsBuilt();
    placePanelGroups(QStringLiteral("right"), layout);
    layout->addStretch(1);
    return panel;
}

void DraftingFeature::ensureInspectorGroupsBuilt()
{
    if (!m_inspectorGroups.isEmpty()) {
        return; // built once per feature instance; panels only place them
    }

    // F1: the document as a browsable list — group "object_list", default
    // home the left panel. Built first so it tops whichever panel hosts it.
    QVBoxLayout *objectListGroup = beginInspectorGroup(QStringLiteral("object_list"));
    m_objectList = new QListWidget;
    m_objectList->setObjectName(QStringLiteral("objectList"));
    m_objectList->setFixedHeight(140);
    connect(m_objectList, &QListWidget::itemClicked, this, [this](QListWidgetItem *item) {
        m_controller->selectObjectById(item->data(Qt::UserRole).toString());
    });
    objectListGroup->addWidget(m_objectList);
    m_objectListEmpty = new QLabel(QStringLiteral("No objects yet"));
    m_objectListEmpty->setObjectName(QStringLiteral("objectListEmpty"));
    objectListGroup->addWidget(m_objectListEmpty);

    // F2: the inspector is a context-keyed stack (planDraftingInspector).
    // Every group is built exactly once, in a fixed order chosen so every
    // context reads top-to-bottom; refreshInspector only toggles visibility.
    // The obvious alternative — rebuild the panel per context — would retire
    // and recreate live editors on each selection change; the geometry editor
    // keeps that rebuild pattern only because its fields depend on the
    // selected object, not just on the context.

    // Tool options ride above everything: creation auto-selects the new
    // object, so a draw loop shows them together with the object's properties.
    QVBoxLayout *group = beginInspectorGroup(QStringLiteral("tool_polygon"));
    group->addWidget(makeSectionLabel(QStringLiteral("Polygon Tool")));
    {
        auto *sidesRow = new QWidget;
        auto *sidesLayout = new QHBoxLayout(sidesRow);
        clearLayoutMargins(sidesLayout);
        sidesLayout->setSpacing(6);
        sidesLayout->addWidget(new QLabel(QStringLiteral("Sides")));
        m_polygonSidesSpin = new QSpinBox;
        m_polygonSidesSpin->setObjectName(QStringLiteral("polygonSidesSpin"));
        m_polygonSidesSpin->setRange(3, 24);
        m_polygonSidesSpin->setValue(m_controller->polygonSides());
        connect(m_polygonSidesSpin, &QSpinBox::valueChanged, m_controller, [this](int sides) {
            m_controller->setPolygonSides(sides);
        });
        sidesLayout->addWidget(m_polygonSidesSpin, 1);
        group->addWidget(sidesRow);
    }

    // N4: rectangle variant options + aspect-lock. Like the polygon group,
    // these mirror controller tool-option state (set at build, pushed back on
    // edit); they are tool modes, not per-object fields.
    group = beginInspectorGroup(QStringLiteral("tool_rectangle"));
    group->addWidget(makeSectionLabel(QStringLiteral("Rectangle Tool")));
    {
        const auto addRectSpin = [&](const QString &label, const QString &name, double value,
                                     const std::function<void(double)> &onChange) {
            auto *row = new QWidget;
            auto *rowLayout = new QHBoxLayout(row);
            clearLayoutMargins(rowLayout);
            rowLayout->setSpacing(6);
            rowLayout->addWidget(new QLabel(label));
            auto *spin = new QDoubleSpinBox;
            spin->setObjectName(name);
            spin->setDecimals(3);
            spin->setSingleStep(0.01);
            spin->setRange(0.0, 1.0);
            spin->setValue(value);
            connect(spin, &QDoubleSpinBox::valueChanged, m_controller, onChange);
            rowLayout->addWidget(spin, 1);
            group->addWidget(row);
            return spin;
        };
        addRectSpin(QStringLiteral("Radius"), QStringLiteral("rectCornerRadiusSpin"),
                    m_controller->rectCornerRadius(), [this](double r) { m_controller->setRectCornerRadius(r); });
        addRectSpin(QStringLiteral("Inset"), QStringLiteral("rectInsetSpin"),
                    m_controller->rectInset(), [this](double i) { m_controller->setRectInset(i); });
        m_aspectLockToggle = makeToggle(QStringLiteral("aspectLockCheckbox"), QStringLiteral("Lock aspect ratio"),
            [this](bool on) { m_controller->setAspectLockEnabled(on); }, m_controller->aspectLockEnabled());
        group->addWidget(m_aspectLockToggle);
    }

    // #30: shared radius option for the radius-from-gesture tools (circle,
    // arc, regular polygon). One group several tools show — option groups
    // are per-concern, not per-tool (see draftingToolOptionsGroups).
    group = beginInspectorGroup(QStringLiteral("tool_radius"));
    group->addWidget(makeSectionLabel(QStringLiteral("Radius")));
    {
        auto *row = new QWidget;
        auto *rowLayout = new QHBoxLayout(row);
        clearLayoutMargins(rowLayout);
        rowLayout->setSpacing(6);
        rowLayout->addWidget(new QLabel(QStringLiteral("Radius (0 = drag)")));
        auto *spin = new QDoubleSpinBox;
        spin->setObjectName(QStringLiteral("fixedRadiusSpin"));
        spin->setDecimals(3);
        spin->setSingleStep(0.01);
        spin->setRange(0.0, 1.0);
        spin->setValue(m_controller->fixedRadius());
        connect(spin, &QDoubleSpinBox::valueChanged, m_controller, [this](double r) {
            m_controller->setFixedRadius(r);
        });
        rowLayout->addWidget(spin, 1);
        group->addWidget(row);
    }

    group = beginInspectorGroup(QStringLiteral("empty_state"));
    {
        auto *emptyState = makeValueLabel(QStringLiteral("Nothing selected."));
        emptyState->setObjectName(QStringLiteral("inspectorEmptyState"));
        group->addWidget(emptyState);
    }

    group = beginInspectorGroup(QStringLiteral("selection_summary"));
    group->addWidget(makeSectionLabel(QStringLiteral("Selection")));
    m_selectedValue = makeValueLabel();
    group->addWidget(m_selectedValue);

    // De-bloat (user feedback): heavy sections fold. The identity labels
    // stay open; the readout pile and the placement buttons start folded —
    // reachable, not shouting. Open/collapsed defaults are the data here.
    {
        FoldBox details = makeFoldBox();
        m_objectKindValue = makeValueLabel();
        m_objectBoundsValue = makeValueLabel();
        m_objectGeometryValue = makeValueLabel();
        m_objectLayerValue = makeValueLabel();
        m_objectMeasurementValue = makeValueLabel();
        m_objectPlotSafetyValue = makeValueLabel();
        m_selectionPlotBoundsValue = makeValueLabel();
        details.layout->addWidget(m_objectKindValue);
        details.layout->addWidget(m_objectBoundsValue);
        details.layout->addWidget(m_objectGeometryValue);
        details.layout->addWidget(m_objectLayerValue);
        details.layout->addWidget(m_objectMeasurementValue);
        details.layout->addWidget(m_objectPlotSafetyValue);
        details.layout->addWidget(m_selectionPlotBoundsValue);
        group->addWidget(makeCollapsibleSection(QStringLiteral("Selected Object"), details.box, true));

        FoldBox place = makeFoldBox();
        place.layout->addWidget(makeConditionalButton(QStringLiteral("fitToDrawableButton"), QStringLiteral("Fit To Drawable"), QStringLiteral("has_selection"), [this]() {
            m_controller->fitSelectionToDrawableBounds();
        }));
        place.layout->addWidget(makeConditionalButton(QStringLiteral("centerInDrawableButton"), QStringLiteral("Center In Drawable"), QStringLiteral("has_selection"), [this]() {
            m_controller->centerSelectionInDrawable();
        }));
        place.layout->addWidget(makeConditionalButton(QStringLiteral("moveToDrawableOriginButton"), QStringLiteral("Move To Drawable Origin"), QStringLiteral("has_selection"), [this]() {
            m_controller->moveSelectionToDrawableOrigin();
        }));
        group->addWidget(makeCollapsibleSection(QStringLiteral("Place In Drawable"), place.box, false));
    }
    group->addWidget(buildObjectFlagControls());
    group->addWidget(makeSectionLabel(QStringLiteral("Object Layer")));
    m_selectedObjectLayer = makeDataCombo(QStringLiteral("selectedObjectLayerCombo"), {}, [this](const QString &layerId) {
        m_controller->moveSelectedObjectToLayer(layerId);
    });
    group->addWidget(m_selectedObjectLayer);

    // N3 semantic/export metadata, folded (reference data, not the first
    // thing reached for on selecting an object).
    {
        FoldBox meta = makeFoldBox();
        m_objectRole = makeDataCombo(QStringLiteral("objectRoleCombo"), {
            {QStringLiteral("Role: none"), QStringLiteral("none")},
            {QStringLiteral("Role: wall"), QStringLiteral("wall")},
            {QStringLiteral("Role: floor"), QStringLiteral("floor")},
            {QStringLiteral("Role: cutout"), QStringLiteral("cutout")},
            {QStringLiteral("Role: collider"), QStringLiteral("collider")},
        }, [this](const QString &roleId) {
            m_controller->setSelectedObjectRole(roleId);
        });
        meta.layout->addWidget(m_objectRole);
        // M1.3: a wall's neutral render type. Enabled only when a wall is
        // selected (refreshed in DraftingFeatureInspector); changes how the band
        // draws, never what it means.
        m_wallType = makeDataCombo(QStringLiteral("wallTypeCombo"), {
            {QStringLiteral("Wall: solid"), QStringLiteral("solid")},
            {QStringLiteral("Wall: door"), QStringLiteral("door")},
            {QStringLiteral("Wall: window"), QStringLiteral("window")},
            {QStringLiteral("Wall: secret"), QStringLiteral("secret")},
        }, [this](const QString &typeId) {
            m_controller->setSelectedWallType(typeId);
        });
        meta.layout->addWidget(m_wallType);
        m_objectMaterial = new QLineEdit;
        m_objectMaterial->setObjectName(QStringLiteral("objectMaterialField"));
        m_objectMaterial->setPlaceholderText(QStringLiteral("Material"));
        connect(m_objectMaterial, &QLineEdit::editingFinished, this, [this]() {
            m_controller->setSelectedObjectMaterial(m_objectMaterial->text());
        });
        meta.layout->addWidget(m_objectMaterial);
        m_objectExportGroup = new QLineEdit;
        m_objectExportGroup->setObjectName(QStringLiteral("objectExportGroupField"));
        m_objectExportGroup->setPlaceholderText(QStringLiteral("Export group"));
        connect(m_objectExportGroup, &QLineEdit::editingFinished, this, [this]() {
            m_controller->setSelectedObjectExportGroup(m_objectExportGroup->text());
        });
        meta.layout->addWidget(m_objectExportGroup);
        m_objectTags = new QLineEdit;
        m_objectTags->setObjectName(QStringLiteral("objectTagsField"));
        m_objectTags->setPlaceholderText(QStringLiteral("Tags (comma-separated)"));
        connect(m_objectTags, &QLineEdit::editingFinished, this, [this]() {
            // The controller owns trimming/blank-dropping; the shell only splits.
            m_controller->setSelectedObjectTags(m_objectTags->text().split(QLatin1Char(',')));
        });
        meta.layout->addWidget(m_objectTags);
        group->addWidget(makeCollapsibleSection(QStringLiteral("Metadata"), meta.box, false));
    }

    // Per-object style: color/width override the layer (empty/zero =
    // inherit), line style is object-only. Free-string hex color — the
    // art-tool door: anything QColor parses is a legal stroke.
    group = beginInspectorGroup(QStringLiteral("style"));
    {
        FoldBox style = makeFoldBox();
        // Text-annotation content: a STRING field (the geometry editor is numeric
        // only). Built here, shown only for text objects — gated in the refresh.
        m_textContentRow = new QWidget;
        auto *textContentLayout = new QHBoxLayout(m_textContentRow);
        textContentLayout->setContentsMargins(0, 0, 0, 0);
        textContentLayout->setSpacing(8);
        auto *textContentLabel = new QLabel(QStringLiteral("Text"));
        textContentLabel->setObjectName(QStringLiteral("fieldLabel"));
        m_textContentField = new QLineEdit;
        m_textContentField->setObjectName(QStringLiteral("textContentField"));
        m_textContentField->setPlaceholderText(QStringLiteral("text"));
        connect(m_textContentField, &QLineEdit::editingFinished, this, [this]() {
            m_controller->setSelectedObjectTextContent(m_textContentField->text());
        });
        textContentLayout->addWidget(textContentLabel);
        textContentLayout->addWidget(m_textContentField, 1);
        style.layout->addWidget(m_textContentRow);

        auto *colorRow = new QWidget;
        auto *colorRowLayout = new QHBoxLayout(colorRow);
        colorRowLayout->setContentsMargins(0, 0, 0, 0);
        colorRowLayout->setSpacing(8);
        auto *colorLabel = new QLabel(QStringLiteral("Color"));
        colorLabel->setObjectName(QStringLiteral("fieldLabel"));
        m_styleColorField = new QLineEdit;
        m_styleColorField->setObjectName(QStringLiteral("styleColorField"));
        m_styleColorField->setPlaceholderText(QStringLiteral("layer"));
        connect(m_styleColorField, &QLineEdit::editingFinished, this, [this]() {
            m_controller->setSelectedObjectStrokeColor(m_styleColorField->text());
        });
        colorRowLayout->addWidget(colorLabel);
        colorRowLayout->addWidget(m_styleColorField, 1);
        style.layout->addWidget(colorRow);

        auto *widthRow = new QWidget;
        auto *widthRowLayout = new QHBoxLayout(widthRow);
        widthRowLayout->setContentsMargins(0, 0, 0, 0);
        widthRowLayout->setSpacing(8);
        auto *widthLabel = new QLabel(QStringLiteral("Width"));
        widthLabel->setObjectName(QStringLiteral("fieldLabel"));
        m_styleWidthSpin = new QDoubleSpinBox;
        m_styleWidthSpin->setObjectName(QStringLiteral("styleWidthSpin"));
        m_styleWidthSpin->setRange(0.0, 50.0);
        m_styleWidthSpin->setDecimals(2);
        m_styleWidthSpin->setSingleStep(0.25);
        m_styleWidthSpin->setSpecialValueText(QStringLiteral("layer")); // 0 = inherit
        connect(m_styleWidthSpin, &QDoubleSpinBox::editingFinished, this, [this]() {
            m_controller->setSelectedObjectStrokeWidth(m_styleWidthSpin->value());
        });
        widthRowLayout->addWidget(widthLabel);
        widthRowLayout->addWidget(m_styleWidthSpin, 1);
        style.layout->addWidget(widthRow);

        auto *opacityRow = new QWidget;
        auto *opacityRowLayout = new QHBoxLayout(opacityRow);
        opacityRowLayout->setContentsMargins(0, 0, 0, 0);
        opacityRowLayout->setSpacing(8);
        auto *opacityLabel = new QLabel(QStringLiteral("Opacity"));
        opacityLabel->setObjectName(QStringLiteral("fieldLabel"));
        m_styleOpacitySpin = new QDoubleSpinBox;
        m_styleOpacitySpin->setObjectName(QStringLiteral("styleOpacitySpin"));
        m_styleOpacitySpin->setRange(0.0, 1.0);
        m_styleOpacitySpin->setDecimals(2);
        m_styleOpacitySpin->setSingleStep(0.05);
        m_styleOpacitySpin->setValue(1.0);
        connect(m_styleOpacitySpin, &QDoubleSpinBox::editingFinished, this, [this]() {
            m_controller->setSelectedObjectStrokeOpacity(m_styleOpacitySpin->value());
        });
        opacityRowLayout->addWidget(opacityLabel);
        opacityRowLayout->addWidget(m_styleOpacitySpin, 1);
        style.layout->addWidget(opacityRow);

        auto *fillColorRow = new QWidget;
        auto *fillColorRowLayout = new QHBoxLayout(fillColorRow);
        fillColorRowLayout->setContentsMargins(0, 0, 0, 0);
        fillColorRowLayout->setSpacing(8);
        auto *fillColorLabel = new QLabel(QStringLiteral("Fill"));
        fillColorLabel->setObjectName(QStringLiteral("fieldLabel"));
        m_styleFillColorField = new QLineEdit;
        m_styleFillColorField->setObjectName(QStringLiteral("styleFillColorField"));
        m_styleFillColorField->setPlaceholderText(QStringLiteral("none"));
        connect(m_styleFillColorField, &QLineEdit::editingFinished, this, [this]() {
            m_controller->setSelectedObjectFillColor(m_styleFillColorField->text());
        });
        fillColorRowLayout->addWidget(fillColorLabel);
        fillColorRowLayout->addWidget(m_styleFillColorField, 1);
        style.layout->addWidget(fillColorRow);

        auto *fillOpacityRow = new QWidget;
        auto *fillOpacityRowLayout = new QHBoxLayout(fillOpacityRow);
        fillOpacityRowLayout->setContentsMargins(0, 0, 0, 0);
        fillOpacityRowLayout->setSpacing(8);
        auto *fillOpacityLabel = new QLabel(QStringLiteral("Fill opacity"));
        fillOpacityLabel->setObjectName(QStringLiteral("fieldLabel"));
        m_styleFillOpacitySpin = new QDoubleSpinBox;
        m_styleFillOpacitySpin->setObjectName(QStringLiteral("styleFillOpacitySpin"));
        m_styleFillOpacitySpin->setRange(0.0, 1.0);
        m_styleFillOpacitySpin->setDecimals(2);
        m_styleFillOpacitySpin->setSingleStep(0.05);
        connect(m_styleFillOpacitySpin, &QDoubleSpinBox::editingFinished, this, [this]() {
            m_controller->setSelectedObjectFillOpacity(m_styleFillOpacitySpin->value());
        });
        fillOpacityRowLayout->addWidget(fillOpacityLabel);
        fillOpacityRowLayout->addWidget(m_styleFillOpacitySpin, 1);
        style.layout->addWidget(fillOpacityRow);

        m_styleLineCombo = makeDataCombo(QStringLiteral("styleLineCombo"), {
            {QStringLiteral("Solid"), QStringLiteral("solid")},
            {QStringLiteral("Dash"), QStringLiteral("dash")},
            {QStringLiteral("Dot"), QStringLiteral("dot")},
        }, [this](const QString &lineStyle) {
            m_controller->setSelectedObjectLineStyle(lineStyle);
        });
        style.layout->addWidget(m_styleLineCombo);
        group->addWidget(makeCollapsibleSection(QStringLiteral("Style"), style.box, true));
    }

    group = beginInspectorGroup(QStringLiteral("geometry"));
    m_geometryEditor = buildGeometryEditor();
    group->addWidget(m_geometryEditor);
    m_geometryEditStatus = makeValueLabel();
    m_geometryEditStatus->setObjectName(QStringLiteral("editErrorLabel"));
    m_geometryEditStatus->setVisible(false);
    group->addWidget(m_geometryEditStatus);

    group = beginInspectorGroup(QStringLiteral("dimension"));
    group->addWidget(makeSectionLabel(QStringLiteral("Dimension")));
    m_dimensionReadout = makeValueLabel(QStringLiteral("Dimension: none"));
    group->addWidget(m_dimensionReadout);
    m_dimensionKind = makeDataCombo(QStringLiteral("dimensionKindCombo"), {
        {QStringLiteral("Distance"), QStringLiteral("distance")},
        {QStringLiteral("Width"), QStringLiteral("width")},
        {QStringLiteral("Height"), QStringLiteral("height")},
        {QStringLiteral("Radius"), QStringLiteral("radius")},
        {QStringLiteral("Diameter"), QStringLiteral("diameter")},
        // DR-13: Angular is a structural kind (vertex + two rays); selecting it
        // on an EXISTING linear dimension is intentionally rejected by
        // planDimensionKindChange — Angular dimensions are created fresh via the
        // angular_dimension_tool belt cell, not converted from linear ones.
        // The entry is present so a selected Angular dimension shows its kind
        // correctly and can be read in tests by objectName "dimensionKindCombo".
        {QStringLiteral("Angular"), QStringLiteral("angular")},
    }, [this](const QString &kindId) {
        m_controller->setSelectedDimensionKind(kindId);
    });
    group->addWidget(m_dimensionKind);
    m_dimensionShowLabel = makeToggle(QStringLiteral("dimensionShowLabelCheckbox"), QStringLiteral("Show dimension label"), [this](bool checked) {
        m_controller->setSelectedDimensionLabelVisible(checked);
    });
    group->addWidget(m_dimensionShowLabel);

    group = beginInspectorGroup(QStringLiteral("guide_position"));
    group->addWidget(makeSectionLabel(QStringLiteral("Guide")));
    group->addWidget(makeConditionalButton(QStringLiteral("guideToDrawableOriginButton"), QStringLiteral("Guide To Drawable Origin"), QStringLiteral("guide_drawable_controls"), [this]() {
        m_controller->moveSelectedGuideToDrawableOrigin();
    }));
    group->addWidget(makeConditionalButton(QStringLiteral("centerGuideButton"), QStringLiteral("Center Guide"), QStringLiteral("guide_drawable_controls"), [this]() {
        m_controller->centerSelectedGuideInDrawable();
    }));
    group->addWidget(makeConditionalButton(QStringLiteral("guideToDrawableMaxButton"), QStringLiteral("Guide To Drawable Max"), QStringLiteral("guide_drawable_controls"), [this]() {
        m_controller->moveSelectedGuideToDrawableMax();
    }));
    {
        const QVector<QPair<QString, QString>> guideOffsetButtons {
            {QStringLiteral("negative_fine"), QStringLiteral("- Fine")},
            {QStringLiteral("positive_fine"), QStringLiteral("+ Fine")},
            {QStringLiteral("negative_grid"), QStringLiteral("- Grid")},
            {QStringLiteral("positive_grid"), QStringLiteral("+ Grid")},
            {QStringLiteral("negative_coarse"), QStringLiteral("- Coarse")},
            {QStringLiteral("positive_coarse"), QStringLiteral("+ Coarse")},
        };
        QVector<QPushButton *> buttons;
        for (const auto &buttonSpec : guideOffsetButtons) {
            const QStringList parts = buttonSpec.first.split(QLatin1Char('_'));
            auto *button = makeActionButton(QStringLiteral("guideOffset_%1").arg(buttonSpec.first), buttonSpec.second, [this, direction = parts.value(0), stepMode = parts.value(1)]() {
                m_controller->offsetSelectedGuide(direction, stepMode);
            });
            m_guideOffsetButtons.insert(buttonSpec.first, button);
            buttons.push_back(button);
        }
        FoldBox offsets = makeFoldBox();
        offsets.layout->addWidget(makeButtonGrid(buttons));
        group->addWidget(makeCollapsibleSection(QStringLiteral("Nudge Guide"), offsets.box, false));
    }
    group->addWidget(makeConditionalButton(QStringLiteral("deleteSelectedGuideButton"), QStringLiteral("Delete Selected Guide"), QStringLiteral("guide_drawable_controls"), [this]() {
        m_controller->deleteSelectedGuide();
    }));

    group = beginInspectorGroup(QStringLiteral("guide_visuals"));
    group->addWidget(makeSectionLabel(QStringLiteral("Guide Visuals")));
    m_guideLabel = new QLineEdit;
    m_guideLabel->setObjectName(QStringLiteral("guideLabelField"));
    m_guideLabel->setPlaceholderText(QStringLiteral("Default guide label"));
    connect(m_guideLabel, &QLineEdit::editingFinished, this, [this]() {
        m_controller->setSelectedGuideLabel(m_guideLabel->text());
    });
    group->addWidget(m_guideLabel);
    m_guideColor = makeDataCombo(QStringLiteral("guideColorCombo"), {
        {QStringLiteral("Guide blue"), QStringLiteral("#83aeca")},
        {QStringLiteral("Guide teal"), QStringLiteral("#54d2c6")},
        {QStringLiteral("Guide amber"), QStringLiteral("#f6c65b")},
        {QStringLiteral("Guide red"), QStringLiteral("#d98b8b")},
        {QStringLiteral("Guide green"), QStringLiteral("#91c89b")},
    }, [this](const QString &color) {
        m_controller->setSelectedGuideColor(color);
    });
    group->addWidget(m_guideColor);
    m_guideDashStyle = makeDataCombo(QStringLiteral("guideDashStyleCombo"), {
        {QStringLiteral("Dash line"), QStringLiteral("dash")},
        {QStringLiteral("Solid line"), QStringLiteral("solid")},
        {QStringLiteral("Dot line"), QStringLiteral("dot")},
    }, [this](const QString &dashStyle) {
        m_controller->setSelectedGuideDashStyle(dashStyle);
    });
    group->addWidget(m_guideDashStyle);
    m_guideShowLabel = makeToggle(QStringLiteral("guideShowLabelCheckbox"), QStringLiteral("Show guide label"), [this](bool checked) {
        m_controller->setSelectedGuideLabelVisible(checked);
    });
    group->addWidget(m_guideShowLabel);

    group = beginInspectorGroup(QStringLiteral("construction"));
    group->addWidget(makeConditionalButton(QStringLiteral("fitConstructionToDrawableButton"), QStringLiteral("Fit Construction To Drawable"), QStringLiteral("construction_drawable_controls"), [this]() {
        m_controller->fitSelectedConstructionLineToDrawable();
    }));

    group = beginInspectorGroup(QStringLiteral("transform"));
    group->addWidget(makeCollapsibleSection(QStringLiteral("Nudge"), buildNudgeControls(), false));
    group->addWidget(makeCollapsibleSection(QStringLiteral("Align"), buildAlignControls(), false));
    {
        // Offset / mirror / repeat are one mental action (make more of it):
        // one fold instead of three headers.
        FoldBox duplicate = makeFoldBox();
        duplicate.layout->addWidget(buildOffsetControls());
        duplicate.layout->addWidget(buildMirrorControls());
        duplicate.layout->addWidget(buildRepeatControls());
        group->addWidget(makeCollapsibleSection(QStringLiteral("Duplicate"), duplicate.box, false));
    }
    {
        // Modify verbs reshape the selected object in place (trim, fillet) —
        // distinct from Duplicate, which makes copies.
        FoldBox modify = makeFoldBox();
        auto *trim = makeActionButton(QStringLiteral("trimButton"), QStringLiteral("Trim"), [this]() {
            // Arms a pick-a-point capture: the next canvas click chooses the
            // part of the selected line to trim away (back to a crossing line).
            m_controller->beginTrimSelectedLine();
        });
        modify.layout->addWidget(trim);
        auto *fillet = makeActionButton(QStringLiteral("filletButton"), QStringLiteral("Fillet"), [this]() {
            // Arms a pick-a-point capture: the next canvas click picks the other
            // line + the corner; both lines round into an arc of the radius below.
            m_controller->beginFilletSelectedLine();
        });
        modify.layout->addWidget(fillet);
        {
            auto *row = new QWidget;
            auto *rowLayout = new QHBoxLayout(row);
            clearLayoutMargins(rowLayout);
            rowLayout->setSpacing(6);
            rowLayout->addWidget(new QLabel(QStringLiteral("Fillet radius")));
            auto *spin = new QDoubleSpinBox;
            spin->setObjectName(QStringLiteral("filletRadiusSpin"));
            spin->setDecimals(3);
            spin->setSingleStep(0.01);
            spin->setRange(0.001, 1.0);
            spin->setValue(m_controller->filletRadius());
            connect(spin, &QDoubleSpinBox::valueChanged, m_controller, [this](double r) {
                m_controller->setFilletRadius(r);
            });
            rowLayout->addWidget(spin, 1);
            modify.layout->addWidget(row);
        }
        group->addWidget(makeCollapsibleSection(QStringLiteral("Modify"), modify.box, false));
    }

    group = beginInspectorGroup(QStringLiteral("object_guides"));
    {
        const QVector<QPair<QString, QString>> boundsGuideButtons {
            {QStringLiteral("left"), QStringLiteral("Left")},
            {QStringLiteral("right"), QStringLiteral("Right")},
            {QStringLiteral("top"), QStringLiteral("Top")},
            {QStringLiteral("bottom"), QStringLiteral("Bottom")},
            {QStringLiteral("vertical_center"), QStringLiteral("V Center")},
            {QStringLiteral("horizontal_center"), QStringLiteral("H Center")},
        };
        QVector<QPushButton *> buttons;
        for (const auto &buttonSpec : boundsGuideButtons) {
            auto *button = makeActionButton(QStringLiteral("boundsGuide_%1").arg(buttonSpec.first), buttonSpec.second, [this, placementId = buttonSpec.first]() {
                m_controller->createGuideFromSelectedBounds(placementId);
            });
            m_boundsGuideButtons.insert(buttonSpec.first, button);
            buttons.push_back(button);
        }
        FoldBox fold = makeFoldBox();
        fold.layout->addWidget(makeButtonGrid(buttons));
        group->addWidget(makeCollapsibleSection(QStringLiteral("Bounds Guides"), fold.box, false));
    }
    {
        const QVector<QPair<QString, QString>> offsetGuideButtons {
            {QStringLiteral("left"), QStringLiteral("V Left")},
            {QStringLiteral("right"), QStringLiteral("V Right")},
            {QStringLiteral("top"), QStringLiteral("H Top")},
            {QStringLiteral("bottom"), QStringLiteral("H Bottom")},
        };
        QVector<QPushButton *> buttons;
        for (const auto &buttonSpec : offsetGuideButtons) {
            auto *button = makeActionButton(QStringLiteral("offsetGuide_%1").arg(buttonSpec.first), buttonSpec.second, [this, placementId = buttonSpec.first]() {
                m_controller->createOffsetGuideFromSelectedBounds(placementId, QStringLiteral("grid"));
            });
            m_offsetGuideButtons.insert(buttonSpec.first, button);
            buttons.push_back(button);
        }
        FoldBox fold = makeFoldBox();
        fold.layout->addWidget(makeButtonGrid(buttons));
        group->addWidget(makeCollapsibleSection(QStringLiteral("Offset Guides"), fold.box, false));
    }
    {
        const QVector<QPair<QString, QString>> alignToGuideButtons {
            {QStringLiteral("left"), QStringLiteral("V Left")},
            {QStringLiteral("center_x"), QStringLiteral("V Center")},
            {QStringLiteral("right"), QStringLiteral("V Right")},
            {QStringLiteral("top"), QStringLiteral("H Top")},
            {QStringLiteral("center_y"), QStringLiteral("H Center")},
            {QStringLiteral("bottom"), QStringLiteral("H Bottom")},
        };
        QVector<QPushButton *> buttons;
        for (const auto &buttonSpec : alignToGuideButtons) {
            auto *button = makeActionButton(QStringLiteral("alignToGuide_%1").arg(buttonSpec.first), buttonSpec.second, [this, modeId = buttonSpec.first]() {
                m_controller->alignSelectionToNearestGuide(modeId);
            });
            m_alignToGuideButtons.insert(buttonSpec.first, button);
            buttons.push_back(button);
        }
        FoldBox fold = makeFoldBox();
        fold.layout->addWidget(makeButtonGrid(buttons));
        group->addWidget(makeCollapsibleSection(QStringLiteral("Align To Guide"), fold.box, false));
    }

    // DM-15 "Block instance" inspector group — visible only for selected placed
    // instances. The plan table puts this group in the object_shape context so its
    // container widget exists whenever an object is selected; refreshInspector adds
    // a second gate on the document-level "has_block_instance_selection" projection
    // bool, hiding the group for non-instance objects. Two-gate pattern: plan =
    // context ownership, refreshInspector = projection-bool sub-gate.
    //
    // The two spins are DELTA-only: they carry a delta rotation (degrees) and a
    // scale FACTOR (multiplicative), not absolute values. They are NOT bound to a
    // controller setter — only transformInstanceButton reads them at click time.
    // This is intentional: no auto-commit on focus-out, no programmatic write-back
    // during refresh, so a model change can never loop. Identity defaults (0.0 deg /
    // 1.0 scale) are set before any connect() call (signal-safety: seed then wire).
    group = beginInspectorGroup(QStringLiteral("block_instance"));
    {
        FoldBox instanceFold = makeFoldBox();

        // Delta rotation spin — degrees. Range matches the DM-14 placement spin.
        {
            auto *row = new QWidget;
            auto *rowLayout = new QHBoxLayout(row);
            clearLayoutMargins(rowLayout);
            rowLayout->setSpacing(6);
            rowLayout->addWidget(new QLabel(QStringLiteral("Rotation delta")));
            auto *spin = new QDoubleSpinBox;
            spin->setObjectName(QStringLiteral("instanceRotationSpin"));
            spin->setDecimals(1);
            spin->setSingleStep(1.0);
            spin->setRange(-360.0, 360.0);
            // Seed BEFORE connect — no programmatic-write loop possible.
            spin->setValue(0.0);
            // No controller setter: this is read at click time only.
            // QDoubleSpinBox::valueChanged fires on any programmatic setValue too,
            // so connecting it to a setter here would loop on every refresh.
            m_instanceRotationSpin = spin;
            rowLayout->addWidget(spin, 1);
            instanceFold.layout->addWidget(row);
        }

        // Scale factor spin — multiplicative. 1.0 = no change.
        {
            auto *row = new QWidget;
            auto *rowLayout = new QHBoxLayout(row);
            clearLayoutMargins(rowLayout);
            rowLayout->setSpacing(6);
            rowLayout->addWidget(new QLabel(QStringLiteral("Scale factor")));
            auto *spin = new QDoubleSpinBox;
            spin->setObjectName(QStringLiteral("instanceScaleSpin"));
            spin->setDecimals(3);
            spin->setSingleStep(0.1);
            spin->setRange(0.01, 100.0);
            // Seed BEFORE connect — identity factor avoids accidental shrink/grow.
            spin->setValue(1.0);
            m_instanceScaleSpin = spin;
            rowLayout->addWidget(spin, 1);
            instanceFold.layout->addWidget(row);
        }

        // The action button reads the spins + the active object's instance_id
        // from the projection at click time. One controller call = one undo step.
        // makeActionButton is always enabled; the group's own visibility (gated by
        // refreshInspector on has_block_instance_selection) provides the real gate.
        m_transformInstanceButton = makeActionButton(
            QStringLiteral("transformInstanceButton"),
            QStringLiteral("Transform Instance"),
            [this]() {
                const QVariantMap document = m_controller->modelDocument();
                // Re-read instance_id at click time — it may have changed since
                // the last refresh (user clicked a different object before clicking
                // the button). Using the live document projection is safe here
                // because this handler runs after the last modelChanged.
                const QString instanceId = document.value(QStringLiteral("instance_id")).toString();
                if (instanceId.isEmpty()) {
                    return; // guard: button visible only for instances, but be safe
                }
                const double deltaRotDeg = m_instanceRotationSpin != nullptr
                    ? m_instanceRotationSpin->value() : 0.0;
                const double scaleFactor = m_instanceScaleSpin != nullptr
                    ? m_instanceScaleSpin->value() : 1.0;
                m_controller->transformBlockInstance(instanceId, deltaRotDeg, scaleFactor);
            });
        instanceFold.layout->addWidget(m_transformInstanceButton);
        group->addWidget(makeCollapsibleSection(QStringLiteral("Block Instance"), instanceFold.box, true));
    }

    // DM-10 region fill: a document-context action that arms a canvas pick. Region
    // fill needs no selected object (it is a "click inside a room" verb), so per the
    // settled fork it lives as an inspector action button in the document context —
    // mirroring how the Radial/Fillet verbs are inspector buttons — rather than a
    // belt cell. makeActionButton is always enabled (no enable-key gate), which is
    // correct here: the pick itself, not a selection, decides what gets filled.
    group = beginInspectorGroup(QStringLiteral("region_fill_document"));
    {
        FoldBox fill = makeFoldBox();
        auto *fillRegion = makeActionButton(QStringLiteral("fillRegionButton"), QStringLiteral("Fill Region"), [this]() {
            // Arms a pick-a-point capture; the next canvas click seeds the
            // enclosed-region fill (a new closed Polygon with a non-zero fill).
            m_controller->beginRegionFillPick();
        });
        fill.layout->addWidget(fillRegion);
        group->addWidget(makeCollapsibleSection(QStringLiteral("Region Fill"), fill.box, false));
    }

    group = beginInspectorGroup(QStringLiteral("layers_document"));
    group->addWidget(makeCollapsibleSection(QStringLiteral("Layers"), buildLayerControls(), true));

    group = beginInspectorGroup(QStringLiteral("guides_document"));
    {
        const QVector<QPair<QString, QString>> guidePresetButtons {
            {QStringLiteral("drawable_bounds"), QStringLiteral("Bounds")},
            {QStringLiteral("drawable_centerlines"), QStringLiteral("Centerlines")},
            {QStringLiteral("thirds"), QStringLiteral("Thirds")},
            {QStringLiteral("quarters"), QStringLiteral("Quarters")},
            {QStringLiteral("margin_safe"), QStringLiteral("Margin Safe")},
        };
        QVector<QPushButton *> buttons;
        for (const auto &buttonSpec : guidePresetButtons) {
            buttons.push_back(makeActionButton(QStringLiteral("guidePreset_%1").arg(buttonSpec.first), buttonSpec.second, [this, presetId = buttonSpec.first]() {
                m_controller->applyGuidePreset(presetId);
            }));
        }
        FoldBox fold = makeFoldBox();
        fold.layout->addWidget(makeButtonGrid(buttons));
        group->addWidget(makeCollapsibleSection(QStringLiteral("Guide Presets"), fold.box, false));
    }
    {
        FoldBox fold = makeFoldBox();
        m_deleteAllGuidesButton = makeActionButton(QStringLiteral("deleteAllGuidesButton"), QStringLiteral("Delete All"), [this]() {
            m_controller->deleteAllGuides();
        });
        m_mergeDuplicateGuidesButton = makeActionButton(QStringLiteral("mergeDuplicateGuidesButton"), QStringLiteral("Merge Dups"), [this]() {
            m_controller->mergeDuplicateGuides();
        });
        m_hideAllGuidesButton = makeActionButton(QStringLiteral("hideAllGuidesButton"), QStringLiteral("Hide All"), [this]() {
            m_controller->setAllGuidesVisible(false);
        });
        m_showAllGuidesButton = makeActionButton(QStringLiteral("showAllGuidesButton"), QStringLiteral("Show All"), [this]() {
            m_controller->setAllGuidesVisible(true);
        });
        m_lockAllGuidesButton = makeActionButton(QStringLiteral("lockAllGuidesButton"), QStringLiteral("Lock All"), [this]() {
            m_controller->setAllGuidesLocked(true);
        });
        m_unlockAllGuidesButton = makeActionButton(QStringLiteral("unlockAllGuidesButton"), QStringLiteral("Unlock All"), [this]() {
            m_controller->setAllGuidesLocked(false);
        });
        fold.layout->addWidget(makeButtonGrid({m_deleteAllGuidesButton, m_mergeDuplicateGuidesButton,
            m_hideAllGuidesButton, m_showAllGuidesButton, m_lockAllGuidesButton, m_unlockAllGuidesButton}));
        group->addWidget(makeCollapsibleSection(QStringLiteral("All Guides"), fold.box, false));
    }

    group = beginInspectorGroup(QStringLiteral("calibration_document"));
    group->addWidget(makeCollapsibleSection(QStringLiteral("Calibration"), buildCalibrationControls(), false));

    group = beginInspectorGroup(QStringLiteral("document_info"));
    group->addWidget(makeSectionLabel(QStringLiteral("Document")));
    m_toolValue = makeValueLabel();
    m_objectsValue = makeValueLabel();
    m_guidesValue = makeValueLabel();
    m_revisionValue = makeValueLabel();
    group->addWidget(m_toolValue);
    group->addWidget(m_objectsValue);
    group->addWidget(m_guidesValue);
    group->addWidget(m_revisionValue);

    group = beginInspectorGroup(QStringLiteral("canvas_state"));
    FoldBox canvasState = makeFoldBox();
    m_snapValue = makeValueLabel();
    m_gridValue = makeValueLabel();
    m_plotValue = makeValueLabel();
    m_plotBoundsValue = makeValueLabel();
    m_plotLayerStatsValue = makeValueLabel();
    m_plotPenStatsValue = makeValueLabel();
    m_plotReadinessValue = makeValueLabel();
    m_plotOrderMode = makeDataCombo(QStringLiteral("plotOrderMode"), {
        {QStringLiteral("Plot order: layer"), QStringLiteral("layer_order")},
        {QStringLiteral("Plot order: nearest"), QStringLiteral("nearest_next")},
    }, [this](const QString &modeId) {
        m_controller->setPlotOrderModeId(modeId);
    }, m_controller->plotOrderModeId());
    m_plotDirectionMode = makeDataCombo(QStringLiteral("plotDirectionMode"), {
        {QStringLiteral("Direction: preserve"), QStringLiteral("preserve_direction")},
        {QStringLiteral("Direction: reversible"), QStringLiteral("allow_reverse_segments")},
    }, [this](const QString &modeId) {
        m_controller->setPlotDirectionModeId(modeId);
    }, m_controller->plotDirectionModeId());
    m_plotPreviewVisible = makeToggle(QStringLiteral("plotPreviewCheckbox"), QStringLiteral("Show plot preview"), [this](bool checked) {
        if (m_canvas != nullptr) {
            m_canvas->setPlotPreviewVisible(checked);
        }
    }, false);
    m_pointerValue = makeValueLabel();
    m_quickMeasureValue = makeValueLabel();
    m_guideDragValue = makeValueLabel();
    m_previewValue = makeValueLabel();
    canvasState.layout->addWidget(m_snapValue);
    canvasState.layout->addWidget(m_gridValue);
    canvasState.layout->addWidget(m_plotValue);
    canvasState.layout->addWidget(m_plotBoundsValue);
    canvasState.layout->addWidget(m_plotLayerStatsValue);
    canvasState.layout->addWidget(m_plotPenStatsValue);
    canvasState.layout->addWidget(m_plotReadinessValue);
    canvasState.layout->addWidget(m_plotOrderMode);
    canvasState.layout->addWidget(m_plotDirectionMode);
    canvasState.layout->addWidget(m_plotPreviewVisible);
    canvasState.layout->addWidget(m_pointerValue);
    canvasState.layout->addWidget(m_quickMeasureValue);
    canvasState.layout->addWidget(m_guideDragValue);
    canvasState.layout->addWidget(m_previewValue);
    group->addWidget(makeCollapsibleSection(QStringLiteral("Canvas State"), canvasState.box, false));
}

QString DraftingFeature::defaultPanelSlot(const QString &groupId)
{
    // ONE home for the default — the review found this ternary copied four
    // times, which is four chances to disagree.
    return groupId == QStringLiteral("object_list") ? QStringLiteral("left") : QStringLiteral("right");
}

QString DraftingFeature::assignedPanelSlot(const QString &groupId) const
{
    if (m_actions.panelSlotForGroup) {
        const QString slot = m_actions.panelSlotForGroup(groupId);
        if (!slot.isEmpty()) {
            return slot;
        }
    }
    return defaultPanelSlot(groupId);
}

QString DraftingFeature::placementSlot(const QString &groupId) const
{
    // Where the group's widget PARENTS: hidden parks in its default panel
    // (ownership through remounts; visibility gating hides it), and an
    // assignment naming a slot this layout never mounted falls back to the
    // default rather than leaving the widget parentless (a leak) and gone.
    QString slot = assignedPanelSlot(groupId);
    if (slot == QStringLiteral("hidden") || !m_panelGroupHosts.contains(slot)) {
        slot = defaultPanelSlot(groupId);
    }
    if (!m_panelGroupHosts.contains(slot) && !m_panelGroupHosts.isEmpty()) {
        slot = m_panelGroupHosts.firstKey(); // partial layout: any host beats a leak
    }
    return slot;
}

void DraftingFeature::placePanelGroups(const QString &slotName, QVBoxLayout *layout)
{
    m_panelGroupHosts.insert(slotName, layout);
    for (const QString &groupId : m_groupBuildOrder) {
        if (placementSlot(groupId) == slotName) {
            layout->addWidget(m_inspectorGroups.value(groupId));
        }
    }
}

void DraftingFeature::applyPanelAssignments()
{
    // Live re-place: pull every group out of its host, then re-run the
    // placement order per host, inserting before each host's trailing
    // widget (the stretch, or the terminal's status label). Groups are
    // REPARENTED, never rebuilt — live editors and the settings page's own
    // controls keep their connections, the same live-edit contract as the
    // belt re-dress.
    for (auto it = m_inspectorGroups.cbegin(); it != m_inspectorGroups.cend(); ++it) {
        QWidget *group = it.value();
        if (group->parentWidget() != nullptr && group->parentWidget()->layout() != nullptr) {
            group->parentWidget()->layout()->removeWidget(group);
        }
        group->setParent(nullptr);
        group->hide(); // no momentary top-level flicker
    }
    for (auto hostIt = m_panelGroupHosts.cbegin(); hostIt != m_panelGroupHosts.cend(); ++hostIt) {
        QVBoxLayout *host = hostIt.value();
        for (const QString &groupId : m_groupBuildOrder) {
            if (placementSlot(groupId) == hostIt.key()) {
                host->insertWidget(host->count() - 1, m_inspectorGroups.value(groupId));
            }
        }
    }
    refreshInspector(); // re-applies plan visibility (and un-hides what belongs visible)
}

QVector<QPair<QString, QString>> DraftingFeature::panelGroupInventory()
{
    // The settings page's vocabulary: group id -> human label, in canonical
    // (build) order. Ids are the same strings the inspector plan speaks.
    return {
        {QStringLiteral("object_list"), QStringLiteral("Object List")},
        {QStringLiteral("tool_polygon"), QStringLiteral("Polygon Tool")},
        {QStringLiteral("tool_rectangle"), QStringLiteral("Rectangle Tool")},
        {QStringLiteral("tool_radius"), QStringLiteral("Tool Radius")},
        {QStringLiteral("empty_state"), QStringLiteral("Empty State")},
        {QStringLiteral("selection_summary"), QStringLiteral("Selection")},
        {QStringLiteral("style"), QStringLiteral("Style")},
        {QStringLiteral("geometry"), QStringLiteral("Geometry")},
        {QStringLiteral("dimension"), QStringLiteral("Dimension")},
        {QStringLiteral("guide_position"), QStringLiteral("Guide Position")},
        {QStringLiteral("guide_visuals"), QStringLiteral("Guide Visuals")},
        {QStringLiteral("construction"), QStringLiteral("Construction")},
        {QStringLiteral("transform"), QStringLiteral("Transform")},
        {QStringLiteral("object_guides"), QStringLiteral("Object Guides")},
        {QStringLiteral("layers_document"), QStringLiteral("Layers")},
        {QStringLiteral("guides_document"), QStringLiteral("Guides")},
        {QStringLiteral("calibration_document"), QStringLiteral("Calibration")},
        {QStringLiteral("document_info"), QStringLiteral("Document")},
        {QStringLiteral("canvas_state"), QStringLiteral("Canvas State")},
    };
}

QVBoxLayout *DraftingFeature::beginInspectorGroup(const QString &groupId)
{
    auto *groupWidget = new QWidget;
    // The object name mirrors the plan's group id so tests assert per-context
    // visibility against the same vocabulary the pure module speaks.
    groupWidget->setObjectName(QStringLiteral("inspectorGroup_%1").arg(groupId));
    auto *groupLayout = new QVBoxLayout(groupWidget);
    clearLayoutMargins(groupLayout);
    groupLayout->setSpacing(8); // matches the scroll panel's content spacing
    // No parent here: groups are built once, parentless, and PLACED by the
    // assignment pass (modular panels — the user decides which panel hosts
    // which group). m_groupBuildOrder is the canonical top-to-bottom order
    // every panel preserves for its own subset.
    m_inspectorGroups.insert(groupId, groupWidget);
    m_groupBuildOrder.push_back(groupId);
    return groupLayout;
}

QWidget *DraftingFeature::buildGeometryEditor()
{
    return makeControlGrid(QStringLiteral("geometryEditor")).panel;
}

QWidget *DraftingFeature::buildObjectFlagControls()
{
    auto *panel = new QWidget;
    panel->setObjectName(QStringLiteral("objectFlagControls"));
    auto *layout = new QHBoxLayout(panel);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(10);

    m_selectedLocked = makeToggle(QStringLiteral("objectFlagCheckbox"), QStringLiteral("Locked"), [this](bool checked) {
        m_controller->setSelectedObjectLocked(checked);
    });

    m_selectedVisible = makeToggle(QStringLiteral("objectFlagCheckbox"), QStringLiteral("Visible"), [this](bool checked) {
        m_controller->setSelectedObjectVisible(checked);
    });

    layout->addWidget(m_selectedLocked);
    layout->addWidget(m_selectedVisible);
    layout->addStretch(1);
    return panel;
}

QWidget *DraftingFeature::buildLayerControls()
{
    auto *panel = new QWidget;
    panel->setObjectName(QStringLiteral("layerControls"));
    auto *layout = new QVBoxLayout(panel);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(4);

    m_activeLayer = makeDataCombo(QStringLiteral("activeLayerCombo"), {}, [this](const QString &layerId) {
        m_controller->setActiveLayerId(layerId);
    });
    layout->addWidget(m_activeLayer);

    m_addLayerButton = makeActionButton(QStringLiteral("addLayerButton"), QStringLiteral("Add Layer"), [this]() {
        m_controller->createLayer();
    });
    layout->addWidget(m_addLayerButton);

    auto *orderRow = new QWidget;
    auto *orderLayout = new QHBoxLayout(orderRow);
    orderLayout->setContentsMargins(0, 0, 0, 0);
    orderLayout->setSpacing(6);

    m_layerDownButton = makeActionButton(QStringLiteral("layerOrderButton"), QStringLiteral("Down"), [this]() {
        m_controller->moveActiveLayer(QStringLiteral("down"));
    });

    m_layerUpButton = makeActionButton(QStringLiteral("layerOrderButton"), QStringLiteral("Up"), [this]() {
        m_controller->moveActiveLayer(QStringLiteral("up"));
    });

    orderLayout->addWidget(m_layerDownButton);
    orderLayout->addWidget(m_layerUpButton);
    orderLayout->addStretch(1);
    layout->addWidget(orderRow);

    auto *row = new QWidget;
    auto *rowLayout = new QHBoxLayout(row);
    rowLayout->setContentsMargins(0, 0, 0, 0);
    rowLayout->setSpacing(10);

    m_defaultLayerLocked = makeToggle(QStringLiteral("layerFlagCheckbox"), QStringLiteral("Locked"), [this](bool checked) {
        m_controller->setActiveLayerLocked(checked);
    });

    m_defaultLayerVisible = makeToggle(QStringLiteral("layerFlagCheckbox"), QStringLiteral("Visible"), [this](bool checked) {
        m_controller->setActiveLayerVisible(checked);
    });

    rowLayout->addWidget(m_defaultLayerLocked);
    rowLayout->addWidget(m_defaultLayerVisible);
    rowLayout->addStretch(1);
    layout->addWidget(row);

    m_activeLayerPlotEnabled = makeToggle(QStringLiteral("layerPlotCheckbox"), QStringLiteral("Plot"), [this](bool checked) {
        m_controller->setActiveLayerPlotEnabled(checked);
    });
    layout->addWidget(m_activeLayerPlotEnabled);

    m_activeLayerPen = makeDataCombo(QStringLiteral("layerPenCombo"), {
        {QStringLiteral("Black pen"), QStringLiteral("pen_black")},
        {QStringLiteral("Blue pen"), QStringLiteral("pen_blue")},
        {QStringLiteral("Red pen"), QStringLiteral("pen_red")},
    }, [this](const QString &presetId) {
        m_controller->setActiveLayerPenPreset(presetId);
    });
    layout->addWidget(m_activeLayerPen);

    m_activeLayerStrokeWidth = makeDataCombo(QStringLiteral("layerStrokeWidthCombo"), {
        {QStringLiteral("Fine stroke"), QStringLiteral("fine")},
        {QStringLiteral("Normal stroke"), QStringLiteral("normal")},
        {QStringLiteral("Bold stroke"), QStringLiteral("bold")},
    }, [this](const QString &presetId) {
        m_controller->setActiveLayerStrokeWidthPreset(presetId);
    });
    layout->addWidget(m_activeLayerStrokeWidth);
    // The move-object-to-layer combo lives in the selection_summary group now
    // (F2): it acts on the selection, not on the document's layer table.
    return panel;
}

QWidget *DraftingFeature::buildNudgeControls()
{
    const auto [panel, layout] = makeControlGrid(QStringLiteral("nudgeControls"));

    auto addButton = [this, layout](const QString &label, const QString &direction, const QString &stepMode, int row, int column) {
        auto *button = makeActionButton(QStringLiteral("nudgeButton"), label, [this, direction, stepMode]() {
            m_controller->nudgeSelection(direction, stepMode);
        });
        layout->addWidget(button, row, column);
    };
    auto addSafeButton = [this, layout](const QString &label, const QString &direction, int row, int column) {
        auto *button = makeActionButton(QStringLiteral("nudgeButton"), label, [this, direction]() {
            m_controller->nudgeSelectionInsideDrawable(direction, QStringLiteral("grid"));
        });
        layout->addWidget(button, row, column);
    };

    addButton(QStringLiteral("Grid Up"), QStringLiteral("up"), QStringLiteral("grid"), 1, 1);
    addButton(QStringLiteral("Grid Left"), QStringLiteral("left"), QStringLiteral("grid"), 2, 0);
    addButton(QStringLiteral("Grid Right"), QStringLiteral("right"), QStringLiteral("grid"), 2, 2);
    addButton(QStringLiteral("Grid Down"), QStringLiteral("down"), QStringLiteral("grid"), 3, 1);
    addSafeButton(QStringLiteral("Safe Up"), QStringLiteral("up"), 1, 3);
    addSafeButton(QStringLiteral("Safe Left"), QStringLiteral("left"), 2, 3);
    addSafeButton(QStringLiteral("Safe Right"), QStringLiteral("right"), 3, 3);
    addSafeButton(QStringLiteral("Safe Down"), QStringLiteral("down"), 4, 3);
    addButton(QStringLiteral("Fine Up"), QStringLiteral("up"), QStringLiteral("fine"), 4, 1);
    addButton(QStringLiteral("Fine Left"), QStringLiteral("left"), QStringLiteral("fine"), 5, 0);
    addButton(QStringLiteral("Fine Right"), QStringLiteral("right"), QStringLiteral("fine"), 5, 2);
    addButton(QStringLiteral("Fine Down"), QStringLiteral("down"), QStringLiteral("fine"), 6, 1);

    return panel;
}

QWidget *DraftingFeature::buildAlignControls()
{
    const auto [panel, layout] = makeControlGrid(QStringLiteral("alignControls"));

    auto addAlignButton = [this, layout](const QString &label, const QString &modeId, int row, int column) {
        auto *button = makeActionButton(QStringLiteral("alignButton"), label, [this, modeId]() {
            m_controller->alignSelection(modeId);
        });
        layout->addWidget(button, row, column);
    };
    auto addDistributeButton = [this, layout](const QString &label, const QString &axisId, int row, int column) {
        auto *button = makeActionButton(QStringLiteral("distributeButton"), label, [this, axisId]() {
            m_controller->distributeSelection(axisId);
        });
        layout->addWidget(button, row, column);
    };

    addAlignButton(QStringLiteral("Left"), QStringLiteral("left"), 1, 0);
    addAlignButton(QStringLiteral("Center X"), QStringLiteral("center_x"), 1, 1);
    addAlignButton(QStringLiteral("Right"), QStringLiteral("right"), 1, 2);
    addAlignButton(QStringLiteral("Top"), QStringLiteral("top"), 2, 0);
    addAlignButton(QStringLiteral("Center Y"), QStringLiteral("center_y"), 2, 1);
    addAlignButton(QStringLiteral("Bottom"), QStringLiteral("bottom"), 2, 2);
    addDistributeButton(QStringLiteral("Distribute X"), QStringLiteral("x"), 3, 0);
    addDistributeButton(QStringLiteral("Distribute Y"), QStringLiteral("y"), 3, 1);

    return panel;
}

QWidget *DraftingFeature::buildOffsetControls()
{
    const auto [panel, layout] = makeControlGrid(QStringLiteral("offsetControls"));


    auto *left = makeActionButton(QStringLiteral("offsetButton"), QStringLiteral("Left +0.05"), [this]() {
        m_controller->offsetSelectedObject(QStringLiteral("left"));
    });
    layout->addWidget(left, 1, 0);

    auto *right = makeActionButton(QStringLiteral("offsetButton"), QStringLiteral("Right +0.05"), [this]() {
        m_controller->offsetSelectedObject(QStringLiteral("right"));
    });
    layout->addWidget(right, 1, 1);

    return panel;
}

QWidget *DraftingFeature::buildMirrorControls()
{
    const auto [panel, layout] = makeControlGrid(QStringLiteral("mirrorControls"));

    // The add-row helper mirrors buildRepeatControls: label in col 0, editor
    // in col 1 of the same grid so the two controls share a column width.
    const auto addOptionRow = [&](int row, const QString &label, QWidget *editor) {
        layout->addWidget(new QLabel(label), row, 0);
        layout->addWidget(editor, row, 1);
    };

    // DR-11: axis-count spin for the Kaleidoscope operation. The controller
    // stores this in m_arrayCount (the same field the Repeat section uses),
    // which is the minimal-footprint choice: kaleidoscope shares the count
    // concept with radial array (N copies along N axes). A dedicated member
    // would add a separate code path with no semantic difference — one shared
    // count is the data-oriented design here. Seed value at build; push back
    // on edit (same pattern as arrayCountSpin in buildRepeatControls).
    auto *axisCountSpin = new QSpinBox;
    axisCountSpin->setObjectName(QStringLiteral("axisCountSpin"));
    axisCountSpin->setRange(1, 36); // 1 axis = single-line mirror; 36 = every 5 degrees
    axisCountSpin->setValue(m_controller->arrayCount());
    connect(axisCountSpin, &QSpinBox::valueChanged, m_controller, [this](int count) {
        m_controller->setArrayCount(count);
    });
    addOptionRow(0, QStringLiteral("Axes"), axisCountSpin);

    auto *horizontal = makeActionButton(QStringLiteral("mirrorButton"), QStringLiteral("Mirror H"), [this]() {
        m_controller->mirrorSelectedObject(QStringLiteral("horizontal"));
    });
    layout->addWidget(horizontal, 1, 0);

    auto *vertical = makeActionButton(QStringLiteral("mirrorButton"), QStringLiteral("Mirror V"), [this]() {
        m_controller->mirrorSelectedObject(QStringLiteral("vertical"));
    });
    layout->addWidget(vertical, 1, 1);

    // DR-11 Kaleidoscope button: arms a pick-a-point capture; the next canvas
    // click sets the symmetry centre and runs kaleidoscopeMirror with
    // arrayCount() evenly-spaced axes through it. Same data-oriented pattern
    // as beginRadialArrayCenterPick / beginRotateCopiesCenterPick: one button,
    // one intent enum value, one resolution branch in resolvePointCapture.
    auto *kaleidoscope = makeActionButton(QStringLiteral("kaleidoscopeButton"), QStringLiteral("Kaleidoscope"), [this]() {
        m_controller->beginKaleidoscopeCenterPick();
    });
    layout->addWidget(kaleidoscope, 2, 0, 1, 2); // full-width so the label fits

    return panel;
}

QWidget *DraftingFeature::buildRepeatControls()
{
    const auto [panel, layout] = makeControlGrid(QStringLiteral("repeatControls"));

    // #30: the array options (count + per-axis spacing) sit with the actions
    // that consume them. Spins mirror controller tool-option state, exactly
    // like the polygon/rectangle groups: set at build, pushed back on edit.
    const auto addOptionRow = [&](int row, const QString &label, QWidget *editor) {
        layout->addWidget(new QLabel(label), row, 0);
        layout->addWidget(editor, row, 1);
    };

    auto *countSpin = new QSpinBox;
    countSpin->setObjectName(QStringLiteral("arrayCountSpin"));
    countSpin->setRange(1, 99);
    countSpin->setValue(m_controller->arrayCount());
    connect(countSpin, &QSpinBox::valueChanged, m_controller, [this](int count) {
        m_controller->setArrayCount(count);
    });
    addOptionRow(0, QStringLiteral("Count"), countSpin);

    const auto makeSpacingSpin = [this](const QString &name, double value,
                                        const std::function<void(double)> &onChange) {
        auto *spin = new QDoubleSpinBox;
        spin->setObjectName(name);
        spin->setDecimals(3);
        spin->setSingleStep(0.01);
        spin->setRange(-1.0, 1.0); // negative spacing marches the array left/up
        spin->setValue(value);
        connect(spin, &QDoubleSpinBox::valueChanged, m_controller, onChange);
        return spin;
    };
    addOptionRow(1, QStringLiteral("Step X"),
                 makeSpacingSpin(QStringLiteral("arraySpacingXSpin"), m_controller->arraySpacingX(),
                                 [this](double s) { m_controller->setArraySpacingX(s); }));
    addOptionRow(2, QStringLiteral("Step Y"),
                 makeSpacingSpin(QStringLiteral("arraySpacingYSpin"), m_controller->arraySpacingY(),
                                 [this](double s) { m_controller->setArraySpacingY(s); }));

    // DR-10 rotate-copies rosette: a tool-option toggle plus a total-angle spin.
    // The toggle picks which op the shared Radial button (below) runs; off (the
    // default) preserves today's placement-only radial array byte-for-byte. The
    // angle spin mirrors arraySpacingXSpin — set at build, pushed back on edit —
    // and binds to the controller's rotateCopiesTotalAngle param (default 360).
    auto *rotateCopiesToggle = makeToggle(QStringLiteral("rotateCopiesToggle"),
                                          QStringLiteral("Rotate copies"),
                                          [this](bool on) { m_rotateCopies = on; },
                                          m_rotateCopies);
    layout->addWidget(rotateCopiesToggle, 3, 0, 1, 2);

    auto *rosetteAngleSpin = new QDoubleSpinBox;
    rosetteAngleSpin->setObjectName(QStringLiteral("rosetteAngleSpin"));
    rosetteAngleSpin->setDecimals(1);
    rosetteAngleSpin->setSingleStep(1.0);
    rosetteAngleSpin->setRange(-360.0, 360.0); // a full ring by default; signed = spin direction
    rosetteAngleSpin->setValue(m_controller->rotateCopiesTotalAngle());
    connect(rosetteAngleSpin, &QDoubleSpinBox::valueChanged, m_controller, [this](double deg) {
        m_controller->setRotateCopiesTotalAngle(deg);
    });
    addOptionRow(4, QStringLiteral("Total angle"), rosetteAngleSpin);

    auto *x = makeActionButton(QStringLiteral("repeatButton"), QStringLiteral("Repeat X"), [this]() {
        m_controller->repeatSelectedObject(QStringLiteral("x"));
    });
    layout->addWidget(x, 5, 0);

    auto *y = makeActionButton(QStringLiteral("repeatButton"), QStringLiteral("Repeat Y"), [this]() {
        m_controller->repeatSelectedObject(QStringLiteral("y"));
    });
    layout->addWidget(y, 5, 1);

    auto *grid = makeActionButton(QStringLiteral("gridArrayButton"), QStringLiteral("Grid"), [this]() {
        m_controller->gridArraySelectedObject();
    });
    layout->addWidget(grid, 6, 0);

    auto *radial = makeActionButton(QStringLiteral("radialArrayButton"), QStringLiteral("Radial"), [this]() {
        // Arms a pick-a-point capture; the next canvas click sets the ring
        // centre (replacing the old hardcoded drawable centre). DR-10: the
        // "Rotate copies" toggle flips which op this one button arms — rosette
        // (each copy rotated to its spoke) vs. placement-only radial array. The
        // toggle defaults off, so the default behaviour is unchanged.
        if (m_rotateCopies) {
            m_controller->beginRotateCopiesCenterPick();
        } else {
            m_controller->beginRadialArrayCenterPick();
        }
    });
    layout->addWidget(radial, 6, 1);

    return panel;
}

QWidget *DraftingFeature::buildBottomPanel()
{
    auto *panel = makeRegionFrame(QStringLiteral("bottomPanel"));
    const PanelSpec bottomSpec = panelSpec(ShellSlot::Bottom);
    panel->setMinimumHeight(bottomSpec.minSize);
    if (bottomSpec.maxSize > 0) {
        panel->setMaximumHeight(bottomSpec.maxSize);
    }

    auto *layout = new QVBoxLayout(panel);
    layout->setContentsMargins(12, 8, 12, 8);
    layout->setSpacing(6);

    auto *tabs = new QWidget;
    auto *tabsLayout = new QHBoxLayout(tabs);
    tabsLayout->setContentsMargins(0, 0, 0, 0);
    tabsLayout->setSpacing(6);
    tabsLayout->addWidget(makeRailButton(QStringLiteral("State"), QStringLiteral("Current drafting state"), true));
    tabsLayout->addWidget(makeRailButton(QStringLiteral("Commands"), QStringLiteral("Command review"), false, false));
    tabsLayout->addWidget(makeRailButton(QStringLiteral("Notes"), QStringLiteral("Workspace notes"), false, false));
    tabsLayout->addStretch(1);

    auto *status = makeValueLabel(QStringLiteral("C++ Widgets shell. Drafting state is owned by DrawingDocumentController."));
    status->setObjectName(QStringLiteral("bottomStatus"));
    status->setAlignment(Qt::AlignTop | Qt::AlignLeft);

    // Tabs pinned to the panel's top edge, content taking ALL remaining
    // space. Without the stretch factor Qt distributes extra height between
    // the two rows, so growing the terminal made the tabs drift and opened a
    // gap above the content.
    layout->addWidget(tabs);
    // Groups the user assigned "bottom" sit between the tabs and the status
    // text — the terminal keeps its own formatting around them.
    ensureInspectorGroupsBuilt();
    placePanelGroups(QStringLiteral("bottom"), layout);
    layout->addWidget(status, 1);
    return panel;
}

QWidget *DraftingFeature::buildCalibrationControls()
{
    const auto [panel, layout] = makeControlGrid(QStringLiteral("calibrationControls"));

    auto addButton = [this, layout](const QString &label, const QString &patternId, int row, int column) {
        auto *button = makeActionButton(QStringLiteral("calibrationButton"), label, [this, patternId]() {
            m_controller->createCalibrationPattern(patternId);
        });
        layout->addWidget(button, row, column);
    };

    addButton(QStringLiteral("Test square"), QStringLiteral("test_square"), 1, 0);
    addButton(QStringLiteral("Test circle"), QStringLiteral("test_circle"), 1, 1);
    addButton(QStringLiteral("Line spacing"), QStringLiteral("line_spacing"), 1, 2);

    auto *measuredLabel = new QLabel(QStringLiteral("Measured"));
    measuredLabel->setObjectName(QStringLiteral("fieldLabel"));
    layout->addWidget(measuredLabel, 2, 0);

    m_calibrationMeasuredValue = new QDoubleSpinBox;
    m_calibrationMeasuredValue->setObjectName(QStringLiteral("geometryField"));
    m_calibrationMeasuredValue->setRange(0.000001, 1000000.0);
    m_calibrationMeasuredValue->setDecimals(6);
    m_calibrationMeasuredValue->setSingleStep(0.001);
    m_calibrationMeasuredValue->setValue(0.24);
    layout->addWidget(m_calibrationMeasuredValue, 2, 1);

    auto *record = makeActionButton(QStringLiteral("calibrationButton"), QStringLiteral("Record"), [this]() {
        if (m_calibrationMeasuredValue != nullptr) {
            m_controller->recordCalibrationMeasurement(m_calibrationMeasuredValue->value());
        }
    });
    layout->addWidget(record, 2, 2);

    auto *applyScale = makeActionButton(QStringLiteral("calibrationButton"), QStringLiteral("Apply scale"), [this]() {
        m_controller->applyCalibrationCorrection();
    });
    layout->addWidget(applyScale, 3, 0, 1, 3);

    m_calibrationMeasurementValue = makeValueLabel(QStringLiteral("Calibration measurement: none"));
    layout->addWidget(m_calibrationMeasurementValue, 4, 0, 1, 3);

    return panel;
}

QPushButton *DraftingFeature::makeActionButton(const QString &objectName, const QString &label, const std::function<void()> &action)
{
    auto *button = new QPushButton(label);
    button->setObjectName(objectName);
    connect(button, &QPushButton::clicked, this, action);
    return button;
}

QPushButton *DraftingFeature::makeConditionalButton(const QString &objectName, const QString &label,
                                                    const QString &enableKey, const std::function<void()> &action)
{
    auto *button = makeActionButton(objectName, label, action);
    m_conditionalButtons.append({button, enableKey});
    return button;
}

QCheckBox *DraftingFeature::makeToggle(const QString &objectName, const QString &label, const std::function<void(bool)> &onToggled,
                                       std::optional<bool> initialChecked)
{
    auto *checkbox = new QCheckBox(label);
    checkbox->setObjectName(objectName);
    if (initialChecked) {
        checkbox->setChecked(*initialChecked);
    }
    connect(checkbox, &QCheckBox::toggled, this, onToggled);
    return checkbox;
}

QComboBox *DraftingFeature::makeDataCombo(const QString &objectName,
                                          const QVector<QPair<QString, QString>> &items,
                                          const std::function<void(const QString &)> &onData,
                                          const QString &initialData)
{
    auto *combo = new QComboBox;
    combo->setObjectName(objectName);
    for (const auto &item : items) {
        combo->addItem(item.first, item.second);
    }
    if (!initialData.isEmpty()) {
        const int index = combo->findData(initialData);
        if (index >= 0) {
            combo->setCurrentIndex(index);
        }
    }
    connect(combo, &QComboBox::currentIndexChanged, this, [combo, onData](int index) {
        if (index < 0) {
            return;
        }
        onData(combo->itemData(index).toString());
    });
    return combo;
}

QDoubleSpinBox *DraftingFeature::makeGeometryFieldSpin(const GeometryFieldSpec &spec)
{
    auto *spin = new QDoubleSpinBox;
    spin->setObjectName(QStringLiteral("geometryField"));
    spin->setDecimals(spec.decimals);
    spin->setSingleStep(spec.step);
    spin->setRange(spec.minimum, spec.maximum);
    spin->setValue(spec.value);
    spin->setProperty("fieldId", spec.fieldId);
    spin->setProperty("fieldMode", spec.fieldMode);
    spin->setProperty("editInvalid", false);
    connect(spin, &QDoubleSpinBox::editingFinished, this, [this, spin]() {
        applyGeometryFieldEdit(spin);
    });
    return spin;
}

void DraftingFeature::applyGeometryFieldEdit(QDoubleSpinBox *spin)
{
    const QString fieldId = spin->property("fieldId").toString();
    const bool physical = spin->property("fieldMode").toString() == QStringLiteral("physical");
    const bool ok = physical
        ? m_controller->updateSelectedObjectPhysicalGeometryField(fieldId, spin->value())
        : m_controller->updateSelectedObjectGeometryField(fieldId, spin->value());
    if (!ok) {
        refreshInspector();
    }
}

QLabel *DraftingFeature::makeValueLabel(const QString &text) const
{
    auto *label = new QLabel(text);
    label->setObjectName(QStringLiteral("valueLabel"));
    label->setWordWrap(true);
    return label;
}

