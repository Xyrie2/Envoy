#pragma once
#include <QByteArray>
#include <QtGlobal>
#include <vector>

#include "Core/RadarConstants.h"

/// <summary>
/// @brief  计算数据校验和
/// @param  data 待计算的数据
/// @return 累加和校验值
/// </summary>
quint32 calculateChecksum(const QByteArray& data);

/// <summary>
/// @brief  验证包尾累加和
/// @param  ba 完整数据包
/// @param  payloadSize 参与累加的字节数
/// @param  expected 包内读出的校验字段值
/// @return true = 校验通过；ba 长度不足时返回 false，超出 payloadSize 的尾部字节不参与累加
/// </summary>
bool verifyChecksum(const QByteArray& ba, int payloadSize, quint32 expected);

#pragma pack(push, 1)
/// <summary>
/// @brief 航迹协议结构体
/// </summary>
struct RadarTrack
{
    quint32 frameHeader;        /// 帧头标识
    quint16 packetLength;       /// 整个数据包字节长度
    quint16 radarId;            /// 雷达设备唯一标识
    quint64 timestampUtc;       /// UTC时间戳（毫秒精度）
    quint16 snrValue;           /// 信噪比（Signal-to-Noise Ratio）
    quint16 trackStatus;        /// 航迹状态：0=起批未确认, 1=确认目标, 2=已丢弃
    quint16 targetId;           /// 目标编号（1-999循环）
    quint16 trackPointCount;    /// 关联点迹数量
    quint16 headingAngle;       /// 目标运动航向角（度）
    qint16 elevationAngle;      /// 目标仰角（与水平面夹角）
    quint32 slantRange;         /// 目标斜距（米）
    qint16 trueVelocity;        /// 目标真实速度（0.1米/秒）
    qint16 radialVelocity;      /// 目标径向速度（0.1米/秒，远离雷达为正）
    qint16 azimuthAngle;        /// 目标偏北角（0.01度，-18000~18000）
    double latitude;            /// 目标纬度（度）
    double longitude;           /// 目标经度（度）
    float altitude;             /// 目标海拔高度（米）
    quint8 trackSendMode;       /// 航迹发送模式：0=全部, 1=单条选中, 2=多条选中
    quint8 selectionState;      /// 目标选中状态：0=未选中, 1=已选中
    quint8 targetCategory;      /// 目标类型：0其它, 1车辆, 2四旋翼, 3固定翼, 4鸟, 5船, 6行人, 7客机, 255待识别
    quint8 confidenceLevel;     /// 分类置信度（0-100）
    quint16 amplitude;          /// 目标回波幅度
    quint16 rcsValue;           /// 雷达散射截面（RCS）
    quint16 relativeAzimuth;    /// 目标相对方位角（含频率偏角）
    quint8 threatLevel;         /// 威胁等级
    quint32 beamIdentifier;     /// 波位号
    quint32 frameCounter;       /// 帧计数
    quint8 selectionSource;     /// 选择来源：0=自动, 1=手动
    quint16 noiseAmplitude;     /// 噪声幅度
    quint64 firstDetectionTime; /// 首次发现时间戳
    quint8 trackingMode;        /// 跟踪标志（预留字段）0x10未跟踪, 0x11单/多目标跟踪
    char reserved1[35];         /// 预留扩展空间
    quint32 checksum;           /// 数据校验和（累加和）  

    bool fromByteArray(const QByteArray& ba);
};


/// <summary>
/// @brief 状态协议结构体
/// </summary>
struct RadarState
{
    quint32 frameHeader;               /// 帧头
    quint16 packetLength;              /// 报文总字节数
    quint64 timestamp;                 /// 时间戳

    quint16 receiveAttenuation : 6;    /// 接收衰减（6bit）
    quint16 excitationSwitch : 1;      /// 激励开关（1bit）
    quint16 radarWaveform : 3;         /// 雷达波形（3bit）
    quint16 pulseIntegrationCount : 3; /// 积累脉冲数（3bit）
    quint16 operationMode : 3;         /// 工作模式（3bit）

    quint16 detectionThreshold;        /// 检测门限
    quint16 thresholdFactor;           /// 门限因子

