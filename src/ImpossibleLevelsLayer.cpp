#include "ImpossibleLevelsLayer.hpp"
#include "LevelCell.hpp"
#include "FilterPopup.hpp"
#include <Geode/binding/GameLevelManager.hpp>
#include <Geode/binding/GJSearchObject.hpp>
#include <Geode/binding/GJGameLevel.hpp>
#include <Geode/binding/LevelInfoLayer.hpp>

using namespace geode::prelude;

ImpossibleLevelsLayer* ImpossibleLevelsLayer::create() {
    auto ret = new ImpossibleLevelsLayer();
    if (ret->init()) {
        ret->autorelease();
        return ret;
    }
    delete ret;
    return nullptr;
}

cocos2d::CCScene* ImpossibleLevelsLayer::scene() {
    auto scene = CCScene::create();
    scene->addChild(ImpossibleLevelsLayer::create());
    return scene;
}

bool ImpossibleLevelsLayer::init() {
    if (!CCLayer::init()) return false;

    auto winSize = CCDirector::sharedDirector()->getWinSize();

    // Fond similaire aux autres menus du jeu
    auto bg = CCSprite::create("GJ_gradientBG.png");
    bg->setScaleX(winSize.width / bg->getContentSize().width);
    bg->setScaleY(winSize.height / bg->getContentSize().height);
    bg->setPosition({ winSize.width / 2.f, winSize.height / 2.f });
    bg->setColor({ 0, 40, 80 });
    addChild(bg, -1);

    // Titre
    auto title = CCLabelBMFont::create("Impossible Levels List", "goldFont.fnt");
    title->setScale(0.9f);
    title->setPosition({ winSize.width / 2.f, winSize.height - 25.f });
    addChild(title, 5);

    // --- Menu du bas (retour + site web) --------------------------------
    auto bottomMenu = CCMenu::create();
    bottomMenu->setPosition({ 0, 0 });
    addChild(bottomMenu, 5);

    auto backSpr = CCSprite::createWithSpriteFrameName("GJ_arrow_01_001.png");
    backSpr->setFlipX(true);
    backSpr->setScale(0.9f);
    auto backBtn = CCMenuItemSpriteExtra::create(backSpr, this, menu_selector(ImpossibleLevelsLayer::onBack));
    backBtn->setPosition({ 25.f, 25.f });
    bottomMenu->addChild(backBtn);

    auto siteSpr = ButtonSprite::create("Site web", "goldFont.fnt", "GJ_button_04.png", 0.8f);
    siteSpr->setScale(0.65f);
    auto siteBtn = CCMenuItemSpriteExtra::create(siteSpr, this, menu_selector(ImpossibleLevelsLayer::onOpenWebsite));
    siteBtn->setPosition({ winSize.width - 60.f, 25.f });
    bottomMenu->addChild(siteBtn);

    auto refreshSpr = CCSprite::createWithSpriteFrameName("GJ_updateBtn_001.png");
    refreshSpr->setScale(0.8f);
    auto refreshBtn = CCMenuItemSpriteExtra::create(refreshSpr, this, menu_selector(ImpossibleLevelsLayer::onRefresh));
    refreshBtn->setPosition({ winSize.width - 130.f, 25.f });
    bottomMenu->addChild(refreshBtn);

    // --- Onglets Tous / Semaine / Mois -----------------------------------
    m_tabMenu = CCMenu::create();
    m_tabMenu->setPosition({ winSize.width / 2.f, winSize.height - 60.f });
    addChild(m_tabMenu, 5);

    struct TabDef { const char* label; ill::ListCategory cat; };
    std::vector<TabDef> tabs = {
        { "Tous", ill::ListCategory::All },
        { "Nouveautes", ill::ListCategory::Recent }
    };

    float tabX = -75.f;
    for (auto& t : tabs) {
        auto spr = ButtonSprite::create(t.label, "bigFont.fnt", "GJ_button_02.png", 0.9f);
        spr->setScale(0.7f);
        auto btn = CCMenuItemSpriteExtra::create(spr, this, menu_selector(ImpossibleLevelsLayer::onTab));
        btn->setPosition({ tabX, 0 });
        btn->setTag(static_cast<int>(t.cat));
        m_tabMenu->addChild(btn);
        tabX += 150.f;
    }

    // --- Barre de recherche + filtres -------------------------------------
    auto searchMenu = CCMenu::create();
    searchMenu->setPosition({ 0, 0 });
    addChild(searchMenu, 5);

    m_searchInput = TextInput::create(220.f, "Rechercher un niveau ou createur...");
    m_searchInput->setPosition({ winSize.width / 2.f - 60.f, winSize.height - 95.f });
    m_searchInput->setCallback([this](std::string const& text) {
        m_searchQuery = text;
        rebuildList();
    });
    addChild(m_searchInput, 5);

    auto filterSpr = CCSprite::createWithSpriteFrameName("GJ_optionsBtn_001.png");
    filterSpr->setScale(0.8f);
    auto filterBtn = CCMenuItemSpriteExtra::create(filterSpr, this, menu_selector(ImpossibleLevelsLayer::onFilters));
    filterBtn->setPosition({ winSize.width / 2.f + 90.f, winSize.height - 95.f });
    searchMenu->addChild(filterBtn);

    // --- Zone de liste (ScrollLayer) --------------------------------------
    float listWidth = std::min(480.f, winSize.width - 40.f);
    float listHeight = winSize.height - 190.f;

    auto listBgSprite = cocos2d::extension::CCScale9Sprite::create("square02b_001.png");
    listBgSprite->setContentSize({ listWidth + 6.f, listHeight + 6.f });
    listBgSprite->setPosition({ winSize.width / 2.f, winSize.height / 2.f - 25.f });
    listBgSprite->setOpacity(120);
    addChild(listBgSprite, 3);

    m_scrollLayer = ScrollLayer::create({ listWidth, listHeight });
    m_scrollLayer->setPosition({ winSize.width / 2.f - listWidth / 2.f, winSize.height / 2.f - 25.f - listHeight / 2.f });
    addChild(m_scrollLayer, 4);

    m_statusLabel = CCLabelBMFont::create("Chargement...", "bigFont.fnt");
    m_statusLabel->setScale(0.6f);
    m_statusLabel->setPosition({ winSize.width / 2.f, winSize.height / 2.f - 25.f });
    addChild(m_statusLabel, 6);

    reloadData(false);

    setKeypadEnabled(true);
    return true;
}

