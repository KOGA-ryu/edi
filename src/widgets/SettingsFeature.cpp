#include "widgets/SettingsFeature.h"

#include <QColor>
#include <QColorDialog>
#include <QFontComboBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QVBoxLayout>
#include <QWidget>

#include <utility>

#include "widgets/ShellWidgetHelpers.h"

using namespace edi::shell;

SettingsFeature::SettingsFeature(ShellHooks hooks, QObject *parent)
    : QObject(parent)
    , m_hooks(std::move(hooks))
{
}

QWidget *SettingsFeature::buildPanel(ShellSlot slot)
{
    if (slot == ShellSlot::Main) {
        return buildSettingsPage();
    }
    return nullptr;
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

void SettingsFeature::addColorRow(QWidget *page, const QString &label, const QString &fieldName,
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
}

QWidget *SettingsFeature::buildSettingsPage()
{
    auto [panel, layout] = makeScrollablePanel(QStringLiteral("settingsPanel"), 0, 0);

    auto *title = new QLabel(QStringLiteral("Settings"));
    title->setObjectName(QStringLiteral("panelTitle"));
    layout->addWidget(title);

    auto *themeHeader = new QLabel(QStringLiteral("Theme"));
    themeHeader->setObjectName(QStringLiteral("sectionLabel"));
    layout->addWidget(themeHeader);

    auto *host = new QWidget;
    auto *hostLayout = new QVBoxLayout(host);
    hostLayout->setContentsMargins(0, 0, 0, 0);
    hostLayout->setSpacing(6);
    layout->addWidget(host);

    addColorRow(host, QStringLiteral("Base"), QStringLiteral("themeBaseField"), &ShellThemeInputs::base);
    addColorRow(host, QStringLiteral("Surface"), QStringLiteral("themeSurfaceField"), &ShellThemeInputs::surface);
    addColorRow(host, QStringLiteral("Accent"), QStringLiteral("themeAccentField"), &ShellThemeInputs::accent);
    addColorRow(host, QStringLiteral("Text"), QStringLiteral("themeTextField"), &ShellThemeInputs::text);

    auto *typographyHeader = new QLabel(QStringLiteral("Typography"));
    typographyHeader->setObjectName(QStringLiteral("sectionLabel"));
    layout->addWidget(typographyHeader);

    const ShellThemeInputs current = m_hooks.themeInputs ? m_hooks.themeInputs() : ShellThemeInputs{};

    auto *fontRow = new QWidget;
    auto *fontLayout = new QHBoxLayout(fontRow);
    fontLayout->setContentsMargins(0, 0, 0, 0);
    fontLayout->setSpacing(8);
    auto *fontLabel = new QLabel(QStringLiteral("UI font"));
    fontLabel->setObjectName(QStringLiteral("fieldLabel"));
    fontLabel->setMinimumWidth(70);
    fontLayout->addWidget(fontLabel);
    auto *fontCombo = new QFontComboBox;
    fontCombo->setObjectName(QStringLiteral("uiFontCombo"));
    fontCombo->setCurrentFont(QFont(current.uiFont));
    fontLayout->addWidget(fontCombo);
    auto *sizeSpin = new QSpinBox;
    sizeSpin->setObjectName(QStringLiteral("uiFontSizeSpin"));
    sizeSpin->setRange(9, 28); // deriveShellTheme clamps to the same band
    sizeSpin->setValue(current.uiFontSize);
    fontLayout->addWidget(sizeSpin);
    fontLayout->addStretch(1);
    layout->addWidget(fontRow);

    auto *codeRow = new QWidget;
    auto *codeLayout = new QHBoxLayout(codeRow);
    codeLayout->setContentsMargins(0, 0, 0, 0);
    codeLayout->setSpacing(8);
    auto *codeLabel = new QLabel(QStringLiteral("Code font"));
    codeLabel->setObjectName(QStringLiteral("fieldLabel"));
    codeLabel->setMinimumWidth(70);
    codeLayout->addWidget(codeLabel);
    auto *codeCombo = new QFontComboBox;
    codeCombo->setObjectName(QStringLiteral("codeFontCombo"));
    codeCombo->setCurrentFont(QFont(current.codeFont));
    codeLayout->addWidget(codeCombo);
    auto *codeSizeSpin = new QSpinBox;
    codeSizeSpin->setObjectName(QStringLiteral("codeFontSizeSpin"));
    codeSizeSpin->setRange(9, 28);
    codeSizeSpin->setValue(current.codeFontSize);
    codeLayout->addWidget(codeSizeSpin);
    codeLayout->addStretch(1);
    layout->addWidget(codeRow);

    connect(fontCombo, &QFontComboBox::currentFontChanged, this, [this](const QFont &font) {
        pushInputs([&font](ShellThemeInputs &inputs) { inputs.uiFont = font.family(); });
    });
    connect(sizeSpin, &QSpinBox::valueChanged, this, [this](int value) {
        pushInputs([value](ShellThemeInputs &inputs) { inputs.uiFontSize = value; });
    });
    connect(codeCombo, &QFontComboBox::currentFontChanged, this, [this](const QFont &font) {
        pushInputs([&font](ShellThemeInputs &inputs) { inputs.codeFont = font.family(); });
    });
    connect(codeSizeSpin, &QSpinBox::valueChanged, this, [this](int value) {
        pushInputs([value](ShellThemeInputs &inputs) { inputs.codeFontSize = value; });
    });

    layout->addStretch(1);
    return panel;
}
