#include "Ui/MapView.h"
#include "Core/RadarConstants.h"
#include "Core/RadarTrackStore.h"
#include "Core/RadarTrackText.h"
#include "Ui/MapGeometry.h"
#include "Ui/Theme.h"

#include <QFontMetrics>
#include <QMouseEvent>
#include <QPair>
#include <QPaintEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPolygonF>
#include <QResizeEvent>
#include <QStringList>
#include <QWheelEvent>
#include <algorithm>
#include <cmath>

namespace
{
/// @brief 波束扇形路径
QPainterPath ptzBeamPath(const QPointF& center, double rMax, double azDeg, double halfDeg)
{
    const QRectF beamRect(center.x() - rMax, center.y() - rMax, rMax * 2.0, rMax * 2.0);
    QPainterPath beam;
    beam.moveTo(center);
    beam.arcTo(beamRect, 90.0 - (azDeg + halfDeg), halfDeg * 2.0);
    beam.closeSubpath();
    return beam;
}
}

MapView::MapView(RadarTrackStore* store, QWidget* parent) : QWidget(parent), m_store(store)
{
    setObjectName(QStringLiteral("mapView"));
    setMinimumWidth(480);
    setMouseTracking(true);


    connect(m_store, &RadarTrackStore::targetAdded, this, [this](quint16) { scheduleRefresh(); });
    connect(m_store, &RadarTrackStore::targetUpdated, this, [this](quint16 id)
    {
        ++m_trailVersion[id];
        scheduleRefresh();
    });
    connect(m_store, &RadarTrackStore::targetRemoved, this, [this](quint16 id, bool byDrop)
    {
        if (byDrop)
        {
            startFade(id);
        }
        m_trailVersion.remove(id);
        m_trailCache.remove(id);
        m_drawn.remove(id);
        scheduleRefresh();
    });

    m_fadeTimer = new QTimer(this);
    m_fadeTimer->setInterval(30);
    connect(m_fadeTimer, &QTimer::timeout, this, [this]()
    {
        for (FadingDot& f : m_fading)
        {
            f.t += 0.03 / 0.8;
        }
        for (int i = m_fading.size() - 1; i >= 0; --i)
        {
            if (m_fading.at(i).t >= 1.0)
            {
                m_fading.removeAt(i);
            }
        }
        if (m_fading.isEmpty())
        {
            m_fadeTimer->stop();
        }
        update();
    });

    m_refreshTimer = new QTimer(this);
    m_refreshTimer->setSingleShot(true);
    m_refreshTimer->setInterval(100);
    connect(m_refreshTimer, &QTimer::timeout, this, [this]()
    {
        if (m_dataDirty)
        {
            m_dataDirty = false;
            update();
        }
    });

    m_plotClock.start();
    m_plotTimer = new QTimer(this);
    m_plotTimer->setInterval(PlotTickMs);
    connect(m_plotTimer, &QTimer::timeout, this, [this]()
    {
        const int dt = static_cast<int>(m_plotClock.restart());

        int out = 0;
        for (int i = 0; i < m_plots.size(); ++i)
        {
            Plot& pl = m_plots[i];
            pl.ageMs += dt;
            if (pl.ageMs >= PlotLifeMs)
            {
                plotBucketRemove(pl);
                m_plotRow.remove(pl.seq);
                continue;
            }
            if (out != i)
            {
                m_plots[out] = pl;
                m_plotRow[pl.seq] = out;
            }
            ++out;
        }
        while (m_plots.size() > out)
        {
            m_plots.removeLast();
        }
        if (m_plots.isEmpty())
        {
            m_plotTimer->stop();
        }
        update();
    });

    m_staticRebuildTimer = new QTimer(this);
    m_staticRebuildTimer->setSingleShot(true);
    m_staticRebuildTimer->setInterval(50);
    connect(m_staticRebuildTimer, &QTimer::timeout, this, [this]()
    {
        markStaticDirty();
        update();
    });
}

double MapView::defaultScale() const
{
    const int half = std::min(width(), height()) / 2;
    return MapGeo::clampScale((half - 40) / m_defaultRangeM);
}

void MapView::resetView()
{
    m_pan = QPointF(0, 0);
    m_scale = defaultScale();
    m_userZoomed = false;
    markStaticDirty();
    update();
}

void MapView::setSectors(const double startDeg[4], const double endDeg[4])
{
    for (int i = 0; i < 4; ++i)
    {
        m_sectorStart[i] = startDeg[i];
        m_sectorEnd[i] = endDeg[i];
    }
    m_hasSectors = true;
    markStaticDirty();
    update();
}