    quint32 reserved1 : 27;            /// 保留字段（27bit）
    quint32 stcSwitch : 1;             /// STC开关（1bit）
    quint32 rotationSelection : 1;     /// 轮飞选择（1bit）
    quint32 trackingSwitch : 1;        /// 跟踪开关（1bit）
    quint32 trackingSelection : 1;     /// 跟踪选择（1bit）
    quint32 radarConnectionStatus : 1; /// 雷达连接状态（1bit）

    float cpiTime;                     /// CPI时间
    quint32 cycleDuration;             /// 周期持续时间
    quint32 lostPacketCount;           /// 丢包数
    float turntablePitchData;          /// 转台俯仰数据
    float modulationPitchData;         /// 调制俯仰数据
    quint16 trackingTargetId;          /// 跟踪目标编号
    qint16 angleCorrection;            /// 角度修正
    quint16 turntableAzimuthAngle;     /// 转台方位角
    qint16 turntablePitchAngle;        /// 转台俯仰角
    quint16 compassAzimuth;            /// 罗盘方位角
    qint16 compassPitch;               /// 罗盘俯仰
    qint16 compassRoll;                /// 罗盘横倾
    double radarLongitude;             /// 雷达经度
    double radarLatitude;              /// 雷达纬度
    float radarHeight;                 /// 雷达高度
    quint16 radarType;                 /// 雷达类型
    quint32 radarId;                   /// 雷达ID
    quint8 clutterMap;                 /// 杂波图门限
    quint8 satelliteCount;             /// 卫星个数
    quint8 reserved2;                  /// 预留
    quint16 velocityMax;               /// 速度上限
    quint16 velocityMin;               /// 速度下限
    quint16 heightMin;                 /// 高度下限
    quint16 heightMax;                 /// 高度上限
    quint32 distanceMin;               /// 距离下限
    quint32 distanceMax;               /// 距离上限
    quint16 startAngle;                /// 起始角度
    quint16 endAngle;                  /// 终止角度
    quint8 trackingMode;               /// 跟踪模式
    quint8 searchMode;                 /// 搜索模式
    quint8 ptzAzimuthLockStatus;       /// 方位自锁状态
    quint16 arrayCurrent;              /// 阵面电流
    quint8 arrayCurrentStatus;         /// 电流状态
    quint8 digitalConnectionStatus;    /// 数处连接状态
    quint8 insPositioningStatus;       /// 差分定向状态
    quint8 reserved3;                  /// 预留
    quint32 checksum;                  /// 校验码

    bool fromByteArray(const QByteArray& ba);
};

/// <summary>
/// @brief 状态协议结构体
/// </summary>
struct RadarState2
{
    qint32 frameHeader;          /// 帧头
    qint32 packetLength;         /// 字节数
    double terminalTimeMs;       /// 显控utc毫秒
    quint32 emitSwitch;          /// 发射开关
    float radarVoltage;          /// 阵面电压
    float radarCurrent;          /// 阵面电流
    quint64 fpgaSerialNumber;    /// FPGA序列号
    quint8 radarId[80];          /// 雷达编号

    quint16 spNetState;          /// 信处连接状态
    quint16 dpNetState;          /// 数处连接状态
    quint8 reserved0[24];        /// 预留

    double rtkLongitude;         /// 差分经度
    double rtkLatitude;          /// 差分纬度
    double rtkHeight;            /// 差分高度
    quint16 rtkYear;             /// 差分utc时间 年
    quint8 rtkMonth;             /// 差分utc时间 月
    quint8 rtkDay;               /// 差分utc时间 日
    quint8 rtkHour;              /// 差分utc时间 时
    quint8 rtkMinute;            /// 差分utc时间 分
    quint8 rtkSecond;            /// 差分utc时间 秒
    quint8 reserved;             /// 预留
    qint32 rtkMillisecond;       /// 毫差分utc时间毫秒
    float rtkDirection;          /// 差分指向
    quint8 rtkPositionState;     /// 差分 定位状态
    quint8 rtkOrientationState;  /// 差分 定向状态
    quint16 rtkSatelliteCount;   /// 差分 卫星个数
    quint16 rtkReserved;         /// 差分 卫星个数
    quint16 reserved2;           /// 差分 卫星个数
    qint32 reserved3;            /// 差分 卫星个数
    qint32 reserved4;            /// 差分 卫星个数

