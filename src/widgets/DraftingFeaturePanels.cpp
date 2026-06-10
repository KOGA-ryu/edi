#include "widgets/DraftingFeature.h"

#include <QAbstractButton>
#include <QButtonGroup>
#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFileInfo>
#include <QFrame>
#include <QScrollArea>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPair>
#include <QPushButton>
#include <QSizePolicy>
#include <QSpinBox>
#include <QStyle>
#include <QStringList>
#include <QVariantList>
#include <QVariantMap>
#include <QVBoxLayout>
#include <QVector>

#include <optional>
#include <utility>

#include "core/DrawingCore.h"
#include "widgets/DrawingCanvasWidget.h"
#include "widgets/ShellPanels.h"
#include "widgets/ShellWidgetHelpers.h"

using namespace edi::shell;

DraftingFeature::DraftingFeature(DrawingDocumentController *controller, ShellActions actions, QObject *parent)
    : QObject(parent)
    , m_controller(controller)
    , m_actions(std::move(actions))
{
    connect(m_controller, &DrawingDocumentController::modelChanged, this, &DraftingFeature::refreshInspector);
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

void DraftingFeature::setRecentFiles(const QStringList &paths)
{
    m_recentFiles = paths;
    rebuildRecentFileButtons();
}

QWidget *DraftingFeature::buildLeftPanel()
{
    const PanelSpec leftSpec = panelSpec(ShellSlot::Left);
    auto [panel, layout] = makeScrollablePanel(QStringLiteral("leftPanel"), leftSpec.minSize, leftSpec.maxSize);

    auto *title = new QLabel(QStringLiteral("EDI Drafting"));
    title->setObjectName(QStringLiteral("panelTitle"));
    layout->addWidget(title);

    layout->addWidget(makeSectionLabel(QStringLiteral("Tools")));
    m_toolGroup = new QButtonGroup(panel);
    m_toolGroup->setExclusive(true);

    const QVector<QPair<QString, QString>> tools {
        {QStringLiteral("select_move"), QStringLiteral("Select")},
        {QStringLiteral("point_tool"), QStringLiteral("Point")},
        {QStringLiteral("line_tool"), QStringLiteral("Line")},
        {QStringLiteral("rectangle_tool"), QStringLiteral("Rectangle")},
        {QStringLiteral("circle_tool"), QStringLiteral("Circle")},
        {QStringLiteral("arc_tool"), QStringLiteral("Arc")},
        {QStringLiteral("regular_polygon_tool"), QStringLiteral("Polygon")},
        {QStringLiteral("horizontal_guide_tool"), QStringLiteral("H Guide")},
        {QStringLiteral("vertical_guide_tool"), QStringLiteral("V Guide")},
        {QStringLiteral("horizontal_construction_line_tool"), QStringLiteral("H Construct")},
        {QStringLiteral("vertical_construction_line_tool"), QStringLiteral("V Construct")},
        {QStringLiteral("angled_construction_line_tool"), QStringLiteral("A Construct")},
        {QStringLiteral("distance_dimension_tool"), QStringLiteral("Dim Distance")},
        {QStringLiteral("width_dimension_tool"), QStringLiteral("Dim Width")},
        {QStringLiteral("height_dimension_tool"), QStringLiteral("Dim Height")},
        {QStringLiteral("radius_dimension_tool"), QStringLiteral("Dim Radius")},
        {QStringLiteral("diameter_dimension_tool"), QStringLiteral("Dim Diameter")},
    };

    for (const auto &tool : tools) {
        layout->addWidget(makeToolButton(tool.first, tool.second));
    }

    connect(m_toolGroup, &QButtonGroup::buttonClicked, m_controller, [this](QAbstractButton *button) {
        m_controller->setSelectedToolId(button->property("toolId").toString());
    });

    layout->addWidget(makeSectionLabel(QStringLiteral("Tool Options")));
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
        layout->addWidget(sidesRow);
    }

    layout->addWidget(makeSectionLabel(QStringLiteral("Snap")));
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

    layout->addWidget(makeSectionLabel(QStringLiteral("Edit")));
    m_undoButton = makeActionButton(QStringLiteral("undoButton"), QStringLiteral("Undo"), [this]() {
        m_controller->undo();
    });
    layout->addWidget(m_undoButton);
    m_redoButton = makeActionButton(QStringLiteral("redoButton"), QStringLiteral("Redo"), [this]() {
        m_controller->redo();
    });
    layout->addWidget(m_redoButton);

    layout->addWidget(makeSectionLabel(QStringLiteral("Project Files")));
    layout->addWidget(makeActionButton(QStringLiteral("openDrawingButton"), QStringLiteral("Open…"), [this]() {
        m_actions.openDrawing();
    }));
    layout->addWidget(makeActionButton(QStringLiteral("saveDrawingButton"), QStringLiteral("Save"), [this]() {
        m_actions.saveDrawing();
    }));
    layout->addWidget(makeActionButton(QStringLiteral("saveDrawingAsButton"), QStringLiteral("Save As…"), [this]() {
        m_actions.saveDrawingAs();
    }));
    layout->addWidget(makeActionButton(QStringLiteral("exportSvgButton"), QStringLiteral("Export SVG…"), [this]() {
        m_actions.exportSvg();
    }));
    layout->addWidget(makeActionButton(QStringLiteral("exportHpglButton"), QStringLiteral("Export HPGL…"), [this]() {
        m_actions.exportHpgl();
    }));

    m_recentFilesContainer = new QWidget;
    m_recentFilesContainer->setObjectName(QStringLiteral("recentFilesContainer"));
    auto *recentLayout = new QVBoxLayout(m_recentFilesContainer);
    clearLayoutMargins(recentLayout);
    recentLayout->setSpacing(4);
    layout->addWidget(m_recentFilesContainer);

    layout->addWidget(makeSectionLabel(QStringLiteral("Next Surfaces")));
    layout->addWidget(makeValueLabel(QStringLiteral("Text editor")));
    layout->addWidget(makeValueLabel(QStringLiteral("Settings")));
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
    return m_canvas;
}

