#include "Ui/NetworkSettingsPanel.h"

#include "Ui/ChannelEditDialog.h"

#include <QFontMetrics>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QPushButton>
#include <QResizeEvent>
#include <QScrollArea>
#include <QScrollBar>
#include <QSpacerItem>
#include <QTableWidget>
#include <QVBoxLayout>

namespace
{
constexpr int ColName = 0;
constexpr int ColMode = 1;
constexpr int ColSrcIp = 2;
constexpr int ColSrcPort = 3;
constexpr int ColDstIp = 4;
constexpr int ColDstPort = 5;
constexpr int ColOps = 6;
constexpr int ColumnCount = 7;

constexpr int MaxContentWidth = 900;
constexpr int CellPadding = 10;
constexpr int HeaderPadding = 12;
constexpr int MinSectionWidth = 24;
constexpr int ActionButtonWidth = 48;

/// <summary>
/// @brief 通道表格：列宽随可用宽度自适应，始终保持左右留白对称
/// </summary>
class ChannelTable : public QTableWidget
{
public:
    explicit ChannelTable(QWidget* parent = nullptr) : QTableWidget(0, ColumnCount, parent) {}

    /// <summary>
    /// @brief 按内容与当前可用宽度重新分配列宽
    /// </summary>
    void UpdateColumnWidths()
    {
        const int cols = columnCount();
        const int avail = viewport()->width();
        if (cols <= 0 || avail <= 0)
        {
            return;
        }

        const QFontMetrics fm(font());
        QVector<int> headerNeed(cols, 0); // 表头文字所需宽度
        QVector<int> cellNeed(cols, 0);   // 单元格数据（或操作按钮）所需宽度
        QVector<int> need(cols, 0);       // 表头与数据都完整所需的宽度
        int needSum = 0;
        int cellSum = 0;
        for (int c = 0; c < cols; ++c)
        {
            if (QTableWidgetItem* head = horizontalHeaderItem(c))
            {
                headerNeed[c] = fm.horizontalAdvance(head->text()) + HeaderPadding;
            }
            if (c == ColOps)
            {
                cellNeed[c] = ActionButtonWidth + CellPadding;
            }
            else
            {
                for (int r = 0; r < rowCount(); ++r)
                {
                    if (QTableWidgetItem* cell = item(r, c))
                    {
                        cellNeed[c] = qMax(cellNeed[c], fm.horizontalAdvance(cell->text()) + CellPadding);
                    }
                }
            }
            cellNeed[c] = qMax(cellNeed[c], MinSectionWidth);
            need[c] = qMax(headerNeed.at(c), cellNeed.at(c));
            needSum += need.at(c);
            cellSum += cellNeed.at(c);
        }

        QVector<int> width = need;
        if (avail >= needSum)
        {
            const int share = (avail - needSum) / cols;
            for (int c = 0; c < cols; ++c)
            {
                width[c] += share;
            }
        }
        else if (avail > cellSum)
        {
            width = cellNeed;
            int budget = avail - cellSum;
            while (budget > 0)
            {
                int target = -1;
                int targetGap = 0;
                for (int c = 0; c < cols; ++c)
                {
                    const int gap = headerNeed.at(c) - width.at(c);
                    if (gap > 0 && (target < 0 || gap < targetGap))
                    {
                        target = c;
                        targetGap = gap;
                    }
                }
                if (target < 0)
                {
                    break;
                }
                const int take = qMin(budget, targetGap);
                width[target] += take;
                budget -= take;
            }
            for (int c = 0; c < cols; ++c)
            {
                width[c] += budget / cols;
            }
        }
        else
        {
            for (int c = 0; c < cols; ++c)
            {
                width[c] = qMax(MinSectionWidth, cellNeed.at(c) * avail / cellSum);
            }
        }

        for (int c = 0; c < cols; ++c)
        {
            setColumnWidth(c, width.at(c));
        }

        int total = 0;
        for (int c = 0; c < cols; ++c)
        {
            total += columnWidth(c);
        }
        if (total < avail)
        {
            setColumnWidth(cols - 1, columnWidth(cols - 1) + avail - total);
        }
        while (total > avail)
        {
            int widest = 0;
            for (int c = 1; c < cols; ++c)
            {
                if (columnWidth(c) > columnWidth(widest))
                {
                    widest = c;
                }
            }
            const int take = qMin(total - avail, columnWidth(widest) - MinSectionWidth);
            if (take <= 0)
            {
                break;
            }
            setColumnWidth(widest, columnWidth(widest) - take);
            total -= take;
        }
    }

