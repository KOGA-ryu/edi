#include "widgets/SettingsFeature.h"

#include <QButtonGroup>
#include <QCheckBox>
#include <QColor>
#include <QColorDialog>
#include <QComboBox>
#include <QFontComboBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QStackedWidget>
#include <QVBoxLayout>
#include <QWidget>

#include <functional>
#include <utility>
#include <vector>

#include "widgets/ShellWidgetHelpers.h"

using namespace edi::shell;

SettingsFeature::SettingsFeature(ShellHooks hooks, QObject *parent)
    : QObject(parent)
    , m_hooks(std::move(hooks))
{
}

QWidget *SettingsFeature::buildPanel(ShellSlot slot)
{
    if (slot != ShellSlot::Main) {
        return nullptr;
    }

    // The page table: adding a settings page is appending a row here.
    struct PageSpec {
        QString id;
        QString label;
        std::function<QWidget *()> build;
    };
    const std::vector<PageSpec> pages = {
        {QStringLiteral("theme"), QStringLiteral("Theme"), [this]() { return buildSettingsPage(); }},
        {QStringLiteral("tool_belt"), QStringLiteral("Tool Belt"), [this]() { return buildBeltPage(); }},
        {QStringLiteral("panels"), QStringLiteral("Panels"), [this]() { return buildPanelsPage(); }},
    };

    auto *host = new QWidget;
    host->setObjectName(QStringLiteral("settingsHost"));
    auto *hostLayout = new QHBoxLayout(host);
    hostLayout->setContentsMargins(0, 0, 0, 0);
    hostLayout->setSpacing(0);

    auto *strip = new QWidget;
    strip->setObjectName(QStringLiteral("settingsPageStrip"));
    auto *stripLayout = new QVBoxLayout(strip);
    stripLayout->setContentsMargins(8, 8, 8, 8);
    stripLayout->setSpacing(6);

    auto *stack = new QStackedWidget;
    stack->setObjectName(QStringLiteral("settingsPageStack"));

    auto *pageGroup = new QButtonGroup(host);
    pageGroup->setExclusive(true);
    for (std::size_t i = 0; i < pages.size(); ++i) {
        QWidget *page = pages[i].build();
        const int index = stack->addWidget(page);
        QPushButton *button = makeRailButton(pages[i].label, pages[i].label, i == 0);
        button->setObjectName(QStringLiteral("settingsPageButton"));
        button->setProperty("pageId", pages[i].id);
        pageGroup->addButton(button);
        stripLayout->addWidget(button);
        connect(button, &QPushButton::clicked, stack, [stack, index]() { stack->setCurrentIndex(index); });
    }
    stripLayout->addStretch(1);

    hostLayout->addWidget(strip);
    hostLayout->addWidget(stack, 1);
    return host;
}

void SettingsFeature::pushInputs(const std::function<void(ShellThemeInputs &)> &edit)
{
    if (!m_hooks.themeInputs || !m_hooks.setThemeInputs) {
        return;
    }
    ShellThemeInputs inputs = m_hooks.themeInputs();
    edit(inputs);
    m_hooks.setThemeInputs(inputs);
}

namespace {

// The swatch shows the row's current color; its style is data from the user's
// input, not a themed literal.
void paintSwatch(QPushButton *swatch, const QString &hex)
{
    swatch->setStyleSheet(QStringLiteral("background: %1; border: 1px solid %1; min-width: 22px; max-width: 22px;")
        .arg(QColor(hex).isValid() ? hex : QStringLiteral("#000000")));
}

} // namespace

