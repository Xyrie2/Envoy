#include "Ui/TrackTableController.h"
#include "Core/RadarConfig.h"
#include "Core/RadarTrackStore.h"
#include "Core/RadarTrackText.h"
#include "Ui/TargetListPanel.h"

#include <algorithm>

#include <QAction>
#include <QCursor>
#include <QEvent>
#include <QGroupBox>
#include <QHeaderView>
#include <QLabel>
#include <QLayout>
#include <QMenu>
#include <QPushButton>
#include <QScrollBar>
#include <QSignalBlocker>
#include <QSplitter>
#include <QTableWidget>
#include <QTimer>
#include <QToolButton>
#include <QToolTip>

namespace TrackColumns
{
namespace
{
/// 列定义表
const Spec kSpecs[] = {
    { Col::Id, "id", QT_TR_NOOP("编号"), 60, true  }, 
    { Col::Range, "range", QT_TR_NOOP("斜距"), 72, false },
    { Col::Alt, "alt", QT_TR_NOOP("高度"), 72, false },
    { Col::Az, "az", QT_TR_NOOP("方位"), 72, false },
    { Col::El, "el", QT_TR_NOOP("俯仰"), 72, false },
    { Col::Amp, "amp", QT_TR_NOOP("幅度"), 64, false },
    { Col::Snr, "snr", QT_TR_NOOP("信噪比"), 72, false },
    { Col::Category, "category", QT_TR_NOOP("类型"), 72, false },
    { Col::Conf, "conf", QT_TR_NOOP("置信度"), 68, false },
    { Col::Points, "points", QT_TR_NOOP("跟踪次数"), 76, false },
};
static_assert(sizeof(kSpecs) / sizeof(kSpecs[0]) == static_cast<size_t>(Count), "列定义表行数与 TrackColumns::Col 数量不一致");
}

const Spec* Specs()
{
    return kSpecs;
}

int IndexOfKey(const QString& key)
{
    for (int i = 0; i < Count; ++i)
    {
        if (QString::compare(key, QLatin1String(kSpecs[i].key), Qt::CaseInsensitive) == 0)
        {
            return i;
        }
    }
    return -1;
}
}

namespace
{

using TrackCol = TrackColumns::Col;
constexpr int TrackColCount = TrackColumns::Count;
constexpr int MinColumnWidth = 24;
constexpr int MaxColumnWidth = 600;
constexpr int SaveDebounceMs = 300;
constexpr int FlushIntervalMs = 150;
constexpr int RawValueRole = Qt::UserRole + 1;

QTableWidgetItem* EnsureCell(QTableWidget* table, int row, int col)
{
    if (QTableWidgetItem* existing = table->item(row, col))
    {
        return existing;
    }
    QTableWidgetItem* item = new QTableWidgetItem;
    item->setTextAlignment(Qt::AlignCenter);
    table->setItem(row, col, item);
    return item;
}

void SetCell(QTableWidget* table, int row, int col, const QString& text,
             const QVariant& userData = QVariant())
{
    QTableWidgetItem* item = EnsureCell(table, row, col);
    if (item->text() != text)
    {
        item->setText(text);
    }
    if (userData.isValid() && item->data(Qt::UserRole) != userData)
    {
        item->setData(Qt::UserRole, userData);
    }
}

void SetNumericCell(QTableWidget* table, int row, int col, double value, int decimals)
{
    QTableWidgetItem* item = EnsureCell(table, row, col);
    const QVariant cached = item->data(RawValueRole);
    if (cached.isValid() && cached.toDouble() == value)
    {
        return;
    }
    item->setData(RawValueRole, value);
    item->setText(decimals < 0 ? QString::number(static_cast<qint64>(value))
                               : QString::number(value, 'f', decimals));
}

class UpdateBatch
{
public:
    explicit UpdateBatch(QTableWidget* table) : m_table(table)
    {
        if (m_table)
        {
            m_table->setUpdatesEnabled(false);
        }
    }
    ~UpdateBatch()
    {
        if (m_table)
        {
            m_table->setUpdatesEnabled(true);
        }
    }
    UpdateBatch(const UpdateBatch&) = delete;
    UpdateBatch& operator=(const UpdateBatch&) = delete;

private:
    QTableWidget* m_table;
};
}