QWidget *DraftingFeature::buildRightPanel()
{
    const PanelSpec rightSpec = panelSpec(ShellSlot::Right);
    auto [panel, layout] = makeScrollablePanel(QStringLiteral("rightPanel"), rightSpec.minSize, rightSpec.maxSize);

    auto *title = new QLabel(QStringLiteral("Inspector"));
    title->setObjectName(QStringLiteral("panelTitle"));
    layout->addWidget(title);

    layout->addWidget(makeSectionLabel(QStringLiteral("Selection")));
    m_selectedValue = makeValueLabel();
    layout->addWidget(m_selectedValue);

    layout->addWidget(makeSectionLabel(QStringLiteral("Selected Object")));
    m_objectKindValue = makeValueLabel();
    m_objectBoundsValue = makeValueLabel();
    m_objectGeometryValue = makeValueLabel();
    m_objectLayerValue = makeValueLabel();
    m_objectMeasurementValue = makeValueLabel();
    m_objectPlotSafetyValue = makeValueLabel();
    m_selectionPlotBoundsValue = makeValueLabel();
    layout->addWidget(m_objectKindValue);
    layout->addWidget(m_objectBoundsValue);
    layout->addWidget(m_objectGeometryValue);
    layout->addWidget(m_objectLayerValue);
    layout->addWidget(m_objectMeasurementValue);
    layout->addWidget(m_objectPlotSafetyValue);
    layout->addWidget(m_selectionPlotBoundsValue);
    layout->addWidget(makeConditionalButton(QStringLiteral("fitToDrawableButton"), QStringLiteral("Fit To Drawable"), QStringLiteral("has_selection"), [this]() {
        m_controller->fitSelectionToDrawableBounds();
    }));
    layout->addWidget(makeConditionalButton(QStringLiteral("centerInDrawableButton"), QStringLiteral("Center In Drawable"), QStringLiteral("has_selection"), [this]() {
        m_controller->centerSelectionInDrawable();
    }));
    layout->addWidget(makeConditionalButton(QStringLiteral("moveToDrawableOriginButton"), QStringLiteral("Move To Drawable Origin"), QStringLiteral("has_selection"), [this]() {
        m_controller->moveSelectionToDrawableOrigin();
    }));
    layout->addWidget(makeConditionalButton(QStringLiteral("guideToDrawableOriginButton"), QStringLiteral("Guide To Drawable Origin"), QStringLiteral("guide_drawable_controls"), [this]() {
        m_controller->moveSelectedGuideToDrawableOrigin();
    }));
    layout->addWidget(makeConditionalButton(QStringLiteral("centerGuideButton"), QStringLiteral("Center Guide"), QStringLiteral("guide_drawable_controls"), [this]() {
        m_controller->centerSelectedGuideInDrawable();
    }));
    layout->addWidget(makeConditionalButton(QStringLiteral("guideToDrawableMaxButton"), QStringLiteral("Guide To Drawable Max"), QStringLiteral("guide_drawable_controls"), [this]() {
        m_controller->moveSelectedGuideToDrawableMax();
    }));
    const QVector<QPair<QString, QString>> guideOffsetButtons {
        {QStringLiteral("negative_fine"), QStringLiteral("Guide - Fine")},
        {QStringLiteral("positive_fine"), QStringLiteral("Guide + Fine")},
        {QStringLiteral("negative_grid"), QStringLiteral("Guide - Grid")},
        {QStringLiteral("positive_grid"), QStringLiteral("Guide + Grid")},
        {QStringLiteral("negative_coarse"), QStringLiteral("Guide - Coarse")},
        {QStringLiteral("positive_coarse"), QStringLiteral("Guide + Coarse")},
    };
    for (const auto &buttonSpec : guideOffsetButtons) {
        const QStringList parts = buttonSpec.first.split(QLatin1Char('_'));
        auto *button = makeActionButton(QStringLiteral("guideOffset_%1").arg(buttonSpec.first), buttonSpec.second, [this, direction = parts.value(0), stepMode = parts.value(1)]() {
            m_controller->offsetSelectedGuide(direction, stepMode);
        });
        m_guideOffsetButtons.insert(buttonSpec.first, button);
        layout->addWidget(button);
    }
    layout->addWidget(makeSectionLabel(QStringLiteral("Guide Presets")));
    const QVector<QPair<QString, QString>> guidePresetButtons {
        {QStringLiteral("drawable_bounds"), QStringLiteral("Preset Bounds")},
        {QStringLiteral("drawable_centerlines"), QStringLiteral("Preset Centerlines")},
        {QStringLiteral("thirds"), QStringLiteral("Preset Thirds")},
        {QStringLiteral("quarters"), QStringLiteral("Preset Quarters")},
        {QStringLiteral("margin_safe"), QStringLiteral("Preset Margin Safe")},
    };
    for (const auto &buttonSpec : guidePresetButtons) {
        auto *button = makeActionButton(QStringLiteral("guidePreset_%1").arg(buttonSpec.first), buttonSpec.second, [this, presetId = buttonSpec.first]() {
            m_controller->applyGuidePreset(presetId);
        });
        layout->addWidget(button);
    }
    layout->addWidget(makeConditionalButton(QStringLiteral("fitConstructionToDrawableButton"), QStringLiteral("Fit Construction To Drawable"), QStringLiteral("construction_drawable_controls"), [this]() {
        m_controller->fitSelectedConstructionLineToDrawable();
    }));
    layout->addWidget(makeSectionLabel(QStringLiteral("Bounds Guides")));
    const QVector<QPair<QString, QString>> boundsGuideButtons {
        {QStringLiteral("left"), QStringLiteral("Guide Left")},
        {QStringLiteral("right"), QStringLiteral("Guide Right")},
        {QStringLiteral("top"), QStringLiteral("Guide Top")},
        {QStringLiteral("bottom"), QStringLiteral("Guide Bottom")},
        {QStringLiteral("vertical_center"), QStringLiteral("Guide V Center")},
        {QStringLiteral("horizontal_center"), QStringLiteral("Guide H Center")},
    };
    for (const auto &buttonSpec : boundsGuideButtons) {
        auto *button = makeActionButton(QStringLiteral("boundsGuide_%1").arg(buttonSpec.first), buttonSpec.second, [this, placementId = buttonSpec.first]() {
            m_controller->createGuideFromSelectedBounds(placementId);
        });
        m_boundsGuideButtons.insert(buttonSpec.first, button);
        layout->addWidget(button);
    }
    layout->addWidget(makeSectionLabel(QStringLiteral("Offset Guides")));
    const QVector<QPair<QString, QString>> offsetGuideButtons {
        {QStringLiteral("left"), QStringLiteral("Offset V Left")},
        {QStringLiteral("right"), QStringLiteral("Offset V Right")},
        {QStringLiteral("top"), QStringLiteral("Offset H Top")},
        {QStringLiteral("bottom"), QStringLiteral("Offset H Bottom")},
    };
    for (const auto &buttonSpec : offsetGuideButtons) {
        auto *button = makeActionButton(QStringLiteral("offsetGuide_%1").arg(buttonSpec.first), buttonSpec.second, [this, placementId = buttonSpec.first]() {
            m_controller->createOffsetGuideFromSelectedBounds(placementId, QStringLiteral("grid"));
        });
        m_offsetGuideButtons.insert(buttonSpec.first, button);
        layout->addWidget(button);
    }
    layout->addWidget(makeSectionLabel(QStringLiteral("Align To Guide")));
    const QVector<QPair<QString, QString>> alignToGuideButtons {
        {QStringLiteral("left"), QStringLiteral("To V Guide Left")},
        {QStringLiteral("center_x"), QStringLiteral("To V Guide Center")},
        {QStringLiteral("right"), QStringLiteral("To V Guide Right")},
        {QStringLiteral("top"), QStringLiteral("To H Guide Top")},
        {QStringLiteral("center_y"), QStringLiteral("To H Guide Center")},
        {QStringLiteral("bottom"), QStringLiteral("To H Guide Bottom")},
    };
    for (const auto &buttonSpec : alignToGuideButtons) {
        auto *button = makeActionButton(QStringLiteral("alignToGuide_%1").arg(buttonSpec.first), buttonSpec.second, [this, modeId = buttonSpec.first]() {
            m_controller->alignSelectionToNearestGuide(modeId);
        });
        m_alignToGuideButtons.insert(buttonSpec.first, button);
        layout->addWidget(button);
    }
    layout->addWidget(makeSectionLabel(QStringLiteral("Guide Lifecycle")));
    layout->addWidget(makeConditionalButton(QStringLiteral("deleteSelectedGuideButton"), QStringLiteral("Delete Selected Guide"), QStringLiteral("guide_drawable_controls"), [this]() {
        m_controller->deleteSelectedGuide();
    }));
    m_deleteAllGuidesButton = makeActionButton(QStringLiteral("deleteAllGuidesButton"), QStringLiteral("Delete All Guides"), [this]() {
        m_controller->deleteAllGuides();
    });
    layout->addWidget(m_deleteAllGuidesButton);
    m_mergeDuplicateGuidesButton = makeActionButton(QStringLiteral("mergeDuplicateGuidesButton"), QStringLiteral("Merge Duplicate Guides"), [this]() {
        m_controller->mergeDuplicateGuides();
    });
    layout->addWidget(m_mergeDuplicateGuidesButton);
    m_hideAllGuidesButton = makeActionButton(QStringLiteral("hideAllGuidesButton"), QStringLiteral("Hide All Guides"), [this]() {
        m_controller->setAllGuidesVisible(false);
    });
    layout->addWidget(m_hideAllGuidesButton);
    m_showAllGuidesButton = makeActionButton(QStringLiteral("showAllGuidesButton"), QStringLiteral("Show All Guides"), [this]() {
        m_controller->setAllGuidesVisible(true);
    });
    layout->addWidget(m_showAllGuidesButton);
    m_lockAllGuidesButton = makeActionButton(QStringLiteral("lockAllGuidesButton"), QStringLiteral("Lock All Guides"), [this]() {
        m_controller->setAllGuidesLocked(true);
    });
    layout->addWidget(m_lockAllGuidesButton);
    m_unlockAllGuidesButton = makeActionButton(QStringLiteral("unlockAllGuidesButton"), QStringLiteral("Unlock All Guides"), [this]() {
        m_controller->setAllGuidesLocked(false);
    });
    layout->addWidget(m_unlockAllGuidesButton);
    layout->addWidget(makeSectionLabel(QStringLiteral("Guide Visuals")));
    m_guideLabel = new QLineEdit;
    m_guideLabel->setObjectName(QStringLiteral("guideLabelField"));
    m_guideLabel->setPlaceholderText(QStringLiteral("Default guide label"));
    connect(m_guideLabel, &QLineEdit::editingFinished, this, [this]() {
        m_controller->setSelectedGuideLabel(m_guideLabel->text());
    });
    layout->addWidget(m_guideLabel);
    m_guideColor = makeDataCombo(QStringLiteral("guideColorCombo"), {
        {QStringLiteral("Guide blue"), QStringLiteral("#83aeca")},
        {QStringLiteral("Guide teal"), QStringLiteral("#54d2c6")},
        {QStringLiteral("Guide amber"), QStringLiteral("#f6c65b")},
        {QStringLiteral("Guide red"), QStringLiteral("#d98b8b")},
        {QStringLiteral("Guide green"), QStringLiteral("#91c89b")},
    }, [this](const QString &color) {
        m_controller->setSelectedGuideColor(color);
    });
    layout->addWidget(m_guideColor);
    m_guideDashStyle = makeDataCombo(QStringLiteral("guideDashStyleCombo"), {
        {QStringLiteral("Dash line"), QStringLiteral("dash")},
        {QStringLiteral("Solid line"), QStringLiteral("solid")},
        {QStringLiteral("Dot line"), QStringLiteral("dot")},
    }, [this](const QString &dashStyle) {
        m_controller->setSelectedGuideDashStyle(dashStyle);
    });
    layout->addWidget(m_guideDashStyle);
    m_guideShowLabel = makeToggle(QStringLiteral("guideShowLabelCheckbox"), QStringLiteral("Show guide label"), [this](bool checked) {
        m_controller->setSelectedGuideLabelVisible(checked);
    });
    layout->addWidget(m_guideShowLabel);
    layout->addWidget(makeSectionLabel(QStringLiteral("Dimension")));
    m_dimensionReadout = makeValueLabel(QStringLiteral("Dimension: none"));
    layout->addWidget(m_dimensionReadout);
    m_dimensionKind = makeDataCombo(QStringLiteral("dimensionKindCombo"), {
        {QStringLiteral("Distance"), QStringLiteral("distance")},
        {QStringLiteral("Width"), QStringLiteral("width")},
        {QStringLiteral("Height"), QStringLiteral("height")},
        {QStringLiteral("Radius"), QStringLiteral("radius")},
        {QStringLiteral("Diameter"), QStringLiteral("diameter")},
    }, [this](const QString &kindId) {
        m_controller->setSelectedDimensionKind(kindId);
    });
    layout->addWidget(m_dimensionKind);
    m_dimensionShowLabel = makeToggle(QStringLiteral("dimensionShowLabelCheckbox"), QStringLiteral("Show dimension label"), [this](bool checked) {
        m_controller->setSelectedDimensionLabelVisible(checked);
    });
    layout->addWidget(m_dimensionShowLabel);
    layout->addWidget(buildObjectFlagControls());
    layout->addWidget(buildLayerControls());
    m_geometryEditor = buildGeometryEditor();
    layout->addWidget(m_geometryEditor);
    m_geometryEditStatus = makeValueLabel();
    m_geometryEditStatus->setObjectName(QStringLiteral("editErrorLabel"));
    m_geometryEditStatus->setVisible(false);
    layout->addWidget(m_geometryEditStatus);
    layout->addWidget(buildNudgeControls());
    layout->addWidget(buildAlignControls());
    layout->addWidget(buildOffsetControls());
    layout->addWidget(buildMirrorControls());
    layout->addWidget(buildRepeatControls());
    layout->addWidget(buildCalibrationControls());

    layout->addWidget(makeSectionLabel(QStringLiteral("Document")));
    m_toolValue = makeValueLabel();
    m_objectsValue = makeValueLabel();
    m_guidesValue = makeValueLabel();
    m_revisionValue = makeValueLabel();
    layout->addWidget(m_toolValue);
    layout->addWidget(m_objectsValue);
    layout->addWidget(m_guidesValue);
    layout->addWidget(m_revisionValue);

    layout->addWidget(makeSectionLabel(QStringLiteral("Canvas State")));
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
    layout->addWidget(m_snapValue);
    layout->addWidget(m_gridValue);
    layout->addWidget(m_plotValue);
    layout->addWidget(m_plotBoundsValue);
    layout->addWidget(m_plotLayerStatsValue);
    layout->addWidget(m_plotPenStatsValue);
    layout->addWidget(m_plotReadinessValue);
    layout->addWidget(m_plotOrderMode);
    layout->addWidget(m_plotDirectionMode);
    layout->addWidget(m_plotPreviewVisible);
    layout->addWidget(m_pointerValue);
    layout->addWidget(m_quickMeasureValue);
    layout->addWidget(m_guideDragValue);
    layout->addWidget(m_previewValue);
    layout->addStretch(1);

    return panel;
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

    layout->addWidget(makeSectionLabel(QStringLiteral("Layers")));

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

    m_selectedObjectLayer = makeDataCombo(QStringLiteral("selectedObjectLayerCombo"), {}, [this](const QString &layerId) {
        m_controller->moveSelectedObjectToLayer(layerId);
    });
    layout->addWidget(m_selectedObjectLayer);
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

    layout->addWidget(makeSectionLabel(QStringLiteral("Nudge")), 0, 0, 1, 4);
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

    layout->addWidget(makeSectionLabel(QStringLiteral("Align")), 0, 0, 1, 3);
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

    layout->addWidget(makeSectionLabel(QStringLiteral("Offset")), 0, 0, 1, 2);

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

    layout->addWidget(makeSectionLabel(QStringLiteral("Mirror")), 0, 0, 1, 2);

    auto *horizontal = makeActionButton(QStringLiteral("mirrorButton"), QStringLiteral("Mirror H"), [this]() {
        m_controller->mirrorSelectedObject(QStringLiteral("horizontal"));
    });
    layout->addWidget(horizontal, 1, 0);

    auto *vertical = makeActionButton(QStringLiteral("mirrorButton"), QStringLiteral("Mirror V"), [this]() {
        m_controller->mirrorSelectedObject(QStringLiteral("vertical"));
    });
    layout->addWidget(vertical, 1, 1);

    return panel;
}