QLineEdit *SettingsFeature::addColorRow(QWidget *page, const QString &label, const QString &fieldName,
    QString ShellThemeInputs::*member)
{
    auto *row = new QWidget;
    auto *rowLayout = new QHBoxLayout(row);
    rowLayout->setContentsMargins(0, 0, 0, 0);
    rowLayout->setSpacing(8);

    auto *name = new QLabel(label);
    name->setObjectName(QStringLiteral("fieldLabel"));
    name->setMinimumWidth(70);
    rowLayout->addWidget(name);

    auto *field = new QLineEdit(m_hooks.themeInputs ? m_hooks.themeInputs().*member : QString());
    field->setObjectName(fieldName);
    field->setMaxLength(7);
    rowLayout->addWidget(field);

    auto *swatch = new QPushButton;
    swatch->setObjectName(fieldName + QStringLiteral("Swatch"));
    swatch->setToolTip(QStringLiteral("Pick %1 color").arg(label.toLower()));
    paintSwatch(swatch, field->text());
    rowLayout->addWidget(swatch);
    rowLayout->addStretch(1);

    // Typing a valid hex re-themes live; invalid text just waits — the theme
    // never sees a half-typed color.
    connect(field, &QLineEdit::textChanged, this, [this, member, swatch](const QString &text) {
        if (!QColor(text).isValid() || !text.startsWith(QLatin1Char('#')) || text.size() != 7) {
            return;
        }
        paintSwatch(swatch, text);
        pushInputs([member, &text](ShellThemeInputs &inputs) { inputs.*member = text; });
    });
    connect(swatch, &QPushButton::clicked, this, [this, member, field]() {
        const QString current = m_hooks.themeInputs ? m_hooks.themeInputs().*member : QString();
        const QColor picked = QColorDialog::getColor(QColor(current), field->window());
        if (picked.isValid()) {
            field->setText(picked.name()); // textChanged does the rest
        }
    });

    page->layout()->addWidget(row);
    return field;
}