    float ptzAzimuth;            /// 差分 卫星个数
    float ptzElevation;          /// 差分 卫星个数
    quint16 ptzAzimuthLockState; /// 方位自锁状态
    quint16 ptzPitchLockState;   /// 俯仰自锁状态
    qint32 reserved5;            /// 俯仰自锁状态
    float compassAzimuth;        /// 俯仰自锁状态
    float compassElevation;      /// 俯仰自锁状态
    float compassRoll;           /// 俯仰自锁状态
    qint32 reserved6;            /// 俯仰自锁状态

    double insLongitude;         /// 差分经度
    double insLatitude;          /// 差分纬度
    double insHeight;            /// 差分高度
    float insVx;                 /// 差分速度x
    float insVy;                 /// 差分速度y
    float insVz;                 /// 差分速度z
    float insYaw;                /// 差分方位角
    float insPitch;              /// 差分俯仰角
    float insRoll;               /// 差分横倾角
    float insDirection;          /// 差分指向
    double insUtcMs;             /// 差分utc毫秒时间
    quint8 reserved7[20];        /// 预留

    quint32 terminalVersion;     /// uint 显控版本
    double authorizedDays;       /// 剩余使用天数
    quint8 recordState;          /// 记录状态：0=不记录, 1=记录
    quint8 reserved8;            /// 预留
    quint8 reserved9;            /// 预留
    quint8 reserved10;           /// 预留
    float northCorrection;       /// 偏北修正
    float elevationCorrection;   /// 俯仰修正
    float rangeCorrection;       /// 距离修正
    float antennaElevation;      /// 天线仰角
    float startAngle;            /// 起始角度
    float endAngle;              /// 终止角度
    quint8 spTemperature;        /// 信处温度
    quint8 reserved11[63];       /// 预留
    quint32 checksum;            /// 整个数据包字节的累加和

    static constexpr int PacketSize = 420;
    bool fromByteArray(const QByteArray& ba);
    bool isFrameValid(quint32 syncWord) const;
};

/// <summary>
/// @brief 状态协议结构体
/// </summary>
struct RadarState3
{
    quint32 frameHeader;              /// 帧头
    quint32 packetLength;             /// 报文总字节数
    quint32 radarId;                  /// 雷达id
    quint64 timestamp;                /// 时间戳

    quint8 radarConnectionStatus;     /// 雷达连接状态
    quint8 signalProcessingFault;     /// 信处故障
    quint8 dataProcessingFault;       /// 数处故障
    quint8 beamControlFault;          /// 波控故障
    quint8 frequencySynthesizerFault; /// 频综故障
    quint8 powerSupplyFault;          /// 电源故障
    quint8 turntableFault;            /// 转台故障
    quint8 environmentOverheatFault;  /// 环境过热故障
    quint8 trModuleFault;             /// TR故障
    quint8 faultReserved[32];         /// 故障预留

    double gnssLongitude;             /// GNSS 经度
    double gnssLatitude;              /// GNSS 纬度
    float gnssAltitude;               /// GNSS 高度
    quint8 gnssPositionStatus;        /// GNSS定位状态
    quint8 gnssHeadingStatus;         /// GNSS定向状态
    float gnssHeadingAngle;           /// GNSS定向角度
    quint8 gnssSatelliteCount;        /// GNSS卫星个数
    quint8 exciterSwitch;             /// 激励开关

    quint16 turntableAzimuthAngle;    /// 转台方位角
    quint16 compassAzimuth;           /// 罗盘方位角
    qint16 compassPitch;              /// 罗盘俯仰
    qint16 compassRoll;               /// 罗盘横倾
    double radarLongitude;            /// 雷达经度
    double radarLatitude;             /// 雷达纬度
    float radarAltitude;              /// 雷达高度
    qint16 angleCorrection;           /// 角度修正
    quint8 azimuthLockState;          /// 方位自锁状态

    quint8 reserved[388];             /// 预留
    quint32 checksum;                 /// 校验码

    static constexpr int PacketSize = 512;
    bool fromByteArray(const QByteArray& ba);
    bool isFrameValid(quint32 syncWord) const;
};


/// <summary>
/// @brief 点迹输出结构体
/// </summary>
struct PointOut
{
    quint16 azimuthAngle;   /// 目标方位角
    qint16 elevationAngle;  /// 目标仰角
    qint32 distance;        /// 目标距离
    qint32 velocity;        /// 目标速度
    qint32 amplitude;       /// 目标幅度
};


