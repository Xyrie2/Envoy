#pragma once
#include <QColor>
#include <QString>
#include <QVector>
#include <cmath>
#include <QtGlobal>

/// <summary>
/// @brief 协议帧头定义
/// </summary>
namespace FrameHeader
{
constexpr quint32 Track = 0xCC5555CC;           /// 航迹协议帧头
constexpr quint32 State = 0x55DDDD55;           /// 状态协议帧头（状态1）
constexpr quint32 State2 = 0x19576492;          /// 状态协议帧头（状态2）
constexpr quint32 State3 = 0xCC1111CC;          /// 状态协议帧头（状态3）
constexpr quint32 Ptz = 0x55CCCC55;             /// 过界协议帧头
constexpr quint32 Point = 0xAA5555AA;           /// 点迹协议帧头
constexpr quint32 SignalConfig = 0x55AAAA55;    /// 信处参数帧头
constexpr quint32 DataConfig = 0x55222255;      /// 数处参数帧头
constexpr quint32 Control = 0x56482785;         /// 控制协议帧头
constexpr quint32 ControlRespect = 0xDD5555DD;  /// 控制协议回传帧头
}

/// <summary>
/// @brief 控制协议功能号定义
/// </summary>
namespace ControlFunctionId
{
constexpr qint32 PowerControl = 1;        /// 开关机
constexpr qint32 Sector0 = 3;             /// 扇区0
constexpr qint32 WorkMode = 5;            /// 工作模式控制
constexpr qint32 NorthCorrection = 8;     /// 偏北修正
constexpr qint32 ElevationCorrection = 9; /// 俯仰修正
constexpr qint32 RangeCorrection = 10;    /// 斜距修正
constexpr qint32 RadarPosition = 11;      /// 雷达经纬高
constexpr qint32 SpeedRange = 21;         /// 处理速度范围
constexpr qint32 HeightRange = 22;        /// 处理高度范围
constexpr qint32 DistanceRange = 23;      /// 处理距离范围
constexpr qint32 QueryParameters = 25;    /// 参数查询
constexpr qint32 Sector1 = 44;            /// 扇区1
constexpr qint32 Sector2 = 45;            /// 扇区2
constexpr qint32 Sector3 = 46;            /// 扇区3
}

/// <summary>
/// @brief 默认参数值
/// </summary>
namespace DefaultValues
{
constexpr double RadarLongitude = 117.214480;         /// 默认雷达经度
constexpr double RadarLatitude = 31.714677;           /// 默认雷达纬度
const QString LocalIp = QStringLiteral("127.0.0.1");  /// 默认本地IP
const QString RemoteIp = QStringLiteral("127.0.0.1"); /// 默认远程IP
constexpr quint16 LocalPort = 20001;                  /// 默认本地端口
constexpr quint16 RemotePort = 20000;                 /// 默认远程端口
constexpr int UdpBufferSize = 1024 * 1024;            /// UDP接收缓冲区大小
}

/// <summary>
/// @brief 航迹显示默认参数
/// </summary>
namespace DisplayDefaults
{
constexpr int TrailAlpha = 100;      /// 默认航迹绘制透明度（%）
constexpr int TrailAlphaMin = 0;     /// 航迹绘制透明度下限（%）
constexpr int TrailAlphaMax = 100;   /// 航迹绘制透明度上限（%）
constexpr int TrailAlphaStep = 5;    /// 航迹绘制透明度调节步长（%）
constexpr int TrailLineAlpha = 210;  /// 航迹线绘制亮度基准（0-255，整条均匀显示档）
}

/// <summary>
/// @brief 波束光柱显示默认参数
/// </summary>
namespace BeamDisplayDefaults
{
constexpr double WidthDeg = 5.0;     /// 默认光柱全张角宽度
constexpr double WidthDegMax = 30.0; /// 光柱宽度上限
inline QColor Color(int slot)        /// 默认配色
{
    static const QColor kColors[4] = {
        QColor(0x60, 0xa5, 0xfa), QColor(0x22, 0xd3, 0xee), QColor(0xfb, 0xbf, 0x24), QColor(0xc0, 0x84, 0xfc),
    };
    return kColors[slot % 4];
}
}