void MapView::setFocusTarget(quint16 targetId)
{
    if (m_focusId == targetId)
    {
        return;
    }
    m_focusId = targetId;
    update();
}

void MapView::setPtzBeams(const QVector<BeamAngles>& beams)
{
    if (m_ptzBeams.size() == beams.size())
    {
        bool same = true;
        for (int i = 0; i < beams.size() && same; ++i)
        {
            same = qFuzzyCompare(m_ptzBeams.at(i).trueNorthDeg + 3600.0, beams.at(i).trueNorthDeg + 3600.0)
                    && m_ptzBeams.at(i).slot == beams.at(i).slot;
        }
        if (same)
        {
            return;
        }
    }
    m_ptzBeams = beams;
    update();
}

QColor MapView::beamColor(int beamIndex) const
{
    if (beamIndex < 0)
    {
        beamIndex = 0;
    }
    return BeamDisplayDefaults::Color(beamIndex);
}

qint64 MapView::plotBucketKey(int azBin, int rangeBin)
{
    return static_cast<qint64>(rangeBin) * PlotAzBinCount + azBin;
}

void MapView::plotBucketAdd(const Plot& pl)
{
    m_plotBuckets[plotBucketKey(pl.azBin, pl.rangeBin)].append(pl.seq);
}

void MapView::plotBucketRemove(const Plot& pl)
{
    const auto it = m_plotBuckets.find(plotBucketKey(pl.azBin, pl.rangeBin));
    if (it == m_plotBuckets.end())
    {
        return;
    }
    it->removeOne(pl.seq);
    if (it->isEmpty())
    {
        m_plotBuckets.erase(it);
    }
}

void MapView::setPoints(const QList<PointOut>& points)
{
    for (const PointOut& po : points)
    {
        double azDeg = std::fmod(po.azimuthAngle / 100.0, 360.0);
        if (azDeg < 0.0)
        {
            azDeg += 360.0;
        }
        const double rangeM = po.distance;
        const double arcDeg = PlotMatchRangeM / std::max(1.0, rangeM) * 180.0 / 3.14159265358979323846;
        const double tolAz = arcDeg < PlotMatchAzDeg ? arcDeg : PlotMatchAzDeg;

        const int azBin = static_cast<int>(azDeg / PlotMatchAzDeg) % PlotAzBinCount;
        const int rangeBin = static_cast<int>(std::floor(rangeM / PlotMatchRangeM));
        bool matched = false;
        for (int dAz = -1; dAz <= 1 && !matched; ++dAz)
        {
            const int nbAz = (azBin + dAz + PlotAzBinCount) % PlotAzBinCount;
            for (int dR = -1; dR <= 1 && !matched; ++dR)
            {
                const auto bucket = m_plotBuckets.constFind(plotBucketKey(nbAz, rangeBin + dR));
                if (bucket == m_plotBuckets.constEnd())
                {
                    continue;
                }
                for (quint32 seq : bucket.value())
                {
                    const int row = m_plotRow.value(seq, -1);
                    if (row < 0)
                    {
                        continue;
                    }
                    Plot& pl = m_plots[row];
                    if (std::fabs(pl.rangeM - rangeM) > PlotMatchRangeM)
                    {
                        continue;
                    }
                    double dAz2 = std::fabs(pl.azDeg - azDeg);
                    if (dAz2 > 180.0)
                    {
                        dAz2 = 360.0 - dAz2;
                    }
                    if (dAz2 <= tolAz)
                    {
                        pl.ageMs = 0;
                        matched = true;
                        break;
                    }
                }
            }
        }
        if (!matched)
        {
            Plot pl;
            pl.azDeg = azDeg;
            pl.rangeM = rangeM;
            const double azRad = qDegreesToRadians(azDeg);
            pl.polar = QPointF(rangeM * std::sin(azRad), -rangeM * std::cos(azRad));
            pl.ageMs = 0;
            pl.seq = ++m_plotSeq;
            pl.azBin = azBin;
            pl.rangeBin = rangeBin;
            m_plots.append(pl);
            m_plotRow.insert(pl.seq, m_plots.size() - 1);
            plotBucketAdd(pl);
        }
    }

    if (m_plots.size() > PlotMaxCount)
    {
        const int removeCount = m_plots.size() - PlotMaxCount;
        for (int i = 0; i < removeCount; ++i)
        {
            const Plot& oldest = m_plots.at(i);
            plotBucketRemove(oldest);
            m_plotRow.remove(oldest.seq);
        }
        m_plots.erase(m_plots.begin(), m_plots.begin() + removeCount);
        for (int i = 0; i < m_plots.size(); ++i)
        {
            m_plotRow[m_plots.at(i).seq] = i;
        }
    }

    if (!m_plots.isEmpty() && !m_plotTimer->isActive())
    {
        m_plotClock.restart();
        m_plotTimer->start();
    }
    update();
}

