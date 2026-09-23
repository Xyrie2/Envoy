#pragma once
#include <QMainWindow>

#include "Core/RadarConfig.h"
#include "Core/RadarProtocol.h"
#include "Core/RadarTrackRules.h"
#include "Core/RadarTrackStore.h"

class UdpClass;
class TargetListPanel;
class ControlPanel;
class MapView;
class AppStatusBar;
class TrackTableController;
class NetworkSettingsPanel;
class DisplaySettingsPanel;
class QPushButton;
class QTimer;
class QPlainTextEdit;
class QTabWidget;
class QSplitter;
class QToolButton;

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;
private slots:
    void on_modeListButton_clicked();
    void on_modeFocusButton_clicked();
    void on_applyNetworkPushButton_clicked();
    void TrackDisplayEdited();
    void on_powerOnPushButton_clicked();
    void on_powerOffPushButton_clicked();
    void on_spParamQueryPushButton_clicked();
    void on_dpParamQueryPushButton_clicked();
    void ProcessDatagram(const QByteArray& data);
    void ForeignDatagram(const QString& channelName, const QString& senderAddress, quint16 senderPort);
    void OnUdpSendFailed(const QString& address, quint16 port, const QString& error);
private:
    UIConfig m_config;
    QString m_localIp;
    QString m_remoteIp;
    quint16 m_localPort = 0;
    quint16 m_remotePort = 0;
    qint64 m_lastRxMs = 0;
    bool m_everRx = false;
    UdpClass* m_radarUDP = nullptr;
    DisplayConfig m_displayConfig;

    void HandleTrackFrame(const QByteArray& data);        /// 航迹(0xCC5555CC)
    void HandleState1Frame(const QByteArray& data);       /// 状态1(0x55DDDD55)
    void HandleState2Frame(const QByteArray& data);       /// 状态2(0x19576492)
    void HandleState3Frame(const QByteArray& data);       /// 状态3(0xCC1111CC)
    void HandlePtzFrame(const QByteArray& data);          /// 过界(0x55CCCC55)
    void HandlePointFrame(const QByteArray& data);        /// 点迹(0xAA5555AA)
    void HandleSignalConfigFrame(const QByteArray& data); /// 信处参数(0x55AAAA55)
    void HandleDataConfigFrame(const QByteArray& data);   /// 数处配置(0x55222255)

    void LoadConfig();
    void SetMapSectors();
    void WriteConfigToUi();
    bool ReadConfigFromUi();
    bool ConfirmDangerousAction(const QString& title, const QString& text);
    void DisableSenderBriefly(QPushButton* btn);
    void ReportBindResult(bool radarOk);
    void LogReceivePlan(bool bindOk);
    void OnChannelEdited(const QString& channelName, const QString& detail);
    void OnPendingApplyChanged(bool pending);
    void OnRadarModeApplyRequested(int modeIndex);
    void OnParameterGroupApplyRequested(int group);
    QString ReceiveSummary() const;
    void SendParameterQuery(quint8 kind, QPushButton* trigger);
    void ApplyMulticastReceiver();
    void UpdatePtzBeams();
    void WriteUdpToUi();
    void WriteDisplayToUi();
    void ApplyDisplayConfig(const DisplayConfig& cfg);
    void ToggleRightPanel();
    void ToggleTrackDisplay();
    void UpdateTrackToggleUi();
    void BuildCentralLayout();
    void DeferredSaveConfig();
    void LogMessage(const QString& text);

    static QString TimeStamp();
    static QString HexDump(const QByteArray& data);
    static QString HexDumpPreview(const QByteArray& data, int maxBytes = 64);

    MapView* m_mapView = nullptr;
    QPlainTextEdit* m_logEdit = nullptr;
    QWidget* m_rightPanel = nullptr;
    QTimer* m_statusTimer = nullptr;
    QTabWidget* m_rightTabs = nullptr;
    AppStatusBar* m_statusBar = nullptr;
    QSplitter* m_rightSplitter = nullptr;
    TargetListPanel* m_listPanel = nullptr;
    ControlPanel* m_controlPanel = nullptr;
    NetworkSettingsPanel* m_networkPanel = nullptr;
    DisplaySettingsPanel* m_displayPanel = nullptr;
    QSplitter* m_contentSplitter = nullptr;
    QToolButton* m_panelToggleBtn = nullptr;
    QToolButton* m_trackToggleBtn = nullptr;
    bool m_pointVisible = true;
    bool m_panelCollapsed = false;
    RadarTrackStore* m_trackStore = nullptr;
    RadarTrackRuleEngine m_ruleEngine;
    TrackTableController* m_tableController = nullptr;

    RadarPTZ m_lastPtz;
    bool m_hasPtzFrame = false;
    quint64 m_ptzFrameCount = 0;
    qint64 m_lastPtzFrameMs = 0;
    quint16 m_ptzLoggedNorth = 0;
    quint16 m_ptzLoggedMechanical = 0;
    qint64 m_lastPtzLogMs = 0;

    /// 未识别帧警告节流状态
    quint64 m_unknownFrameCount = 0; /// 未识别帧累计计数
    qint64 m_unknownLastWarnMs = 0;  /// 未识别帧警告上次输出时刻

    /// 来源过滤状态
    bool m_sourceFilterEnabled = true;  /// 来源过滤开关
    quint64 m_foreignPacketCount = 0;   /// 非法来源包累计计数
    qint64 m_foreignLastWarnMs = 0;     /// 非法来源包告警上次输出时刻

    /// 组播接收设置
    bool m_multicastEnabled = false;
    QString m_multicastGroup;
    quint16 m_multicastPort = 0;
    UdpClass* m_multicastUDP = nullptr;
};