    /// <summary>
    /// @brief 表格高度贴合实际行数
    /// </summary>
    void UpdateTableHeight()
    {
        const int headerHeight = qMax(horizontalHeader()->height(), horizontalHeader()->sizeHint().height());
        int rowsHeight = 0;
        for (int r = 0; r < rowCount(); ++r)
        {
            rowsHeight += rowHeight(r);
        }
        QScrollBar* hBar = horizontalScrollBar();
        const int barHeight = (hBar && hBar->isVisible()) ? hBar->sizeHint().height() : 0;
        setFixedHeight(headerHeight + rowsHeight + barHeight + 2 * frameWidth());
    }

protected:
    void resizeEvent(QResizeEvent* event) override
    {
        QTableWidget::resizeEvent(event);
        UpdateColumnWidths();
        UpdateTableHeight();
    }

    void showEvent(QShowEvent* event) override
    {
        QTableWidget::showEvent(event);
        UpdateColumnWidths();
        UpdateTableHeight();
    }
};

ChannelTable* asChannelTable(QTableWidget* table)
{
    return static_cast<ChannelTable*>(table);
}
}

NetworkSettingsPanel::NetworkSettingsPanel(QWidget* parent) : QWidget(parent)
{
    setObjectName(QStringLiteral("networkSettingsPage"));
    QVBoxLayout* pageLayout = new QVBoxLayout(this);
    pageLayout->setContentsMargins(2, 2, 2, 2);
    pageLayout->setSpacing(2);

    // 滚动区
    QScrollArea* scrollArea = new QScrollArea(this);
    scrollArea->setObjectName(QStringLiteral("networkScrollArea"));
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setWidgetResizable(true);

    QWidget* content = new QWidget;
    content->setObjectName(QStringLiteral("networkPanel"));

    QVBoxLayout* contentLayout = new QVBoxLayout(content);
    contentLayout->setContentsMargins(4, 4, 4, 4);
    contentLayout->setSpacing(0);

    // 居中容器：内容整体
    QWidget* centerHost = new QWidget(content);
    centerHost->setObjectName(QStringLiteral("networkCenterHost"));
    QHBoxLayout* centerLayout = new QHBoxLayout(centerHost);
    centerLayout->setContentsMargins(0, 0, 0, 0);
    centerLayout->setSpacing(0);

    QWidget* centerColumn = new QWidget(centerHost);
    centerColumn->setObjectName(QStringLiteral("networkCenterColumn"));
    centerColumn->setMaximumWidth(MaxContentWidth);

    QVBoxLayout* columnLayout = new QVBoxLayout(centerColumn);
    columnLayout->setContentsMargins(0, 0, 0, 0);
    columnLayout->setSpacing(6);

    // 通道表格
    QGroupBox* channelGroupBox = new QGroupBox(tr("通道列表"), centerColumn);
    channelGroupBox->setObjectName(QStringLiteral("channelGroupBox"));
    channelGroupBox->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    QVBoxLayout* channelLayout = new QVBoxLayout(channelGroupBox);
    channelLayout->setContentsMargins(10, 10, 10, 10);
    channelLayout->setSpacing(5);

    m_channelTable = new ChannelTable(channelGroupBox);
    m_channelTable->setObjectName(QStringLiteral("channelTable"));
    m_channelTable->setHorizontalHeaderLabels(QStringList() << tr("名称") << tr("通信方式") << tr("源IP")
                                              << tr("源端口") << tr("目的IP") << tr("目的端口")
                                              << tr("操作"));
    QHeaderView* header = m_channelTable->horizontalHeader();
    header->setDefaultAlignment(Qt::AlignCenter);
    header->setSectionsClickable(false);
    header->setHighlightSections(false);
    header->setMinimumSectionSize(MinSectionWidth);
    for (int col = 0; col < ColumnCount; ++col)
    {
        header->setSectionResizeMode(col, QHeaderView::Fixed);
        if (QTableWidgetItem* head = m_channelTable->horizontalHeaderItem(col))
        {
            head->setToolTip(head->text());
        }
    }
    m_channelTable->verticalHeader()->setVisible(false);
    m_channelTable->verticalHeader()->setDefaultSectionSize(30);
    m_channelTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_channelTable->setSelectionMode(QAbstractItemView::NoSelection);
    m_channelTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_channelTable->setFocusPolicy(Qt::NoFocus);
    m_channelTable->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
    m_channelTable->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_channelTable->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    channelLayout->addWidget(m_channelTable);
    columnLayout->addWidget(channelGroupBox);

    // 应用按钮
    applyPushButton = new QPushButton(tr("应用网络设置"), centerColumn);
    applyPushButton->setObjectName(QStringLiteral("applyNetworkPushButton"));
    columnLayout->addWidget(applyPushButton);
    columnLayout->addItem(new QSpacerItem(20, 20, QSizePolicy::Minimum, QSizePolicy::Expanding));

    centerLayout->addStretch(1);
    centerLayout->addWidget(centerColumn, 100);
    centerLayout->addStretch(1);
    contentLayout->addWidget(centerHost);

    scrollArea->setWidget(content);
    pageLayout->addWidget(scrollArea);
}