TrackTableController::TrackTableController(TargetListPanel* panel, RadarTrackStore* store, QSplitter* rightSplitter, QObject* parent)
    : QObject(parent), m_panel(panel), m_store(store), m_rightSplitter(rightSplitter)
{
    m_columnVisibleMask = (1u << TrackColumns::Count) - 1u;
    for (int i = 0; i < TrackColumns::Count; ++i)
    {
        m_columnWidth[i] = TrackColumns::Specs()[i].defaultWidth;
    }
    LoadColumnPrefs();

    m_saveTimer = new QTimer(this);
    m_saveTimer->setSingleShot(true);
    m_saveTimer->setInterval(SaveDebounceMs);
    connect(m_saveTimer, &QTimer::timeout, this, &TrackTableController::SaveColumnPrefs);

    m_flushTimer = new QTimer(this);
    m_flushTimer->setSingleShot(true);
    m_flushTimer->setInterval(FlushIntervalMs);
    connect(m_flushTimer, &QTimer::timeout, this, &TrackTableController::FlushPendingUpdates);

    connect(m_store, &RadarTrackStore::targetAdded, this, &TrackTableController::OnTargetAdded);
    connect(m_store, &RadarTrackStore::targetUpdated, this, &TrackTableController::OnTargetUpdated);
    connect(m_store, &RadarTrackStore::targetRemoved, this, &TrackTableController::OnTargetRemoved);
    connect(m_panel->trackTableWidget, &QTableWidget::itemSelectionChanged, this, &TrackTableController::OnTableSelectionChanged);
}

TrackTableController::~TrackTableController()
{
    if (m_saveTimer && m_saveTimer->isActive())
    {
        SaveColumnPrefs();
    }
}

void TrackTableController::InitTables()
{
    QTableWidget* table = m_panel->trackTableWidget;
    const TrackColumns::Spec* specs = TrackColumns::Specs();

    QStringList headers;
    headers.reserve(TrackColCount);
    for (int i = 0; i < TrackColCount; ++i)
    {
        headers << tr(specs[i].title);
    }

    table->setColumnCount(TrackColCount);
    table->setHorizontalHeaderLabels(headers);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setSelectionMode(QAbstractItemView::SingleSelection);
    table->setAlternatingRowColors(true);
    table->verticalHeader()->setVisible(false);
    table->setSortingEnabled(false);

    QHeaderView* header = table->horizontalHeader();
    header->setSectionResizeMode(QHeaderView::Interactive);
    header->setStretchLastSection(false);
    table->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
    table->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    table->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    for (int i = 0; i < TrackColCount; ++i)
    {
        table->setColumnWidth(i, m_columnWidth[i]);
    }

    connect(header, &QHeaderView::sectionResized, this, &TrackTableController::OnSectionResized);
    header->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(header, &QHeaderView::customContextMenuRequested, this, [this, header](const QPoint& pos)
    {
        if (m_columnMenu)
        {
            m_columnMenu->popup(header->mapToGlobal(pos));
        }
    });

    table->viewport()->installEventFilter(this);
    QTimer::singleShot(0, this, [this]()
    {
        UpdateTrackTableMaxHeight();
        RefreshLayout();
    });

    ApplyColumnVisibility();
    BuildColumnMenu();

    m_panel->TrackListGroupBox->setTitle(tr("航迹列表 · 目标列表"));
    SetViewMode(ViewMode::TargetList);
    UpdateStatsLabel();
}