/// <summary>
/// @brief 方位角显示口径统一
/// </summary>
namespace Azimuth
{
inline double Normalize(double azDeg)
{
    double az = std::fmod(azDeg, 360.0);
    if (az < 0.0)
    {
        az += 360.0;
    }
    return az;
}
}

/// <summary>
/// @brief 过界/波束角度显示约定
/// </summary>
namespace PtzDisplay
{
constexpr int FaceCount = 4;           /// 默认显示全部 4 个波束槽位
constexpr quint16 RawAngleMax = 36000; /// 原始角度值域上限（0.01 度），越界视为无效
constexpr int PtzStaleMs = 1500;       /// 过界帧超过该时长未更新则视为失效

/// <summary>
/// @brief 角度原始值是否在协议值域内（0~36000，即 0~360.00 度）
/// </summary>
inline bool IsValidRawAngle(quint16 rawAngle)
{
    return rawAngle <= RawAngleMax;
}

/// <summary>
/// @brief 该波束槽位是否无数据（机械与偏北原始值同时为 0；协议预留/未使用槽位通常填 0）
/// </summary>
inline bool IsEmptySlot(quint16 mechanicalRaw, quint16 trueNorthRaw)
{
    return mechanicalRaw == 0 && trueNorthRaw == 0;
}

/// <summary>
/// @brief 协议原始角度（0.01 度）转度
/// </summary>
inline double RawToDegree(quint16 rawAngle)
{
    return rawAngle / 100.0;
}
}

/// <summary>
/// @brief 航迹状态定义
/// </summary>
namespace TrackStatus
{
constexpr quint16 Initiated = 0;  /// 起批未确认
constexpr quint16 Confirmed = 1;  /// 确认目标
constexpr quint16 Dropped = 2;    /// 已丢弃
}

/// <summary>
/// @brief 目标类型定义
/// </summary>
namespace TargetCategory
{
constexpr quint8 Other = 0;          /// 其它
constexpr quint8 Vehicle = 1;        /// 车辆
constexpr quint8 Quadcopter = 2;     /// 四旋翼
constexpr quint8 FixedWing = 3;      /// 固定翼
constexpr quint8 Bird = 4;           /// 鸟
constexpr quint8 Ship = 5;           /// 船
constexpr quint8 Pedestrian = 6;     /// 行人
constexpr quint8 Airliner = 7;       /// 客机
constexpr quint8 Unidentified = 255; /// 待识别
}

/// <summary>
/// @brief 目标类型航迹颜色表
/// </summary>
namespace TrackDisplayColors
{
struct CategoryColor
{
    quint8 category;     /// 目标类型编号
    const char* title;   /// 类型名称（UTF-8）
    QColor defaultColor; /// 默认颜色
};

/// @brief 全部颜色桶
inline const QVector<CategoryColor>& Table()
{
    static const QVector<CategoryColor> table = {
        {TargetCategory::Other, "其它", QColor(0x94, 0xa3, 0xb8)},
        {TargetCategory::Vehicle, "车辆", QColor(0x06, 0xb6, 0xd4)},
        {TargetCategory::Quadcopter, "四旋翼", QColor(0xef, 0x44, 0x44)},
        {TargetCategory::FixedWing, "固定翼", QColor(0xf9, 0x73, 0x16)},
        {TargetCategory::Bird, "鸟", QColor(0xea, 0xb3, 0x08)},
        {TargetCategory::Ship, "船", QColor(0x3b, 0x82, 0xf6)},
        {TargetCategory::Pedestrian, "行人", QColor(0x22, 0xc5, 0x5e)},
        {TargetCategory::Airliner, "客机", QColor(0xa8, 0x55, 0xf7)},
        {TargetCategory::Unidentified, "待识别", QColor(0x64, 0x74, 0x8b)},
    };
    return table;
}

/// @brief 类型默认颜色
inline QColor DefaultColorFor(quint8 category)
{
    const QVector<CategoryColor>& table = Table();
    for (const CategoryColor& item : table)
    {
        if (item.category == category)
        {
            return item.defaultColor;
        }
    }
    return QColor(0x94, 0xa3, 0xb8);
}

/// @brief 类型是否为可配置颜色桶
inline bool IsKnownCategory(quint8 category)
{
    const QVector<CategoryColor>& table = Table();
    for (const CategoryColor& item : table)
    {
        if (item.category == category)
        {
            return true;
        }
    }
    return false;
}
}
