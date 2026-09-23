#include "App/MainWindow.h"

#include "Core/ConfigBackfill.h"
#include "Core/RadarTrackText.h"
#include "Net/UdpClass.h"
#include "Ui/AppMessageDialog.h"
#include "Ui/AppStatusBar.h"
#include "Ui/ControlPanel.h"
#include "Ui/DisplaySettingsPanel.h"
#include "Ui/MapView.h"
#include "Ui/NetworkSettingsPanel.h"
#include "Ui/TargetListPanel.h"
#include "Ui/Theme.h"
#include "Ui/TrackTableController.h"

#include <QDateTime>
#include <QDebug>
#include <QFrame>
#include <QHostAddress>
#include <QLineEdit>
#include <QMetaObject>
#include <QPainter>
#include <QPainterPath>
#include <QPushButton>
#include <QSettings>
#include <QSplitter>
#include <QStringList>
#include <QTabWidget>
#include <QPlainTextEdit>
#include <QTimer>
#include <QToolButton>
#include <QVBoxLayout>
#include <cmath>
#include <cstring>

namespace
{

constexpr double kSectorMin = ConfigBackfill::SectorMin, kSectorMax = ConfigBackfill::SectorMax;                      /// 扇区角度（度）
constexpr double kNorthCorrMin = ConfigBackfill::NorthCorrMin, kNorthCorrMax = ConfigBackfill::NorthCorrMax;          /// 偏北修正（度）
constexpr double kPitchCorrMin = ConfigBackfill::PitchCorrMin, kPitchCorrMax = ConfigBackfill::PitchCorrMax;          /// 俯仰修正（度）
constexpr double kRangeCorrMin = ConfigBackfill::RangeCorrMin, kRangeCorrMax = ConfigBackfill::RangeCorrMax;          /// 斜距修正（米）
constexpr double kLonMin = ConfigBackfill::LonMin, kLonMax = ConfigBackfill::LonMax;                                  /// 经度
constexpr double kLatMin = ConfigBackfill::LatMin, kLatMax = ConfigBackfill::LatMax;                                  /// 纬度
constexpr double kHeightLimitMin = ConfigBackfill::HeightLimitMin, kHeightLimitMax = ConfigBackfill::HeightLimitMax;  /// 起批高度限制（米）
constexpr double kSpeedLimitMin = ConfigBackfill::SpeedLimitMin, kSpeedLimitMax = ConfigBackfill::SpeedLimitMax;      /// 起批速度限制（m/s）
constexpr double kDistLimitMin = ConfigBackfill::DistLimitMin, kDistLimitMax = ConfigBackfill::DistLimitMax;          /// 起批距离限制（米）
const QLatin1String kInvalidFieldStyle("QLineEdit { border:1px solid #ff6b6b; }");

template <typename T>
T ReadNum(QLineEdit* edit, const QString& title, double minV, double maxV, QStringList& errors)
{
    const QString text = edit->text().trimmed();
    bool ok = false;
    const double v = text.toDouble(&ok);
    if (text.isEmpty())
    {
        errors << QObject::tr("%1：不能为空").arg(title);
        edit->setStyleSheet(kInvalidFieldStyle);
        return T(0);
    }
    if (!ok)
    {
        errors << QObject::tr("%1：\"%2\" 不是有效数字").arg(title, text);
        edit->setStyleSheet(kInvalidFieldStyle);
        return T(0);
    }
    if (v < minV || v > maxV)
    {
        errors << QObject::tr("%1：%2 超出范围 [%3, %4]").arg(title).arg(v).arg(minV).arg(maxV);
        edit->setStyleSheet(kInvalidFieldStyle);
        return T(0);
    }
    edit->setStyleSheet(QString());
    return static_cast<T>(v);
}

void CheckOrderedPair(QLineEdit* minEdit, QLineEdit* maxEdit, const QString& title, double minV, double maxV, QStringList& errors)
{
    if (minV <= maxV)
    {
        return;
    }
    errors << QObject::tr("%1：下限(%2) 大于 上限(%3)").arg(title).arg(minV).arg(maxV);
    minEdit->setStyleSheet(kInvalidFieldStyle);
    maxEdit->setStyleSheet(kInvalidFieldStyle);
}

QPixmap PaintEyePixmap(bool open)
{
    const QColor color = Theme::Pal::textMuted();
    constexpr int kLogical = 16;
    constexpr qreal kDpr = 2.0;
    const int px = qRound(kLogical * kDpr);

    QPixmap pm(px, px);
    pm.fill(Qt::transparent);

    QPainter painter(&pm);
    painter.setRenderHint(QPainter::Antialiasing);

    const qreal w = px;
    const qreal cx = w / 2.0;
    const qreal cy = w / 2.0;
    const qreal halfW = w * 0.36;
    const qreal arch = w * 0.24;

    QPen pen(color, w * 0.07);
    pen.setCapStyle(Qt::RoundCap);
    painter.setPen(pen);

    if (open)
    {
        QPainterPath outline;
        outline.moveTo(cx - halfW, cy);
        outline.quadTo(cx, cy - arch * 1.5, cx + halfW, cy);
        outline.quadTo(cx, cy + arch * 1.5, cx - halfW, cy);
        painter.drawPath(outline);

        painter.setPen(Qt::NoPen);
        painter.setBrush(color);
        painter.drawEllipse(QPointF(cx, cy), w * 0.10, w * 0.10);
    }
    else
    {
        const QPointF p0(cx - halfW, cy - arch * 0.4);
        const QPointF p2(cx + halfW, cy - arch * 0.4);
        const QPointF ctrl(cx, cy + arch * 1.35);

        QPainterPath lid;
        lid.moveTo(p0);
        lid.quadTo(ctrl, p2);
        painter.drawPath(lid);

        const qreal lashLen = w * 0.16;
        const auto bezPoint = [p0, ctrl, p2](qreal t)
        {
            const qreal u = 1.0 - t;
            return QPointF(u * u * p0.x() + 2.0 * u * t * ctrl.x() + t * t * p2.x(), u * u * p0.y() + 2.0 * u * t * ctrl.y() + t * t * p2.y());
        };
        for (qreal t : {0.2, 0.5, 0.8})
        {
            const QPointF pt = bezPoint(t);
            QPointF tangent = 2.0 * (1.0 - t) * (ctrl - p0) + 2.0 * t * (p2 - ctrl);
            QPointF normal(-tangent.y(), tangent.x());
            if (normal.y() < 0.0)
            {
                normal = -normal;
            }
            const qreal n = std::sqrt(normal.x() * normal.x() + normal.y() * normal.y());
            if (n > 0.0)
            {
                normal /= n;
            }
            painter.drawLine(pt, pt + normal * lashLen);
        }
    }

    pm.setDevicePixelRatio(kDpr);
    return pm;
}

QIcon MakeEyeIcon(bool open)
{
    return QIcon(PaintEyePixmap(open));
}

}

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent), m_radarUDP(new UdpClass(this)), m_trackStore(new RadarTrackStore(TrackTableController::MaxFocusRows, TrackTableController::MaxTargets, this))
{
    setStyleSheet(Theme::appQss());
    BuildCentralLayout();
    m_tableController = new TrackTableController(m_listPanel, m_trackStore, m_rightSplitter, this);
    LoadConfig();

    connect(m_trackStore, &RadarTrackStore::targetAdded, this, [this](quint16 targetId) {
        LogMessage(tr("【新目标】目标 %1 首次收到航迹（%2）").arg(targetId).arg(TrackText::StatusText(m_trackStore->latest(targetId).trackStatus)));
    });
    connect(m_trackStore, &RadarTrackStore::targetRemoved, this, [this](quint16 targetId, bool byDrop) {
        if (byDrop) {
            LogMessage(tr("【消批】目标 %1 已消批（起批标志=2），条目已从列表移除").arg(targetId));
        }
    });

    connect(m_tableController, &TrackTableController::FocusTargetChanged, m_mapView, &MapView::setFocusTarget);
    if (!qEnvironmentVariableIsSet("RADAR_NO_UDP")) {
        const bool radarOk = m_radarUDP->bind(m_localIp, m_localPort);
        ReportBindResult(radarOk);

        if (m_sourceFilterEnabled) {
            m_radarUDP->setAllowedSource(m_remoteIp, m_remotePort);
        }
        LogReceivePlan(radarOk);
        ApplyMulticastReceiver();
    }
    connect(m_radarUDP, &UdpClass::dataReceived, this, &MainWindow::ProcessDatagram);
    connect(m_radarUDP, &UdpClass::sendFailed, this, &MainWindow::OnUdpSendFailed);
    connect(m_radarUDP, &UdpClass::foreignDatagramReceived, this,
            [this](const QString& address, quint16 port) { ForeignDatagram(tr("雷达"), address, port);
    });
    connect(m_networkPanel, &NetworkSettingsPanel::channelEdited, this, &MainWindow::OnChannelEdited);
    connect(m_networkPanel, &NetworkSettingsPanel::pendingApplyChanged, this, &MainWindow::OnPendingApplyChanged);
    connect(m_controlPanel, &ControlPanel::radarModeApplyRequested, this, &MainWindow::OnRadarModeApplyRequested);
    connect(m_controlPanel, &ControlPanel::parameterGroupApplyRequested, this, &MainWindow::OnParameterGroupApplyRequested);
    connect(m_mapView, &MapView::trackSelected, this, [this](quint16 targetId) {
        m_tableController->ShowFocusHistoryOf(targetId);
    });
    connect(m_mapView, &MapView::focusClearRequested, this, [this]() {
        m_tableController->ClearFocus();
    });
    connect(m_displayPanel, &DisplaySettingsPanel::trackDisplayEdited, this, &MainWindow::TrackDisplayEdited);

    m_statusTimer = new QTimer(this);
    connect(m_statusTimer, &QTimer::timeout, this, [this]() {
        const qint64 elapsedMs = QDateTime::currentMSecsSinceEpoch() - m_lastRxMs;
        if (m_everRx && elapsedMs <= 3000) {
            m_statusBar->setConnectionState(AppStatusBar::ConnState::Linked, 0);
        } else if (m_everRx) {
            m_statusBar->setConnectionState(AppStatusBar::ConnState::Lost, static_cast<int>(elapsedMs / 1000));
        } else {
            m_statusBar->setConnectionState(AppStatusBar::ConnState::Idle, 0);
        }
        m_statusBar->setClock(QDateTime::currentDateTime().toString(AppStatusBar::ClockFormat));
        UpdatePtzBeams();
    });
    m_statusTimer->start(1000);
}