void TrackTableController::UpdateTrackTableMaxHeight()
{
    QTableWidget* table = m_panel ? m_panel->trackTableWidget : nullptr;
    if (!table)
    {
        return;
    }

    table->insertRow(0);
    SetCell(table, 0, 0, QStringLiteral("0"));
    const int rowH = table->rowHeight(0);
    table->removeRow(0);
    if (rowH <= 0)
    {
        return;
    }

    const int scrollBarH = (table->horizontalScrollBarPolicy() == Qt::ScrollBarAlwaysOff)
            ? 0
            : table->horizontalScrollBar()->sizeHint().height();
    const int maxH = table->horizontalHeader()->height() + rowH * 10 + 2 * table->frameWidth() + 1 + scrollBarH;
    table->setMaximumHeight(maxH);

    if (m_rightSplitter && m_rightSplitter->count() >= 2)
    {
        QSplitterHandle* handle = m_rightSplitter->handle(1);
        if (handle && handle->isEnabled())
        {
            handle->setEnabled(false);
        }
    }

    QGroupBox* groupBox = m_panel->TrackListGroupBox;
    QLayout* lay = groupBox ? groupBox->layout() : nullptr;
    if (!groupBox || !lay || table->parentWidget() != groupBox)
    {
        return;
    }
    QPushButton* headerBtn = m_panel->modeListButton;
    const int headerH = qMax(headerBtn ? headerBtn->sizeHint().height() : 0, m_panel->statsLabel->sizeHint().height());
    const QRect gbContents = groupBox->contentsRect();
    const int chromeTop = gbContents.y() + lay->contentsMargins().top() + headerH + lay->spacing();
    const int chromeBottom =
            groupBox->height() - gbContents.y() - gbContents.height() + lay->contentsMargins().bottom();
    if (chromeTop <= 0 || chromeBottom < 0)
    {
        return;
    }
    if (maxH < table->minimumHeight())
    {
        table->setMinimumHeight(maxH);
    }
    m_panel->setFixedHeight(chromeTop + maxH + chromeBottom);
}

void TrackTableController::SetViewMode(ViewMode mode)
{
    m_panel->modeListButton->setChecked(mode == ViewMode::TargetList);
    m_panel->modeFocusButton->setChecked(mode == ViewMode::FocusHistory);

    if (m_viewMode == mode)
    {
        return;
    }
    m_viewMode = mode;

    m_dirtyTargets.clear();
    m_focusHistoryDirty = false;
    m_focusSeen = 0;
    m_focusFront = 0;
    if (m_flushTimer)
    {
        m_flushTimer->stop();
    }

    QTableWidget* table = m_panel->trackTableWidget;
    table->clearContents();
    table->setRowCount(0);
    m_rowIds.clear();

    if (mode == ViewMode::TargetList)
    {
        m_panel->TrackListGroupBox->setTitle(tr("航迹列表 · 目标列表"));
        RebuildTargetListView();
    }
    else
    {
        if (m_hasFocusTarget && m_store->contains(m_focusTargetId))
        {
            m_panel->TrackListGroupBox->setTitle(tr("航迹列表 · 焦点目标 %1 历史帧").arg(m_focusTargetId));
            FillFocusHistory(m_focusTargetId);
        }
        else
        {
            ClearFocus();
        }
    }
}

void TrackTableController::ShowFocusHistoryOf(quint16 targetId)
{
    if (!m_store->contains(targetId))
    {
        return;
    }

    m_focusTargetId = targetId;
    m_hasFocusTarget = true;
    m_focusSeen = 0;
    m_focusFront = 0;
    emit FocusTargetChanged(targetId);

    if (m_viewMode != ViewMode::FocusHistory)
    {
        SetViewMode(ViewMode::FocusHistory);
    }
    else
    {
        QTableWidget* table = m_panel->trackTableWidget;
        table->clearContents();
        table->setRowCount(0);
        FillFocusHistory(targetId);
        m_panel->TrackListGroupBox->setTitle(tr("航迹列表 · 焦点目标 %1 历史帧").arg(targetId));
    }
}

void TrackTableController::OnTargetAdded(quint16 targetId)
{
    ++m_statTotal;
    UpdateStatsLabel();

    if (m_viewMode == ViewMode::TargetList)
    {
        InsertTargetRow(targetId);
    }
}

void TrackTableController::OnTargetUpdated(quint16 targetId)
{
    if (m_viewMode == ViewMode::TargetList)
    {
        m_dirtyTargets.insert(targetId);
        if (!m_flushTimer->isActive())
        {
            m_flushTimer->start();
        }
    }
    else if (m_hasFocusTarget && m_focusTargetId == targetId)
    {
        m_focusHistoryDirty = true;
        ++m_focusSeen;
        if (!m_flushTimer->isActive())
        {
            m_flushTimer->start();
        }
    }
}