void ImpossibleLevelsLayer::reloadData(bool forceNetwork) {
    if (m_statusLabel) {
        m_statusLabel->setString("Chargement...");
        m_statusLabel->setVisible(true);
    }

    ill::ImpossibleLevelsAPI::get()->fetchLevels(forceNetwork, [this](std::vector<ill::ImpossibleLevel> const& levels, bool success, std::string error) {
        if (!success && levels.empty()) {
            if (m_statusLabel) {
                m_statusLabel->setString(fmt::format("Echec du chargement :\n{}", error).c_str());
                m_statusLabel->setVisible(true);
            }
            return;
        }
        rebuildList();
    });
}

void ImpossibleLevelsLayer::rebuildList() {
    if (!m_scrollLayer) return;

    for (auto child : CCArrayExt<CCNode*>(m_scrollLayer->m_contentLayer->getChildren())) {
        child->removeFromParent();
    }

    int recentCount = static_cast<int>(Mod::get()->getSettingValue<int64_t>("recent-count"));
    int maxRows = static_cast<int>(Mod::get()->getSettingValue<int64_t>("max-rows"));

    float width = m_scrollLayer->getContentSize().width;
    float y = 0.f;
    float rowHeight = 42.f;
    float featuredHeight = 60.f;

    std::vector<ill::ImpossibleLevel> mainList = ill::ImpossibleLevelsAPI::get()->filter(
        m_category, m_searchQuery, m_minRank, m_maxRank
    );

    // La liste complete fait ~2100 niveaux : construire autant de cellules
    // d'un coup fige le jeu. On plafonne le rendu et on l'annonce, la
    // recherche et le filtre de rang servent a atteindre le reste.
    size_t total = mainList.size();
    bool truncated = maxRows > 0 && total > static_cast<size_t>(maxRows);
    if (truncated) mainList.resize(static_cast<size_t>(maxRows));

    // --- Bandeau "nouveautes" en tete de l'onglet Tous, seulement si aucune
    //     recherche/filtre n'est actif.
    bool showFeatured = (m_category == ill::ListCategory::All) && m_searchQuery.empty() && m_maxRank == 0;

    std::vector<cocos2d::CCNode*> cellsBottomToTop;

    if (showFeatured) {
        auto recent = ill::ImpossibleLevelsAPI::get()->filter(ill::ListCategory::Recent, "", 0, 0);
        if (!recent.empty()) {
            auto header = CCLabelBMFont::create(
                fmt::format("Derniers ajouts a la liste ({})", recentCount).c_str(), "goldFont.fnt");
            header->setScale(0.5f);
            header->setAnchorPoint({ 0.f, 0.5f });
            cellsBottomToTop.push_back(header);
            for (size_t i = 0; i < std::min<size_t>(5, recent.size()); i++) {
                cellsBottomToTop.push_back(ILLLevelCell::create(
                    recent[i], true, width, featuredHeight,
                    [this](auto const& lvl) { requestPlayLevel(lvl); },
                    [this](auto const& lvl) { requestOpenShowcase(lvl); }
                ));
            }

            auto sep = CCLabelBMFont::create("Liste complete", "goldFont.fnt");
            sep->setScale(0.5f);
            sep->setAnchorPoint({ 0.f, 0.5f });
            cellsBottomToTop.push_back(sep);
        }
    }

    for (auto const& lvl : mainList) {
        cellsBottomToTop.push_back(ILLLevelCell::create(
            lvl, false, width, rowHeight,
            [this](auto const& l) { requestPlayLevel(l); },
            [this](auto const& l) { requestOpenShowcase(l); }
        ));
    }

    if (truncated) {
        auto more = CCLabelBMFont::create(
            fmt::format("{} premiers sur {} - affine la recherche ou le filtre de rang",
                        maxRows, total).c_str(), "chatFont.fnt");
        more->setScale(0.5f);
        more->setAnchorPoint({ 0.f, 0.5f });
        cellsBottomToTop.push_back(more);
    }

    // Place les elements de haut en bas dans le ScrollLayer (Y decroissant)
    float totalHeight = 0.f;
    for (auto* node : cellsBottomToTop) {
        float h = rowHeight;
        if (auto label = typeinfo_cast<CCLabelBMFont*>(node)) h = 26.f;
        else if (auto cell = typeinfo_cast<ILLLevelCell*>(node)) h = cell->getContentSize().height;
        totalHeight += h;
    }

    y = totalHeight;
    for (auto* node : cellsBottomToTop) {
        float h = rowHeight;
        bool isHeader = false;
        if (auto label = typeinfo_cast<CCLabelBMFont*>(node)) { h = 26.f; isHeader = true; }
        else if (auto cell = typeinfo_cast<ILLLevelCell*>(node)) h = cell->getContentSize().height;

        y -= h;
        node->setPosition({ isHeader ? 10.f : 0.f, y });
        if (!isHeader) node->setAnchorPoint({ 0.f, 0.f });
        m_scrollLayer->m_contentLayer->addChild(node);
    }

    m_scrollLayer->m_contentLayer->setContentSize({ width, std::max(totalHeight, m_scrollLayer->getContentSize().height) });
    m_scrollLayer->scrollToTop();

    if (m_statusLabel) {
        m_statusLabel->setVisible(cellsBottomToTop.empty());
        if (cellsBottomToTop.empty()) m_statusLabel->setString("Aucun niveau ne correspond a ta recherche/tes filtres.");
    }
}

