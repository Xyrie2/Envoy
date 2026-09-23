#pragma once
#include <QList>
#include <QString>
#include <QtGlobal>

#include "Core/RadarConstants.h"
#include "Core/RadarProtocol.h"

/// <summary>
/// @brief 航迹字段修改规则抽象接口。
/// </summary>
class RadarTrackRule
{
public:
    virtual ~RadarTrackRule() = default;
    virtual QString name() const = 0;
    virtual bool apply(RadarTrack& track) const = 0;
};

/// <summary>
/// @brief 规则1（目标类型）：跟踪次数 <= 6 且目标类型为 0(其它) 时，将目标类型修改为 255(待识别)
/// </summary>
class UnidentifiedCategoryRule : public RadarTrackRule
{
public:
    static constexpr quint16 MaxTrackPointCount = 6;
    QString name() const override;
    bool apply(RadarTrack& track) const override;
};

/// <summary>
/// @brief 航迹字段修改规则引擎
/// </summary>
class RadarTrackRuleEngine
{
public:
    RadarTrackRuleEngine();
    ~RadarTrackRuleEngine();
    RadarTrackRuleEngine(const RadarTrackRuleEngine&) = delete;
    RadarTrackRuleEngine& operator=(const RadarTrackRuleEngine&) = delete;

    void registerRule(RadarTrackRule* rule);
    bool applyAll(RadarTrack& track) const;
private:
    QList<RadarTrackRule*> m_rules;
};
