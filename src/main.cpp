#include <Geode/Geode.hpp>
#include <Geode/modify/LevelSearchLayer.hpp>
#include "ImpossibleLevelsLayer.hpp"

using namespace geode::prelude;

// Ajoute un bouton dans l'écran "Search Levels" (recherche de niveaux),
// positionné juste au-dessus du bouton retour en bas à gauche.
class $modify(ILLSearchLayer, LevelSearchLayer) {
    bool init(int type) {
        if (!LevelSearchLayer::init(type)) return false;

        auto spr = CCSprite::create("ill-logo.png"_spr);
        if (!spr) {
            log::warn("[ImpossibleLevels] ill-logo.png introuvable, sprite de repli utilise.");
            spr = CCSprite::createWithSpriteFrameName("GJ_challengeBtn_001.png");
        }
        if (!spr) return true;

        // Le logo est un carre : on le borne a une taille de bouton au lieu
        // d'un facteur d'echelle en dur, pour rester correct quelle que soit
        // la variante (sd/hd/uhd) que cocos charge selon la resolution.
        geode::cocos::limitNodeSize(spr, { 30.f, 30.f }, 1.f, 0.05f);

        auto btn = CCMenuItemSpriteExtra::create(
            spr, this, menu_selector(ILLSearchLayer::onImpossibleLevels)
        );
        btn->setID("impossible-levels-button"_spr);
        // CCMenuItemSpriteExtra prend la taille NON mise a l'echelle du
        // sprite : sans ca le bouton garde la zone de clic de l'image
        // d'origine et le sprite est decale dedans.
        btn->setContentSize(spr->getScaledContentSize());
        spr->setPosition(ccp(btn->getContentSize().width / 2.f,
                             btn->getContentSize().height / 2.f));

        // `other-filter-menu` est la colonne de boutons de droite de l'ecran
        // de recherche (filtres, filtres avances, listes). Geode lui pose une
        // ColumnLayout : y ajouter notre bouton le fait placer automatiquement
        // et repousse les autres, au lieu de le poser a une coordonnee fixe ou
        // il finissait par en recouvrir un autre.
        if (auto menu = typeinfo_cast<CCMenu*>(this->getChildByID("other-filter-menu"))) {
            menu->addChild(btn);
            menu->updateLayout();
            return true;
        }

        // Repli si l'ID disparait : une colonne a nous, ancree en haut a
        // gauche sous le bouton retour, avec sa propre ColumnLayout.
        log::warn("[ImpossibleLevels] 'other-filter-menu' introuvable, colonne de repli utilisee.");

        auto ownMenu = CCMenu::create();
        ownMenu->setContentSize({ 40.f, 120.f });
        ownMenu->setAnchorPoint({ 0.f, 1.f });
        ownMenu->setPosition({ 6.f, CCDirector::sharedDirector()->getWinSize().height - 60.f });
        ownMenu->setLayout(
            ColumnLayout::create()
                ->setGap(8.f)
                ->setAxisAlignment(AxisAlignment::End)
                ->setAutoScale(false)
        );
        ownMenu->addChild(btn);
        ownMenu->updateLayout();
        this->addChild(ownMenu, 100);

        return true;
    }

    void onImpossibleLevels(CCObject*) {
        CCDirector::sharedDirector()->pushScene(
            CCTransitionFade::create(0.5f, ImpossibleLevelsLayer::scene())
        );
    }
};