void TrackTableController::FlushPendingUpdates()
{
    QTableWidget* table = m_panel ? m_panel->trackTableWidget : nullptr;
    if (!table)
    {
        return;
    }

    if (m_viewMode == ViewMode::TargetList)
    {
        if (m_dirtyTargets.isEmpty())
        {
            return;
        }
        const QSet<quint16> pending = m_dirtyTargets;
        m_dirtyTargets.clear();
        UpdateBatch batch(table);
        for (quint16 id : pending)
        {
            if (m_store->contains(id))
            {
                RefreshTargetRow(id);
            }
        }
    }
    else if (m_hasFocusTarget && m_focusHistoryDirty)
    {
        m_focusHistoryDirty = false;
        if (m_store->contains(m_focusTargetId))
        {
            FillFocusHistory(m_focusTargetId);
        }
    }
}

void TrackTableController::OnTargetRemoved(quint16 targetId, bool byDrop)
{
    if (byDrop)
    {
        ++m_statDropped;
    }

    m_dirtyTargets.remove(targetId);
    if (m_hasFocusTarget && m_focusTargetId == targetId)
    {
        m_focusHistoryDirty = false;
        m_focusSeen = 0;
        m_focusFront = 0;
    }

    if (m_viewMode == ViewMode::TargetList)
    {
        RemoveTargetRow(targetId);
    }

    if (m_hasFocusTarget && m_focusTargetId == targetId)
    {
        ClearFocus();
    }

    UpdateStatsLabel();
}

void TrackTableController::OnTableSelectionChanged()
{
    if (m_viewMode != ViewMode::TargetList)
    {
        return;
    }

    QTableWidget* table = m_panel->trackTableWidget;
    QTableWidgetItem* item = table->item(table->currentRow(), static_cast<int>(TrackCol::Id));
    if (!item)
    {
        return;
    }

    const quint32 raw = item->data(Qt::UserRole).toUInt();
    if (!m_store->contains(static_cast<quint16>(raw)))
    {
        return;
    }

    const quint16 id = static_cast<quint16>(raw);
    if (m_hasFocusTarget && m_focusTargetId == id)
    {
        return;
    }

    m_focusTargetId = id;
    m_hasFocusTarget = true;
    emit FocusTargetChanged(id);
    SetViewMode(ViewMode::FocusHistory);
}

void TrackTableController::ResetDataStats()
{
    m_statTotal = 0;
    m_statDropped = 0;
    m_rowIds.clear();
    m_dirtyTargets.clear();
    ClearFocus();
    UpdateStatsLabel();
}

void TrackTableController::ClearFocus()
{
    m_hasFocusTarget = false;
    m_focusTargetId = 0;
    m_focusHistoryDirty = false;
    m_focusSeen = 0;
    m_focusFront = 0;
    emit FocusTargetChanged(0);
    if (m_viewMode == ViewMode::FocusHistory)
    {
        m_panel->trackTableWidget->setRowCount(0);
        m_panel->TrackListGroupBox->setTitle(tr("航迹列表 · 焦点历史（未选择目标）"));
    }
}

void TrackTableController::RebuildTargetListView()
{
    QTableWidget* table = m_panel->trackTableWidget;
    UpdateBatch batch(table);

    table->setRowCount(0);
    m_rowIds.clear();

    const QList<quint16> ids = m_store->targetIds();
    for (quint16 id : ids)
    {
        InsertTargetRow(id);
    }
}

void TrackTableController::FillFocusHistory(quint16 id)
{
    const QList<RadarTrack>& frames = m_store->history(id);
    QTableWidget* table = m_panel->trackTableWidget;
    const int total = frames.size();
    UpdateBatch batch(table);
    const int shown = table->rowCount();
    const int frontNow = qMax(0, m_focusSeen - total);
    const int removeCount = frontNow - m_focusFront;
    const int appendStart = m_focusFront + shown - frontNow;
    const bool aligned = shown > 0 && removeCount >= 0 && removeCount <= shown
            && appendStart >= 0 && appendStart <= total;

    if (!aligned)
    {
        table->setRowCount(0);
        for (int i = 0; i < total; ++i)
        {
            const int row = table->rowCount();
            table->insertRow(row);
            FillTrackRow(row, frames.at(i));
        }
    }
    else
    {
        for (int j = 0; j < removeCount; ++j)
        {
            table->removeRow(0);
        }
        for (int i = appendStart; i < total; ++i)
        {
            const int row = table->rowCount();
            table->insertRow(row);
            FillTrackRow(row, frames.at(i));
        }
    }
    m_focusFront = frontNow;

    if (table->rowCount() > 0)
    {
        table->scrollToBottom();
    }
}