void MapView::setPointsVisible(bool visible)
{
    if (m_pointsVisible == visible)
    {
        return;
    }
    m_pointsVisible = visible;
    update();
}

void MapView::setCategoryColorOverrides(const QHash<quint8, QColor>& colors)
{
    QHash<quint8, QColor> sanitized;
    for (auto it = colors.constBegin(); it != colors.constEnd(); ++it)
    {
        if (TrackDisplayColors::IsKnownCategory(it.key()) && it.value().isValid())
        {
            sanitized.insert(it.key(), it.value());
        }
    }
    if (m_categoryColors == sanitized)
    {
        return;
    }
    m_categoryColors = sanitized;
    update();
}

void MapView::setCategoryAlphaOverrides(const QHash<quint8, int>& alphas)
{
    QHash<quint8, int> sanitized;
    for (auto it = alphas.constBegin(); it != alphas.constEnd(); ++it)
    {
        if (TrackDisplayColors::IsKnownCategory(it.key()))
        {
            sanitized.insert(it.key(), qBound(DisplayDefaults::TrailAlphaMin, it.value(), DisplayDefaults::TrailAlphaMax));
        }
    }
    if (m_categoryAlphas == sanitized)
    {
        return;
    }
    m_categoryAlphas = sanitized;
    update();
}

void MapView::setCategoryVisibilityOverrides(const QHash<quint8, bool>& visibility)
{
    QHash<quint8, bool> sanitized;
    for (auto it = visibility.constBegin(); it != visibility.constEnd(); ++it)
    {
        if (TrackDisplayColors::IsKnownCategory(it.key()) && !it.value())
        {
            sanitized.insert(it.key(), false);
        }
    }
    if (m_categoryVisibility == sanitized)
    {
        return;
    }
    m_categoryVisibility = sanitized;
    update();
}

QColor MapView::trailColor(quint8 category) const
{
    if (m_categoryColors.contains(category))
    {
        return m_categoryColors.value(category);
    }
    return TrackDisplayColors::DefaultColorFor(category);
}

int MapView::trailAlpha(quint8 category) const
{
    const int alpha = m_categoryAlphas.value(category, DisplayDefaults::TrailAlpha);
    return qBound(DisplayDefaults::TrailAlphaMin, alpha, DisplayDefaults::TrailAlphaMax);
}

bool MapView::isCategoryVisible(quint8 category) const
{
    return m_categoryVisibility.value(category, true);
}

void MapView::startFade(quint16 targetId)
{
    const auto it = m_drawn.constFind(targetId);
    if (it == m_drawn.constEnd())
    {
        return;
    }
    FadingDot dot;
    dot.pos = it->pos;
    dot.color = it->color;
    dot.label = QStringLiteral("No.%1").arg(targetId);
    dot.t = 0.0;
    m_fading.append(dot);

    if (m_focusId == targetId)
    {
        m_focusId = 0;
    }
    if (!m_fadeTimer->isActive())
    {
        m_fadeTimer->start();
    }
    update();
}

void MapView::scheduleRefresh()
{
    m_dataDirty = true;
    if (!m_refreshTimer->isActive())
    {
        m_refreshTimer->start();
    }
}

QPointF MapView::viewCenter() const
{
    return QRectF(rect()).center();
}

QPointF MapView::toScreen(double rangeM, double azDeg) const
{
    return MapGeo::polarToScreen(rangeM, azDeg, m_scale, viewCenter() + m_pan);
}

quint16 MapView::hitTest(const QPoint& pos) const
{
    if (!m_store)
    {
        return 0;
    }

    for (auto it = m_drawn.constBegin(); it != m_drawn.constEnd(); ++it)
    {
        if (!m_store->contains(it.key()))
        {
            continue;
        }
        if (QLineF(it.value().pos, pos).length() <= HitRadius)
        {
            return it.key();
        }
    }
    return 0;
}