MainWindow::~MainWindow()
{
    QSettings settings;
    if (m_contentSplitter) {
        settings.setValue(QStringLiteral("layout/contentSplitter"), m_contentSplitter->saveState());
    }
    if (m_rightSplitter) {
        settings.setValue(QStringLiteral("layout/rightSplitter"), m_rightSplitter->saveState());
    }
    settings.setValue(QStringLiteral("layout/rightPanelCollapsed"), m_panelCollapsed);
    settings.setValue(QStringLiteral("layout/pointsVisible"), m_pointVisible);
}

/// <summary>
/// @brief UDP收包分发入口
/// </summary>
void MainWindow::ProcessDatagram(const QByteArray& data)
{
    if (data.size() < 4)
    {
        qWarning() << "ProcessDatagram: 数据包过短，忽略：" << data.size();
        return;
    }

    quint32 head;
    std::memcpy(&head, data.constData(), sizeof(head));

    const bool knownHeader = head == FrameHeader::Track || head == FrameHeader::State
            || head == FrameHeader::State2 || head == FrameHeader::State3
            || head == FrameHeader::Ptz || head == FrameHeader::Point
            || head == FrameHeader::SignalConfig || head == FrameHeader::DataConfig
            || head == FrameHeader::ControlRespect;
    if (knownHeader) {
        m_lastRxMs = QDateTime::currentMSecsSinceEpoch();
        m_everRx = true;
    }

    switch (head)
    {
    case FrameHeader::Track:
        HandleTrackFrame(data);
        break;
    case FrameHeader::State:
        HandleState1Frame(data);
        break;
    case FrameHeader::State2:
        HandleState2Frame(data);
        break;
    case FrameHeader::State3:
        HandleState3Frame(data);
        break;
    case FrameHeader::Ptz:
        HandlePtzFrame(data);
        break;
    case FrameHeader::Point:
        HandlePointFrame(data);
        break;
    case FrameHeader::SignalConfig:
        HandleSignalConfigFrame(data);
        break;
    case FrameHeader::DataConfig:
        HandleDataConfigFrame(data);
        break;
    case FrameHeader::ControlRespect:
        break;
    default:
    {
        ++m_unknownFrameCount;
        const qint64 now = QDateTime::currentMSecsSinceEpoch();
        if (m_unknownLastWarnMs == 0 || now - m_unknownLastWarnMs >= 1000) {
            qWarning().noquote() << QString("ProcessDatagram: 未识别帧头 0x%1，长度 %2，忽略（累计 %3 个）")
                                    .arg(head, 8, 16, QLatin1Char('0'))
                                    .arg(data.size())
                                    .arg(m_unknownFrameCount);
            m_unknownFrameCount = 0;
            m_unknownLastWarnMs = now;
        }
        break;
    }
    }
}

void MainWindow::HandleTrackFrame(const QByteArray& data)
{
    RadarTrack track;
    if (!track.fromByteArray(data)) {
        qWarning() << "HandleTrackFrame: 航迹帧无效（长度/帧头/校验和不符），长度" << data.size();
        return;
    }

    m_ruleEngine.applyAll(track);
    m_trackStore->addTrack(track);
}

void MainWindow::HandleState1Frame(const QByteArray& data)
{
    RadarState state;
    if (!state.fromByteArray(data)) {
        return;
    }

    m_statusBar->setTelemetry(tr("位置 %1°E · %2°N · 高 %3m · 偏北修正 %4°")
                              .arg(state.radarLongitude, 0, 'f', 6)
                              .arg(state.radarLatitude, 0, 'f', 6)
                              .arg(state.radarHeight, 0, 'f', 1)
                              .arg(Azimuth::Normalize(state.angleCorrection / 100.0), 0, 'f', 2));
}

void MainWindow::HandleState2Frame(const QByteArray& data)
{
    RadarState2 state;
    if (!state.fromByteArray(data)) {
        return;
    }
}

void MainWindow::HandleState3Frame(const QByteArray& data)
{
    RadarState3 state;
    if (!state.fromByteArray(data)) {
        qWarning() << "HandleState3Frame: 状态3帧无效（长度/帧头/校验和不符），长度" << data.size();
        return;
    }
}

