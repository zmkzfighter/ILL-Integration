#include <Geode/Geode.hpp>
#include <Geode/modify/LevelSearchLayer.hpp>
#include "ImpossibleLevelsLayer.hpp"

using namespace geode::prelude;

// Ajoute un bouton dans l'écran "Search Levels" (recherche de niveaux),
// positionné juste au-dessus du bouton retour en bas à gauche.
class $modify(ILLSearchLayer, LevelSearchLayer) {
    bool init() {
        if (!LevelSearchLayer::init()) return false;

        // Geode assigne automatiquement un ID "back-button" au bouton retour
        // de quasiment tous les écrans du jeu. On s'en sert comme repère pour
        // positionner notre bouton juste au-dessus, sans dépendre de la
        // disposition exacte du reste du menu.
        auto backBtn = this->getChildByID("back-button");
        cocos2d::CCPoint anchorPos;
        cocos2d::CCNode* parentMenu = nullptr;

        if (backBtn) {
            anchorPos = backBtn->getPosition();
            parentMenu = backBtn->getParent();
        } else {
            // Repli si jamais l'ID change un jour : coin bas-gauche standard.
            auto winSize = CCDirector::sharedDirector()->getWinSize();
            anchorPos = { 25.f, 25.f };
            parentMenu = this;
            log::warn("[ImpossibleLevels] 'back-button' introuvable, position de repli utilisee.");
        }

        auto spr = CCSprite::createWithSpriteFrameName("GJ_challengeBtn_001.png");
        if (!spr) spr = CCSprite::create("GJ_challengeBtn_001.png");
        spr->setScale(0.85f);

        auto btn = CCMenuItemSpriteExtra::create(
            spr, this, menu_selector(ILLSearchLayer::onImpossibleLevels)
        );
        btn->setID("impossible-levels-button"_spr);

        // Positionné directement au-dessus du bouton retour.
        float offsetY = backBtn ? (backBtn->getContentSize().height * backBtn->getScaleY() + 12.f) : 55.f;
        btn->setPosition({ anchorPos.x, anchorPos.y + offsetY });

        if (auto menu = typeinfo_cast<CCMenu*>(parentMenu)) {
            menu->addChild(btn);
        } else {
            // Le parent du bouton retour n'est pas un CCMenu (cas rare selon
            // les versions) : on crée notre propre petit menu à cet endroit.
            auto ownMenu = CCMenu::create();
            ownMenu->setPosition({ 0, 0 });
            btn->setPosition(anchorPos + cocos2d::CCPoint{ 0, offsetY });
            ownMenu->addChild(btn);
            this->addChild(ownMenu, 100);
        }

        return true;
    }

    void onImpossibleLevels(CCObject*) {
        CCDirector::sharedDirector()->pushScene(
            CCTransitionFade::create(0.5f, ImpossibleLevelsLayer::scene())
        );
    }
};