void MapView::rebuildStaticLayer()
{
    const qreal dpr = devicePixelRatioF();
    const QSize logicalSz = staticLayerSize();
    const QSize deviceSz(qMax(1, qRound(logicalSz.width() * dpr)), qMax(1, qRound(logicalSz.height() * dpr)));
    if (m_staticLayer.size() != deviceSz || !qFuzzyCompare(m_staticLayer.devicePixelRatioF(), dpr))
    {
        m_staticLayer = QPixmap(deviceSz);
        m_staticLayer.setDevicePixelRatio(dpr);
    }
    m_staticLayer.fill(Qt::transparent);

    QPainter p(&m_staticLayer);
    // 抗锯齿
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setRenderHint(QPainter::TextAntialiasing, true);
    p.setRenderHint(QPainter::SmoothPixmapTransform, true);
    const QPointF c = QRectF(QPointF(0, 0), QSizeF(logicalSz)).center();

    if (m_hasSectors)
    {
        for (int i = 0; i < 4; ++i)
        {
            const double s = m_sectorStart[i];
            const double e = m_sectorEnd[i];
            if (e <= s || (e - s) >= 359.9)
            {
                continue;
            }
            const double rOuter = std::hypot(c.x(), c.y());
            const double startQt = 90.0 - e;
            const double sweep = e - s;
            QPainterPath wedge;
            wedge.moveTo(c);
            wedge.arcTo(QRectF(c.x() - rOuter, c.y() - rOuter, rOuter * 2, rOuter * 2), startQt, sweep);
            wedge.closeSubpath();
            p.fillPath(wedge, Theme::Pal::mapSector());
        }
    }

    const double maxRm = (qMin(c.x(), c.y()) - 20) / m_scale;
    const double step = MapGeo::niceRangeStep(m_scale, 120.0);
    QFont ringFont = font();
    ringFont.setPointSize(8);
    p.setFont(ringFont);
    for (double r = step; r <= maxRm; r += step)
    {
        const double rp = r * m_scale;
        p.setPen(QPen(Theme::Pal::mapRing(), 1));
        p.setBrush(Qt::NoBrush);
        p.drawEllipse(c, rp, rp);
        p.setPen(Theme::Pal::mapLabel());
        const QString label = r >= 1000.0
                ? QString::number(r / 1000.0, 'f', (std::fmod(r, 1000.0) > 0.5) ? 1 : 0) + " km"
                : QString::number(r, 'f', 0) + " m";
        p.drawText(QPointF(c.x() + 4, c.y() - rp - 3), label);
    }

    const double rMax = qMin(c.x(), c.y()) - 12;
    for (int az = 0; az < 360; az += 10)
    {
        const double azRad = qDegreesToRadians(double(az));
        const QPointF dir(std::sin(azRad), -std::cos(azRad));
        if (az % 30 == 0)
        {
            p.setPen(QPen(Theme::Pal::mapSpoke(), 1));
            p.drawLine(QLineF(c, c + dir * rMax));
        }
        else
        {
            p.setPen(QPen(Theme::Pal::mapTick(), 1));
            p.drawLine(QLineF(c + dir * (rMax - 8), c + dir * rMax));
        }
    }

    p.setPen(QPen(Theme::Pal::mapRadar(), 2));
    p.setBrush(Theme::Pal::mapRadar());
    p.drawEllipse(c, 4, 4);
    p.setBrush(Qt::NoBrush);
    p.setPen(QPen(Theme::Pal::mapRadar(), 1));
    p.drawEllipse(c, 9, 9);
    p.drawLine(QLineF(c + QPointF(-14, 0), c + QPointF(14, 0)));
    p.drawLine(QLineF(c + QPointF(0, -14), c + QPointF(0, 14)));

    m_staticDirty = false;
}

void MapView::drawPtzBeams(QPainter& p)
{
    if (m_ptzBeams.isEmpty())
    {
        return;
    }

    const QPointF c = toScreen(0.0, 0.0);
    const double rMax = qMax(qMax(std::hypot(c.x(), c.y()), std::hypot(width() - c.x(), c.y())),
                             qMax(std::hypot(c.x(), height() - c.y()), std::hypot(width() - c.x(), height() - c.y())));
    if (rMax <= 1.0)
    {
        return;
    }

    p.setRenderHint(QPainter::Antialiasing, true);

    for (int i = 0; i < m_ptzBeams.size(); ++i)
    {
        const BeamAngles& beam = m_ptzBeams.at(i);
        if (beam.trueNorthDeg < 0.0)
        {
            continue;
        }
        const int slot = beam.slot >= 0 ? beam.slot : i;
        const QColor base = beamColor(slot);
        const double halfDeg = qBound(0.25, BeamDisplayDefaults::WidthDeg * 0.5, BeamDisplayDefaults::WidthDegMax * 0.5);
        const double azNorm = Azimuth::Normalize(beam.trueNorthDeg);

        QColor head = base;
        QColor mid = base;
        QColor far = base;
        QColor tip = base;
        head.setAlpha(PtzBeamAlpha);
        mid.setAlpha(PtzBeamAlpha * PtzBeamMidAlphaPercent / 100);
        far.setAlpha(PtzBeamAlpha * PtzBeamFarAlphaPercent / 100);
        tip.setAlpha(0);
        QRadialGradient grad(c, rMax);
        grad.setColorAt(0.0, head);
        grad.setColorAt(PtzBeamMidStop, mid);
        grad.setColorAt(PtzBeamFarStop, far);
        grad.setColorAt(1.0, tip);

        p.setPen(Qt::NoPen);
        p.setBrush(grad);
        const QPainterPath beamShape = ptzBeamPath(c, rMax, azNorm, halfDeg);
        p.drawPath(beamShape);
    }
}