void MainWindow::HandlePtzFrame(const QByteArray& data)
{
    RadarPTZ ptz;
    if (!ptz.fromByteArray(data)) {
        return;
    }

    m_lastPtz = ptz;
    m_hasPtzFrame = true;
    ++m_ptzFrameCount;
    m_lastPtzFrameMs = QDateTime::currentMSecsSinceEpoch();

    const bool anglesChanged = m_ptzLoggedNorth != ptz.trueNorthAzimuth || m_ptzLoggedMechanical != ptz.mechanicalAzimuth;
    const bool logDue = m_lastPtzLogMs == 0 || (QDateTime::currentMSecsSinceEpoch() - m_lastPtzLogMs) >= 1000;
    if (m_ptzFrameCount == 1 || (anglesChanged && logDue)) {
        m_lastPtzLogMs = QDateTime::currentMSecsSinceEpoch();
        m_ptzLoggedNorth = ptz.trueNorthAzimuth;
        m_ptzLoggedMechanical = ptz.mechanicalAzimuth;
        LogMessage(tr("【过界数据】第 %1 帧：波束1 真北 %2° / 机械 %3°")
                   .arg(m_ptzFrameCount)
                   .arg(PtzDisplay::RawToDegree(ptz.trueNorthAzimuth), 0, 'f', 2)
                   .arg(PtzDisplay::RawToDegree(ptz.mechanicalAzimuth), 0, 'f', 2));
    }

    UpdatePtzBeams();
}

void MainWindow::UpdatePtzBeams()
{
    if (!m_mapView) {
        return;
    }

    const quint16 rawMechanical[4] = {m_lastPtz.mechanicalAzimuth, m_lastPtz.mechanicalAzimuth1, m_lastPtz.mechanicalAzimuth2, m_lastPtz.mechanicalAzimuth3};
    const quint16 rawTrueNorth[4] = {m_lastPtz.trueNorthAzimuth, m_lastPtz.trueNorthAzimuth1, m_lastPtz.trueNorthAzimuth2, m_lastPtz.trueNorthAzimuth3};

    QVector<MapView::BeamAngles> beams;

    if (m_hasPtzFrame)
    {
        for (int i = 0; i < PtzDisplay::FaceCount; ++i)
        {
            if (PtzDisplay::IsEmptySlot(rawMechanical[i], rawTrueNorth[i]))
            {
                continue;
            }
            MapView::BeamAngles beam;
            beam.slot = i;
            if (PtzDisplay::IsValidRawAngle(rawTrueNorth[i]))
            {
                beam.trueNorthDeg = PtzDisplay::RawToDegree(rawTrueNorth[i]);
            }
            beams.append(beam);
        }

        const qint64 ageMs = QDateTime::currentMSecsSinceEpoch() - m_lastPtzFrameMs;
        if (ageMs > PtzDisplay::PtzStaleMs)
        {
            beams.clear();
        }
    }

    m_mapView->setPtzBeams(beams);
}

void MainWindow::HandlePointFrame(const QByteArray& data)
{
    RadarPoint point;
    if (!point.fromByteArray(data))
    {
        qWarning() << "HandlePointFrame: 点迹帧无效（长度/帧头/校验和不符），长度" << data.size();
        return;
    }

    int count = static_cast<int>(point.targetCount);
    if (count > RadarPoint::PointOutCapacity)
    {
        count = RadarPoint::PointOutCapacity;
    }

    QList<PointOut> plots;
    plots.reserve(count);
    for (int i = 0; i < count; ++i)
    {
        plots.append(point.pointOut[i]);
    }

    if (m_mapView)
    {
        m_mapView->setPoints(plots);
    }
}

void MainWindow::HandleSignalConfigFrame(const QByteArray& data)
{
    SignalProcessingConfig cfg;
    if (!cfg.fromByteArray(data))
    {
        LogMessage(tr("【参数回填】信处配置回传解析失败（报文长度、帧头或校验和不符），已丢弃"));
        return;
    }

    LogMessage("收到信处配置回传！");

    QStringList skipped;
    UIConfig next = m_config;
    const int applied = ConfigBackfill::ExtractSignalConfig(cfg, next, skipped);
    if (applied > 0)
    {
        const bool sector0Changed = (m_config.startScan0 != next.startScan0) || (m_config.endScan0 != next.endScan0);
        m_config = next;
        WriteConfigToUi();
        if (sector0Changed)
        {
            SetMapSectors();
        }
        DeferredSaveConfig();
    }
    LogMessage(ConfigBackfill::SummaryText(tr("信处"), applied, skipped));
}

void MainWindow::HandleDataConfigFrame(const QByteArray& data)
{
    DataProcessingConfig cfg;
    if (!cfg.fromByteArray(data))
    {
        LogMessage(tr("【参数回填】数处配置回传解析失败（报文长度、帧头或校验和不符），已丢弃"));
        return;
    }

    LogMessage("收到数处配置回传！");

    QStringList skipped;
    UIConfig next = m_config;
    const int applied = ConfigBackfill::ExtractDataConfig(cfg, next, skipped);
    if (applied > 0)
    {
        m_config = next;
        WriteConfigToUi();
        DeferredSaveConfig();
    }
    LogMessage(ConfigBackfill::SummaryText(tr("数处"), applied, skipped));
}



/// <summary>
/// @brief 控制指令发送
/// </summary>
void MainWindow::on_powerOnPushButton_clicked()
{
    if (!ConfirmDangerousAction(tr("确认开机"), tr("即将向雷达发送【开机】指令，设备开始工作。")))
    {
        return;
    }

    RadarControl control(1);
    control.blocks[0].functionId = ControlFunctionId::PowerControl;
    control.blocks[0].data0 = 1;

    QByteArray data = control.serialize();
    m_radarUDP->sendData(data, m_remoteIp, m_remotePort);
    LogMessage(tr("控制指令：开机\r\nHEX: %1").arg(HexDumpPreview(data)));
    DisableSenderBriefly(m_controlPanel->powerOnPushButton);
}

void MainWindow::on_powerOffPushButton_clicked()
{
    if (!ConfirmDangerousAction(tr("确认关机"), tr("即将向雷达发送【关机】指令，设备停止工作。")))
    {
        return;
    }

    RadarControl control(1);
    control.blocks[0].functionId = ControlFunctionId::PowerControl;
    control.blocks[0].data0 = 0;

    QByteArray data = control.serialize();
    m_radarUDP->sendData(data, m_remoteIp, m_remotePort);
    LogMessage(tr("控制指令：关机\r\nHEX: %1").arg(HexDumpPreview(data)));
    DisableSenderBriefly(m_controlPanel->powerOffPushButton);
}

void MainWindow::on_spParamQueryPushButton_clicked()
{
    SendParameterQuery(1, m_controlPanel->spParamQueryPushButton);
}

void MainWindow::on_dpParamQueryPushButton_clicked()
{
    SendParameterQuery(2, m_controlPanel->dpParamQueryPushButton);
}

void MainWindow::SendParameterQuery(quint8 kind, QPushButton* trigger)
{
    RadarControl control(1);
    control.blocks[0].functionId = ControlFunctionId::QueryParameters;
    control.blocks[0].data0 = static_cast<qint32>(kind);

    const QByteArray data = control.serialize();
    m_radarUDP->sendData(data, m_remoteIp, m_remotePort);
    const QString kindText = kind == 1 ? tr("信处") : tr("数处");
    LogMessage(tr("控制指令：查询%1参数\r\nHEX: %2").arg(kindText, HexDumpPreview(data)));
    DisableSenderBriefly(trigger);
}

