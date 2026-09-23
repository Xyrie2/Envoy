#pragma once
#include <QObject>
#include <QString>
#include <QUdpSocket>

#include "Core/RadarConstants.h"

class UdpClass : public QObject
{
    Q_OBJECT
public:
    UdpClass(QObject* parent = nullptr);
    ~UdpClass() override = default;
    bool bind(const QString& localAddress, quint16 port);
    void sendData(const QByteArray& data, const QString& address, quint16 port);
    void setAllowedSource(const QString& address, quint16 port);
    bool joinMulticastGroup(const QString& group);
    void closeSocket();
signals:
    void dataReceived(const QByteArray& data);
    void foreignDatagramReceived(const QString& senderAddress, quint16 senderPort);
    void sendFailed(const QString& address, quint16 port, const QString& error);
private slots:
    void readPendingDatagrams();
private:
    QUdpSocket* m_udpSocket;
    QHostAddress m_allowedAddress;      /// 允许的来源地址
    quint16 m_allowedPort = 0;          /// 允许的来源端口
    bool m_sourceFilterEnabled = false; /// 来源过滤开关
    static constexpr int BufferSize = DefaultValues::UdpBufferSize;
};
