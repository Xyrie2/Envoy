#pragma once
#include <QPointF>
#include <QtMath>
#include <algorithm>

namespace MapGeo
{
constexpr double MinScale = 0.005;
constexpr double MaxScale = 2.0;

inline QPointF polarToScreen(double rangeM, double azDeg, double scalePxPerM, const QPointF& centerPx)
{
    const double azRad = qDegreesToRadians(azDeg);
    return centerPx + QPointF(rangeM * std::sin(azRad), -rangeM * std::cos(azRad)) * scalePxPerM;
}

inline double clampScale(double scale)
{
    return std::min(MaxScale, std::max(MinScale, scale));
}

inline double niceRangeStep(double scalePxPerM, double targetPx = 120.0)
{
    const double raw = targetPx / scalePxPerM;
    const double p = std::pow(10.0, std::floor(std::log10(raw)));
    static const double mult[] = {1.0, 2.0, 5.0, 10.0};
    for (double m : mult)
    {
        if (m * p >= raw)
        {
            return m * p;
        }
    }
    return 10.0 * p;
}

}
