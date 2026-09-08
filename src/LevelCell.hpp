#pragma once

#include <Geode/Geode.hpp>
#include "ImpossibleLevelsAPI.hpp"

// Une cellule représentant un niveau dans la liste (mode normal ou "featured"
// pour les blocs Nouveautés semaine/mois, plus grands et mis en valeur).
class LevelCell : public cocos2d::CCLayerColor {
protected:
    ill::ImpossibleLevel m_level;
    bool m_featured;
    std::function<void(ill::ImpossibleLevel const&)> m_onPlay;
    std::function<void(ill::ImpossibleLevel const&)> m_onRecords;

    bool init(ill::ImpossibleLevel const& level, bool featured, float width, float height);

    void onPlay(cocos2d::CCObject*);
    void onRecords(cocos2d::CCObject*);
    void onCopyId(cocos2d::CCObject*);

public:
    static LevelCell* create(
        ill::ImpossibleLevel const& level,
        bool featured,
        float width,
        float height,
        std::function<void(ill::ImpossibleLevel const&)> onPlay,
        std::function<void(ill::ImpossibleLevel const&)> onRecords = nullptr
    );
};