void MapView::drawBeamLegend(QPainter& p)
{
    if (m_ptzBeams.isEmpty())
    {
        return;
    }

    QVector<QPair<QColor, QString>> rows;
    for (int i = 0; i < m_ptzBeams.size(); ++i)
    {
        const BeamAngles& beam = m_ptzBeams.at(i);
        if (beam.trueNorthDeg < 0.0)
        {
            continue;
        }
        const double azDeg = std::lround(Azimuth::Normalize(beam.trueNorthDeg) * 100.0) % 36000 / 100.0;
        const QString angleText = tr("真北 %1°").arg(azDeg, 6, 'f', 2, QLatin1Char('0'));

        const int slot = beam.slot >= 0 ? beam.slot : i;
        rows.append(qMakePair(beamColor(slot), tr("波束%1  %2").arg(slot + 1).arg(angleText)));
    }
    if (rows.isEmpty())
    {
        return;
    }

    QFont legendFont = font();
    legendFont.setPointSize(8);
    p.setFont(legendFont);
    const QFontMetrics fm(legendFont);

    int textWidth = 0;
    for (const QPair<QColor, QString>& row : rows)
    {
        textWidth = qMax(textWidth, fm.horizontalAdvance(row.second));
    }
    const int padX = 8;
    const int padY = 6;
    const int swatch = 9;
    const int gap = 6;
    const int lineHeight = fm.lineSpacing();
    const QRectF box(10.0, 10.0, textWidth + padX * 2 + swatch + gap, rows.size() * lineHeight + padY * 2);

    p.setPen(Theme::Pal::floatBorder());
    p.setBrush(Theme::Pal::floatBg());
    p.drawRoundedRect(box, 5, 5);

    for (int i = 0; i < rows.size(); ++i)
    {
        const double top = box.top() + padY + i * lineHeight;
        const QRectF swatchRect(box.left() + padX, top + (lineHeight - swatch) / 2.0, swatch, swatch);
        p.setPen(Qt::NoPen);
        p.setBrush(rows.at(i).first);
        p.drawRect(swatchRect);
        p.setPen(Theme::Pal::mapFg());
        p.drawText(QRectF(swatchRect.right() + gap, top, textWidth, lineHeight), Qt::AlignLeft | Qt::AlignVCenter,
                   rows.at(i).second);
    }
}