/// <summary>
/// @brief 点迹协议结构体
/// </summary>
struct RadarPoint
{
    static constexpr int PointOutCapacity = 30;

    quint32 frameHeader;                 /// 帧头
    quint16 packetLength;                /// 报文字节数
    quint16 radarIdentifier;             /// 雷达ID
    quint64 timestamp;                   /// 时间戳
    quint16 reservedField0;              /// 预留
    quint16 targetCount;                 /// 目标个数
    PointOut pointOut[PointOutCapacity]; /// 目标点信息
    char reservedField1[8];              /// 预留
    qint32 crc;                          /// 校验码

    bool fromByteArray(const QByteArray& ba);
};

/// <summary>
/// @brief 过界协议结构体
/// </summary>
struct RadarPTZ
{
    quint32 frameHeader;        /// 帧头
    quint16 packetLength;       /// 报文字节数
    quint16 radarIdentifier;    /// 雷达ID
    quint64 timestamp;          /// 时间戳
    quint16 mechanicalAzimuth;  /// 波束1机械方位角（0.01度，单面阵/转台设备有效）
    quint16 trueNorthAzimuth;   /// 波束1偏北方位角（0.01度，单面阵/转台设备有效；真北坐标系用此字段）
    quint16 mechanicalAzimuth1; /// 波束2机械方位角（仅多面阵）
    quint16 trueNorthAzimuth1;  /// 波束2偏北方位角（仅多面阵）
    quint16 mechanicalAzimuth2; /// 波束3机械方位角（仅多面阵）
    quint16 trueNorthAzimuth2;  /// 波束3偏北方位角（仅多面阵）
    quint16 mechanicalAzimuth3; /// 波束4机械方位角（仅多面阵）
    quint16 trueNorthAzimuth3;  /// 波束4偏北方位角（仅多面阵）
    char reservedField[28];     /// 预留
    quint32 crc;                /// 校验码

    bool fromByteArray(const QByteArray& ba);
};

/// <summary>
/// @brief 通用控制帧头结构体
/// </summary>
struct RadarHeader
{
    quint32 frameHeader = FrameHeader::Control;
    quint32 length = 0;
    char reserved0[56] = {};
    quint32 blockCount = 0;
};


/// <summary>
/// @brief 控制块结构体
/// </summary>
struct ControlBlock
{
    qint32 functionId = 0; /// 功能号
    qint32 reserved0 = 0;  /// 预留
    qint32 reserved1 = 0;  /// 预留
    qint32 data0 = 0;      /// 控制参数0
    qint32 data1 = 0;      /// 控制参数1
    qint32 data2 = 0;      /// 控制参数2
    qint32 data3 = 0;      /// 控制参数3
    double data4 = 0.0;    /// 控制参数4
    double data5 = 0.0;    /// 控制参数5
    double data6 = 0.0;    /// 控制参数6
    double data7 = 0.0;    /// 控制参数7
};

/// <summary>
/// @brief 控制协议结构体
/// </summary>
struct RadarControl
{
    RadarHeader header;               /// 通用帧
    std::vector<ControlBlock> blocks; /// 数据块
    quint32 checksumValue;            /// 校验和

    RadarControl(quint32 count = 0)
    {
        header.frameHeader = FrameHeader::Control;
        header.blockCount = count;
        blocks.resize(count);
        header.length = sizeof(RadarHeader) + static_cast<quint32>(count * sizeof(ControlBlock));
    }

    QByteArray serialize()
    {
        QByteArray data;
        data.append(reinterpret_cast<const char*>(&header), sizeof(header));
        data.append(reinterpret_cast<const char*>(blocks.data()),
                    static_cast<int>(blocks.size() * sizeof(ControlBlock)));
        checksumValue = calculateChecksum(data);
        data.append(reinterpret_cast<const char*>(&checksumValue), sizeof(checksumValue));
        return data;
    }
};