int TrackTableController::RowIndexOfTarget(quint16 targetId) const
{
    const auto it = std::lower_bound(m_rowIds.cbegin(), m_rowIds.cend(), targetId);
    if (it != m_rowIds.cend() && *it == targetId)
    {
        return static_cast<int>(it - m_rowIds.cbegin());
    }
    return -1;
}

void TrackTableController::RefreshTargetRow(quint16 targetId)
{
    const int row = RowIndexOfTarget(targetId);
    if (row < 0)
    {
        InsertTargetRow(targetId);
        return;
    }
    FillTrackRow(row, m_store->latest(targetId));
}

void TrackTableController::InsertTargetRow(quint16 targetId)
{
    QTableWidget* table = m_panel->trackTableWidget;

    const auto it = std::lower_bound(m_rowIds.cbegin(), m_rowIds.cend(), targetId);
    const int insertRow = static_cast<int>(it - m_rowIds.cbegin());
    table->insertRow(insertRow);
    m_rowIds.insert(insertRow, targetId);

    FillTrackRow(insertRow, m_store->latest(targetId));
}

void TrackTableController::RemoveTargetRow(quint16 targetId)
{
    const int row = RowIndexOfTarget(targetId);
    if (row < 0)
    {
        return;
    }
    m_panel->trackTableWidget->removeRow(row);
    m_rowIds.remove(row);
}

void TrackTableController::FillTrackRow(int row, const RadarTrack& track)
{
    QTableWidget* table = m_panel->trackTableWidget;
    SetCell(table, row, static_cast<int>(TrackCol::Id), QString::number(track.targetId), track.targetId);
    SetNumericCell(table, row, static_cast<int>(TrackCol::Range), track.slantRange, -1);
    SetNumericCell(table, row, static_cast<int>(TrackCol::Alt), track.altitude, 1);
    const double azDeg = Azimuth::Normalize(track.azimuthAngle / 100.0);
    SetNumericCell(table, row, static_cast<int>(TrackCol::Az), azDeg, 2);
    SetNumericCell(table, row, static_cast<int>(TrackCol::El), track.elevationAngle / 100.0, 2);
    SetNumericCell(table, row, static_cast<int>(TrackCol::Amp), track.amplitude, -1);
    SetNumericCell(table, row, static_cast<int>(TrackCol::Snr), track.snrValue, -1);
    SetCell(table, row, static_cast<int>(TrackCol::Category), TrackText::CategoryText(track.targetCategory));
    SetNumericCell(table, row, static_cast<int>(TrackCol::Conf), track.confidenceLevel, -1);
    SetNumericCell(table, row, static_cast<int>(TrackCol::Points), track.trackPointCount, -1);
}

void TrackTableController::UpdateStatsLabel()
{
    const int alive = m_store->targetCount();
    m_panel->statsLabel->setText(tr("累计: %1　　存活: %2　　消批: %3").arg(m_statTotal).arg(alive).arg(m_statDropped));
}

bool TrackTableController::IsColumnVisible(TrackColumns::Col col) const
{
    const int idx = static_cast<int>(col);
    if (idx < 0 || idx >= TrackColumns::Count)
    {
        return false;
    }
    return (m_columnVisibleMask & (1u << idx)) != 0u;
}

void TrackTableController::SetColumnVisible(TrackColumns::Col col, bool visible)
{
    const int idx = static_cast<int>(col);
    if (idx < 0 || idx >= TrackColumns::Count)
    {
        return;
    }
    if (TrackColumns::Specs()[idx].required)
    {
        visible = true;
    }
    if (IsColumnVisible(col) == visible)
    {
        return;
    }

    if (!visible)
    {
        int visibleCount = 0;
        for (int i = 0; i < TrackColumns::Count; ++i)
        {
            if (m_columnVisibleMask & (1u << i))
            {
                ++visibleCount;
            }
        }
        if (visibleCount <= 1)
        {
            QToolTip::showText(QCursor::pos(), tr("至少需要保留一列"), m_panel->trackTableWidget);
            SyncColumnMenu();
            return;
        }
    }

    if (visible)
    {
        m_columnVisibleMask |= (1u << idx);
    }
    else
    {
        m_columnVisibleMask &= ~(1u << idx);
    }
    m_panel->trackTableWidget->setColumnHidden(idx, !visible);
    SyncColumnMenu();
    RefreshLayout();
    ScheduleColumnPrefsSave();
}