void MapView::drawDynamicLayer(QPainter& p)
{
    if (!m_store)
    {
        return;
    }

    if (m_trailScale != m_scale)
    {
        m_trailCache.clear();
        m_trailScale = m_scale;
    }
    const QPointF radarCenter = toScreen(0.0, 0.0);

    drawPtzBeams(p);
    QVector<QVector<QRectF>> placedBands;
    qreal placedMaxW = 0.0;

    if (m_pointsVisible && !m_plots.isEmpty())
    {
        p.setPen(Qt::NoPen);
        for (const Plot& pl : m_plots)
        {
            qreal ratio = 1.0;
            if (pl.ageMs > PlotHoldMs)
            {
                ratio = 1.0 - double(pl.ageMs - PlotHoldMs) / double(PlotFadeMs);
            }
            ratio = qBound(qreal(0.0), ratio, qreal(1.0));

            QColor pc = Theme::Pal::mapPoint();
            pc.setAlpha(int(200.0 * ratio + 0.5));
            p.setBrush(pc);
            const double r = 1.8 - 0.6 * (1.0 - ratio);
            p.drawEllipse(radarCenter + pl.polar * m_scale, r, r);
        }
    }

    for (const FadingDot& f : m_fading)
    {
        const qreal alpha = 1.0 - f.t;
        const qreal r = 5.0 + 6.0 * f.t;
        QColor c = f.color;
        c.setAlphaF(alpha);
        p.setPen(Qt::NoPen);
        p.setBrush(c);
        p.drawEllipse(f.pos, r, r);
        {
            QColor lc = Theme::Pal::mapFg();
            lc.setAlpha(int(220 * alpha));
            p.setPen(lc);
        }
        p.drawText(f.pos + QPointF(7, -8), f.label);
    }

    const QList<quint16>& ids = m_store->targetIdsView();
    const QFontMetrics labelFm(p.fontMetrics());
    for (quint16 id : ids)
    {
        const QList<RadarTrack>& frames = m_store->history(id);
        if (frames.isEmpty())
        {
            continue;
        }

        const quint8 category = frames.constLast().targetCategory;
        const int alphaPct = trailAlpha(category);
        if (!isCategoryVisible(category) || alphaPct <= 0)
        {
            continue;
        }
        const QColor color = trailColor(category);
        const int lineAlpha = qRound(DisplayDefaults::TrailLineAlpha * alphaPct / 100.0);
        const int dotAlpha = qRound(255.0 * alphaPct / 100.0);
        const int total = frames.size();
        const int from = (m_focusId == id) ? 0 : std::max(0, total - m_trailMax);

        TrailCache& cache = m_trailCache[id];
        if (cache.version != m_trailVersion.value(id, 0) || cache.total != total || cache.from != from
                || cache.offsets.size() != total - from)
        {
            cache.offsets.clear();
            cache.offsets.reserve(total - from);
            for (int i = from; i < total; ++i)
            {
                const RadarTrack& f = frames.at(i);
                const double azRad = qDegreesToRadians(f.azimuthAngle / 100.0);
                const double rangeM = static_cast<double>(f.slantRange);
                cache.offsets << QPointF(rangeM * std::sin(azRad), -rangeM * std::cos(azRad)) * m_scale;
            }
            cache.version = m_trailVersion.value(id, 0);
            cache.total = total;
            cache.from = from;
        }

        if (cache.offsets.size() >= 2)
        {
            p.setPen(QPen(QColor(color.red(), color.green(), color.blue(), lineAlpha), 2));
            p.save();
            p.translate(radarCenter);
            p.drawPolyline(cache.offsets.constData(), cache.offsets.size());
            p.restore();
        }

        const RadarTrack& t = frames.constLast();
        const QPointF sp = cache.offsets.isEmpty() ? toScreen(t.slantRange, t.azimuthAngle / 100.0)
                                                   : radarCenter + cache.offsets.constLast();

        QColor dotColor = color;
        dotColor.setAlpha(dotAlpha);
        p.setPen(Qt::NoPen);
        p.setBrush(dotColor);
        p.drawEllipse(sp, 5, 5);
        if (t.trackStatus == TrackStatus::Initiated)
        {
            p.setBrush(Theme::Pal::mapBg());
            p.drawEllipse(sp, 2, 2);
        }
        m_drawn.insert(id, {sp, dotColor});

        if (m_hoverId == id)
        {
            p.setBrush(Qt::NoBrush);
            p.setPen(QPen(QColor(255, 255, 255, 220), 2));
            p.drawEllipse(sp, 8, 8);
        }

        if (m_focusId == id)
        {
            p.setBrush(Qt::NoBrush);
            p.setPen(QPen(Theme::Pal::mapRadar(), 2));
            p.drawEllipse(sp, 11, 11);
            p.drawLine(QLineF(sp + QPointF(-17, 0), sp + QPointF(-11, 0)));
            p.drawLine(QLineF(sp + QPointF(11, 0), sp + QPointF(17, 0)));
            p.drawLine(QLineF(sp + QPointF(0, -17), sp + QPointF(0, -11)));
            p.drawLine(QLineF(sp + QPointF(0, 11), sp + QPointF(0, 17)));
        }

        const QString label = QStringLiteral("No.%1").arg(id);
        const qreal tagW = labelFm.horizontalAdvance(label) + 10.0;
        const qreal tagH = labelFm.height() + 5.0;
        const QSizeF tagSize(tagW, tagH);
        const QPointF candidateOffsets[4] = {
            QPointF(7.0, -tagH - 3.0),
            QPointF(7.0, 3.0),
            QPointF(-tagW - 7.0, -tagH - 3.0),
            QPointF(-tagW - 7.0, 3.0),
        };

        const auto bandOf = [](qreal y) { return static_cast<int>(y / 48.0); };
        const auto overlapsPlaced = [&](const QRectF& r)
        {
            const int b0 = qMax(0, bandOf(r.top()));
            const int b1 = bandOf(r.bottom());
            for (int b = b0; b <= b1 && b < placedBands.size(); ++b)
            {
                const QVector<QRectF>& band = placedBands.at(b);
                auto it = std::lower_bound(band.cbegin(), band.cend(), r.left() - placedMaxW,
                                           [](const QRectF& placed, qreal bound) { return placed.left() < bound; });
                for (; it != band.cend() && it->left() <= r.right(); ++it)
                {
                    if (it->intersects(r))
                    {
                        return true;
                    }
                }
            }
            return false;
        };
        const auto addToBands = [&](const QRectF& r)
        {
            const int b0 = qMax(0, bandOf(r.top()));
            const int b1 = qMax(b0, bandOf(r.bottom()));
            while (placedBands.size() <= b1)
            {
                placedBands.append(QVector<QRectF>());
            }
            for (int b = b0; b <= b1; ++b)
            {
                QVector<QRectF>& band = placedBands[b];
                band.insert(std::upper_bound(band.begin(), band.end(), r.left(), [](qreal bound, const QRectF& placed) {
                    return bound < placed.left(); 
                }),r);
            }
            placedMaxW = qMax(placedMaxW, tagW);
        };

        QRectF bg(sp + candidateOffsets[0], tagSize);
        for (int cand = 1; cand < 4; ++cand)
        {
            const QRectF candidate(sp + candidateOffsets[cand], tagSize);
            if (!overlapsPlaced(candidate))
            {
                bg = candidate;
                break;
            }
        }
        bg.moveLeft(qBound<qreal>(1.0, bg.left(), qMax<qreal>(1.0, width() - tagW - 1.0)));
        bg.moveTop(qBound<qreal>(1.0, bg.top(), qMax<qreal>(1.0, height() - tagH - 1.0)));
        addToBands(bg);

        QColor tagBg = Theme::Pal::floatBg();
        tagBg.setAlpha(255);
        p.setPen(QPen(Theme::Pal::floatBorder(), 1));
        p.setBrush(tagBg);
        p.drawRoundedRect(bg, 3, 3);
        p.setPen(Theme::Pal::mapFg());
        p.drawText(bg, Qt::AlignCenter, label);
    }
}

