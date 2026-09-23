#include "Core/RadarTrackRules.h"

QString UnidentifiedCategoryRule::name() const
{
    return QStringLiteral("规则1-低跟踪次数目标类型待识别");
}

bool UnidentifiedCategoryRule::apply(RadarTrack& track) const
{
    if (track.trackPointCount <= MaxTrackPointCount && track.targetCategory == TargetCategory::Other)
    {
        track.targetCategory = TargetCategory::Unidentified;
        return true;
    }
    return false;
}

RadarTrackRuleEngine::RadarTrackRuleEngine()
{
    registerRule(new UnidentifiedCategoryRule());
}

RadarTrackRuleEngine::~RadarTrackRuleEngine()
{
    qDeleteAll(m_rules);
}

void RadarTrackRuleEngine::registerRule(RadarTrackRule* rule)
{
    if (rule != nullptr) {
        m_rules.append(rule);
    }
}

bool RadarTrackRuleEngine::applyAll(RadarTrack& track) const
{
    bool modified = false;
    for (RadarTrackRule* rule : m_rules) {
        if (rule->apply(track))
        {
            modified = true;
        }
    }
    return modified;
}