QWidget *DraftingFeature::buildRepeatControls()
{
    const auto [panel, layout] = makeControlGrid(QStringLiteral("repeatControls"));

    layout->addWidget(makeSectionLabel(QStringLiteral("Repeat")), 0, 0, 1, 2);

    auto *x = makeActionButton(QStringLiteral("repeatButton"), QStringLiteral("Repeat X"), [this]() {
        m_controller->repeatSelectedObject(QStringLiteral("x"));
    });
    layout->addWidget(x, 1, 0);

    auto *y = makeActionButton(QStringLiteral("repeatButton"), QStringLiteral("Repeat Y"), [this]() {
        m_controller->repeatSelectedObject(QStringLiteral("y"));
    });
    layout->addWidget(y, 1, 1);

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

    layout->addWidget(makeSectionLabel(QStringLiteral("Calibration")), 0, 0, 1, 3);
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

QPushButton *DraftingFeature::makeToolButton(const QString &toolId, const QString &label)
{
    auto *button = new QPushButton(label);
    button->setCheckable(true);
    button->setProperty("toolId", toolId);
    button->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    if (toolId == m_controller->selectedToolId()) {
        button->setChecked(true);
    }
    m_toolGroup->addButton(button);
    return button;
}

QLabel *DraftingFeature::makeValueLabel(const QString &text) const
{
    auto *label = new QLabel(text);
    label->setObjectName(QStringLiteral("valueLabel"));
    label->setWordWrap(true);
    return label;
}

void DraftingFeature::rebuildRecentFileButtons()
{
    if (m_recentFilesContainer == nullptr) {
        return;
    }
    auto *layout = qobject_cast<QVBoxLayout *>(m_recentFilesContainer->layout());
    if (layout == nullptr) {
        return;
    }
    QLayoutItem *item = nullptr;
    while ((item = layout->takeAt(0)) != nullptr) {
        delete item->widget();
        delete item;
    }
    // Surface the five most recent files as quick-open buttons.
    const int shown = qMin(m_recentFiles.size(), 5);
    for (int i = 0; i < shown; ++i) {
        const QString path = m_recentFiles.at(i);
        auto *button = new QPushButton(QFileInfo(path).fileName());
        button->setObjectName(QStringLiteral("recentFileButton"));
        button->setToolTip(path);
        connect(button, &QPushButton::clicked, this, [this, path]() {
            m_actions.openDrawingAtPath(path);
        });
        layout->addWidget(button);
    }
}
