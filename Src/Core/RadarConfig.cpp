#include "Core/RadarConfig.h"
#include "Core/RadarConstants.h"

#include <QCoreApplication>
#include <QDir>
#include <QSettings>

#include <algorithm>

QString RadarConfig::FilePath()
{
    return QDir::cleanPath(QCoreApplication::applicationDirPath() + QDir::separator() + QStringLiteral("RadarConfig.ini"));
}

UIConfig RadarConfig::LoadUi()
{
    UIConfig cfg;
    QSettings settings(FilePath(), QSettings::IniFormat);

    cfg.startScan0 = settings.value("UIConfig/startScan0", cfg.startScan0).toFloat();
    cfg.startScan1 = settings.value("UIConfig/startScan1", cfg.startScan1).toFloat();
    cfg.startScan2 = settings.value("UIConfig/startScan2", cfg.startScan2).toFloat();
    cfg.startScan3 = settings.value("UIConfig/startScan3", cfg.startScan3).toFloat();
    cfg.endScan0 = settings.value("UIConfig/endScan0", cfg.endScan0).toFloat();
    cfg.endScan1 = settings.value("UIConfig/endScan1", cfg.endScan1).toFloat();
    cfg.endScan2 = settings.value("UIConfig/endScan2", cfg.endScan2).toFloat();
    cfg.endScan3 = settings.value("UIConfig/endScan3", cfg.endScan3).toFloat();

    cfg.northCorrection = settings.value("UIConfig/northCorrection", cfg.northCorrection).toFloat();
    cfg.pitchCorrection = settings.value("UIConfig/pitchCorrection", cfg.pitchCorrection).toFloat();
    cfg.distanceCorrection = settings.value("UIConfig/distanceCorrection", cfg.distanceCorrection).toFloat();

    cfg.radarLongitude = settings.value("UIConfig/radarLongitude", DefaultValues::RadarLongitude).toDouble();
    cfg.radarLatitude = settings.value("UIConfig/radarLatitude", DefaultValues::RadarLatitude).toDouble();
    cfg.radarHeight = settings.value("UIConfig/radarHeight", cfg.radarHeight).toFloat();

    cfg.speedMin = settings.value("UIConfig/speedMin", cfg.speedMin).toFloat();
    cfg.speedMax = settings.value("UIConfig/speedMax", cfg.speedMax).toFloat();
    cfg.heightMin = settings.value("UIConfig/heightMin", cfg.heightMin).toFloat();
    cfg.heightMax = settings.value("UIConfig/heightMax", cfg.heightMax).toFloat();
    cfg.distanceMin = settings.value("UIConfig/distanceMin", cfg.distanceMin).toFloat();
    cfg.distanceMax = settings.value("UIConfig/distanceMax", cfg.distanceMax).toFloat();

    return cfg;
}

UdpSettings RadarConfig::LoadUdp()
{
    QSettings settings(FilePath(), QSettings::IniFormat);

    UdpSettings udp;
    udp.localIp = settings.value("UDP/localIP", DefaultValues::LocalIp).toString();
    udp.localPort = static_cast<quint16>(settings.value("UDP/localPort", DefaultValues::LocalPort).toInt());
    udp.remoteIp = settings.value("UDP/remoteIP", DefaultValues::RemoteIp).toString();
    udp.remotePort = static_cast<quint16>(settings.value("UDP/remotePort", DefaultValues::RemotePort).toInt());

    udp.sourceFilterEnabled = settings.value("UDP/sourceFilter", 1).toBool();
    udp.multicastEnabled = settings.value("UDP/multicastEnabled", 0).toBool();
    udp.multicastGroup = settings.value("UDP/multicastGroup").toString();
    udp.multicastPort = static_cast<quint16>(settings.value("UDP/multicastPort", 0).toInt());
    return udp;
}

void RadarConfig::SaveUdp(const UdpSettings& udp)
{
    QSettings settings(FilePath(), QSettings::IniFormat);

    settings.setValue("UDP/localIP", udp.localIp);
    settings.setValue("UDP/localPort", udp.localPort);
    settings.setValue("UDP/remoteIP", udp.remoteIp);
    settings.setValue("UDP/remotePort", udp.remotePort);
    settings.setValue("UDP/sourceFilter", udp.sourceFilterEnabled);
    settings.setValue("UDP/multicastEnabled", udp.multicastEnabled);
    settings.setValue("UDP/multicastGroup", udp.multicastGroup);
    settings.setValue("UDP/multicastPort", udp.multicastPort);
}