namespace
{
/// 单个参数块的构造数据
struct ParamBlockSpec
{
    qint32 functionId;
    double data4;
    double data5;
    double data6;
};

/// 按分组构造参数块：只包含该分组对应的功能号
QVector<ParamBlockSpec> MakeGroupSpecs(int group, const UIConfig& cfg)
{
    QVector<ParamBlockSpec> specs;
    switch (static_cast<ControlPanel::ParamGroup>(group))
    {
    case ControlPanel::ParamGroup::Location:
        specs = {{ControlFunctionId::NorthCorrection, cfg.northCorrection, 0.0, 0.0},
                 {ControlFunctionId::ElevationCorrection, cfg.pitchCorrection, 0.0, 0.0},
                 {ControlFunctionId::RangeCorrection, cfg.distanceCorrection, 0.0, 0.0},
                 {ControlFunctionId::RadarPosition, cfg.radarLongitude, cfg.radarLatitude, cfg.radarHeight}};
        break;
    case ControlPanel::ParamGroup::ScanArea:
        specs = {{ControlFunctionId::Sector0, cfg.startScan0, cfg.endScan0, 0.0},
                 {ControlFunctionId::Sector1, cfg.startScan1, cfg.endScan1, 0.0},
                 {ControlFunctionId::Sector2, cfg.startScan2, cfg.endScan2, 0.0},
                 {ControlFunctionId::Sector3, cfg.startScan3, cfg.endScan3, 0.0}};
        break;
    case ControlPanel::ParamGroup::TrackLimit:
        specs = {{ControlFunctionId::SpeedRange, cfg.speedMin, cfg.speedMax, 0.0},
                 {ControlFunctionId::HeightRange, cfg.heightMin, cfg.heightMax, 0.0},
                 {ControlFunctionId::DistanceRange, cfg.distanceMin, cfg.distanceMax, 0.0}};
        break;
    }
    return specs;
}
}

/// <summary>
/// @brief 分组级下发：只把该分组的参数块发给雷达，其余分组不受影响
/// </summary>
void MainWindow::OnParameterGroupApplyRequested(int group)
{
    if (!ReadConfigFromUi())
    {
        return;
    }

    const QVector<ParamBlockSpec> specs = MakeGroupSpecs(group, m_config);
    if (specs.isEmpty())
    {
        return;
    }

    QString groupName = tr("参数");
    QPushButton* groupButton = nullptr;
    switch (static_cast<ControlPanel::ParamGroup>(group))
    {
    case ControlPanel::ParamGroup::Location:
        groupName = tr("阵地信息");
        groupButton = m_controlPanel->locationApplyPushButton;
        break;
    case ControlPanel::ParamGroup::ScanArea:
        groupName = tr("扇区设置");
        groupButton = m_controlPanel->scanAreaApplyPushButton;
        break;
    case ControlPanel::ParamGroup::TrackLimit:
        groupName = tr("目标起批限制");
        groupButton = m_controlPanel->trackLimitApplyPushButton;
        break;
    }

    if (!ConfirmDangerousAction(tr("确认下发%1").arg(groupName),
                                tr("即将把【%1】的 %2 个参数块下发到雷达 %3:%4。")
                                .arg(groupName)
                                .arg(specs.size())
                                .arg(m_remoteIp)
                                .arg(m_remotePort)))
    {
        LogMessage(tr("【参数控制】用户取消下发：%1").arg(groupName));
        return;
    }

    DeferredSaveConfig();
    if (static_cast<ControlPanel::ParamGroup>(group) == ControlPanel::ParamGroup::ScanArea)
    {
        SetMapSectors();
    }

    RadarControl control(static_cast<quint32>(specs.size()));
    for (int i = 0; i < specs.size(); ++i)
    {
        control.blocks[i].functionId = specs.at(i).functionId;
        control.blocks[i].data4 = specs.at(i).data4;
        control.blocks[i].data5 = specs.at(i).data5;
        control.blocks[i].data6 = specs.at(i).data6;
    }

    const QByteArray data = control.serialize();
    m_radarUDP->sendData(data, m_remoteIp, m_remotePort);
    LogMessage(tr("控制指令：应用【%1】（%2 块）\r\nHEX: %3")
               .arg(groupName)
               .arg(specs.size())
               .arg(HexDumpPreview(data)));
    DisableSenderBriefly(groupButton);
}



/// <summary>
/// @brief 辅助函数
/// </summary>
void MainWindow::on_modeListButton_clicked()
{
    m_tableController->SetViewMode(TrackTableController::ViewMode::TargetList);
}

void MainWindow::on_modeFocusButton_clicked()
{
    m_tableController->SetViewMode(TrackTableController::ViewMode::FocusHistory);
}

