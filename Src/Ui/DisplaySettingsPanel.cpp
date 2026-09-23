#include "Ui/DisplaySettingsPanel.h"
#include "Core/RadarConstants.h"
#include "Ui/Theme.h"

#include <QApplication>
#include <QCheckBox>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QPushButton>
#include <QScreen>
#include <QScrollArea>
#include <QSignalBlocker>
#include <QSlider>
#include <QSpacerItem>
#include <QTableWidget>
#include <QTimer>
#include <QVBoxLayout>

namespace
{
/// @brief 颜色筛选弹窗的可选颜色盘
QVector<QColor> pickerPalette()
{
    static const QVector<QColor> palette = {
        QColor(0xef, 0x44, 0x44), QColor(0xf4, 0x3f, 0x5e), QColor(0xec, 0x48, 0x99), QColor(0xd9, 0x46, 0xef),
        QColor(0xa8, 0x55, 0xf7), QColor(0x8b, 0x5c, 0xf6), QColor(0x63, 0x66, 0xf1), QColor(0x3b, 0x82, 0xf6),
        QColor(0x0e, 0xa5, 0xe9), QColor(0x06, 0xb6, 0xd4), QColor(0x14, 0xb8, 0xa6), QColor(0x10, 0xb9, 0x81),
        QColor(0x22, 0xc5, 0x5e), QColor(0x84, 0xcc, 0x16), QColor(0xea, 0xb3, 0x08), QColor(0xf5, 0x9e, 0x0b),
        QColor(0xf9, 0x73, 0x16), QColor(0x94, 0xa3, 0xb8), QColor(0x64, 0x74, 0x8b), QColor(0x6b, 0x72, 0x80),
        QColor(0x1f, 0x29, 0x37), QColor(0x06, 0x5f, 0x46), QColor(0x1d, 0x4e, 0xd8), QColor(0x7c, 0x2d, 0x12),
    };
    return palette;
}
}