DisplayConfig RadarConfig::LoadDisplay()
{
    QSettings settings(FilePath(), QSettings::IniFormat);

    DisplayConfig cfg;
    const QString pairs = settings.value("Display/categoryColors").toString();
    const QStringList parts = pairs.split(QLatin1Char(','), QString::SkipEmptyParts);
    for (const QString& part : parts)
    {
        const int sep = part.indexOf(QLatin1Char(':'));
        if (sep <= 0)
        {
            continue;
        }
        bool okCategory = false;
        const int category = part.left(sep).trimmed().toInt(&okCategory);
        const QColor color(part.mid(sep + 1).trimmed());
        if (okCategory && color.isValid() && TrackDisplayColors::IsKnownCategory(static_cast<quint8>(category)))
        {
            cfg.categoryColors.insert(static_cast<quint8>(category), color);
        }
    }

    const QString alphaPairs = settings.value("Display/categoryAlphas").toString();
    const QStringList alphaParts = alphaPairs.split(QLatin1Char(','), QString::SkipEmptyParts);
    for (const QString& part : alphaParts)
    {
        const int sep = part.indexOf(QLatin1Char(':'));
        if (sep <= 0)
        {
            continue;
        }
        bool okCategory = false;
        bool okAlpha = false;
        const int category = part.left(sep).trimmed().toInt(&okCategory);
        const int rawAlpha = part.mid(sep + 1).trimmed().toInt(&okAlpha);
        if (!okCategory || !okAlpha || !TrackDisplayColors::IsKnownCategory(static_cast<quint8>(category)))
        {
            continue;
        }
        const int alpha = qBound(DisplayDefaults::TrailAlphaMin, rawAlpha, DisplayDefaults::TrailAlphaMax);
        if (alpha != DisplayDefaults::TrailAlpha)
        {
            cfg.categoryAlphas.insert(static_cast<quint8>(category), alpha);
        }
    }

    // 显隐覆盖
    const QString visibilityPairs = settings.value("Display/categoryVisibility").toString();
    const QStringList visibilityParts = visibilityPairs.split(QLatin1Char(','), QString::SkipEmptyParts);
    for (const QString& part : visibilityParts)
    {
        const int sep = part.indexOf(QLatin1Char(':'));
        if (sep <= 0)
        {
            continue;
        }
        bool okCategory = false;
        bool okVisible = false;
        const int category = part.left(sep).trimmed().toInt(&okCategory);
        const int rawVisible = part.mid(sep + 1).trimmed().toInt(&okVisible);
        if (!okCategory || !okVisible || !TrackDisplayColors::IsKnownCategory(static_cast<quint8>(category)))
        {
            continue;
        }
        if (rawVisible == 0)
        {
            cfg.categoryVisibility.insert(static_cast<quint8>(category), false);
        }
    }

    return cfg;
}

void RadarConfig::SaveDisplay(const DisplayConfig& cfg)
{
    QSettings settings(FilePath(), QSettings::IniFormat);
    QStringList pairs;
    QList<quint8> categories = cfg.categoryColors.keys();
    std::sort(categories.begin(), categories.end());
    for (quint8 category : categories)
    {
        pairs << QStringLiteral("%1:%2").arg(category).arg(cfg.categoryColors.value(category).name());
    }
    settings.setValue("Display/categoryColors", pairs.join(QLatin1Char(',')));

    QStringList alphaPairs;
    QList<quint8> alphaCategories = cfg.categoryAlphas.keys();
    std::sort(alphaCategories.begin(), alphaCategories.end());
    for (quint8 category : alphaCategories)
    {
        alphaPairs << QStringLiteral("%1:%2").arg(category).arg(cfg.categoryAlphas.value(category));
    }
    settings.setValue("Display/categoryAlphas", alphaPairs.join(QLatin1Char(',')));

    QStringList visibilityPairs;
    QList<quint8> visibilityCategories = cfg.categoryVisibility.keys();
    std::sort(visibilityCategories.begin(), visibilityCategories.end());
    for (quint8 category : visibilityCategories)
    {
        if (!cfg.categoryVisibility.value(category))
        {
            visibilityPairs << QStringLiteral("%1:0").arg(category);
        }
    }
    settings.setValue("Display/categoryVisibility", visibilityPairs.join(QLatin1Char(',')));
}

namespace
{
QString fixedStr(float v, int decimals)
{
    return QString::number(static_cast<double>(v), 'f', decimals);
}
QString fixedStr(double v, int decimals)
{
    return QString::number(v, 'f', decimals);
}
}