void MainWindow::BuildCentralLayout()
{
    QWidget* central = new QWidget(this);
    central->setObjectName(QStringLiteral("centralwidget"));

    QVBoxLayout* centralLayout = new QVBoxLayout(central);
    centralLayout->setContentsMargins(6, 6, 6, 6);
    centralLayout->setSpacing(6);

    m_contentSplitter = new QSplitter(Qt::Horizontal, central);
    QSplitter* contentSplitter = m_contentSplitter;
    contentSplitter->setObjectName(QStringLiteral("contentSplitter"));
    contentSplitter->setHandleWidth(6);
    contentSplitter->setChildrenCollapsible(false);

    // 地图区
    m_mapView = new MapView(m_trackStore, contentSplitter);
    m_mapView->setMinimumWidth(480);

    m_rightPanel = new QWidget(contentSplitter);
    QWidget* rightPanel = m_rightPanel;
    rightPanel->setObjectName(QStringLiteral("rightPanel"));
    rightPanel->setMinimumWidth(320);
    QVBoxLayout* rightLayout = new QVBoxLayout(rightPanel);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    rightLayout->setSpacing(0);

    m_rightSplitter = new QSplitter(Qt::Vertical, rightPanel);
    QSplitter* rightSplitter = m_rightSplitter;
    rightSplitter->setObjectName(QStringLiteral("rightSplitter"));
    rightSplitter->setHandleWidth(6);
    rightSplitter->setChildrenCollapsible(false);

    m_listPanel = new TargetListPanel(rightSplitter);

    m_rightTabs = new QTabWidget(rightSplitter);
    m_rightTabs->setObjectName(QStringLiteral("rightTabWidget"));
    m_rightTabs->setMinimumHeight(200);

    m_controlPanel = new ControlPanel;
    m_rightTabs->addTab(m_controlPanel, tr("参数控制"));

    m_networkPanel = new NetworkSettingsPanel;
    m_rightTabs->addTab(m_networkPanel, tr("网络设置"));

    m_displayPanel = new DisplaySettingsPanel;
    m_rightTabs->addTab(m_displayPanel, tr("显示设置"));

    QWidget* logPage = new QWidget;
    logPage->setObjectName(QStringLiteral("logPage"));
    QVBoxLayout* logLayout = new QVBoxLayout(logPage);
    logLayout->setContentsMargins(0, 0, 0, 0);
    m_logEdit = new QPlainTextEdit(logPage);
    m_logEdit->setObjectName(QStringLiteral("LoggerTextEdit"));
    m_logEdit->setReadOnly(true);
    m_logEdit->setFrameShape(QFrame::NoFrame);
    m_logEdit->setMaximumBlockCount(2000);
    logLayout->addWidget(m_logEdit);
    m_rightTabs->addTab(logPage, tr("日志记录"));

    rightLayout->addWidget(rightSplitter);

    contentSplitter->addWidget(m_mapView);
    contentSplitter->addWidget(rightPanel);
    contentSplitter->setStretchFactor(0, 2);
    contentSplitter->setStretchFactor(1, 1);
    contentSplitter->setSizes(QList<int>() << 880 << 440);
    rightSplitter->addWidget(m_listPanel);
    rightSplitter->addWidget(m_rightTabs);
    rightSplitter->setStretchFactor(0, 55);
    rightSplitter->setStretchFactor(1, 45);
    rightSplitter->setSizes(QList<int>() << 550 << 450);
    centralLayout->addWidget(contentSplitter, 1);

    // 底部状态栏
    m_statusBar = new AppStatusBar(central);
    centralLayout->addWidget(m_statusBar, 0);

    // 「点迹显示/隐藏」按钮
    m_trackToggleBtn = new QToolButton(m_statusBar);
    m_trackToggleBtn->setObjectName(QStringLiteral("trackToggleBtn"));
    m_trackToggleBtn->setToolTip(tr("显示/隐藏目标点迹"));
    m_trackToggleBtn->setIconSize(QSize(16, 16));
    m_trackToggleBtn->setCursor(Qt::PointingHandCursor);
    m_trackToggleBtn->setFocusPolicy(Qt::NoFocus);
    m_statusBar->layout()->addWidget(m_trackToggleBtn);
    connect(m_trackToggleBtn, &QToolButton::clicked, this, &MainWindow::ToggleTrackDisplay);

    m_panelToggleBtn = new QToolButton(m_statusBar);
    m_panelToggleBtn->setObjectName(QStringLiteral("panelToggleBtn"));
    m_panelToggleBtn->setText(tr("▲ 收起面板"));
    m_panelToggleBtn->setToolTip(tr("收起/展开右侧面板（小屏给地图腾空间）"));
    m_panelToggleBtn->setCursor(Qt::PointingHandCursor);
    m_panelToggleBtn->setFocusPolicy(Qt::NoFocus);
    m_statusBar->layout()->addWidget(m_panelToggleBtn);
    connect(m_panelToggleBtn, &QToolButton::clicked, this, &MainWindow::ToggleRightPanel);
    setCentralWidget(central);

    // 布局持久化
    QSettings settings;
    const QByteArray contentState = settings.value(QStringLiteral("layout/contentSplitter")).toByteArray();
    if (!contentState.isEmpty()) {
        m_contentSplitter->restoreState(contentState);
    }
    const QByteArray rightState = settings.value(QStringLiteral("layout/rightSplitter")).toByteArray();
    if (!rightState.isEmpty()) {
        m_rightSplitter->restoreState(rightState);
    }

    m_panelCollapsed = settings.value(QStringLiteral("layout/rightPanelCollapsed"), false).toBool();
    if (m_panelCollapsed) {
        m_rightPanel->setVisible(false);
        m_panelToggleBtn->setText(tr("▼ 展开面板"));
    }

    m_pointVisible = settings.value(QStringLiteral("layout/pointsVisible"), true).toBool();
    if (m_mapView) {
        m_mapView->setPointsVisible(m_pointVisible);
    }
    UpdateTrackToggleUi();

    QMetaObject::connectSlotsByName(this);
}

void MainWindow::ToggleRightPanel()
{
    if (!m_rightPanel || !m_panelToggleBtn) {
        return;
    }

    m_panelCollapsed = !m_panelCollapsed;
    m_rightPanel->setVisible(!m_panelCollapsed);
    m_panelToggleBtn->setText(m_panelCollapsed ? tr("▼ 展开面板") : tr("▲ 收起面板"));
    QSettings settings;
    settings.setValue(QStringLiteral("layout/rightPanelCollapsed"), m_panelCollapsed);
}

void MainWindow::ToggleTrackDisplay()
{
    if (!m_trackToggleBtn) {
        return;
    }
    m_pointVisible = !m_pointVisible;
    UpdateTrackToggleUi();
    if (m_mapView) {
        m_mapView->setPointsVisible(m_pointVisible);
    }

    QSettings settings;
    settings.setValue(QStringLiteral("layout/pointsVisible"), m_pointVisible);
}

void MainWindow::UpdateTrackToggleUi()
{
    if (!m_trackToggleBtn) {
        return;
    }
    m_trackToggleBtn->setIcon(MakeEyeIcon(m_pointVisible));
    m_trackToggleBtn->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    m_trackToggleBtn->setText(m_pointVisible ? tr("点迹隐藏") : tr("点迹显示"));
}

void MainWindow::SetMapSectors()
{
    if (!m_mapView) {
        return;
    }
    const double start[4] = {m_config.startScan0, m_config.startScan1, m_config.startScan2, m_config.startScan3};
    const double end[4] = {m_config.endScan0, m_config.endScan1, m_config.endScan2, m_config.endScan3};
    m_mapView->setSectors(start, end);
}

void MainWindow::LoadConfig()
{
    m_config = RadarConfig::LoadUi();

    const UdpSettings udp = RadarConfig::LoadUdp();
    m_localIp = udp.localIp;
    m_localPort = udp.localPort;
    m_remoteIp = udp.remoteIp;
    m_remotePort = udp.remotePort;
    m_sourceFilterEnabled = udp.sourceFilterEnabled;
    m_multicastEnabled = udp.multicastEnabled;
    m_multicastGroup = udp.multicastGroup;
    m_multicastPort = udp.multicastPort;
    m_displayConfig = RadarConfig::LoadDisplay();

    m_tableController->InitTables();
    WriteConfigToUi();
    WriteUdpToUi();
    WriteDisplayToUi();
    DeferredSaveConfig();

    SetMapSectors();
    ApplyDisplayConfig(m_displayConfig);
}