void MapView::drawOverlay(QPainter& p)
{
    QFont f = font();
    f.setPointSize(8);
    p.setFont(f);

    if (m_hoverId != 0 && m_store && m_store->contains(m_hoverId))
    {
        const RadarTrack t = m_store->latest(m_hoverId);
        const QString text = QStringLiteral("编号: %1\n斜距: %2 m\n方位: %3°\n类型: %4")
                .arg(t.targetId)
                .arg(t.slantRange)
                .arg(Azimuth::Normalize(t.azimuthAngle / 100.0), 0, 'f', 2)
                .arg(TrackText::CategoryText(t.targetCategory));

        QFontMetrics fm(f);
        int textW = 0;
        const QStringList tipLines = text.split(QLatin1Char('\n'));
        for (const QString& line : tipLines)
        {
            textW = qMax(textW, fm.horizontalAdvance(line));
        }
        textW += 20;
        const int textH = fm.lineSpacing() * tipLines.size() + 8;

        int bx = m_hoverPos.x() + 14;
        int by = m_hoverPos.y() + 12;
        if (bx + textW > width() - 4)
        {
            bx = m_hoverPos.x() - textW - 10;
        }
        if (by + textH > height() - 4)
        {
            by = m_hoverPos.y() - textH - 10;
        }
        const QRectF box(bx, by, textW, textH);
        p.setPen(Theme::Pal::floatBorder());
        p.setBrush(Theme::Pal::floatBg());
        p.drawRoundedRect(box, 5, 5);
        p.setPen(Theme::Pal::mapFg());
        p.drawText(box.adjusted(8, 4, -8, -4), Qt::AlignLeft | Qt::AlignTop, text);
    }

    {
        const qreal R = 34.0;       
        const qreal rRing = R - 5.0;
        const QPointF cc(width() - 14.0 - R, 14.0 + R);

        p.setPen(QPen(Theme::Pal::floatBorder(), 1.5));
        p.setBrush(QColor(0, 0, 0, 150));
        p.drawEllipse(cc, R, R);
        p.setPen(QPen(Theme::Pal::floatBorder(), 1));
        p.setBrush(Qt::NoBrush);
        p.drawEllipse(cc, rRing, rRing);

        for (int i = 0; i < 24; ++i)
        {
            if (i == 0 || i == 12)
            {
                continue;
            }
            const bool major = (i % 3 == 0);
            const double a = qDegreesToRadians(i * 15.0);
            const QPointF dir(std::sin(a), -std::cos(a));
            p.setPen(QPen(major ? Theme::Pal::mapLabel() : Theme::Pal::mapTick(), major ? 1.6 : 1.0));
            const qreal rIn = major ? rRing - 6.0 : rRing - 3.5;
            p.drawLine(QLineF(cc + dir * rIn, cc + dir * rRing));
        }

        const qreal needleHalf = R - 14.0;
        const qreal needleW = 5.5;
        QPolygonF northTip, southTip;
        northTip << cc + QPointF(0.0, -needleHalf) << cc + QPointF(-needleW, 0.0)
                 << cc + QPointF(needleW, 0.0);
        southTip << cc + QPointF(0.0, needleHalf) << cc + QPointF(-needleW, 0.0)
                 << cc + QPointF(needleW, 0.0);
        p.setPen(Qt::NoPen);
        p.setBrush(Theme::Pal::statusLost());
        p.drawPolygon(northTip);
        QColor southColor = Theme::Pal::textPrimary();
        southColor.setAlpha(200);
        p.setBrush(southColor);
        p.drawPolygon(southTip);
        p.setBrush(Theme::Pal::textPrimary());
        p.drawEllipse(cc, 2.5, 2.5);

        QFont cf = font();
        cf.setPointSize(8);
        cf.setBold(true);
        p.setFont(cf);
        p.setPen(Theme::Pal::statusLost());
        p.drawText(QRectF(cc.x() - 10.0, cc.y() - R + 1.0, 20.0, 12.0), Qt::AlignCenter, QStringLiteral("N"));
        p.setPen(Theme::Pal::textMuted());
        p.drawText(QRectF(cc.x() - 10.0, cc.y() + R - 13.0, 20.0, 12.0), Qt::AlignCenter, QStringLiteral("S"));
    }

    drawBeamLegend(p);
}