DisplaySettingsPanel::DisplaySettingsPanel(QWidget* parent) : QWidget(parent)
{
    setObjectName(QStringLiteral("displaySettingsPage"));
    QVBoxLayout* pageLayout = new QVBoxLayout(this);
    pageLayout->setContentsMargins(2, 2, 2, 2);
    pageLayout->setSpacing(2);

    // 滚动区
    QScrollArea* scrollArea = new QScrollArea(this);
    scrollArea->setObjectName(QStringLiteral("displayScrollArea"));
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setWidgetResizable(true);

    QWidget* content = new QWidget;
    content->setObjectName(QStringLiteral("displayPanel"));

    QVBoxLayout* contentLayout = new QVBoxLayout(content);
    contentLayout->setContentsMargins(4, 4, 4, 4);
    contentLayout->setSpacing(6);

    // 航迹显示
    QGroupBox* displayGroupBox = new QGroupBox(tr("航迹显示"), content);
    displayGroupBox->setObjectName(QStringLiteral("TrailDisplayGroupBox"));
    QVBoxLayout* displayLayout = new QVBoxLayout(displayGroupBox);
    displayLayout->setContentsMargins(10, 10, 10, 10); /// 组内四周边距统一
    displayLayout->setSpacing(5);

    const QVector<TrackDisplayColors::CategoryColor>& colorTable = TrackDisplayColors::Table();
    categoryColorTable = new QTableWidget(colorTable.size(), ColumnCount, displayGroupBox);
    categoryColorTable->setObjectName(QStringLiteral("categoryColorTable"));
    categoryColorTable->setHorizontalHeaderLabels(QStringList() << tr("航迹类型") << tr("航迹颜色") << tr("航迹透明度")
                                                               << tr("航迹显示"));
    categoryColorTable->verticalHeader()->setVisible(false);
    categoryColorTable->verticalHeader()->setDefaultSectionSize(30);
    // 四列均为 Stretch 模式 → 每列平分表格总宽度（各占 25%）
    categoryColorTable->horizontalHeader()->setSectionResizeMode(ColumnCategory, QHeaderView::Stretch);
    categoryColorTable->horizontalHeader()->setSectionResizeMode(ColumnColor, QHeaderView::Stretch);
    categoryColorTable->horizontalHeader()->setSectionResizeMode(ColumnAlpha, QHeaderView::Stretch);
    categoryColorTable->horizontalHeader()->setSectionResizeMode(ColumnVisible, QHeaderView::Stretch);
    categoryColorTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    categoryColorTable->setSelectionMode(QAbstractItemView::NoSelection);
    categoryColorTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    categoryColorTable->setFocusPolicy(Qt::NoFocus);
    categoryColorTable->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    for (int i = 0; i < colorTable.size(); ++i)
    {
        DisplayRow row;
        row.category = colorTable.at(i).category;
        row.color = colorTable.at(i).defaultColor;

        QTableWidgetItem* nameItem = new QTableWidgetItem(QString::fromUtf8(colorTable.at(i).title));
        nameItem->setFlags(nameItem->flags() & ~Qt::ItemIsEditable);
        categoryColorTable->setItem(i, ColumnCategory, nameItem);

        QPushButton* colorBtn = new QPushButton(categoryColorTable);
        colorBtn->setObjectName(QStringLiteral("trailColorButton%1").arg(i + 1));
        colorBtn->setFixedSize(48, 20);
        colorBtn->setCursor(Qt::PointingHandCursor);
        colorBtn->setToolTip(tr("点击设置该类型航迹颜色"));
        row.colorButton = colorBtn;
        {
            QHBoxLayout* colorCellLayout = new QHBoxLayout;
            colorCellLayout->setContentsMargins(0, 0, 0, 0);
            colorCellLayout->setSpacing(0);
            colorCellLayout->addStretch(1);
            colorCellLayout->addWidget(colorBtn);
            colorCellLayout->addStretch(1);
            QWidget* colorCell = new QWidget(categoryColorTable);
            colorCell->setLayout(colorCellLayout);
            categoryColorTable->setCellWidget(i, ColumnColor, colorCell);
        }
        connect(colorBtn, &QPushButton::clicked, this, [this, i]() { showColorPickerFor(i); });

        // 透明度
        QWidget* alphaCell = new QWidget(categoryColorTable);
        QHBoxLayout* alphaLayout = new QHBoxLayout(alphaCell);
        alphaLayout->setContentsMargins(6, 0, 6, 0);
        alphaLayout->setSpacing(6);

        QSlider* alphaSlider = new QSlider(Qt::Horizontal, alphaCell);
        alphaSlider->setObjectName(QStringLiteral("trailAlphaSlider"));
        alphaSlider->setRange(DisplayDefaults::TrailAlphaMin, DisplayDefaults::TrailAlphaMax);
        alphaSlider->setSingleStep(DisplayDefaults::TrailAlphaStep);
        alphaSlider->setPageStep(DisplayDefaults::TrailAlphaStep * 2);
        alphaSlider->setValue(row.alpha);
        alphaSlider->setFocusPolicy(Qt::NoFocus);
        alphaSlider->setToolTip(tr("设置该类型航迹的绘制透明度（0 为完全透明）"));

        QLabel* alphaValueLabel = new QLabel(alphaCell);
        alphaValueLabel->setObjectName(QStringLiteral("trailAlphaValueLabel"));
        alphaValueLabel->setFixedWidth(30);
        alphaValueLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        alphaValueLabel->setText(QStringLiteral("%1%").arg(row.alpha));

        alphaLayout->addWidget(alphaSlider, 1);
        alphaLayout->addWidget(alphaValueLabel);

        row.alphaSlider = alphaSlider;
        row.alphaValueLabel = alphaValueLabel;
        categoryColorTable->setCellWidget(i, ColumnAlpha, alphaCell);

        connect(alphaSlider, &QSlider::valueChanged, this, [this, i, alphaSlider](int value)
        {
            updateRowAlphaAt(i, value);
            if (!alphaSlider->isSliderDown())
            {
                emit trackDisplayEdited();
            }
        });
        connect(alphaSlider, &QSlider::sliderReleased, this, [this]() { emit trackDisplayEdited(); });

        QWidget* visibleCell = new QWidget(categoryColorTable);
        QHBoxLayout* visibleLayout = new QHBoxLayout(visibleCell);
        visibleLayout->setContentsMargins(0, 0, 0, 0);
        visibleLayout->setSpacing(0);

        QCheckBox* visibleCheckBox = new QCheckBox(visibleCell);
        visibleCheckBox->setObjectName(QStringLiteral("trailVisibleCheckBox"));
        visibleCheckBox->setChecked(row.visible);
        visibleCheckBox->setFocusPolicy(Qt::NoFocus);
        visibleCheckBox->setToolTip(tr("取消勾选后该类型航迹不参与绘制"));

        visibleLayout->addStretch(1);
        visibleLayout->addWidget(visibleCheckBox);
        visibleLayout->addStretch(1);

        row.visibleCheckBox = visibleCheckBox;
        categoryColorTable->setCellWidget(i, ColumnVisible, visibleCell);

        connect(visibleCheckBox, &QCheckBox::toggled, this, [this, i](bool checked)
        {
            DisplayRow* rowPtr = rowAt(i);
            if (rowPtr)
            {
                rowPtr->visible = checked;
            }
            emit trackDisplayEdited();
        });

        m_rows.append(row);
        updateColorCell(i);
    }

    const int tableContentH = categoryColorTable->horizontalHeader()->sizeHint().height()
                              + categoryColorTable->verticalHeader()->defaultSectionSize() * categoryColorTable->rowCount()
                              + 1;
    categoryColorTable->setMinimumHeight(tableContentH);
    displayLayout->addWidget(categoryColorTable);
    contentLayout->addWidget(displayGroupBox);

    contentLayout->addItem(new QSpacerItem(20, 20, QSizePolicy::Minimum, QSizePolicy::Expanding));

    scrollArea->setWidget(content);
    pageLayout->addWidget(scrollArea);
}

