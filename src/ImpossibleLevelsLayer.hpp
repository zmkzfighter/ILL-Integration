#pragma once

#include <Geode/Geode.hpp>
#include <Geode/ui/ScrollLayer.hpp>
#include <Geode/ui/TextInput.hpp>
#include "ImpossibleLevelsAPI.hpp"

// Ecran plein cadre listant les niveaux de impossiblelevels.com, avec
// onglets Tous / Semaine / Mois, recherche, filtres de rang et mise en avant
// des nouveautés en haut de l'onglet "Tous".
class ImpossibleLevelsLayer : public cocos2d::CCLayer, public LevelManagerDelegate {
protected:
    ill::ListCategory m_category = ill::ListCategory::All;
    std::string m_searchQuery;
    int m_minRank = 0;
    int m_maxRank = 0;
    ill::SortMode m_sort = ill::SortMode::Rank;
    int m_page = 0;                      // page courante, 0 = premiere

    geode::ScrollLayer* m_scrollLayer = nullptr;
    geode::TextInput* m_searchInput = nullptr;
    cocos2d::CCMenu* m_tabMenu = nullptr;
    cocos2d::extension::CCScale9Sprite* m_loadingBg = nullptr;
    cocos2d::CCLabelBMFont* m_statusLabel = nullptr;
    cocos2d::CCLabelBMFont* m_pageLabel = nullptr;
    ButtonSprite* m_sortSprite = nullptr;

    int m_pendingLoadLevelID = 0;

    bool init();
    void reloadData(bool forceNetwork);
    void rebuildList();

    void onBack(cocos2d::CCObject*);
    void keyBackClicked() override;
    void onTab(cocos2d::CCObject*);
    void onRefresh(cocos2d::CCObject*);
    void onFilters(cocos2d::CCObject*);
    void onOpenWebsite(cocos2d::CCObject*);
    void onPrevPage(cocos2d::CCObject*);
    void onNextPage(cocos2d::CCObject*);
    void onCycleSort(cocos2d::CCObject*);

    void requestPlayLevel(ill::ImpossibleLevel const& level);
    void requestOpenShowcase(ill::ImpossibleLevel const& level);

    // LevelManagerDelegate
    // NOTE: la signature exacte de LevelManagerDelegate a legerement varie
    // entre versions de GD/Geode. Les deux formes (avec et sans le param
    // entier final) sont fournies SANS le mot-cle `override` afin que le
    // compilateur retienne automatiquement celle qui correspond vraiment a
    // la classe de base de ton SDK, sans erreur de compilation si l'autre
    // forme n'existe pas.
    void loadLevelsFinished(cocos2d::CCArray* levels, char const* key, int p2);
    void loadLevelsFailed(char const* key, int p2);
    void loadLevelsFinished(cocos2d::CCArray* levels, char const* key);
    void loadLevelsFailed(char const* key);

public:
    static ImpossibleLevelsLayer* create();
    static cocos2d::CCScene* scene();

    void applyFilters(int minRank, int maxRank, ill::SortMode sort);
};