void ImpossibleLevelsLayer::onBack(cocos2d::CCObject*) {
    CCDirector::sharedDirector()->popSceneWithTransition(0.4f, kPopTransitionFade);
}

void ImpossibleLevelsLayer::onTab(cocos2d::CCObject* sender) {
    auto btn = static_cast<CCMenuItemSpriteExtra*>(sender);
    m_category = static_cast<ill::ListCategory>(btn->getTag());
    rebuildList();
}

void ImpossibleLevelsLayer::onRefresh(cocos2d::CCObject*) {
    reloadData(true);
}

void ImpossibleLevelsLayer::onFilters(cocos2d::CCObject*) {
    FilterPopup::create(m_minRank, m_maxRank, [this](int minR, int maxR) {
        setFilterRankRange(minR, maxR);
    })->show();
}

void ImpossibleLevelsLayer::setFilterRankRange(int minRank, int maxRank) {
    m_minRank = minRank;
    m_maxRank = maxRank;
    rebuildList();
}

void ImpossibleLevelsLayer::onOpenWebsite(cocos2d::CCObject*) {
    ill::ImpossibleLevelsAPI::openWebsite();
}

void ImpossibleLevelsLayer::requestPlayLevel(ill::ImpossibleLevel const& level) {
    m_pendingLoadLevelID = level.levelID;

    Notification::create(fmt::format("Chargement du niveau {}...", level.levelID), NotificationIcon::Loading)->show();

    auto glm = GameLevelManager::sharedState();
    glm->m_levelManagerDelegate = this;

    auto searchObj = GJSearchObject::create(SearchType::Search, std::to_string(level.levelID));
    glm->getOnlineLevels(searchObj);
}