/// <summary>
/// @brief 信处配置结构体
/// </summary>
struct SignalProcessingConfig
{
    quint32 frameHeader;                /// 帧头 0x55AAAA55
    quint32 packetLength;               /// 数据包总字节数
    float emissionStartAngle0;          /// 发射起始扇区角度（度）
    float emissionEndAngle0;            /// 发射终止扇区角度（度）
    qint32 emissionSwitch;              /// 激励发射开关（0:关闭, 1:开启）
    qint32 scanMode;                    /// 搜索模式
    qint32 frequencyPoint;              /// 工作频点（0-150）
    qint32 snrThreshold;                /// 信噪比检测门限（dB）
    qint32 amplitudeThreshold;          /// 幅度检测门限（dB）
    qint32 clutterSuppressionBandwidth; /// 杂波抑制带宽（kHz）
    qint32 turntableAxis;               /// 转台旋转轴（0:水平, 1:垂直）
    qint32 turntableMode;               /// 转台运动模式
    float turntablePositionAngle;       /// 转台定位角度（度）
    float turntableSectorCenter;        /// 转台扇扫中心角度（度）
    float turntableSectorSize;          /// 转台扇扫单侧角度范围（度）
    float turntableSpeed;               /// 转台旋转速度（度/秒）
    qint32 stcEnable;                   /// STC使能（0:禁用, 1:启用）
    qint32 stcSelection;                /// STC曲线选择（0-3）
    qint32 frameAccumulationEnable;     /// 时序模式
    qint32 reservedFields[26];          /// 保留字段
    qint32 checksum;                    /// 校验和

    bool fromByteArray(const QByteArray& ba);
};

/// <summary>
/// @brief 数处配置结构体
/// </summary>
struct DataProcessingConfig
{
    quint32 frameHeader;              /// 帧头（固定值） 
    quint16 packetLength;             /// 数据包总字节数
    quint8 radarType;                 /// 雷达类型
    quint8 reservedField0;            /// 保留字段0
    quint32 radarIdentifier;          /// 雷达唯一标识符
    quint32 reservedField1;           /// 保留字段1
    quint32 dataProcessingVersion;    /// 数据处理软件版本号
    quint32 terminalControlVersion;   /// 终端控制软件版本号
    quint16 beamTrackingMode;         /// 波束跟踪模式
    quint16 waveGateSelection;        /// 波门选择方式
    quint16 sectorTrackingMode;       /// 扇区跟踪模式
    quint16 designatedTrackNumber;    /// 指定跟踪航迹号
    quint16 scanTrackingSelection;    /// 扫描/跟踪选择
    quint16 trackInitiationMethod;    /// 航迹起始方法
    quint32 clutterMapEnable;         /// 杂波图开关
    quint16 trackInitiationWindowN;   /// 航迹起始滑窗分母（N值）
    quint16 trackInitiationWindowM;   /// 航迹起始滑窗分子（M值）
    quint16 filteringModel;           /// 滤波模型
    quint16 associationRule;          /// 关联准则
    qint32 trackTerminationCycle;     /// 航迹消亡周期（帧）
    float maxProcessingRange;         /// 处理距离上限（米）
    float minProcessingRange;         /// 处理距离下限（米）
    float maxProcessingVelocity;      /// 处理速度上限（米/秒）
    float minProcessingVelocity;      /// 处理速度下限（米/秒）
    float maxProcessingAltitude;      /// 处理高度上限（米）
    float minProcessingAltitude;      /// 处理高度下限（米）
    qint16 elevationCorrection;       /// 俯仰角修正值（0.01度）
    qint16 rangeCorrection;           /// 距离修正值（米）
    qint16 manualElevationInput;      /// 手动输入俯仰角（0.01度）
    qint16 northCorrection;           /// 偏北角修正值（0.01度）
    qint32 elevationSourceSelection;  /// 俯仰角来源选择
    double radarLongitude;            /// 雷达经度（度）
    double radarLatitude;             /// 雷达纬度（度）
    float radarAltitude;              /// 雷达高度（米）
    qint32 interpolationEnable;       /// 数据插值开关（0:禁用, 1:启用）
    qint32 sidelobeSuppression;       /// 副瓣抑制（0:禁用, 1:启用）
    float clutterMapThreshold;        /// 杂波图检测门限（dB）
    qint32 dataOutputSwitch;          /// 数据输出开关（0:关闭, 1:开启）
    qint32 compassParamSetting;       /// 罗盘参数设置
    qint32 displayInterfaceSwitch;    /// 数据显示界面开关
    qint32 reservedField2;            /// 保留字段2
    qint32 checksum;                  /// 校验和

    bool fromByteArray(const QByteArray& ba);
};

#pragma pack(pop)
