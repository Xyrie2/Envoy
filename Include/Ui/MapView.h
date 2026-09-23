#pragma once
#include <QColor>
#include <QElapsedTimer>
#include <QHash>
#include <QPixmap>
#include <QPoint>
#include <QPolygonF>
#include <QTimer>
#include <QVector>
#include <QWidget>

#include "Core/RadarProtocol.h"

class RadarTrackStore;
class QPainter;

class MapView : public QWidget
{
    Q_OBJECT
public:
    MapView(RadarTrackStore* store, QWidget* parent = nullptr);
    void setSectors(const double startDeg[4], const double endDeg[4]);
    void setFocusTarget(quint16 targetId);
    struct BeamAngles
    {
        double trueNorthDeg = -1.0;
        int slot = -1;
    };
    void setPtzBeams(const QVector<BeamAngles>& beams);

    void setPoints(const QList<PointOut>& points);
    void setPointsVisible(bool visible);
    void setCategoryColorOverrides(const QHash<quint8, QColor>& colors);
    void setCategoryAlphaOverrides(const QHash<quint8, int>& alphas);
    void setCategoryVisibilityOverrides(const QHash<quint8, bool>& visibility);

    QColor trailColor(quint8 category) const;
    int trailAlpha(quint8 category) const;
    bool isCategoryVisible(quint8 category) const;
    void resetView();
signals:
    void trackSelected(quint16 targetId);
    void focusClearRequested();
protected:
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;
    void leaveEvent(QEvent* event) override;

private:
    struct FadingDot
    {
        QPointF pos;
        QColor color;
        QString label;
        qreal t = 0.0;
    };
    void startFade(quint16 targetId);

    void rebuildStaticLayer();
    void drawDynamicLayer(QPainter& p);
    void drawPtzBeams(QPainter& p);
    void drawBeamLegend(QPainter& p);
    void drawOverlay(QPainter& p);

    QColor beamColor(int beamIndex) const;

    QPointF viewCenter() const;
    QPointF toScreen(double rangeM, double azDeg) const;
    quint16 hitTest(const QPoint& pos) const;

    void markStaticDirty() { m_staticDirty = true; }
    void scheduleRefresh();
    double defaultScale() const;

    QSize staticLayerSize() const { return size() + QSize(2 * StaticPad, 2 * StaticPad); }

    RadarTrackStore* m_store = nullptr;

    double m_scale = 0.0;
    QPointF m_pan;
    bool m_dragging = false;
    QPoint m_lastMouse;

    quint16 m_focusId = 0;
    quint16 m_hoverId = 0;
    QPoint m_hoverPos;


    struct DrawCache
    {
        QPointF pos;
        QColor color;
    };
    QHash<quint16, DrawCache> m_drawn;
    QList<FadingDot> m_fading;
    QTimer* m_fadeTimer = nullptr;

    QPixmap m_staticLayer;
    bool m_staticDirty = true;

    QTimer* m_refreshTimer = nullptr;
    bool m_dataDirty = false;
    QTimer* m_staticRebuildTimer = nullptr;

    bool m_userZoomed = false;
    QVector<BeamAngles> m_ptzBeams;

    struct Plot
    {
        double azDeg = 0.0;
        double rangeM = 0.0;
        QPointF polar;
        int ageMs = 0;
        quint32 seq = 0;
        int azBin = 0;
        int rangeBin = 0;
    };
    QList<Plot> m_plots;
    QHash<quint32, int> m_plotRow;
    QHash<qint64, QList<quint32>> m_plotBuckets; 
    quint32 m_plotSeq = 0; 

    static qint64 plotBucketKey(int azBin, int rangeBin);
    void plotBucketAdd(const Plot& pl);
    void plotBucketRemove(const Plot& pl);
    bool m_pointsVisible = true;
    QTimer* m_plotTimer = nullptr;
    QElapsedTimer m_plotClock;

    QHash<quint8, QColor> m_categoryColors;   
    QHash<quint8, int> m_categoryAlphas;      
    QHash<quint8, bool> m_categoryVisibility; 
    int m_trailMax = DefaultTrailMax;

    struct TrailCache
    {
        QPolygonF offsets; 
        quint32 version = 0;
        int total = 0;
        int from = 0;
    };
    QHash<quint16, TrailCache> m_trailCache;
    QHash<quint16, quint32> m_trailVersion;
    double m_trailScale = -1.0;

    static constexpr int PlotTickMs = 50;
    static constexpr int PlotHoldMs = 3000;
    static constexpr int PlotFadeMs = 5000;
    static constexpr int PlotLifeMs = PlotHoldMs + PlotFadeMs;
    static constexpr int PlotMaxCount = 2000;
    static constexpr double PlotMatchRangeM = 40.0;
    static constexpr double PlotMatchAzDeg = 1.5;
    static constexpr int PlotAzBinCount = static_cast<int>(360.0 / PlotMatchAzDeg);

    double m_sectorStart[4] = {0, 0, 0, 0};
    double m_sectorEnd[4] = {0, 0, 0, 0};
    bool m_hasSectors = false;

    double m_defaultRangeM = 5000.0;

    static constexpr int StaticPad = 600;
    static constexpr double HitRadius = 12.0;
    static constexpr int DefaultTrailMax = 200;

    static constexpr int PtzBeamAlpha = 120;
    static constexpr double PtzBeamMidStop = 0.50;
    static constexpr int PtzBeamMidAlphaPercent = 50;
    static constexpr double PtzBeamFarStop = 0.85;
    static constexpr int PtzBeamFarAlphaPercent = 15;
};