void DisplaySettingsPanel::showEvent(QShowEvent* event)
{
    QWidget::showEvent(event);
    if (!m_tableHeightAdjusted)
    {
        m_tableHeightAdjusted = true;
        QTimer::singleShot(0, this, [this]() { adjustTableHeight(); });
    }
}

void DisplaySettingsPanel::adjustTableHeight()
{
    if (categoryColorTable)
    {
        const int exactH = categoryColorTable->horizontalHeader()->height()
                           + categoryColorTable->verticalHeader()->defaultSectionSize() * categoryColorTable->rowCount()
                           + categoryColorTable->frameWidth();
        categoryColorTable->setFixedHeight(exactH);
    }
}

QHash<quint8, QColor> DisplaySettingsPanel::categoryColorOverrides() const
{
    QHash<quint8, QColor> overrides;
    for (const DisplayRow& row : m_rows)
    {
        if (row.color != TrackDisplayColors::DefaultColorFor(row.category))
        {
            overrides.insert(row.category, row.color);
        }
    }
    return overrides;
}

QHash<quint8, int> DisplaySettingsPanel::categoryAlphaOverrides() const
{
    QHash<quint8, int> overrides;
    for (const DisplayRow& row : m_rows)
    {
        if (row.alpha != DisplayDefaults::TrailAlpha)
        {
            overrides.insert(row.category, row.alpha);
        }
    }
    return overrides;
}

QHash<quint8, bool> DisplaySettingsPanel::categoryVisibilityOverrides() const
{
    QHash<quint8, bool> overrides;
    for (const DisplayRow& row : m_rows)
    {
        if (!row.visible)
        {
            overrides.insert(row.category, false);
        }
    }
    return overrides;
}

void DisplaySettingsPanel::setCategoryColors(const QHash<quint8, QColor>& overrides)
{
    for (int i = 0; i < m_rows.size(); ++i)
    {
        const quint8 category = m_rows.at(i).category;
        const QColor color = overrides.contains(category) ? overrides.value(category)
                                                          : TrackDisplayColors::DefaultColorFor(category);
        setRowColor(i, color);
    }
}

void DisplaySettingsPanel::setCategoryAlphas(const QHash<quint8, int>& overrides)
{
    for (int i = 0; i < m_rows.size(); ++i)
    {
        const quint8 category = m_rows.at(i).category;
        const int alpha = overrides.contains(category) ? overrides.value(category) : DisplayDefaults::TrailAlpha;
        setRowAlpha(i, alpha);
    }
}

void DisplaySettingsPanel::setCategoryVisibility(const QHash<quint8, bool>& overrides)
{
    for (int i = 0; i < m_rows.size(); ++i)
    {
        const quint8 category = m_rows.at(i).category;
        const bool visible = overrides.value(category, true);
        setRowVisible(i, visible);
    }
}

DisplaySettingsPanel::DisplayRow* DisplaySettingsPanel::rowAt(int tableRow)
{
    if (tableRow < 0 || tableRow >= m_rows.size())
    {
        return nullptr;
    }
    return &m_rows[tableRow];
}

void DisplaySettingsPanel::updateRowAlphaAt(int tableRow, int alpha)
{
    DisplayRow* row = rowAt(tableRow);
    if (!row)
    {
        return;
    }
    row->alpha = qBound(DisplayDefaults::TrailAlphaMin, alpha, DisplayDefaults::TrailAlphaMax);
    updateAlphaCell(tableRow);
}

void DisplaySettingsPanel::setRowColor(int tableRow, const QColor& color)
{
    DisplayRow* row = rowAt(tableRow);
    if (!row || !color.isValid() || row->color == color)
    {
        return;
    }
    row->color = color;
    updateColorCell(tableRow);
}

void DisplaySettingsPanel::setRowAlpha(int tableRow, int alpha)
{
    DisplayRow* row = rowAt(tableRow);
    if (!row)
    {
        return;
    }
    row->alpha = qBound(DisplayDefaults::TrailAlphaMin, alpha, DisplayDefaults::TrailAlphaMax);
    if (row->alphaSlider)
    {
        const QSignalBlocker blocker(row->alphaSlider);
        row->alphaSlider->setValue(row->alpha);
    }
    updateAlphaCell(tableRow);
}