QTableWidgetItem* NetworkSettingsPanel::readOnlyItem(const QString& text)
{
    QTableWidgetItem* item = new QTableWidgetItem(text);
    item->setFlags(item->flags() & ~Qt::ItemIsEditable);
    item->setTextAlignment(Qt::AlignCenter);
    return item;
}

void NetworkSettingsPanel::setChannels(const QVector<ChannelRow>& rows)
{
    m_channels = rows;
    refreshTable();
    setPendingApply(false);
}

void NetworkSettingsPanel::refreshTable()
{
    m_channelTable->setRowCount(m_channels.size());
    for (int r = 0; r < m_channels.size(); ++r)
    {
        const ChannelRow& row = m_channels.at(r);
        m_channelTable->setItem(r, ColName, readOnlyItem(row.name));
        m_channelTable->setItem(r, ColMode, readOnlyItem(QStringLiteral("UDP")));
        m_channelTable->setItem(r, ColSrcIp, readOnlyItem(row.srcIp));
        m_channelTable->setItem(r, ColSrcPort, readOnlyItem(QString::number(row.srcPort)));
        m_channelTable->setItem(r, ColDstIp, readOnlyItem(row.dstIp));
        m_channelTable->setItem(r, ColDstPort, readOnlyItem(QString::number(row.dstPort)));

        QWidget* ops = new QWidget;
        QHBoxLayout* opsLayout = new QHBoxLayout(ops);
        opsLayout->setContentsMargins(0, 0, 0, 0);
        opsLayout->setSpacing(0);
        QPushButton* editBtn = new QPushButton(tr("编辑"), ops);
        editBtn->setObjectName(QStringLiteral("channelEditBtn_%1").arg(row.id));
        editBtn->setProperty("cellAction", true);
        editBtn->setMinimumWidth(ActionButtonWidth);

        opsLayout->addStretch(1);
        opsLayout->addWidget(editBtn);
        opsLayout->addStretch(1);

        const QString rowId = row.id;
        connect(editBtn, &QPushButton::clicked, this, [this, rowId]() { editChannel(rowId); });
        m_channelTable->setCellWidget(r, ColOps, ops);
    }

    asChannelTable(m_channelTable)->UpdateColumnWidths();
    asChannelTable(m_channelTable)->UpdateTableHeight();
}

void NetworkSettingsPanel::editChannel(const QString& id)
{
    for (int i = 0; i < m_channels.size(); ++i)
    {
        if (m_channels.at(i).id == id)
        {
            const ChannelRow before = m_channels.at(i);
            ChannelRow row = before;
            if (editChannelDialog(row))
            {
                m_channels[i] = row;
                refreshTable();

                QStringList changes;
                if (before.srcIp != row.srcIp)
                {
                    changes << tr("源IP %1→%2").arg(before.srcIp, row.srcIp);
                }
                if (before.srcPort != row.srcPort)
                {
                    changes << tr("源端口 %1→%2").arg(before.srcPort).arg(row.srcPort);
                }
                if (before.dstIp != row.dstIp)
                {
                    changes << tr("目的IP %1→%2").arg(before.dstIp, row.dstIp);
                }
                if (before.dstPort != row.dstPort)
                {
                    changes << tr("目的端口 %1→%2").arg(before.dstPort).arg(row.dstPort);
                }
                if (!changes.isEmpty())
                {
                    setPendingApply(true);
                    emit channelEdited(row.name, changes.join(QStringLiteral("，")));
                }
            }
            return;
        }
    }
}

void NetworkSettingsPanel::setPendingApply(bool pending)
{
    if (!applyPushButton)
    {
        return;
    }
    applyPushButton->setText(pending ? tr("应用网络设置（有未应用的修改）") : tr("应用网络设置"));
    applyPushButton->setToolTip(pending ? tr("通道参数已修改，点击后才会重新绑定端口生效") : QString());
    if (m_pendingApply != pending)
    {
        m_pendingApply = pending;
        emit pendingApplyChanged(pending);
    }
}

bool NetworkSettingsPanel::editChannelDialog(ChannelRow& row)
{
    ChannelEditDialog dialog(this);
    dialog.setChannelName(row.name);
    dialog.setEndpoints(row.srcIp, row.srcPort, row.dstIp, row.dstPort);
    if (dialog.exec() != QDialog::Accepted)
    {
        return false;
    }

    row.srcIp = dialog.srcIp();
    row.srcPort = dialog.srcPort();
    row.dstIp = dialog.dstIp();
    row.dstPort = dialog.dstPort();
    return true;
}