void MapView::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    // 抗锯齿
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setRenderHint(QPainter::TextAntialiasing, true);
    p.setRenderHint(QPainter::SmoothPixmapTransform, true);
    p.fillRect(rect(), Theme::Pal::mapBg());

    if (m_staticDirty)
    {
        rebuildStaticLayer();
    }
    const QPointF radarPos = viewCenter() + m_pan;
    p.drawPixmap(radarPos - QRectF(QPointF(0, 0), QSizeF(staticLayerSize())).center(), m_staticLayer);

    drawDynamicLayer(p);
    drawOverlay(p);
}

void MapView::resizeEvent(QResizeEvent*)
{
    if (!m_userZoomed)
    {
        m_scale = defaultScale();
    }
    markStaticDirty();
}

void MapView::wheelEvent(QWheelEvent* e)
{
    const double factor = e->angleDelta().y() > 0 ? 1.15 : (1.0 / 1.15);
    m_userZoomed = true;
    const QPointF anchor = e->pos();

    const QPointF radar = (anchor - viewCenter() - m_pan) / m_scale;
    m_scale = MapGeo::clampScale(m_scale * factor);
    m_pan = anchor - viewCenter() - radar * m_scale;
    m_staticRebuildTimer->start();
    update();
}

void MapView::mousePressEvent(QMouseEvent* e)
{
    if (e->button() == Qt::RightButton)
    {
        if (m_focusId != 0)
        {
            emit focusClearRequested();
        }
        return;
    }
    if (e->button() != Qt::LeftButton)
    {
        return;
    }
    const quint16 id = hitTest(e->pos());
    if (id != 0)
    {
        emit trackSelected(id);
        m_dragging = false;
        return;
    }
    m_dragging = true;
    m_lastMouse = e->pos();
}

void MapView::mouseMoveEvent(QMouseEvent* e)
{
    if (m_dragging)
    {
        m_pan += QPointF(e->pos() - m_lastMouse);
        m_lastMouse = e->pos();
        update();
        return;
    }

    const quint16 id = hitTest(e->pos());
    if (id != m_hoverId)
    {
        m_hoverId = id;
        m_hoverPos = e->pos();
        update();
    }
    else if (id != 0 && (e->pos() - m_hoverPos).manhattanLength() >= 12)
    {
        m_hoverPos = e->pos();
        update();
    }
}

void MapView::mouseReleaseEvent(QMouseEvent* e)
{
    if (e->button() == Qt::LeftButton)
    {
        m_dragging = false;
    }
}

void MapView::mouseDoubleClickEvent(QMouseEvent* e)
{
    if (e->button() == Qt::LeftButton && hitTest(e->pos()) == 0)
    {
        resetView();
    }
}

void MapView::leaveEvent(QEvent*)
{
    if (m_hoverId != 0)
    {
        m_hoverId = 0;
        update();
    }
}
