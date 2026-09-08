#pragma once

#include <Geode/Geode.hpp>
#include <Geode/ui/Popup.hpp>
#include <Geode/ui/TextInput.hpp>
#include <functional>
#include "ImpossibleLevelsAPI.hpp"

// Petit popup permettant de filtrer la liste par plage de rang
// (ex: du rang 1 au rang 50) en plus de la recherche par nom/createur.
//
// Depuis Geode 5, geode::Popup n'est plus un template : plus de
// Popup<Args...>, plus de setup() ni de initAnchored(). On declare
// donc notre propre init() qui appelle Popup::init(largeur, hauteur).
class FilterPopup : public geode::Popup {
protected:
    geode::TextInput* m_minInput = nullptr;
    geode::TextInput* m_maxInput = nullptr;
    ButtonSprite* m_sortSprite = nullptr;
    ill::SortMode m_sort = ill::SortMode::Rank;
    std::function<void(int, int, ill::SortMode)> m_callback;

    bool init(int minRank, int maxRank, ill::SortMode sort,
              std::function<void(int, int, ill::SortMode)> callback);
    void onApply(cocos2d::CCObject*);
    void onReset(cocos2d::CCObject*);
    void onCycleSort(cocos2d::CCObject*);
    void updateSortLabel();

public:
    static FilterPopup* create(int minRank, int maxRank, ill::SortMode sort,
                               std::function<void(int, int, ill::SortMode)> callback);
};