void TrackTableController::ApplyColumnVisibility()
{
    QTableWidget* table = m_panel->trackTableWidget;
    for (int i = 0; i < TrackColumns::Count; ++i)
    {
        table->setColumnHidden(i, (m_columnVisibleMask & (1u << i)) == 0u);
    }
    SyncColumnMenu();
}

void TrackTableController::ShowOnlyColumns(quint32 mask)
{
    for (int i = 0; i < TrackColumns::Count; ++i)
    {
        if (TrackColumns::Specs()[i].required)
        {
            mask |= (1u << i);
        }
    }
    m_columnVisibleMask = mask;
    ApplyColumnVisibility();
    RefreshLayout();
    ScheduleColumnPrefsSave();
}

void TrackTableController::BuildColumnMenu()
{
    if (!m_panel->columnSettingsButton)
    {
        return;
    }
    if (!m_columnMenu)
    {
        m_columnMenu = new QMenu(m_panel);
    }
    m_columnMenu->clear();

    const TrackColumns::Spec* specs = TrackColumns::Specs();
    for (int i = 0; i < TrackColumns::Count; ++i)
    {
        QAction* action = m_columnMenu->addAction(tr(specs[i].title));
        action->setCheckable(true);
        action->setChecked(IsColumnVisible(specs[i].col));
        action->setData(i);
        action->setObjectName(QStringLiteral("colAction_%1").arg(QString::fromLatin1(specs[i].key)));
        if (specs[i].required)
        {
            action->setEnabled(false);
            action->setToolTip(tr("该列用于关联目标编号，不可隐藏"));
        }
        connect(action, &QAction::toggled, this, &TrackTableController::OnColumnActionToggled);
    }

    m_columnMenu->addSeparator();
    QAction* restoreAct = m_columnMenu->addAction(tr("恢复默认（全部显示）"));
    restoreAct->setObjectName(QStringLiteral("colMenu_restoreAll"));
    connect(restoreAct, &QAction::triggered, this, [this]() { ShowOnlyColumns(0xFFFFFFFFu); });

    QAction* onlyIdAct = m_columnMenu->addAction(tr("仅显示编号"));
    onlyIdAct->setObjectName(QStringLiteral("colMenu_onlyId"));
    connect(onlyIdAct, &QAction::triggered, this,
            [this]() { ShowOnlyColumns(1u << static_cast<int>(TrackColumns::Col::Id)); });

    m_panel->columnSettingsButton->setMenu(m_columnMenu);
}

void TrackTableController::SyncColumnMenu()
{
    if (!m_columnMenu)
    {
        return;
    }
    m_updatingMenu = true;
    const QList<QAction*> actions = m_columnMenu->actions();
    for (QAction* action : actions)
    {
        if (!action->isCheckable())
        {
            continue;
        }
        const int idx = action->data().toInt();
        if (idx < 0 || idx >= TrackColumns::Count)
        {
            continue;
        }
        const TrackColumns::Spec& spec = TrackColumns::Specs()[idx];
        const bool visible = IsColumnVisible(spec.col);
        if (action->isChecked() != visible)
        {
            const QSignalBlocker blocker(action);
            action->setChecked(visible);
        }
        action->setEnabled(!spec.required);
    }
    m_updatingMenu = false;
}

void TrackTableController::OnColumnActionToggled(bool checked)
{
    if (m_updatingMenu)
    {
        return;
    }
    QAction* action = qobject_cast<QAction*>(sender());
    if (!action)
    {
        return;
    }
    const int idx = action->data().toInt();
    if (idx < 0 || idx >= TrackColumns::Count)
    {
        return;
    }
    SetColumnVisible(static_cast<TrackColumns::Col>(idx), checked);
}