bool MainWindow::ReadConfigFromUi()
{
    QStringList errors;
    UIConfig tmp = m_config;

    tmp.startScan0 = ReadNum<float>(m_controlPanel->StartScan0LineEdit, tr("起始扇区0(°)"), kSectorMin, kSectorMax, errors);
    tmp.startScan1 = ReadNum<float>(m_controlPanel->StartScan1LineEdit, tr("起始扇区1(°)"), kSectorMin, kSectorMax, errors);
    tmp.startScan2 = ReadNum<float>(m_controlPanel->StartScan2LineEdit, tr("起始扇区2(°)"), kSectorMin, kSectorMax, errors);
    tmp.startScan3 = ReadNum<float>(m_controlPanel->StartScan3LineEdit, tr("起始扇区3(°)"), kSectorMin, kSectorMax, errors);
    tmp.endScan0 = ReadNum<float>(m_controlPanel->EndScan0LineEdit, tr("结束扇区0(°)"), kSectorMin, kSectorMax, errors);
    tmp.endScan1 = ReadNum<float>(m_controlPanel->EndScan1LineEdit, tr("结束扇区1(°)"), kSectorMin, kSectorMax, errors);
    tmp.endScan2 = ReadNum<float>(m_controlPanel->EndScan2LineEdit, tr("结束扇区2(°)"), kSectorMin, kSectorMax, errors);
    tmp.endScan3 = ReadNum<float>(m_controlPanel->EndScan3LineEdit, tr("结束扇区3(°)"), kSectorMin, kSectorMax, errors);
    tmp.northCorrection = ReadNum<float>(m_controlPanel->NorthCorrectionLineEdit, tr("偏北修正(°)"), kNorthCorrMin, kNorthCorrMax, errors);
    tmp.pitchCorrection = ReadNum<float>(m_controlPanel->ElevationCorrectionLineEdit, tr("俯仰修正(°)"), kPitchCorrMin, kPitchCorrMax, errors);
    tmp.distanceCorrection = ReadNum<float>(m_controlPanel->RangeCorrectionLineEdit, tr("斜距修正(m)"), kRangeCorrMin, kRangeCorrMax, errors);
    tmp.radarLongitude = ReadNum<double>(m_controlPanel->RadarLongitudeLineEdit, tr("雷达经度(°)"), kLonMin, kLonMax, errors);
    tmp.radarLatitude = ReadNum<double>(m_controlPanel->RadarLatitudeLineEdit, tr("雷达纬度(°)"), kLatMin, kLatMax, errors);
    tmp.radarHeight = ReadNum<float>(m_controlPanel->RadarHeightLineEdit, tr("雷达高度(m)"), kHeightLimitMin, kHeightLimitMax, errors);
    tmp.speedMin = ReadNum<float>(m_controlPanel->SpeedMinLineEdit, tr("速度下限(m/s)"), kSpeedLimitMin, kSpeedLimitMax, errors);
    tmp.speedMax = ReadNum<float>(m_controlPanel->SpeedMaxLineEdit, tr("速度上限(m/s)"), kSpeedLimitMin, kSpeedLimitMax, errors);
    tmp.heightMin = ReadNum<float>(m_controlPanel->HeightMinLineEdit, tr("高度下限(m)"), kHeightLimitMin, kHeightLimitMax, errors);
    tmp.heightMax = ReadNum<float>(m_controlPanel->HeightMaxLineEdit, tr("高度上限(m)"), kHeightLimitMin, kHeightLimitMax, errors);
    tmp.distanceMin = ReadNum<float>(m_controlPanel->DistanceMinLineEdit, tr("距离下限(m)"), kDistLimitMin, kDistLimitMax, errors);
    tmp.distanceMax = ReadNum<float>(m_controlPanel->DistanceMaxLineEdit, tr("距离上限(m)"), kDistLimitMin, kDistLimitMax, errors);

    CheckOrderedPair(m_controlPanel->StartScan0LineEdit, m_controlPanel->EndScan0LineEdit, tr("扇区0"), tmp.startScan0, tmp.endScan0, errors);
    CheckOrderedPair(m_controlPanel->StartScan1LineEdit, m_controlPanel->EndScan1LineEdit, tr("扇区1"), tmp.startScan1, tmp.endScan1, errors);
    CheckOrderedPair(m_controlPanel->StartScan2LineEdit, m_controlPanel->EndScan2LineEdit, tr("扇区2"), tmp.startScan2, tmp.endScan2, errors);
    CheckOrderedPair(m_controlPanel->StartScan3LineEdit, m_controlPanel->EndScan3LineEdit, tr("扇区3"), tmp.startScan3, tmp.endScan3, errors);
    CheckOrderedPair(m_controlPanel->SpeedMinLineEdit, m_controlPanel->SpeedMaxLineEdit, tr("处理速度范围"), tmp.speedMin, tmp.speedMax, errors);
    CheckOrderedPair(m_controlPanel->HeightMinLineEdit, m_controlPanel->HeightMaxLineEdit, tr("处理高度范围"), tmp.heightMin, tmp.heightMax, errors);
    CheckOrderedPair(m_controlPanel->DistanceMinLineEdit, m_controlPanel->DistanceMaxLineEdit, tr("处理距离范围"), tmp.distanceMin, tmp.distanceMax, errors);

    if (!errors.isEmpty()) {
        LogMessage(tr("【参数控制】存在 %1 项非法参数，已阻止下发与保存：\r\n- %2")
                   .arg(errors.count())
                   .arg(errors.join(QStringLiteral("\r\n- "))));
        return false;
    }

    m_config = tmp;
    return true;
}

void MainWindow::WriteConfigToUi()
{
    m_controlPanel->StartScan0LineEdit->setText(QString::number(m_config.startScan0, 'f', ConfigDecimals::Angle));
    m_controlPanel->StartScan1LineEdit->setText(QString::number(m_config.startScan1, 'f', ConfigDecimals::Angle));
    m_controlPanel->StartScan2LineEdit->setText(QString::number(m_config.startScan2, 'f', ConfigDecimals::Angle));
    m_controlPanel->StartScan3LineEdit->setText(QString::number(m_config.startScan3, 'f', ConfigDecimals::Angle));

    m_controlPanel->EndScan0LineEdit->setText(QString::number(m_config.endScan0, 'f', ConfigDecimals::Angle));
    m_controlPanel->EndScan1LineEdit->setText(QString::number(m_config.endScan1, 'f', ConfigDecimals::Angle));
    m_controlPanel->EndScan2LineEdit->setText(QString::number(m_config.endScan2, 'f', ConfigDecimals::Angle));
    m_controlPanel->EndScan3LineEdit->setText(QString::number(m_config.endScan3, 'f', ConfigDecimals::Angle));

    m_controlPanel->NorthCorrectionLineEdit->setText(QString::number(m_config.northCorrection, 'f', ConfigDecimals::Angle));
    m_controlPanel->ElevationCorrectionLineEdit->setText(QString::number(m_config.pitchCorrection, 'f', ConfigDecimals::Angle));
    m_controlPanel->RangeCorrectionLineEdit->setText(QString::number(m_config.distanceCorrection, 'f', ConfigDecimals::Meter));

    m_controlPanel->RadarLongitudeLineEdit->setText(QString::number(m_config.radarLongitude, 'f', ConfigDecimals::Degree));
    m_controlPanel->RadarLatitudeLineEdit->setText(QString::number(m_config.radarLatitude, 'f', ConfigDecimals::Degree));
    m_controlPanel->RadarHeightLineEdit->setText(QString::number(m_config.radarHeight, 'f', ConfigDecimals::Meter));

    m_controlPanel->SpeedMinLineEdit->setText(QString::number(m_config.speedMin, 'f', ConfigDecimals::Speed));
    m_controlPanel->SpeedMaxLineEdit->setText(QString::number(m_config.speedMax, 'f', ConfigDecimals::Speed));
    m_controlPanel->HeightMinLineEdit->setText(QString::number(m_config.heightMin, 'f', ConfigDecimals::Meter));
    m_controlPanel->HeightMaxLineEdit->setText(QString::number(m_config.heightMax, 'f', ConfigDecimals::Meter));
    m_controlPanel->DistanceMinLineEdit->setText(QString::number(m_config.distanceMin, 'f', ConfigDecimals::Meter));
    m_controlPanel->DistanceMaxLineEdit->setText(QString::number(m_config.distanceMax, 'f', ConfigDecimals::Meter));
}

void MainWindow::WriteUdpToUi()
{
    QVector<NetworkSettingsPanel::ChannelRow> rows;
    NetworkSettingsPanel::ChannelRow radar;
    radar.id = QStringLiteral("radar");
    radar.name = tr("雷达");
    radar.srcIp = m_localIp;
    radar.srcPort = m_localPort;
    radar.dstIp = m_remoteIp;
    radar.dstPort = m_remotePort;
    rows.append(radar);

    m_networkPanel->setChannels(rows);
}