QWidget *SettingsFeature::buildSettingsPage()
{
    auto [panel, layout] = makeScrollablePanel(QStringLiteral("settingsPanel"), 0, 0);

    //auto *title = new QLabel(QStringLiteral("Settings"));
    //title->setObjectName(QStringLiteral("panelTitle"));
    //layout->addWidget(title);

    layout->addWidget(makeSectionLabel(QStringLiteral("Theme")));

    auto *host = new QWidget;
    auto *hostLayout = new QVBoxLayout(host);
    hostLayout->setContentsMargins(0, 0, 0, 0);
    hostLayout->setSpacing(6);
    layout->addWidget(host);

    QLineEdit *baseField = addColorRow(host, QStringLiteral("Base"), QStringLiteral("themeBaseField"), &ShellThemeInputs::base);
    QLineEdit *surfaceField = addColorRow(host, QStringLiteral("Surface"), QStringLiteral("themeSurfaceField"), &ShellThemeInputs::surface);
    QLineEdit *accentField = addColorRow(host, QStringLiteral("Accent"), QStringLiteral("themeAccentField"), &ShellThemeInputs::accent);
    QLineEdit *textField = addColorRow(host, QStringLiteral("Text"), QStringLiteral("themeTextField"), &ShellThemeInputs::text);

    layout->addWidget(makeSectionLabel(QStringLiteral("Typography")));

    // One row shape, two instances — which font/size members a row edits is
    // data (member pointers), the same idiom as the color rows.
    const auto addFontRow = [this, layout = layout](const QString &label, const QString &comboName,
                                const QString &spinName, QString ShellThemeInputs::*fontMember,
                                int ShellThemeInputs::*sizeMember) {
        const ShellThemeInputs current = m_hooks.themeInputs ? m_hooks.themeInputs() : ShellThemeInputs{};
        auto *row = new QWidget;
        auto *rowLayout = new QHBoxLayout(row);
        rowLayout->setContentsMargins(0, 0, 0, 0);
        rowLayout->setSpacing(8);
        auto *name = new QLabel(label);
        name->setObjectName(QStringLiteral("fieldLabel"));
        name->setMinimumWidth(30);
        rowLayout->addWidget(name);
        auto *combo = new QFontComboBox;
        combo->setObjectName(comboName);
        combo->setCurrentFont(QFont(current.*fontMember));
        rowLayout->addWidget(combo);
        auto *spin = new QSpinBox;
        spin->setObjectName(spinName);
        spin->setRange(9, 28); // deriveShellTheme clamps to the same band
        spin->setValue(current.*sizeMember);
        rowLayout->addWidget(spin);
        rowLayout->addStretch(1);
        layout->addWidget(row);
        connect(combo, &QFontComboBox::currentFontChanged, this, [this, fontMember](const QFont &font) {
            pushInputs([fontMember, &font](ShellThemeInputs &inputs) { inputs.*fontMember = font.family(); });
        });
        connect(spin, &QSpinBox::valueChanged, this, [this, sizeMember](int value) {
            pushInputs([sizeMember, value](ShellThemeInputs &inputs) { inputs.*sizeMember = value; });
        });
        return std::pair{combo, spin};
    };
    auto [fontCombo, sizeSpin] = addFontRow(QStringLiteral("UI font"), QStringLiteral("uiFontCombo"),
        QStringLiteral("uiFontSizeSpin"), &ShellThemeInputs::uiFont, &ShellThemeInputs::uiFontSize);
    auto [codeCombo, codeSizeSpin] = addFontRow(QStringLiteral("Code font"), QStringLiteral("codeFontCombo"),
        QStringLiteral("codeFontSizeSpin"), &ShellThemeInputs::codeFont, &ShellThemeInputs::codeFontSize);

    // Profiles: pick one to load it; type a name and save to snapshot the
    // current theme under it. The page refreshes its rows after a load so the
    // view never shows a stale theme.
    layout->addWidget(makeSectionLabel(QStringLiteral("Profiles")));

    auto *profileRow = new QWidget;
    auto *profileLayout = new QHBoxLayout(profileRow);
    profileLayout->setContentsMargins(0, 0, 0, 0);
    profileLayout->setSpacing(8);
    auto *profileCombo = new QComboBox;
    profileCombo->setObjectName(QStringLiteral("profileCombo"));
    if (m_hooks.profiles) {
        profileCombo->addItems(m_hooks.profiles());
    }
    if (m_hooks.activeProfile) {
        profileCombo->setCurrentText(m_hooks.activeProfile());
    }
    profileLayout->addWidget(profileCombo, 1);
    auto *nameField = new QLineEdit;
    nameField->setObjectName(QStringLiteral("profileNameField"));
    nameField->setPlaceholderText(QStringLiteral("profile name"));
    profileLayout->addWidget(nameField, 1);
    auto *saveButton = new QPushButton(QStringLiteral("Save profile"));
    saveButton->setObjectName(QStringLiteral("saveProfileButton"));
    profileLayout->addWidget(saveButton);
    layout->addWidget(profileRow);

    const auto refreshThemeRows = [this, baseField, surfaceField, accentField, textField, fontCombo, sizeSpin,
                                      codeCombo, codeSizeSpin]() {
        if (!m_hooks.themeInputs) {
            return;
        }
        const ShellThemeInputs inputs = m_hooks.themeInputs();
        baseField->setText(inputs.base);
        surfaceField->setText(inputs.surface);
        accentField->setText(inputs.accent);
        textField->setText(inputs.text);
        fontCombo->setCurrentFont(QFont(inputs.uiFont));
        sizeSpin->setValue(inputs.uiFontSize);
        codeCombo->setCurrentFont(QFont(inputs.codeFont));
        codeSizeSpin->setValue(inputs.codeFontSize);
    };
    connect(profileCombo, &QComboBox::activated, this, [this, profileCombo, refreshThemeRows](int index) {
        if (m_hooks.loadProfile && m_hooks.loadProfile(profileCombo->itemText(index))) {
            refreshThemeRows();
        }
    });
    connect(saveButton, &QPushButton::clicked, this, [this, nameField, profileCombo]() {
        const QString name = nameField->text().isEmpty() ? profileCombo->currentText() : nameField->text();
        if (!m_hooks.saveProfile || name.isEmpty() || !m_hooks.saveProfile(name)) {
            return;
        }
        if (m_hooks.profiles) { // re-list so a new name appears immediately
            profileCombo->clear();
            profileCombo->addItems(m_hooks.profiles());
            profileCombo->setCurrentText(name);
        }
    });

    layout->addStretch(1);
    return panel;
}

