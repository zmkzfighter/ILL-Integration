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

    // Tout est place par rapport a un coin de l'ecran (Anchor) plutot qu'a des
    // coordonnees absolues : le rendu suit la resolution au lieu de deriver.
    // Seule la zone de liste garde un calcul explicite, parce que son fond et
    // sa ScrollLayer doivent partager exactement le meme rectangle.
    const float kListTop = winSize.height - 106.f;
    const float kListBot = 30.f;

    const float listWidth   = std::min(420.f, winSize.width - 60.f);
    const float listHeight  = std::max(60.f, kListTop - kListBot);
    const float listCenterX = winSize.width / 2.f;
    const float listCenterY = (kListTop + kListBot) / 2.f;
    const float searchWidth = listWidth - 46.f;

    auto bg = CCSprite::create("GJ_gradientBG.png");
    bg->setScaleX(winSize.width / bg->getContentSize().width);
    bg->setScaleY(winSize.height / bg->getContentSize().height);
    bg->setColor({ 0, 40, 80 });
    this->addChildAtPosition(bg, Anchor::Center, ccp(0.f, 0.f), false);

    auto title = CCLabelBMFont::create("Impossible Levels List", "goldFont.fnt");
    title->setAnchorPoint({ 0.5f, 1.f });
    title->limitLabelWidth(winSize.width - 140.f, 0.8f, 0.3f);
    this->addChildAtPosition(title, Anchor::Top, ccp(0.f, -12.f), false);
    title->setZOrder(5);

    // --- Menu plein ecran : ses enfants peuvent utiliser les memes ancres.
    auto menu = CCMenu::create();
    menu->setContentSize(winSize);
    menu->setPosition({ 0.f, 0.f });
    menu->setAnchorPoint({ 0.f, 0.f });
    addChild(menu, 5);

    // Retour : en HAUT A GAUCHE et pointant vers la gauche, comme toutes les
    // autres pages du jeu (il etait en bas et retourne vers la droite).
    auto backSpr = CCSprite::createWithSpriteFrameName("GJ_arrow_01_001.png");
    backSpr->setScale(0.8f);
    auto backBtn = CCMenuItemSpriteExtra::create(backSpr, this, menu_selector(ImpossibleLevelsLayer::onBack));
    backBtn->setContentSize(backSpr->getScaledContentSize());
    backSpr->setPosition(ccp(backBtn->getContentSize().width / 2.f,
                             backBtn->getContentSize().height / 2.f));
    menu->addChildAtPosition(backBtn, Anchor::TopLeft, ccp(24.f, -24.f));

    // --- Onglets, espaces par un RowLayout.
    m_tabMenu = CCMenu::create();
    m_tabMenu->setContentSize({ listWidth, 28.f });
    m_tabMenu->setLayout(
        RowLayout::create()
            ->setGap(12.f)
            ->setAxisAlignment(AxisAlignment::Center)
            ->setAutoScale(false)
    );
    this->addChildAtPosition(m_tabMenu, Anchor::Top, ccp(0.f, -46.f), false);
    m_tabMenu->setZOrder(5);

    struct TabDef { const char* label; ill::ListCategory cat; };
    std::vector<TabDef> tabs = {
        { "Tous", ill::ListCategory::All },
        { "Nouveautes", ill::ListCategory::Recent }
    };
    for (auto& t : tabs) {
        auto spr = ButtonSprite::create(t.label, "bigFont.fnt", "GJ_button_02.png", 0.8f);
        spr->setScale(0.6f);
        auto btn = CCMenuItemSpriteExtra::create(spr, this, menu_selector(ImpossibleLevelsLayer::onTab));
        btn->setTag(static_cast<int>(t.cat));
        m_tabMenu->addChild(btn);
    }
    m_tabMenu->updateLayout();

    // --- Recherche + filtres.
    m_searchInput = TextInput::create(searchWidth, "Rechercher un niveau ou createur...");
    m_searchInput->setScale(0.8f);
    m_searchInput->setCallback([this](std::string const& text) {
        m_searchQuery = text;
        rebuildList();
    });
    this->addChildAtPosition(m_searchInput, Anchor::Top, ccp(-20.f, -80.f), false);
    m_searchInput->setZOrder(5);

    auto filterSpr = CCSprite::createWithSpriteFrameName("GJ_optionsBtn_001.png");
    filterSpr->setScale(0.6f);
    auto filterBtn = CCMenuItemSpriteExtra::create(filterSpr, this, menu_selector(ImpossibleLevelsLayer::onFilters));
    filterBtn->setContentSize(filterSpr->getScaledContentSize());
    filterSpr->setPosition(ccp(filterBtn->getContentSize().width / 2.f,
                               filterBtn->getContentSize().height / 2.f));
    menu->addChildAtPosition(filterBtn, Anchor::Top, ccp(listWidth / 2.f - 14.f, -80.f));

    // --- Liste : fond et ScrollLayer issus du MEME rectangle.
    auto listBgSprite = cocos2d::extension::CCScale9Sprite::create("square02b_001.png");
    listBgSprite->setContentSize({ listWidth + 8.f, listHeight + 8.f });
    listBgSprite->setPosition({ listCenterX, listCenterY });
    listBgSprite->setOpacity(90);
    addChild(listBgSprite, 3);

    m_scrollLayer = ScrollLayer::create({ listWidth, listHeight });
    m_scrollLayer->setPosition({ listCenterX - listWidth / 2.f, kListBot });
    addChild(m_scrollLayer, 4);

    m_statusLabel = CCLabelBMFont::create("Chargement...", "bigFont.fnt");
    m_statusLabel->setScale(0.5f);
    m_statusLabel->setPosition({ listCenterX, listCenterY });
    addChild(m_statusLabel, 6);

    // --- Coin haut droit : rafraichir puis site web, empiles par un
    //     RowLayout pour qu'ils ne se recouvrent jamais.
    auto topRightMenu = CCMenu::create();
    topRightMenu->setContentSize({ 150.f, 30.f });
    topRightMenu->setAnchorPoint({ 1.f, 0.5f });
    topRightMenu->setLayout(
        RowLayout::create()
            ->setGap(8.f)
            ->setAxisAlignment(AxisAlignment::End)
            ->setAutoScale(false)
    );
    this->addChildAtPosition(topRightMenu, Anchor::TopRight, ccp(-10.f, -24.f), false);
    topRightMenu->setZOrder(5);

    auto refreshSpr = CCSprite::createWithSpriteFrameName("GJ_updateBtn_001.png");
    refreshSpr->setScale(0.6f);
    topRightMenu->addChild(CCMenuItemSpriteExtra::create(refreshSpr, this, menu_selector(ImpossibleLevelsLayer::onRefresh)));

    auto siteSpr = ButtonSprite::create("Site web", "goldFont.fnt", "GJ_button_04.png", 0.8f);
    siteSpr->setScale(0.5f);
    topRightMenu->addChild(CCMenuItemSpriteExtra::create(siteSpr, this, menu_selector(ImpossibleLevelsLayer::onOpenWebsite)));

    topRightMenu->updateLayout();

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

    // NE PAS iterer les enfants en appelant removeFromParent() dessus :
    // chaque suppression mute le CCArray pendant le parcours, ce qui faisait
    // crasher le jeu (EXCEPTION_ACCESS_VIOLATION) des qu'on tapait dans la
    // barre de recherche, la liste etant alors deja peuplee.
    m_scrollLayer->m_contentLayer->removeAllChildren();

    int recentCount = static_cast<int>(Mod::get()->getSettingValue<int64_t>("recent-count"));
    int maxRows = static_cast<int>(Mod::get()->getSettingValue<int64_t>("max-rows"));

    float width = m_scrollLayer->getContentSize().width;
    float y = 0.f;
    float rowHeight = 42.f;
    float featuredHeight = 60.f;

    std::vector<ill::ImpossibleLevel> mainList = ill::ImpossibleLevelsAPI::get()->filter(
        m_category, m_searchQuery, m_minRank, m_maxRank, m_sort
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
        // Le bandeau montre toujours les derniers ajoutes en premier, quel
        // que soit le tri choisi pour la liste principale.
        auto recent = ill::ImpossibleLevelsAPI::get()->filter(
            ill::ListCategory::Recent, "", 0, 0, ill::SortMode::RecentFirst);
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
        // Les en-tetes sont ancres en (0, 0.5) : il faut les centrer dans
        // leur emplacement, sinon la moitie basse deborde sur la cellule
        // suivante (le titre de section apparaissait coupe).
        node->setPosition({ isHeader ? 10.f : 0.f, isHeader ? y + h / 2.f : y });
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

// Echap sur PC / bouton retour sur mobile. setKeypadEnabled(true) etait deja
// appele dans init(), mais rien ne recuperait l'evenement.
void ImpossibleLevelsLayer::keyBackClicked() {
    this->onBack(nullptr);
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
    FilterPopup::create(m_minRank, m_maxRank, m_sort,
        [this](int minR, int maxR, ill::SortMode sort) {
            applyFilters(minR, maxR, sort);
        })->show();
}

void ImpossibleLevelsLayer::applyFilters(int minRank, int maxRank, ill::SortMode sort) {
    m_minRank = minRank;
    m_maxRank = maxRank;
    m_sort = sort;
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