void MainWindow::WriteDisplayToUi()
{
    m_displayPanel->setCategoryColors(m_displayConfig.categoryColors);
    m_displayPanel->setCategoryAlphas(m_displayConfig.categoryAlphas);
    m_displayPanel->setCategoryVisibility(m_displayConfig.categoryVisibility);
}

void MainWindow::ApplyDisplayConfig(const DisplayConfig& cfg)
{
    if (!m_mapView)
    {
        return;
    }
    m_mapView->setCategoryColorOverrides(cfg.categoryColors);
    m_mapView->setCategoryAlphaOverrides(cfg.categoryAlphas);
    m_mapView->setCategoryVisibilityOverrides(cfg.categoryVisibility);
}

/// <summary>
/// @brief 网络设置应用
/// </summary>
void MainWindow::on_applyNetworkPushButton_clicked()
{
    const QVector<NetworkSettingsPanel::ChannelRow> rows = m_networkPanel->channels();

    QStringList errors;
    QHostAddress addr;
    for (const NetworkSettingsPanel::ChannelRow& row : rows)
    {
        if (row.srcIp.isEmpty() || !addr.setAddress(row.srcIp))
        {
            errors << tr("%1：源IP无效").arg(row.name);
        }
        if (row.srcPort == 0)
        {
            errors << tr("%1：源端口无效（1~65535）").arg(row.name);
        }
        if (row.dstIp.isEmpty() || !addr.setAddress(row.dstIp))
        {
            errors << tr("%1：目的IP无效").arg(row.name);
        }
        if (row.dstPort == 0)
        {
            errors << tr("%1：目的端口无效（1~65535）").arg(row.name);
        }
    }
    if (!errors.isEmpty()) {
        LogMessage(tr("【网络设置】参数无效，未应用：\r\n- %1").arg(errors.join(QStringLiteral("\r\n- "))));
        return;
    }

    const QString prevLocalIp = m_localIp;
    const quint16 prevLocalPort = m_localPort;
    for (const NetworkSettingsPanel::ChannelRow& row : rows)
    {
        if (row.id == QStringLiteral("radar")) {
            m_localIp = row.srcIp;
            m_localPort = row.srcPort;
            m_remoteIp = row.dstIp;
            m_remotePort = row.dstPort;
        }
    }

    // 持久化网络设置
    UdpSettings udp = RadarConfig::LoadUdp();
    udp.localIp = m_localIp;
    udp.localPort = m_localPort;
    udp.remoteIp = m_remoteIp;
    udp.remotePort = m_remotePort;
    udp.sourceFilterEnabled = m_sourceFilterEnabled;
    udp.multicastEnabled = m_multicastEnabled;
    udp.multicastGroup = m_multicastGroup;
    udp.multicastPort = m_multicastPort;
    RadarConfig::SaveUdp(udp);

    const UdpSettings saved = RadarConfig::LoadUdp();
    if (saved.localIp != m_localIp || saved.localPort != m_localPort) {
        LogMessage(tr("【网络告警】配置写入未生效：文件中仍是 雷达 %1:%2，"
                      "请检查 RadarConfig.ini 是否只读或被占用")
                   .arg(saved.localIp)
                   .arg(saved.localPort));
        m_statusBar->setTelemetry(tr("⚠ 配置写入未生效，界面按文件实际值回显"), true);
    }
    m_localIp = saved.localIp;
    m_localPort = saved.localPort;
    m_remoteIp = saved.remoteIp;
    m_remotePort = saved.remotePort;
    m_sourceFilterEnabled = saved.sourceFilterEnabled;
    m_multicastEnabled = saved.multicastEnabled;
    m_multicastGroup = saved.multicastGroup;
    m_multicastPort = saved.multicastPort;
    WriteUdpToUi();

    if (qEnvironmentVariableIsSet("RADAR_NO_UDP")) {
        LogMessage(tr("【网络设置】参数已保存（RADAR_NO_UDP 模式，未重新绑定端口）"));
        LogReceivePlan(false);
        return;
    }

    const bool endpointChanged = prevLocalIp != m_localIp || prevLocalPort != m_localPort;
    if (endpointChanged) {
        m_trackStore->clear();
        m_tableController->ResetDataStats();
        m_everRx = false;
        m_lastRxMs = 0;
        m_foreignPacketCount = 0;
        m_foreignLastWarnMs = 0;
        m_statusBar->setConnectionState(AppStatusBar::ConnState::Idle, 0);
        LogMessage(tr("【网络设置】接收端点已变更，已清空旧端点的航迹/点迹数据，等待新端点数据"));
    }

    // 通道绑定
    const bool radarOk = m_radarUDP->bind(m_localIp, m_localPort);
    if (radarOk && m_sourceFilterEnabled) {
        m_radarUDP->setAllowedSource(m_remoteIp, m_remotePort);
    }
    LogMessage(tr("【网络设置】雷达通道：本地 %1:%2 → 远端 %3:%4，绑定%5")
               .arg(m_localIp)
               .arg(m_localPort)
               .arg(m_remoteIp)
               .arg(m_remotePort)
               .arg(radarOk ? tr("成功") : tr("失败")));

    ReportBindResult(radarOk);
    LogReceivePlan(radarOk);
    ApplyMulticastReceiver();
}

/// <summary>
/// @brief 显示项
/// </summary>
void MainWindow::TrackDisplayEdited()
{
    const DisplayConfig previous = m_displayConfig;
    m_displayConfig.categoryColors = m_displayPanel->categoryColorOverrides();
    m_displayConfig.categoryAlphas = m_displayPanel->categoryAlphaOverrides();
    m_displayConfig.categoryVisibility = m_displayPanel->categoryVisibilityOverrides();
    ApplyDisplayConfig(m_displayConfig);
    RadarConfig::SaveDisplay(m_displayConfig);

    QStringList changes;
    const QVector<TrackDisplayColors::CategoryColor>& colorTable = TrackDisplayColors::Table();

    for (const TrackDisplayColors::CategoryColor& info : colorTable) {
        const QString title = QString::fromUtf8(info.title);

        const QColor oldColor = previous.categoryColors.value(info.category, info.defaultColor);
        const QColor newColor = m_displayConfig.categoryColors.value(info.category, info.defaultColor);
        if (oldColor != newColor) {
            changes << tr("%1颜色→%2").arg(title, newColor.name().toUpper());
        }

        const int oldAlpha = previous.categoryAlphas.value(info.category, DisplayDefaults::TrailAlpha);
        const int newAlpha = m_displayConfig.categoryAlphas.value(info.category, DisplayDefaults::TrailAlpha);
        if (oldAlpha != newAlpha) {
            changes << tr("%1透明度 %2%→%3%").arg(title).arg(oldAlpha).arg(newAlpha);
        }

        const bool oldVisible = previous.categoryVisibility.value(info.category, true);
        const bool newVisible = m_displayConfig.categoryVisibility.value(info.category, true);
        if (oldVisible != newVisible) {
            changes << tr("%1%2绘制").arg(title, newVisible ? tr("恢复") : tr("停止"));
        }
    }

    if (!changes.isEmpty()) {
        LogMessage(tr("【显示设置】已更新并保存：%1").arg(changes.join(QStringLiteral("，"))));
    }
}

void MainWindow::DeferredSaveConfig()
{
    QMetaObject::invokeMethod(this, [this]() { RadarConfig::SaveUi(m_config); }, Qt::QueuedConnection);
}

