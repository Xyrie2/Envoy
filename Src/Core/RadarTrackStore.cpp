#include "Core/RadarTrackStore.h"
#include "Core/RadarConstants.h"

#include <QDateTime>
#include <QDebug>

const QList<RadarTrack> RadarTrackStore::s_emptyHistory;

RadarTrackStore::RadarTrackStore(int maxHistoryPerTarget, int maxTargets, QObject* parent) : QObject(parent)
  , m_maxTargets(maxTargets > 0 ? maxTargets : 500)
  , m_maxHistoryPerTarget(maxHistoryPerTarget > 0 ? maxHistoryPerTarget : 500)
{
}

int RadarTrackStore::targetCount() const
{
    return m_history.size();
}

bool RadarTrackStore::contains(quint16 targetId) const
{
    return m_history.contains(targetId);
}

QList<quint16> RadarTrackStore::targetIds() const
{
    return m_insertOrder;
}

void RadarTrackStore::clear()
{
    const QList<quint16> ids = m_insertOrder;
    m_insertOrder.clear();
    m_history.clear();
    for (quint16 id : ids)
    {
        emit targetRemoved(id, false);
    }
}

const QList<RadarTrack>& RadarTrackStore::history(quint16 targetId) const{
    const auto it = m_history.constFind(targetId);
    return it != m_history.constEnd() ? it.value() : s_emptyHistory;
}

RadarTrack RadarTrackStore::latest(quint16 targetId) const
{
    const auto it = m_history.constFind(targetId);
    if (it == m_history.constEnd() || it.value().isEmpty())
    {
        return RadarTrack();
    }
    return it.value().constLast();
}

void RadarTrackStore::addTrack(const RadarTrack& track)
{
    const quint16 id = track.targetId;

    if (track.trackStatus > TrackStatus::Dropped)
    {
        ++m_badStatusCount;
        const qint64 now = QDateTime::currentMSecsSinceEpoch();
        if (m_lastBadStatusWarnMs == 0 || now - m_lastBadStatusWarnMs >= 1000)
        {
            qWarning().noquote() << QString("RadarTrackStore: 拒绝未知 trackStatus=%1 的航迹（目标 %2，累计 %3 个）")
                                        .arg(track.trackStatus)
                                        .arg(id)
                                        .arg(m_badStatusCount);
            m_badStatusCount = 0;
            m_lastBadStatusWarnMs = now;
        }
        return;
    }

    if (track.trackStatus == TrackStatus::Dropped)
    {
        if (m_history.remove(id) > 0)
        {
            m_insertOrder.removeAll(id);
            emit targetRemoved(id, true);
        }
        return;
    }

    auto it = m_history.find(id);
    if (it != m_history.end())
    {
        QList<RadarTrack>& frames = it.value();
        frames.append(track);
        while (frames.size() > m_maxHistoryPerTarget)
        {
            frames.removeFirst();
        }
        emit targetUpdated(id);
        return;
    }

    if (targetCount() >= m_maxTargets)
    {
        dropOldestTarget();
    }

    m_insertOrder.append(id);
    m_history.insert(id, QList<RadarTrack>() << track);
    emit targetAdded(id);
}

void RadarTrackStore::dropOldestTarget()
{
    if (m_insertOrder.isEmpty())
    {
        return;
    }
    const quint16 oldest = m_insertOrder.first();
    eraseTarget(oldest);
    emit targetRemoved(oldest, false);
}

void RadarTrackStore::eraseTarget(quint16 targetId)
{
    m_history.remove(targetId);
    m_insertOrder.removeAll(targetId);
}
