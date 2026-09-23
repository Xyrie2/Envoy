#pragma once
#include <QObject>
#include <QSet>
#include <QString>
#include <QVector>
#include <QtGlobal>

#include "Core/RadarProtocol.h"

class QMenu;
class QSplitter;
class QTimer;
class RadarTrackStore;
class TargetListPanel;

/// <summary>
/// @brief 目标列表列模型
/// </summary>
namespace TrackColumns
{
enum class Col
{
    Id = 0,   /// 编号
    Range,    /// 斜距
    Alt,      /// 高度
    Az,       /// 方位
    El,       /// 俯仰
    Amp,      /// 幅度
    Snr,      /// 信噪比
    Category, /// 类型
    Conf,     /// 置信度
    Points,   /// 跟踪次数
    Count
};

struct Spec
{
    Col         col;          
    const char* key;          
    const char* title;        
    int         defaultWidth; 
    bool        required;     
};

constexpr int Count = static_cast<int>(Col::Count);

const Spec* Specs();
int IndexOfKey(const QString& key);
} 

class TrackTableController : public QObject
{
    Q_OBJECT
public:
    enum class ViewMode
    {
        TargetList,
        FocusHistory
    };

    static constexpr int MaxFocusRows = 500;
    static constexpr int MaxTargets = 500;

    TrackTableController(TargetListPanel* panel, RadarTrackStore* store, QSplitter* rightSplitter, QObject* parent = nullptr);
    ~TrackTableController() override;
    bool eventFilter(QObject* watched, QEvent* event) override;

    void InitTables();
    void ResetDataStats();
public slots:
    void SetViewMode(ViewMode mode);
    void ShowFocusHistoryOf(quint16 targetId);
    void ClearFocus();
signals:
    void FocusTargetChanged(quint16 targetId);
private slots:
    void OnTargetAdded(quint16 targetId);
    void OnTargetUpdated(quint16 targetId);
    void OnTargetRemoved(quint16 targetId, bool byDrop);
    void OnTableSelectionChanged();
    void OnColumnActionToggled(bool checked);
    void FlushPendingUpdates();
private:
    void RebuildTargetListView();
    void FillFocusHistory(quint16 id);
    void RefreshTargetRow(quint16 targetId);
    void InsertTargetRow(quint16 targetId);
    void RemoveTargetRow(quint16 targetId);
    int RowIndexOfTarget(quint16 targetId) const;
    void FillTrackRow(int row, const RadarTrack& track);
    void UpdateStatsLabel();
    void UpdateTrackTableMaxHeight();

    bool IsColumnVisible(TrackColumns::Col col) const;
    void SetColumnVisible(TrackColumns::Col col, bool visible);
    void ApplyColumnVisibility(); 
    void ShowOnlyColumns(quint32 mask);
    void BuildColumnMenu();
    void SyncColumnMenu();  
    void OnSectionResized(int index, int oldSize, int newSize);

    bool UpdateColumnLayout(); 
    void ClearStretchColumn();
    void RefreshLayout(); 

    void LoadColumnPrefs(); 
    void SaveColumnPrefs(); 
    void ScheduleColumnPrefsSave();

    TargetListPanel* m_panel = nullptr;
    RadarTrackStore* m_store = nullptr;
    QSplitter* m_rightSplitter = nullptr;

    QVector<quint16> m_rowIds;
    ViewMode m_viewMode = ViewMode::TargetList;
    bool m_hasFocusTarget = false;
    quint16 m_focusTargetId = 0;
    int m_statTotal = 0;
    int m_statDropped = 0;

    QSet<quint16> m_dirtyTargets;
    QTimer* m_flushTimer = nullptr;
    bool m_focusHistoryDirty = false;
    int m_focusSeen = 0;
    int m_focusFront = 0;

    QMenu* m_columnMenu = nullptr;
    quint32 m_columnVisibleMask = 0;             
    bool m_updatingMenu = false;                 
    int m_columnWidth[TrackColumns::Count] = {}; 
    QTimer* m_saveTimer = nullptr;               
    int m_stretchColumn = -1;                    
    bool m_updatingLayout = false;              
};