void TrackTableController::LoadColumnPrefs()
{
    const TableConfig cfg = RadarConfig::LoadTable();
    for (int i = 0; i < TrackColumns::Count; ++i)
    {
        m_columnWidth[i] = TrackColumns::Specs()[i].defaultWidth;
    }
    for (auto it = cfg.columnWidths.constBegin(); it != cfg.columnWidths.constEnd(); ++it)
    {
        const int idx = TrackColumns::IndexOfKey(it.key());
        if (idx < 0)
        {
            continue;
        }
        m_columnWidth[idx] = qBound(MinColumnWidth, it.value(), MaxColumnWidth);
    }

    m_columnVisibleMask = (1u << TrackColumns::Count) - 1u;
    for (const QString& key : cfg.hiddenColumns)
    {
        const int idx = TrackColumns::IndexOfKey(key);
        if (idx < 0 || TrackColumns::Specs()[idx].required)
        {
            continue;
        }
        m_columnVisibleMask &= ~(1u << idx);
    }
}

void TrackTableController::SaveColumnPrefs()
{
    TableConfig cfg;
    for (int i = 0; i < TrackColumns::Count; ++i)
    {
        const TrackColumns::Spec& spec = TrackColumns::Specs()[i];
        const QString key = QString::fromLatin1(spec.key);
        if ((m_columnVisibleMask & (1u << i)) == 0u)
        {
            cfg.hiddenColumns << key;
        }
        cfg.columnWidths.insert(key, m_columnWidth[i]);
    }
    RadarConfig::SaveTable(cfg);
}

void TrackTableController::ScheduleColumnPrefsSave()
{
    if (m_saveTimer)
    {
        m_saveTimer->start();
    }
}

void TrackTableController::OnSectionResized(int index, int oldSize, int newSize)
{
    Q_UNUSED(oldSize);
    if (index < 0 || index >= TrackColumns::Count)
    {
        return;
    }

    if (m_updatingLayout || index == m_stretchColumn)
    {
        return;
    }
    if (m_panel->trackTableWidget->horizontalHeader()->sectionResizeMode(index) != QHeaderView::Interactive)
    {
        return;
    }
    if (m_columnWidth[index] == newSize)
    {
        return;
    }
    m_columnWidth[index] = newSize;
    ScheduleColumnPrefsSave();
    RefreshLayout();
}

void TrackTableController::ClearStretchColumn()
{
    if (m_stretchColumn < 0)
    {
        return;
    }
    const int col = m_stretchColumn;
    m_stretchColumn = -1;
    QTableWidget* table = m_panel->trackTableWidget;
    table->horizontalHeader()->setSectionResizeMode(col, QHeaderView::Interactive);
    table->setColumnWidth(col, m_columnWidth[col]);
}

bool TrackTableController::UpdateColumnLayout()
{
    QTableWidget* table = m_panel ? m_panel->trackTableWidget : nullptr;
    if (!table)
    {
        return false;
    }

    int widthSum = 0;
    int lastVisible = -1;
    int visibleCount = 0;
    for (int i = 0; i < TrackColumns::Count; ++i)
    {
        if (m_columnVisibleMask & (1u << i))
        {
            widthSum += m_columnWidth[i];
            lastVisible = i;
            ++visibleCount;
        }
    }
    if (visibleCount == 0)
    {
        return false;
    }

    const bool needScroll = widthSum > table->viewport()->width();
    const int wantStretch = needScroll ? -1 : lastVisible;

    m_updatingLayout = true;
    if (m_stretchColumn >= 0 && m_stretchColumn != wantStretch)
    {
        ClearStretchColumn();
    }
    if (wantStretch >= 0 && m_stretchColumn != wantStretch)
    {
        m_stretchColumn = wantStretch;
        table->horizontalHeader()->setSectionResizeMode(wantStretch, QHeaderView::Stretch);
    }
    m_updatingLayout = false;

    const Qt::ScrollBarPolicy policy = needScroll ? Qt::ScrollBarAlwaysOn : Qt::ScrollBarAlwaysOff;
    const bool policyChanged = table->horizontalScrollBarPolicy() != policy;
    if (policyChanged)
    {
        table->setHorizontalScrollBarPolicy(policy);
    }
    return policyChanged;
}

void TrackTableController::RefreshLayout()
{
    if (UpdateColumnLayout())
    {
        UpdateTrackTableMaxHeight();
    }
}

bool TrackTableController::eventFilter(QObject* watched, QEvent* event)
{
    if (event && event->type() == QEvent::Resize && m_panel && m_panel->trackTableWidget
            && watched == m_panel->trackTableWidget->viewport())
    {
        RefreshLayout();
    }
    return QObject::eventFilter(watched, event);
}
