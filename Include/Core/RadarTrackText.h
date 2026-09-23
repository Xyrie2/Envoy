#pragma once
#include <QString>
#include <QtGlobal>

#include "Core/RadarProtocol.h"

namespace TrackText
{
QString StatusText(quint16 status);                             /// 航迹状态中文名（未知值返回 "未知(N)"）
QString CategoryText(quint8 category);                          /// 目标类型中文名（未知值返回 "未知(N)"）
QString SignalConfigDetails(const SignalProcessingConfig& cfg); /// 信处配置回传详情（0x55AAAA55）
QString DataConfigDetails(const DataProcessingConfig& cfg);     /// 数处配置回传详情（0x55222255）
}
