#include "Net/UdpClass.h"
#include <QHostAddress>

UdpClass::UdpClass(QObject* parent) : QObject(parent), m_udpSocket(new QUdpSocket(this))
{
    QObject::connect(m_udpSocket, &QUdpSocket::readyRead, this, &UdpClass::readPendingDatagrams);
}

bool UdpClass::bind(const QString& localAddress, quint16 port)
{
    const QHostAddress addr(localAddress);

    if (m_udpSocket->state() == QAbstractSocket::BoundState
        && m_udpSocket->localAddress() == addr
        && m_udpSocket->localPort() == port)
    {
        return true;
    }

    const bool wasBound = m_udpSocket->state() == QAbstractSocket::BoundState;
    const QHostAddress prevAddr = m_udpSocket->localAddress();
    const quint16 prevPort = m_udpSocket->localPort();

    m_udpSocket->close();
    m_udpSocket->setSocketOption(QAbstractSocket::ReceiveBufferSizeSocketOption, BufferSize);
    if (m_udpSocket->bind(addr, port))
    {
        return true;
    }

    qWarning() << "UDP bind failed:" << localAddress << ":" << port << "-" << m_udpSocket->errorString();

    if (wasBound)
    {
        m_udpSocket->close();
        m_udpSocket->setSocketOption(QAbstractSocket::ReceiveBufferSizeSocketOption, BufferSize);
        if (m_udpSocket->bind(prevAddr, prevPort))
        {
            qWarning() << "UDP bind reverted to previous endpoint:"
                       << prevAddr.toString() << ":" << prevPort;
        }
    }
    return false;
}

void UdpClass::sendData(const QByteArray& data, const QString& address, quint16 port)
{
    const QHostAddress host(address);
    const qint64 sent = host.isNull() ? -1 : m_udpSocket->writeDatagram(data, host, port);
    if (sent != static_cast<qint64>(data.size()))
    {
        const QString error = host.isNull()
                ? QStringLiteral("地址无效：") + address
                : m_udpSocket->errorString();
        qWarning() << "UDP send failed:" << address << ":" << port << "-" << error
                   << "（待发" << data.size() << "字节）";
        emit sendFailed(address, port, error);
    }
}

void UdpClass::setAllowedSource(const QString& address, quint16 port)
{
    m_allowedAddress = QHostAddress(address);
    m_allowedPort = port;
    m_sourceFilterEnabled = !m_allowedAddress.isNull() && port != 0;
}

bool UdpClass::joinMulticastGroup(const QString& group)
{
    const QHostAddress groupAddr(group);
    if (groupAddr.isNull() || !groupAddr.isMulticast())
    {
        return false;
    }
    return m_udpSocket->joinMulticastGroup(groupAddr);
}

void UdpClass::closeSocket()
{
    m_udpSocket->close();
}

void UdpClass::readPendingDatagrams()
{
    while (m_udpSocket->hasPendingDatagrams())
    {
        const qint64 size = m_udpSocket->pendingDatagramSize();
        if (size <= 0)
        {
            break;
        }

        QByteArray datagram;
        QHostAddress senderAddress;
        quint16 senderPort;

        datagram.resize(static_cast<int>(size));
        m_udpSocket->readDatagram(datagram.data(), datagram.size(), &senderAddress, &senderPort);

        if (m_sourceFilterEnabled
            && !(senderAddress == m_allowedAddress && senderPort == m_allowedPort))
        {
            emit foreignDatagramReceived(senderAddress.toString(), senderPort);
            continue;
        }

        emit dataReceived(datagram);
    }
}
