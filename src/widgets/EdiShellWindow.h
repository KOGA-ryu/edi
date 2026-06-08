#pragma once

#include <QMainWindow>
#include <QString>

class QAbstractButton;
class QButtonGroup;
class QCheckBox;
class QLabel;
class QPushButton;
class QWidget;

class DrawingCanvasWidget;
class DrawingDocumentController;

class EdiShellWindow final : public QMainWindow {
    Q_OBJECT

public:
    explicit EdiShellWindow(QWidget *parent = nullptr);

private slots:
    void refreshInspector();

private:
    QWidget *buildActivityRail();
    QWidget *buildLeftPanel();
    QWidget *buildWorkspaceColumn();
    QWidget *buildRightPanel();
    QWidget *buildBottomPanel();
    QPushButton *makeToolButton(const QString &toolId, const QString &label);
    QPushButton *makeRailButton(const QString &label, const QString &tooltip, bool active = false);
    QLabel *makeSectionLabel(const QString &text) const;
    QLabel *makeValueLabel(const QString &text = QString()) const;
    void applyShellStyle();

    DrawingDocumentController *m_controller = nullptr;
    DrawingCanvasWidget *m_canvas = nullptr;
    QButtonGroup *m_toolGroup = nullptr;
    QCheckBox *m_gridSnap = nullptr;
    QCheckBox *m_objectSnap = nullptr;
    QLabel *m_toolValue = nullptr;
    QLabel *m_selectedValue = nullptr;
    QLabel *m_objectsValue = nullptr;
    QLabel *m_revisionValue = nullptr;
    QLabel *m_snapValue = nullptr;
    QLabel *m_previewValue = nullptr;
    QLabel *m_statusValue = nullptr;
};