void DisplaySettingsPanel::setRowVisible(int tableRow, bool visible)
{
    DisplayRow* row = rowAt(tableRow);
    if (!row)
    {
        return;
    }
    row->visible = visible;
    if (row->visibleCheckBox)
    {
        const QSignalBlocker blocker(row->visibleCheckBox);
        row->visibleCheckBox->setChecked(visible);
    }
    updateVisibleCell(tableRow);
}

void DisplaySettingsPanel::updateColorCell(int tableRow)
{
    DisplayRow* row = rowAt(tableRow);
    if (!row || !row->colorButton)
    {
        return;
    }

    row->colorButton->setStyleSheet(
        QStringLiteral("QPushButton { background:%1; border:1px solid %2; border-radius:4px; }"
                       "QPushButton:hover { border:1px solid #ffffff; }")
            .arg(row->color.name(), Theme::hex(Theme::Pal::borderStrong())));
}

void DisplaySettingsPanel::updateAlphaCell(int tableRow)
{
    DisplayRow* row = rowAt(tableRow);
    if (!row || !row->alphaValueLabel)
    {
        return;
    }
    row->alphaValueLabel->setText(QStringLiteral("%1%").arg(row->alpha));
}

void DisplaySettingsPanel::updateVisibleCell(int tableRow)
{
    DisplayRow* row = rowAt(tableRow);
    if (!row || !row->visibleCheckBox)
    {
        return;
    }
    row->visibleCheckBox->setToolTip(row->visible ? tr("该类型航迹参与绘制，取消勾选后不再绘制")
                                                  : tr("该类型航迹已不参与绘制，勾选后恢复绘制"));
}

void DisplaySettingsPanel::showColorPickerFor(int tableRow)
{
    DisplayRow* row = rowAt(tableRow);
    if (!row || !row->colorButton)
    {
        return;
    }
    const QPoint anchor = row->colorButton->mapToGlobal(QPoint(0, row->colorButton->height()));
    createColorPicker(anchor, [this, tableRow](const QColor& color)
    {
        setRowColor(tableRow, color);
        emit trackDisplayEdited();
    })->show();
}

QWidget* DisplaySettingsPanel::createColorPicker(const QPoint& anchorGlobal, const std::function<void(const QColor&)>& onPick)
{
    if (m_colorPicker)
    {
        m_colorPicker->deleteLater();
        m_colorPicker.clear();
    }
    m_colorPicker = new QWidget(this, Qt::Popup);
    m_colorPicker->setAttribute(Qt::WA_DeleteOnClose);
    m_colorPicker->setAttribute(Qt::WA_TranslucentBackground);
    m_colorPicker->setObjectName(QStringLiteral("colorPickerPopup"));
    m_colorPicker->setStyleSheet(
        QStringLiteral("QWidget { background:%1; border:1px solid %2; border-radius:6px; }")
            .arg(Theme::hex(Theme::Pal::surface()), Theme::hex(Theme::Pal::borderStrong())));

    QGridLayout* grid = new QGridLayout(m_colorPicker);
    grid->setContentsMargins(8, 8, 8, 8);
    grid->setSpacing(5);

    const QVector<QColor> palette = pickerPalette();
    const int columns = 6;
    for (int i = 0; i < palette.size(); ++i)
    {
        const QColor color = palette.at(i);
        QPushButton* swatch = new QPushButton(m_colorPicker);
        swatch->setFixedSize(38, 26);
        swatch->setToolTip(color.name().toUpper());
        swatch->setCursor(Qt::PointingHandCursor);
        swatch->setStyleSheet(
            QStringLiteral("QPushButton { background:%1; border:1px solid %2; border-radius:4px; }"
                           "QPushButton:hover { border:2px solid #ffffff; }")
                .arg(color.name(), Theme::hex(Theme::Pal::border())));
        const QPointer<QWidget> picker = m_colorPicker;
        connect(swatch, &QPushButton::clicked, this, [this, color, onPick, picker]()
        {
            onPick(color);
            if (picker)
            {
                picker->close();
            }
        });
        grid->addWidget(swatch, i / columns, i % columns);
    }

    m_colorPicker->adjustSize();
    QPoint pos = anchorGlobal - QPoint(m_colorPicker->width(), 0) + QPoint(0, 4);
    QScreen* screen = QGuiApplication::screenAt(anchorGlobal);
    if (!screen)
    {
        screen = QGuiApplication::primaryScreen();
    }
    if (screen)
    {
        const QRect avail = screen->availableGeometry();
        if (pos.x() + m_colorPicker->width() > avail.right())
        {
            pos.setX(avail.right() - m_colorPicker->width());
        }
        if (pos.x() < avail.left())
        {
            pos.setX(avail.left());
        }
        if (pos.y() + m_colorPicker->height() > avail.bottom())
        {
            pos.setY(qMax(avail.top(), anchorGlobal.y() - m_colorPicker->height()));
        }
    }
    m_colorPicker->move(pos);
    return m_colorPicker;
}
