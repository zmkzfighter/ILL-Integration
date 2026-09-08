#include <Geode/Geode.hpp>
#include <Geode/modify/LevelSearchLayer.hpp>
#include "ImpossibleLevelsLayer.hpp"

using namespace geode::prelude;

// Ajoute un bouton dans l'écran "Search Levels" (recherche de niveaux),
// positionné juste au-dessus du bouton retour en bas à gauche.
class $modify(ILLSearchLayer, LevelSearchLayer) {
    bool init(int type) {
        if (!LevelSearchLayer::init(type)) return false;

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
            anchorPos = ccp(25.f, 25.f);
            parentMenu = this;
            log::warn("[ImpossibleLevels] 'back-button' introuvable, position de repli utilisee.");
        }

        // Logo ILL fourni par le mod. `_spr` prefixe le nom par l'id du mod,
        // c'est ce que le CLI Geode genere a partir de resources.sprites.
        auto spr = CCSprite::create("ill-logo.png"_spr);
        if (!spr) {
            // Repli sur un sprite du jeu si la ressource manque a l'appel.
            log::warn("[ImpossibleLevels] ill-logo.png introuvable, sprite de repli utilise.");
            spr = CCSprite::createWithSpriteFrameName("GJ_challengeBtn_001.png");
        }
        if (!spr) return true;

        // Le logo est un carre : on le borne a une taille de bouton au lieu
        // d'un facteur d'echelle en dur, pour rester correct quelle que soit
        // la variante (sd/hd/uhd) que cocos charge selon la resolution.
        limitNodeSize(spr, { 30.f, 30.f }, 1.f, 0.05f);

        auto btn = CCMenuItemSpriteExtra::create(
            spr, this, menu_selector(ILLSearchLayer::onImpossibleLevels)
        );
        // CCMenuItemSpriteExtra prend la taille NON mise a l'echelle du
        // sprite : sans ca le bouton reste dimensionne pour l'image d'origine
        // (zone de clic enorme et sprite decale dedans).
        btn->setContentSize(spr->getScaledContentSize());
        spr->setPosition(ccp(btn->getContentSize().width / 2.f,
                             btn->getContentSize().height / 2.f));
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
