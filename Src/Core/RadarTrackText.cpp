#include "Core/RadarTrackText.h"

namespace TrackText
{

QString StatusText(quint16 status)
{
    switch (status)
    {
    case TrackStatus::Initiated:
        return QStringLiteral("起批未确认");
    case TrackStatus::Confirmed:
        return QStringLiteral("确认目标");
    case TrackStatus::Dropped:
        return QStringLiteral("目标丢弃");
    default:
        return QStringLiteral("未知(%1)").arg(status);
    }
}

QString CategoryText(quint8 category)
{
    switch (category)
    {
    case TargetCategory::Other:
        return QStringLiteral("其它");
    case TargetCategory::Vehicle:
        return QStringLiteral("车辆");
    case TargetCategory::Quadcopter:
        return QStringLiteral("四旋翼");
    case TargetCategory::FixedWing:
        return QStringLiteral("固定翼");
    case TargetCategory::Bird:
        return QStringLiteral("鸟");
    case TargetCategory::Ship:
        return QStringLiteral("船");
    case TargetCategory::Pedestrian:
        return QStringLiteral("行人");
    case TargetCategory::Airliner:
        return QStringLiteral("客机");
    case TargetCategory::Unidentified:
        return QStringLiteral("待识别");
    default:
        return QStringLiteral("未知(%1)").arg(category);
    }
}

QString SignalConfigDetails(const SignalProcessingConfig& cfg)
{
    QString str;
    str += QStringLiteral("帧头: %1\n").arg(cfg.frameHeader, 0, 16);
    str += QStringLiteral("字节数: %1\n").arg(cfg.packetLength);
    str += QStringLiteral("发射起始扇区: %1\n").arg(cfg.emissionStartAngle0);
    str += QStringLiteral("发射终止扇区: %1\n").arg(cfg.emissionEndAngle0);
    str += QStringLiteral("激励发射开关: %1\n").arg(cfg.emissionSwitch);
    str += QStringLiteral("搜索模式: %1\n").arg(cfg.scanMode);
    str += QStringLiteral("工作频点: %1\n").arg(cfg.frequencyPoint);
    str += QStringLiteral("信噪比门限: %1\n").arg(cfg.snrThreshold);
    str += QStringLiteral("检测门限: %1\n").arg(cfg.amplitudeThreshold);
    str += QStringLiteral("杂波抑制带宽: %1\n").arg(cfg.clutterSuppressionBandwidth);
    str += QStringLiteral("转台旋转轴: %1\n").arg(cfg.turntableAxis);
    str += QStringLiteral("转台运动模式: %1\n").arg(cfg.turntableMode);
    str += QStringLiteral("转台定位角度: %1\n").arg(cfg.turntablePositionAngle);
    str += QStringLiteral("转台扇扫中心角度: %1\n").arg(cfg.turntableSectorCenter);
    str += QStringLiteral("转台扇扫单侧角度范围: %1\n").arg(cfg.turntableSectorSize);
    str += QStringLiteral("转台旋转速度: %1\n").arg(cfg.turntableSpeed);
    str += QStringLiteral("STC使能: %1\n").arg(cfg.stcEnable);
    str += QStringLiteral("STC曲线: %1\n").arg(cfg.stcSelection);
    str += QStringLiteral("时序模式: %1\n").arg(cfg.frameAccumulationEnable);
    return str;
}

QString DataConfigDetails(const DataProcessingConfig& cfg)
{
    QString str;
    str += QStringLiteral("帧头: %1\n").arg(cfg.frameHeader, 0, 16);
    str += QStringLiteral("字节数: %1\n").arg(cfg.packetLength);
    str += QStringLiteral("调波束跟踪: %1\n").arg(cfg.beamTrackingMode);
    str += QStringLiteral("套波门选择: %1\n").arg(cfg.waveGateSelection);
    str += QStringLiteral("扇形跟踪选择: %1\n").arg(cfg.sectorTrackingMode);
    str += QStringLiteral("指定跟踪航迹号: %1\n").arg(cfg.designatedTrackNumber);
    str += QStringLiteral("搜索跟踪选择: %1\n").arg(cfg.scanTrackingSelection);
    str += QStringLiteral("航迹起始选择: %1\n").arg(cfg.trackInitiationMethod);
    str += QStringLiteral("杂波图开关: %1\n").arg(cfg.clutterMapEnable);
    str += QStringLiteral("起始滑窗分母: %1\n").arg(cfg.trackInitiationWindowN);
    str += QStringLiteral("起始滑窗分子: %1\n").arg(cfg.trackInitiationWindowM);
    str += QStringLiteral("滤波模型: %1\n").arg(cfg.filteringModel);
    str += QStringLiteral("关联准则: %1\n").arg(cfg.associationRule);
    str += QStringLiteral("消亡周期: %1\n").arg(cfg.trackTerminationCycle);
    str += QStringLiteral("处理距离上限: %1\n").arg(cfg.maxProcessingRange);
    str += QStringLiteral("处理距离下限: %1\n").arg(cfg.minProcessingRange);
    str += QStringLiteral("处理速度上限: %1\n").arg(cfg.maxProcessingVelocity);
    str += QStringLiteral("处理速度下限: %1\n").arg(cfg.minProcessingVelocity);
    str += QStringLiteral("处理高度上限: %1\n").arg(cfg.maxProcessingAltitude);
    str += QStringLiteral("处理高度下限: %1\n").arg(cfg.minProcessingAltitude);
    str += QStringLiteral("仰角修正: %1\n").arg(cfg.elevationCorrection);
    str += QStringLiteral("距离修正: %1\n").arg(cfg.rangeCorrection);
    str += QStringLiteral("手动输入仰角值: %1\n").arg(cfg.manualElevationInput);
    str += QStringLiteral("偏北角修正: %1\n").arg(cfg.northCorrection);
    str += QStringLiteral("俯仰角选择: %1\n").arg(cfg.elevationSourceSelection);
    str += QStringLiteral("雷达经度: %1\n").arg(cfg.radarLongitude);
    str += QStringLiteral("雷达纬度: %1\n").arg(cfg.radarLatitude);
    str += QStringLiteral("雷达高度: %1\n").arg(cfg.radarAltitude);
    str += QStringLiteral("是否插值: %1\n").arg(cfg.interpolationEnable);
    str += QStringLiteral("副瓣匿影: %1\n").arg(cfg.sidelobeSuppression);
    str += QStringLiteral("杂波图门限: %1\n").arg(cfg.clutterMapThreshold);
    str += QStringLiteral("数据开关: %1\n").arg(cfg.dataOutputSwitch);
    str += QStringLiteral("罗盘相关参数设置: %1\n").arg(cfg.compassParamSetting);
    str += QStringLiteral("数处界面显示开关: %1\n").arg(cfg.displayInterfaceSwitch);
    return str;
}

}
