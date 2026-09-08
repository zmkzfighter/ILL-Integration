#include "FilterPopup.hpp"

using namespace geode::prelude;

FilterPopup* FilterPopup::create(int minRank, int maxRank, std::function<void(int, int)> callback) {
    auto ret = new FilterPopup();
    if (ret->init(minRank, maxRank, callback)) {
        ret->autorelease();
        return ret;
    }
    delete ret;
    return nullptr;
}

bool FilterPopup::init(int minRank, int maxRank, std::function<void(int, int)> callback) {
    if (!Popup::init(240.f, 160.f)) return false;

    m_callback = callback;
    this->setTitle("Filtrer par rang");

    // m_size = taille reelle du popup, fournie par Popup::init.
    auto size = m_size;

    auto label = CCLabelBMFont::create("Plage de rang dans le classement :", "bigFont.fnt");
    label->setScale(0.35f);
    m_mainLayer->addChildAtPosition(label, Anchor::Center, ccp(0.f, 35.f));

    m_minInput = TextInput::create(60.f, "Min");
    m_minInput->setCommonFilter(CommonFilter::Uint);
    if (minRank > 0) m_minInput->setString(std::to_string(minRank));
    m_mainLayer->addChildAtPosition(m_minInput, Anchor::Center, ccp(-45.f, 5.f));

    auto dash = CCLabelBMFont::create("-", "bigFont.fnt");
    m_mainLayer->addChildAtPosition(dash, Anchor::Center, ccp(0.f, 5.f));

    m_maxInput = TextInput::create(60.f, "Max");
    m_maxInput->setCommonFilter(CommonFilter::Uint);
    if (maxRank > 0) m_maxInput->setString(std::to_string(maxRank));
    m_mainLayer->addChildAtPosition(m_maxInput, Anchor::Center, ccp(45.f, 5.f));

    auto applySpr = ButtonSprite::create("Appliquer", "goldFont.fnt", "GJ_button_01.png", 0.8f);
    applySpr->setScale(0.7f);
    auto applyBtn = CCMenuItemSpriteExtra::create(applySpr, this, menu_selector(FilterPopup::onApply));
    m_buttonMenu->addChildAtPosition(applyBtn, Anchor::Center, ccp(45.f, -40.f));

    auto resetSpr = ButtonSprite::create("Reinitialiser", "goldFont.fnt", "GJ_button_06.png", 0.8f);
    resetSpr->setScale(0.7f);
    auto resetBtn = CCMenuItemSpriteExtra::create(resetSpr, this, menu_selector(FilterPopup::onReset));
    m_buttonMenu->addChildAtPosition(resetBtn, Anchor::Center, ccp(-45.f, -40.f));

    return true;
}

void FilterPopup::onApply(cocos2d::CCObject*) {
    int minRank = 0, maxRank = 0;
    try { minRank = std::stoi(std::string(m_minInput->getString())); } catch (...) {}
    try { maxRank = std::stoi(std::string(m_maxInput->getString())); } catch (...) {}
    if (m_callback) m_callback(minRank, maxRank);
    this->onClose(nullptr);
}

void FilterPopup::onReset(cocos2d::CCObject*) {
    if (m_callback) m_callback(0, 0);
    this->onClose(nullptr);
}