void ImpossibleLevelsLayer::requestOpenShowcase(ill::ImpossibleLevel const& level) {
    // `showcaseLink` est renseigne pour 2274 des 2282 entrees de l'API.
    if (!level.videoUrl.empty()) {
        geode::utils::web::openLinkInBrowser(level.videoUrl);
    } else {
        Notification::create("Aucune video de showcase pour ce niveau.", NotificationIcon::Info)->show();
    }
}

void ImpossibleLevelsLayer::loadLevelsFinished(cocos2d::CCArray* levels, char const* key, int) {
    loadLevelsFinished(levels, key);
}

void ImpossibleLevelsLayer::loadLevelsFailed(char const* key, int) {
    loadLevelsFailed(key);
}

void ImpossibleLevelsLayer::loadLevelsFinished(cocos2d::CCArray* levels, char const*) {
    GameLevelManager::sharedState()->m_levelManagerDelegate = nullptr;

    if (!levels || levels->count() == 0) {
        Notification::create("Niveau introuvable sur les serveurs GD (a-t-il ete supprime ?).", NotificationIcon::Error)->show();
        return;
    }

    GJGameLevel* found = nullptr;
    for (auto lvl : CCArrayExt<GJGameLevel*>(levels)) {
        if (lvl->m_levelID == m_pendingLoadLevelID) {
            found = lvl;
            break;
        }
    }
    if (!found) found = static_cast<GJGameLevel*>(levels->objectAtIndex(0));

    auto scene = CCScene::create();
    scene->addChild(LevelInfoLayer::create(found, false));
    CCDirector::sharedDirector()->pushScene(CCTransitionFade::create(0.5f, scene));
}

void ImpossibleLevelsLayer::loadLevelsFailed(char const*) {
    GameLevelManager::sharedState()->m_levelManagerDelegate = nullptr;
    Notification::create("Impossible de recuperer ce niveau depuis les serveurs GD.", NotificationIcon::Error)->show();
}