void RadarConfig::SaveUi(const UIConfig& cfg)
{
    QSettings settings(FilePath(), QSettings::IniFormat);

    settings.setValue("UIConfig/startScan0", fixedStr(cfg.startScan0, ConfigDecimals::Angle));
    settings.setValue("UIConfig/startScan1", fixedStr(cfg.startScan1, ConfigDecimals::Angle));
    settings.setValue("UIConfig/startScan2", fixedStr(cfg.startScan2, ConfigDecimals::Angle));
    settings.setValue("UIConfig/startScan3", fixedStr(cfg.startScan3, ConfigDecimals::Angle));

    settings.setValue("UIConfig/endScan0", fixedStr(cfg.endScan0, ConfigDecimals::Angle));
    settings.setValue("UIConfig/endScan1", fixedStr(cfg.endScan1, ConfigDecimals::Angle));
    settings.setValue("UIConfig/endScan2", fixedStr(cfg.endScan2, ConfigDecimals::Angle));
    settings.setValue("UIConfig/endScan3", fixedStr(cfg.endScan3, ConfigDecimals::Angle));

    settings.setValue("UIConfig/northCorrection", fixedStr(cfg.northCorrection, ConfigDecimals::Angle));
    settings.setValue("UIConfig/pitchCorrection", fixedStr(cfg.pitchCorrection, ConfigDecimals::Angle));
    settings.setValue("UIConfig/distanceCorrection", fixedStr(cfg.distanceCorrection, ConfigDecimals::Meter));

    settings.setValue("UIConfig/radarLongitude", fixedStr(cfg.radarLongitude, ConfigDecimals::Degree));
    settings.setValue("UIConfig/radarLatitude", fixedStr(cfg.radarLatitude, ConfigDecimals::Degree));
    settings.setValue("UIConfig/radarHeight", fixedStr(cfg.radarHeight, ConfigDecimals::Meter));

    settings.setValue("UIConfig/speedMin", fixedStr(cfg.speedMin, ConfigDecimals::Speed));
    settings.setValue("UIConfig/speedMax", fixedStr(cfg.speedMax, ConfigDecimals::Speed));
    settings.setValue("UIConfig/heightMin", fixedStr(cfg.heightMin, ConfigDecimals::Meter));
    settings.setValue("UIConfig/heightMax", fixedStr(cfg.heightMax, ConfigDecimals::Meter));
    settings.setValue("UIConfig/distanceMin", fixedStr(cfg.distanceMin, ConfigDecimals::Meter));
    settings.setValue("UIConfig/distanceMax", fixedStr(cfg.distanceMax, ConfigDecimals::Meter));
}

TableConfig RadarConfig::LoadTable()
{
    QSettings settings(FilePath(), QSettings::IniFormat);

    TableConfig cfg;
    const QString hidden = settings.value("TableConfig/hiddenColumns").toString();
    const QStringList hiddenParts = hidden.split(QLatin1Char(','), QString::SkipEmptyParts);
    for (const QString& part : hiddenParts)
    {
        const QString key = part.trimmed();
        if (!key.isEmpty() && !cfg.hiddenColumns.contains(key, Qt::CaseInsensitive))
        {
            cfg.hiddenColumns << key;
        }
    }

    const QString widths = settings.value("TableConfig/columnWidths").toString();
    const QStringList widthParts = widths.split(QLatin1Char(','), QString::SkipEmptyParts);
    for (const QString& part : widthParts)
    {
        const int sep = part.indexOf(QLatin1Char(':'));
        if (sep <= 0)
        {
            continue;
        }
        const QString key = part.left(sep).trimmed();
        bool ok = false;
        const int width = part.mid(sep + 1).trimmed().toInt(&ok);
        if (ok && !key.isEmpty() && width > 0)
        {
            cfg.columnWidths.insert(key, width);
        }
    }

    return cfg;
}

void RadarConfig::SaveTable(const TableConfig& cfg)
{
    QSettings settings(FilePath(), QSettings::IniFormat);
    settings.setValue("TableConfig/hiddenColumns", cfg.hiddenColumns.join(QLatin1Char(',')));

    QStringList pairs;
    for (auto it = cfg.columnWidths.constBegin(); it != cfg.columnWidths.constEnd(); ++it)
    {
        pairs << QStringLiteral("%1:%2").arg(it.key()).arg(it.value());
    }
    pairs.sort();
    settings.setValue("TableConfig/columnWidths", pairs.join(QLatin1Char(',')));
}
