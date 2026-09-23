#pragma once
#include <QColor>
#include <QHash>
#include <QString>
#include <QStringList>
#include <QtGlobal>

#include "Core/RadarConstants.h"

/// <summary>
/// @brief 界面参数配置
/// </summary>
struct UIConfig
{
    float startScan0 = 0.0f;
    float startScan1 = 0.0f;
    float startScan2 = 0.0f;
    float startScan3 = 0.0f;
    float endScan0 = 360.0f;
    float endScan1 = 0.0f;
    float endScan2 = 0.0f;
    float endScan3 = 0.0f;
    float northCorrection = 0.0f;
    float pitchCorrection = 0.0f;
    float distanceCorrection = 0.0f;
    double radarLongitude = 0.0;
    double radarLatitude = 0.0;
    float radarHeight = 0.0f;
    float speedMin = 0.5f;
    float speedMax = 50.0f;
    float heightMin = 0.0f;
    float heightMax = 1000.0f;
    float distanceMin = 0.0f;
    float distanceMax = 100000.0f;
};

/// <summary>
/// @brief UDP地址配置
/// </summary>
struct UdpSettings
{
    QString localIp;
    quint16 localPort = 0;
    QString remoteIp;
    quint16 remotePort = 0;
    bool sourceFilterEnabled = false;
    bool multicastEnabled = false;
    QString multicastGroup;
    quint16 multicastPort = 0;
};

/// <summary>
/// @brief 航迹显示配置
/// </summary>
struct DisplayConfig
{
    QHash<quint8, QColor> categoryColors;
    QHash<quint8, int> categoryAlphas;
    QHash<quint8, bool> categoryVisibility;
};

/// <summary>
/// @brief 目标列表列配置
/// </summary>
struct TableConfig
{
    QStringList hiddenColumns;
    QHash<QString, int> columnWidths;
};

/// <summary>
/// @brief 参数回显与落盘的统一小数位数
/// </summary>
namespace ConfigDecimals
{
constexpr int Angle = 2;
constexpr int Degree = 6;
constexpr int Meter = 2;
constexpr int Speed = 2;
}

/// <summary>
/// @brief 雷达配置类
/// </summary>
class RadarConfig
{
  public:
    static QString FilePath();
    static UIConfig LoadUi();
    static UdpSettings LoadUdp();
    static void SaveUdp(const UdpSettings& udp);
    static void SaveUi(const UIConfig& cfg);
    static DisplayConfig LoadDisplay();
    static void SaveDisplay(const DisplayConfig& cfg);
    static TableConfig LoadTable();
    static void SaveTable(const TableConfig& cfg);
};
