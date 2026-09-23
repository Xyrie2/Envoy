#pragma once
#include <QColor>
#include <QHash>
#include <QPointer>
#include <QVector>
#include <QWidget>
#include <functional>

#include "Core/RadarConstants.h"

class QCheckBox;
class QLabel;
class QPushButton;
class QShowEvent;
class QSlider;
class QTableWidget;

/// <summary>
/// @brief 显示设置面板
/// </summary>
class DisplaySettingsPanel : public QWidget
{
    Q_OBJECT
public:
    DisplaySettingsPanel(QWidget* parent = nullptr);

    QHash<quint8, QColor> categoryColorOverrides() const;
    QHash<quint8, int> categoryAlphaOverrides() const;
    QHash<quint8, bool> categoryVisibilityOverrides() const;

    void setCategoryColors(const QHash<quint8, QColor>& overrides);
    void setCategoryAlphas(const QHash<quint8, int>& overrides);
    void setCategoryVisibility(const QHash<quint8, bool>& overrides);

signals:
    void trackDisplayEdited();
protected:
    void showEvent(QShowEvent* event) override;
private slots:
    void showColorPickerFor(int tableRow);
private:
    QWidget* createColorPicker(const QPoint& anchorGlobal, const std::function<void(const QColor&)>& onPick);
    void adjustTableHeight();
    enum Column
    {
        ColumnCategory = 0,
        ColumnColor = 1,
        ColumnAlpha = 2,
        ColumnVisible = 3,
        ColumnCount = 4
    };
    struct DisplayRow
    {
        quint8 category = 0;
        QColor color;
        int alpha = DisplayDefaults::TrailAlpha;
        bool visible = true;
        QPushButton* colorButton = nullptr;
        QSlider* alphaSlider = nullptr;
        QLabel* alphaValueLabel = nullptr;
        QCheckBox* visibleCheckBox = nullptr;
    };

    QTableWidget* categoryColorTable = nullptr;
    QVector<DisplayRow> m_rows;
    QPointer<QWidget> m_colorPicker;
    bool m_tableHeightAdjusted = false;

    DisplayRow* rowAt(int tableRow);
    void setRowColor(int tableRow, const QColor& color);
    void setRowAlpha(int tableRow, int alpha);
    void setRowVisible(int tableRow, bool visible);
    void updateRowAlphaAt(int tableRow, int alpha);
    void updateColorCell(int tableRow);
    void updateAlphaCell(int tableRow);
    void updateVisibleCell(int tableRow);
};
