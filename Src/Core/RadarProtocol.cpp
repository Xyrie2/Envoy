#include "Core/RadarProtocol.h"
#include "Core/RadarConstants.h"

#include <QByteArray>
#include <cstring>

quint32 calculateChecksum(const QByteArray& data)
{
    quint32 sum = 0;
    const quint8* ptr = reinterpret_cast<const quint8*>(data.constData());
    for (int i = 0; i < data.size(); ++i)
    {
        sum += ptr[i];
    }
    return sum;
}

bool verifyChecksum(const QByteArray& ba, int payloadSize, quint32 expected)
{
    if (payloadSize < 0 || ba.size() < payloadSize + static_cast<int>(sizeof(quint32)))
    {
        return false;
    }

    quint32 sum = 0;
    const quint8* ptr = reinterpret_cast<const quint8*>(ba.constData());
    for (int i = 0; i < payloadSize; ++i)
    {
        sum += ptr[i];
    }
    return sum == expected;
}

bool RadarState2::fromByteArray(const QByteArray& ba)
{
    if (ba.size() < static_cast<int>(PacketSize))
    {
        return false;
    }
    std::memcpy(this, ba.constData(), PacketSize);
    return isFrameValid(FrameHeader::State2) && packetLength == static_cast<qint32>(PacketSize)
           && verifyChecksum(ba, RadarState2::PacketSize - static_cast<int>(sizeof(quint32)), checksum);
}

bool RadarState2::isFrameValid(quint32 syncWord) const
{
    return static_cast<quint32>(frameHeader) == syncWord;
}

bool RadarState3::fromByteArray(const QByteArray& ba)
{
    if (ba.size() < static_cast<int>(PacketSize))
    {
        return false;
    }
    std::memcpy(this, ba.constData(), PacketSize);
    return isFrameValid(FrameHeader::State3) && packetLength == static_cast<quint32>(PacketSize)
           && verifyChecksum(ba, RadarState3::PacketSize - static_cast<int>(sizeof(quint32)), checksum);
}

bool RadarState3::isFrameValid(quint32 syncWord) const
{
    return frameHeader == syncWord;
}

bool RadarTrack::fromByteArray(const QByteArray& ba)
{
    if (ba.size() < static_cast<int>(sizeof(RadarTrack)))
    {
        return false;
    }
    std::memcpy(this, ba.constData(), sizeof(RadarTrack));
    return frameHeader == FrameHeader::Track 
           && verifyChecksum(ba, static_cast<int>(sizeof(RadarTrack) - sizeof(quint32)), checksum);
}

bool RadarState::fromByteArray(const QByteArray& ba)
{
    if (ba.size() < static_cast<int>(sizeof(RadarState)))
    {
        return false;
    }
    std::memcpy(this, ba.constData(), sizeof(RadarState));
    return frameHeader == FrameHeader::State
           && verifyChecksum(ba, static_cast<int>(sizeof(RadarState) - sizeof(quint32)), checksum);
}

bool RadarPoint::fromByteArray(const QByteArray& ba)
{
    if (ba.size() < static_cast<int>(sizeof(RadarPoint)))
    {
        return false;
    }
    std::memcpy(this, ba.constData(), sizeof(RadarPoint));
    return frameHeader == FrameHeader::Point
           && verifyChecksum(ba, static_cast<int>(sizeof(RadarPoint) - sizeof(quint32)), static_cast<quint32>(crc));
}

bool RadarPTZ::fromByteArray(const QByteArray& ba)
{
    if (ba.size() < static_cast<int>(sizeof(RadarPTZ)))
    {
        return false;
    }
    std::memcpy(this, ba.constData(), sizeof(RadarPTZ));
    return frameHeader == FrameHeader::Ptz
           && verifyChecksum(ba, static_cast<int>(sizeof(RadarPTZ) - sizeof(quint32)), static_cast<quint32>(crc));
}

bool SignalProcessingConfig::fromByteArray(const QByteArray& ba)
{
    if (ba.size() < static_cast<int>(sizeof(SignalProcessingConfig)))
    {
        return false;
    }
    std::memcpy(this, ba.constData(), sizeof(SignalProcessingConfig));
    return frameHeader == FrameHeader::SignalConfig && verifyChecksum(ba, static_cast<int>(sizeof(SignalProcessingConfig) - sizeof(quint32)), static_cast<quint32>(checksum));
}

bool DataProcessingConfig::fromByteArray(const QByteArray& ba)
{
    if (ba.size() < static_cast<int>(sizeof(DataProcessingConfig)))
    {
        return false;
    }
    std::memcpy(this, ba.constData(), sizeof(DataProcessingConfig));
    return frameHeader == FrameHeader::DataConfig /* && verifyChecksum(ba, static_cast<int>(sizeof(DataProcessingConfig) - sizeof(quint32)), static_cast<quint32>(checksum))*/;
}