QWidget *SettingsFeature::buildBeltPage()
{
    auto [panel, layout] = makeScrollablePanel(QStringLiteral("beltPanel"), 0, 0);

    //auto *title = new QLabel(QStringLiteral("Tool Belt"));
    //title->setObjectName(QStringLiteral("panelTitle"));
    //layout->addWidget(title);

    layout->addWidget(makeSectionLabel(QStringLiteral("On the belt")));

    const QVector<QPair<QString, QString>> inventory =
        m_hooks.toolInventory ? m_hooks.toolInventory() : QVector<QPair<QString, QString>>{};
    const QStringList onBelt = m_hooks.beltToolIds ? m_hooks.beltToolIds() : QStringList{};

    // The checkboxes ARE the model here: a toggle re-reads every box and
    // hands the shell the full enabled list, so there is no second copy of
    // "what's on the belt" to fall out of sync.
    auto *checkHost = new QWidget;
    auto *checkLayout = new QVBoxLayout(checkHost);
    checkLayout->setContentsMargins(0, 0, 0, 0);
    checkLayout->setSpacing(4);
    QVector<QCheckBox *> boxes;
    for (const auto &tool : inventory) {
        auto *box = new QCheckBox(tool.second);
        box->setObjectName(QStringLiteral("beltToolCheckbox"));
        box->setProperty("toolId", tool.first);
        box->setChecked(onBelt.contains(tool.first));
        checkLayout->addWidget(box);
        boxes.push_back(box);
    }
    layout->addWidget(checkHost);

    for (QCheckBox *box : boxes) {
        connect(box, &QCheckBox::toggled, this, [this, boxes]() {
            if (!m_hooks.setBeltToolIds) {
                return;
            }
            QStringList enabled;
            for (QCheckBox *candidate : boxes) {
                if (candidate->isChecked()) {
                    enabled.push_back(candidate->property("toolId").toString());
                }
            }
            m_hooks.setBeltToolIds(enabled);
        });
    }

    layout->addStretch(1);
    return panel;
}

QWidget *SettingsFeature::buildPanelsPage()
{
    // The user's modular-panels page: one row per content group, a combo
    // choosing its panel. Writes flow through the shell hook one assignment
    // at a time; the shell re-places groups LIVE (reparent, no remount), so
    // this page keeps working mid-edit — the same constraint that shaped
    // the belt checklist.
    auto *page = new QWidget;
    page->setObjectName(QStringLiteral("settingsPanelsPage"));
    auto *layout = new QVBoxLayout(page);
    layout->setContentsMargins(12, 12, 12, 12);
    layout->setSpacing(6);
    layout->addWidget(makeSectionLabel(QStringLiteral("Panel Contents")));

    if (m_hooks.panelGroupInventory && m_hooks.panelSlotForGroup && m_hooks.setPanelSlotForGroup) {
        // Not named `slots`: Qt macros that identifier away (same trap the
        // FeatureDescriptor comment records) and the declaration dissolves
        // into a syntax error.
        const QVector<QPair<QString, QString>> slotChoices = {
            {QStringLiteral("Right panel"), QStringLiteral("right")},
            {QStringLiteral("Left panel"), QStringLiteral("left")},
            {QStringLiteral("Terminal"), QStringLiteral("bottom")},
            {QStringLiteral("Hidden"), QStringLiteral("hidden")},
        };
        for (const auto &group : m_hooks.panelGroupInventory()) {
            auto *row = new QWidget;
            auto *rowLayout = new QHBoxLayout(row);
            rowLayout->setContentsMargins(0, 0, 0, 0);
            rowLayout->setSpacing(8);
            auto *label = new QLabel(group.second);
            label->setObjectName(QStringLiteral("fieldLabel"));
            rowLayout->addWidget(label, 1);
            auto *combo = new QComboBox;
            combo->setObjectName(QStringLiteral("panelSlotCombo"));
            combo->setProperty("groupId", group.first);
            for (const auto &choice : slotChoices) {
                combo->addItem(choice.first, choice.second);
            }
            const int current = combo->findData(m_hooks.panelSlotForGroup(group.first));
            combo->setCurrentIndex(current >= 0 ? current : 0);
            const QString groupId = group.first;
            connect(combo, &QComboBox::currentIndexChanged, this, [this, combo, groupId](int index) {
                m_hooks.setPanelSlotForGroup(groupId, combo->itemData(index).toString());
            });
            rowLayout->addWidget(combo);
            layout->addWidget(row);
        }
    }
    layout->addStretch(1);
    return page;
}
