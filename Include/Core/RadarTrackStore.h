#pragma once
#include <QHash>
#include <QList>
#include <QObject>

#include "Core/RadarProtocol.h"

class RadarTrackStore : public QObject
{
    Q_OBJECT
public:
    RadarTrackStore(int maxHistoryPerTarget = 500, int maxTargets = 500, QObject* parent = nullptr);

    int targetCount() const;
    QList<quint16> targetIds() const;
    /// @brief 目标序只读视图
    const QList<quint16>& targetIdsView() const { return m_insertOrder; }
    bool contains(quint16 targetId) const;
    RadarTrack latest(quint16 targetId) const;
    const QList<RadarTrack>& history(quint16 targetId) const;
public slots:
    void addTrack(const RadarTrack& track);

    /// <summary>
    /// @brief 清空全部目标
    /// </summary>
    void clear();
signals:
    void targetAdded(quint16 targetId);
    void targetUpdated(quint16 targetId);
    void targetRemoved(quint16 targetId, bool byDrop);
private:
    void dropOldestTarget();
    void eraseTarget(quint16 id);

    int m_maxTargets;
    int m_maxHistoryPerTarget;
    QList<quint16> m_insertOrder;
    QHash<quint16, QList<RadarTrack>> m_history;
    static const QList<RadarTrack> s_emptyHistory;

    qint64 m_lastBadStatusWarnMs = 0;
    quint64 m_badStatusCount = 0;
};