bool MainWindow::ConfirmDangerousAction(const QString& title, const QString& text)
{
    if (qEnvironmentVariableIsSet("RADAR_NO_UDP"))
    {
        return true;
    }

    return AppMessageDialog::Confirm(this, title, text);
}

void MainWindow::DisableSenderBriefly(QPushButton* btn)
{
    if (!btn)
    {
        return;
    }
    btn->setEnabled(false);
    QTimer::singleShot(1500, btn, [btn]() { btn->setEnabled(true); });
}

QString MainWindow::ReceiveSummary() const
{
    return tr("接收中：雷达 %1:%2 ｜ 来源过滤 %3")
            .arg(m_localIp)
            .arg(m_localPort)
            .arg(m_sourceFilterEnabled ? tr("启用") : tr("关闭"));
}

void MainWindow::LogReceivePlan(bool bindOk)
{
    if (qEnvironmentVariableIsSet("RADAR_NO_UDP"))
    {
        LogMessage(tr("【网络设置】当前为 RADAR_NO_UDP 模式：配置已保存但不绑定端口，"
                      "修改需关闭该环境变量并重启程序后才会生效"));
        m_statusBar->setTelemetry(tr("⚠ 配置已保存但未绑定端口（RADAR_NO_UDP 模式）"), true);
        return;
    }
    LogMessage(tr("【网络设置】%1").arg(ReceiveSummary()));
    if (m_sourceFilterEnabled)
    {
        LogMessage(tr("【网络设置】仅接收 雷达 %1:%2 发来的数据，其它来源会被丢弃")
                   .arg(m_remoteIp)
                   .arg(m_remotePort));
    }

    if (bindOk)
    {
        m_statusBar->setTelemetry(ReceiveSummary());
    }
}

void MainWindow::OnChannelEdited(const QString& channelName, const QString& detail)
{
    LogMessage(tr("【网络设置】%1 通道已修改：%2 —— 需点击「应用网络设置」后才会重新绑定端口生效")
               .arg(channelName, detail));
}

void MainWindow::OnPendingApplyChanged(bool pending)
{
    if (pending)
    {
        m_statusBar->setTelemetry(tr("⚠ 网络设置有未应用的修改，点「应用网络设置」后生效"), true);
        return;
    }
    m_statusBar->setTelemetry(ReceiveSummary());
}

void MainWindow::OnRadarModeApplyRequested(int modeIndex)
{
    if (modeIndex < 0 || modeIndex >= ControlPanel::ModeCount)
    {
        return;
    }

    const QString modeName = tr("模式%1").arg(modeIndex);
    if (!ConfirmDangerousAction(tr("确认应用雷达模式"),
                                tr("即将向雷达发送【%1】指令（模式值 %2）。").arg(modeName).arg(modeIndex)))
    {
        LogMessage(tr("【雷达模式】用户取消应用「%1」").arg(modeName));
        return;
    }

    RadarControl control(1);
    control.blocks[0].functionId = ControlFunctionId::WorkMode;
    control.blocks[0].data0 = static_cast<qint32>(modeIndex);

    const QByteArray data = control.serialize();
    m_radarUDP->sendData(data, m_remoteIp, m_remotePort);
    LogMessage(tr("控制指令：%1\r\nHEX: %2").arg(modeName).arg(HexDumpPreview(data)));
    DisableSenderBriefly(m_controlPanel->modeApplyPushButton);
}

void MainWindow::ReportBindResult(bool radarOk)
{
    if (radarOk)
    {
        return;
    }

    LogMessage(tr("【网络告警】UDP 绑定失败：雷达终端 %1:%2 —— 对应数据将无法接收，请检查端口占用/防火墙/权限")
               .arg(m_localIp)
               .arg(m_localPort));
    m_statusBar->setTelemetry(tr("⚠ UDP 绑定失败：雷达终端 %1:%2").arg(m_localIp).arg(m_localPort), true);
}

void MainWindow::ForeignDatagram(const QString& channelName, const QString& senderAddress, quint16 senderPort)
{
    ++m_foreignPacketCount;
    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    if (m_foreignLastWarnMs == 0 || now - m_foreignLastWarnMs >= 1000)
    {
        LogMessage(tr("【网络告警】%1 通道拒绝非法来源 %2:%3（期望 雷达 %4:%5），1s 内共 %6 个")
                   .arg(channelName, senderAddress)
                   .arg(senderPort)
                   .arg(m_remoteIp)
                   .arg(m_remotePort)
                   .arg(m_foreignPacketCount));
        m_foreignPacketCount = 0;
        m_foreignLastWarnMs = now;
    }
}

void MainWindow::OnUdpSendFailed(const QString& address, quint16 port, const QString& error)
{
    LogMessage(tr("【网络告警】控制指令发送失败：→ %1:%2 —— %3。\r\n"
                  "指令未送达雷达，请检查网络连接、目的地址与防火墙设置后重试。")
               .arg(address)
               .arg(port)
               .arg(error));
    m_statusBar->setTelemetry(tr("⚠ 指令发送失败：→ %1:%2").arg(address).arg(port), true);
}

void MainWindow::ApplyMulticastReceiver()
{
    if (qEnvironmentVariableIsSet("RADAR_NO_UDP"))
    {
        return;
    }

    if (!m_multicastEnabled)
    {
        if (m_multicastUDP)
        {
            m_multicastUDP->closeSocket();
        }
        return;
    }

    if (!m_multicastUDP)
    {
        m_multicastUDP = new UdpClass(this);
        connect(m_multicastUDP, &UdpClass::dataReceived, this, &MainWindow::ProcessDatagram);
        connect(m_multicastUDP, &UdpClass::foreignDatagramReceived, this,
                [this](const QString& address, quint16 port) { ForeignDatagram(tr("组播"), address, port); });
    }

    const bool bound = m_multicastUDP->bind(QStringLiteral("0.0.0.0"), m_multicastPort);
    if (!bound)
    {
        LogMessage(tr("【网络告警】组播接收端口 %1 绑定失败").arg(m_multicastPort));
        return;
    }
    if (m_sourceFilterEnabled)
    {
        m_multicastUDP->setAllowedSource(m_remoteIp, m_remotePort);
    }
    const bool joined = m_multicastUDP->joinMulticastGroup(m_multicastGroup);
    if (joined)
    {
        LogMessage(tr("【网络设置】已加入组播组 %1:%2（来源过滤：%3）")
                   .arg(m_multicastGroup)
                   .arg(m_multicastPort)
                   .arg(m_sourceFilterEnabled ? tr("启用") : tr("关闭")));
    }
    else
    {
        LogMessage(tr("【网络告警】加入组播组 %1:%2 失败，将无法接收组播数据")
                   .arg(m_multicastGroup)
                   .arg(m_multicastPort));
    }
}

QString MainWindow::TimeStamp()
{
    return QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd HH:mm:ss.zzz"));
}

void MainWindow::LogMessage(const QString& text)
{
    m_logEdit->appendPlainText(QStringLiteral("[%1] %2").arg(TimeStamp(), text));
}

QString MainWindow::HexDump(const QByteArray& data)
{
    return QString::fromLatin1(data.toHex(' ').toUpper());
}

QString MainWindow::HexDumpPreview(const QByteArray& data, int maxBytes)
{
    QString s = QString::fromLatin1(data.left(maxBytes).toHex(' ').toUpper());
    if (data.size() > maxBytes)
    {
        s += QStringLiteral(" …（共 %1 字节）").arg(data.size());
    }
    return s;
}
