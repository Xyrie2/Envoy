#pragma once
#include <QString>
#include <QStringList>
#include <QtGlobal>

#include "Core/RadarConfig.h"
#include "Core/RadarProtocol.h"

/// <summary>
/// @brief 信处/数处参数回传 → 界面参数（UIConfig）
/// </summary>
namespace ConfigBackfill
{
constexpr double SectorMin = 0.0, SectorMax = 360.0;                  /// 扇区角度（度）
constexpr double NorthCorrMin = -180.0, NorthCorrMax = 180.0;         /// 偏北修正（度）
constexpr double PitchCorrMin = -90.0, PitchCorrMax = 90.0;           /// 俯仰修正（度）
constexpr double RangeCorrMin = -1.0e6, RangeCorrMax = 1.0e6;         /// 斜距修正（米）
constexpr double LonMin = -180.0, LonMax = 180.0;                     /// 经度（度）
constexpr double LatMin = -90.0, LatMax = 90.0;                       /// 纬度（度）
constexpr double HeightLimitMin = -100.0, HeightLimitMax = 20000.0;   /// 高度限制（米）
constexpr double SpeedLimitMin = 0.0, SpeedLimitMax = 100.0;          /// 速度限制（m/s）
constexpr double DistLimitMin = 0.0, DistLimitMax = 1.0e6;            /// 距离限制（米）

/// <summary>
/// @brief 单值校验
/// </summary>
inline bool IsValidValue(double value, double minV, double maxV, const QString& label, QStringList& skipped)
{
    if (!qIsFinite(value))
    {
        skipped << QStringLiteral("%1：数值无效").arg(label);
        return false;
    }
    if (value < minV || value > maxV)
    {
        skipped << QStringLiteral("%1：%2 超出有效范围 [%3, %4]").arg(label).arg(value).arg(minV).arg(maxV);
        return false;
    }
    return true;
}

/// <summary>
/// @brief 成对校验
/// </summary>
inline bool IsValidPair(double lower, double upper, double boundMin, double boundMax,
                        const QString& lowerLabel, const QString& upperLabel, QStringList& skipped)
{
    const bool lowerOk = IsValidValue(lower, boundMin, boundMax, lowerLabel, skipped);
    const bool upperOk = IsValidValue(upper, boundMin, boundMax, upperLabel, skipped);
    if (!lowerOk || !upperOk)
    {
        return false;
    }
    if (lower > upper)
    {
        skipped << QStringLiteral("%1/%2：下限 %3 大于上限 %4，成对丢弃")
                       .arg(lowerLabel, upperLabel).arg(lower).arg(upper);
        return false;
    }
    return true;
}

/// <summary>
/// @brief 数处回传提取
/// </summary>
inline int ExtractDataConfig(const DataProcessingConfig& cfg, UIConfig& next, QStringList& skipped)
{
    int applied = 0;
    double v = 0.0;

    v = cfg.northCorrection / 100.0;
    if (IsValidValue(v, NorthCorrMin, NorthCorrMax, QStringLiteral("偏北修正(°)"), skipped))
    {
        next.northCorrection = static_cast<float>(v);
        ++applied;
    }

    v = cfg.elevationCorrection / 100.0;
    if (IsValidValue(v, PitchCorrMin, PitchCorrMax, QStringLiteral("俯仰修正(°)"), skipped))
    {
        next.pitchCorrection = static_cast<float>(v);
        ++applied;
    }

    v = cfg.rangeCorrection;
    if (IsValidValue(v, RangeCorrMin, RangeCorrMax, QStringLiteral("斜距修正(m)"), skipped))
    {
        next.distanceCorrection = static_cast<float>(v);
        ++applied;
    }

    if (IsValidValue(cfg.radarLongitude, LonMin, LonMax, QStringLiteral("雷达经度(°)"), skipped))
    {
        next.radarLongitude = cfg.radarLongitude;
        ++applied;
    }

    if (IsValidValue(cfg.radarLatitude, LatMin, LatMax, QStringLiteral("雷达纬度(°)"), skipped))
    {
        next.radarLatitude = cfg.radarLatitude;
        ++applied;
    }

    if (IsValidValue(cfg.radarAltitude, HeightLimitMin, HeightLimitMax, QStringLiteral("雷达高度(m)"), skipped))
    {
        next.radarHeight = cfg.radarAltitude;
        ++applied;
    }

    if (IsValidPair(cfg.minProcessingVelocity, cfg.maxProcessingVelocity, SpeedLimitMin, SpeedLimitMax,
                    QStringLiteral("速度下限(m/s)"), QStringLiteral("速度上限(m/s)"), skipped))
    {
        next.speedMin = cfg.minProcessingVelocity;
        next.speedMax = cfg.maxProcessingVelocity;
        applied += 2;
    }

    if (IsValidPair(cfg.minProcessingAltitude, cfg.maxProcessingAltitude, HeightLimitMin, HeightLimitMax,
                    QStringLiteral("高度下限(m)"), QStringLiteral("高度上限(m)"), skipped))
    {
        next.heightMin = cfg.minProcessingAltitude;
        next.heightMax = cfg.maxProcessingAltitude;
        applied += 2;
    }

    if (IsValidPair(cfg.minProcessingRange, cfg.maxProcessingRange, DistLimitMin, DistLimitMax,
                    QStringLiteral("距离下限(m)"), QStringLiteral("距离上限(m)"), skipped))
    {
        next.distanceMin = cfg.minProcessingRange;
        next.distanceMax = cfg.maxProcessingRange;
        applied += 2;
    }

    return applied;
}

/// <summary>
/// @brief 信处回传提取
/// </summary>
inline int ExtractSignalConfig(const SignalProcessingConfig& cfg, UIConfig& next, QStringList& skipped)
{
    if (!IsValidPair(cfg.emissionStartAngle0, cfg.emissionEndAngle0, SectorMin, SectorMax,
                     QStringLiteral("起始扇区0(°)"), QStringLiteral("结束扇区0(°)"), skipped))
    {
        return 0;
    }
    next.startScan0 = cfg.emissionStartAngle0;
    next.endScan0 = cfg.emissionEndAngle0;
    return 2;
}

/// <summary>
/// @brief 组装回填结果日志
/// </summary>
inline QString SummaryText(const QString& kindText, int applied, const QStringList& skipped)
{
    if (applied <= 0)
    {
        QString msg = QStringLiteral("【参数回填】%1参数全部无效，界面未改动").arg(kindText);
        if (!skipped.isEmpty())
        {
            msg += QStringLiteral("：\r\n- %1").arg(skipped.join(QStringLiteral("\r\n- ")));
        }
        return msg;
    }

    QString msg = QStringLiteral("【参数回填】%1参数已回填 %2 项").arg(kindText).arg(applied);
    if (!skipped.isEmpty())
    {
        msg += QStringLiteral("，跳过 %1 项：\r\n- %2").arg(skipped.count()).arg(skipped.join(QStringLiteral("\r\n- ")));
    }
    return msg;
}
}
